#pragma once

#ifdef XPLPC_SERIALIZER_JSON

#include "xplpc/data/CallbackList.hpp"
#include "xplpc/data/MappingList.hpp"
#include "xplpc/message/Message.hpp"
#include "xplpc/serializer/JsonSerializerTypes.hpp"
#include "xplpc/util/Log.hpp"

#include "nlohmann/json.hpp"
using json = nlohmann::json;

#include <any>
#include <cstddef>
#include <exception>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace xplpc
{
namespace serializer
{

using namespace xplpc::data;

class FunctionNameReader : public nlohmann::json_sax<json>
{
public:
    std::string name;
    bool broken = false;

    bool key(string_t &value) override
    {
        wanted = depth == 1 && value == "f";
        return true;
    }

    bool string(string_t &value) override
    {
        if (!wanted)
        {
            return true;
        }

        name = value;
        return false;
    }

    bool start_object(std::size_t) override
    {
        depth++;
        return true;
    }

    bool end_object() override
    {
        depth--;
        return true;
    }

    bool start_array(std::size_t) override { return true; }
    bool end_array() override { return true; }
    bool null() override { return true; }
    bool boolean(bool) override { return true; }
    bool number_integer(number_integer_t) override { return true; }
    bool number_unsigned(number_unsigned_t) override { return true; }
    bool number_float(number_float_t, const string_t &) override { return true; }
    bool binary(binary_t &) override { return true; }
    bool parse_error(std::size_t, const std::string &, const nlohmann::detail::exception &) override
    {
        // A document that stops being readable is a failure, while one that carries no function name is not.
        broken = name.empty();
        return false;
    }

private:
    bool wanted = false;
    int depth = 0;
};

class Serializer
{
public:
    template <typename Tr, typename... Ts>
    static void execute(const std::string &key, const std::string &data, const std::vector<std::string> &params, const std::string &functionName, const Target &target)
    {
        json jsonData;

        try
        {
            jsonData = json::parse(data);
        }
        catch (const std::exception &e)
        {
            util::Log::e("[JsonSerializer : execute] Error when parse json");
            util::Log::d("[JsonSerializer : execute] Error when parse json: {}", e.what());
            return CallbackList::shared()->execute(key, "");
        }

        // Each declared type is matched against the parameter name at the same position of the mapping signature.
        Message message;
        size_t paramIndex = 0;

        // clang-format off
        (decodeParam<Ts>(jsonData, params, paramIndex++, functionName, message), ...);
        // clang-format on

        try
        {
            // clang-format off
            target(message, [key](const std::any &response) {
                CallbackList::shared()->execute(key, encodeReturnValue<Tr>(response));
            });
            // clang-format on
        }
        catch (const std::exception &e)
        {
            util::Log::e("[JsonSerializer : execute] Error when execute function \"{}\"", functionName);
            util::Log::d("[JsonSerializer : execute] Error when execute function \"{}\": {}", functionName, e.what());
            CallbackList::shared()->execute(key, "");
        }
        catch (...)
        {
            // A mapping may raise something that does not derive from std::exception, and the boundary that would stop it is not the one holding the key.
            util::Log::e("[JsonSerializer : execute] Function \"{}\" raised something that is not an exception", functionName);
            CallbackList::shared()->execute(key, "");
        }
    }

    static std::string decodeFunctionName(const std::string &data)
    {
        // Routing reads one field, so the document is scanned until that field is found rather than built in memory.

        FunctionNameReader reader;
        json::sax_parse(data, &reader);

        if (reader.broken)
        {
            util::Log::e("[JsonSerializer : decodeFunctionName] Error when parse json");
            return "";
        }

        if (reader.name.empty())
        {
            util::Log::e("[JsonSerializer : decodeFunctionName] Function name not found in json");
        }

        return reader.name;
    }

    template <typename T>
    static std::optional<T> decodeFunctionReturnValue(const std::string &data)
    {
        // An empty response is the empty value every failing path answers with, not a document that failed to parse.
        if (data.empty())
        {
            return std::nullopt;
        }

        try
        {
            const auto j = json::parse(data);

            if (!j.contains("r") || j["r"].is_null())
            {
                return std::nullopt;
            }

            return j["r"].template get<T>();
        }
        catch (const std::exception &e)
        {
            util::Log::e("[JsonSerializer : decodeFunctionReturnValue] Error when parse json");
            util::Log::d("[JsonSerializer : decodeFunctionReturnValue] Error when parse json: {}", e.what());
        }

        return std::nullopt;
    }

    template <typename... Args>
    static std::string encodeRequest(const std::string &functionName, Args &&...params)
    {
        json j;

        j["f"] = functionName;
        j["p"] = json::array();

        // clang-format off
        ([&]() {
            json o;
            o["n"] = params.n;

            if (params.v.has_value())
            {
                o["v"] = params.v.value();
            }
            else
            {
                o["v"] = nullptr;
            }

            j["p"].push_back(o);
        }(),...);
        // clang-format on

        return j.dump();
    }

private:
    template <typename T>
    static void decodeParam(const json &jsonData, const std::vector<std::string> &params, size_t index, const std::string &functionName, Message &message)
    {
        if (index >= params.size())
        {
            util::Log::e("[JsonSerializer : decodeParam] Function \"{}\" declares less parameter names than types", functionName);
            return;
        }

        if (!jsonData.contains("p") || !jsonData["p"].is_array())
        {
            return;
        }

        const auto &name = params[index];

        for (const auto &param : jsonData["p"])
        {
            if (!param.contains("n") || param["n"] != name)
            {
                continue;
            }

            if (!param.contains("v") || param["v"].is_null())
            {
                return;
            }

            try
            {
                message.set(name, param["v"].template get<T>());
            }
            catch (const std::exception &e)
            {
                util::Log::e("[JsonSerializer : decodeParam] Error when get data for parameter \"{}\" and function \"{}\"", name, functionName);
                util::Log::d("[JsonSerializer : decodeParam] Error when get data for parameter \"{}\": {}", name, e.what());
            }
            catch (...)
            {
                // Filling a consumer type runs code this library did not write, and this decode is reached before the guard that answers the caller.
                util::Log::e("[JsonSerializer : decodeParam] Parameter \"{}\" of function \"{}\" raised something that is not an exception", name, functionName);
            }

            return;
        }
    }

    template <typename Tr>
    static std::string encodeReturnValue(const std::any &response)
    {
        json j;

        // A mapping is free to answer on a thread of its own, where nothing above this can catch, so the encode never leaves this function by raising.
        try
        {
            if constexpr (std::is_same_v<Tr, void> || std::is_same_v<Tr, std::nullopt_t>)
            {
                j["r"] = nullptr;
            }
            else if (!response.has_value())
            {
                j["r"] = nullptr;
            }
            else
            {
                j["r"] = std::any_cast<Tr>(response);
            }

            return j.dump();
        }
        catch (const std::exception &e)
        {
            util::Log::e("[JsonSerializer : encodeReturnValue] Error when encode data");
            util::Log::d("[JsonSerializer : encodeReturnValue] Error when encode data: {}", e.what());
        }
        catch (...)
        {
            util::Log::e("[JsonSerializer : encodeReturnValue] Value raised something that is not an exception");
        }

        return "";
    }
};

} // namespace serializer
} // namespace xplpc

#endif
