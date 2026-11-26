#pragma once

#ifdef XPLPC_SERIALIZER_JSON

#include "xplpc/core/Router.hpp"
#include "xplpc/data/MappingList.hpp"
#include "xplpc/message/Message.hpp"
#include "xplpc/serializer/JsonRequestScanner.hpp"
#include "xplpc/serializer/JsonSerializerTypes.hpp"
#include "xplpc/serializer/JsonValueGuard.hpp"
#include "xplpc/util/Log.hpp"

#include "nlohmann/json.hpp"
using json = nlohmann::json;

#include <any>
#include <array>
#include <cstddef>
#include <exception>
#include <initializer_list>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <unordered_set>

namespace xplpc
{
namespace serializer
{

using namespace xplpc::data;

class Serializer
{
public:
    template <typename Tr, typename... Ts>
    static void execute(core::Handle call, const std::string &data, const std::array<std::string, sizeof...(Ts)> &params, const std::string &functionName, const Target &target)
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
            return core::Router::shared().answer(call, "");
        }

        // Each declared type is matched against the parameter name at the same position of the mapping signature.
        Message message;
        size_t paramIndex = 0;

        // clang-format off
        (decodeParam<Ts>(jsonData, params[paramIndex++], functionName, message), ...);
        // clang-format on

        try
        {
            // clang-format off
            target(message, [call](const std::any &response) {
                core::Router::shared().answer(call, encodeReturnValue<Tr>(response));
            });
            // clang-format on
        }
        catch (const std::exception &e)
        {
            util::Log::e("[JsonSerializer : execute] Error when execute function \"{}\"", functionName);
            util::Log::d("[JsonSerializer : execute] Error when execute function \"{}\": {}", functionName, e.what());
            core::Router::shared().answer(call, "");
        }
        catch (...)
        {
            util::Log::e("[JsonSerializer : execute] Function \"{}\" raised something that is not an exception", functionName);
            core::Router::shared().answer(call, "");
        }
    }

    static std::string decodeFunctionName(const std::string &data)
    {
        // Routing reads one field, so the document is scanned rather than built in memory, and it is scanned to the end since a repeated field is only seen there.

        JsonRequestScanner scanner;
        json::sax_parse(data, &scanner);

        if (scanner.unreadable)
        {
            util::Log::e("[JsonSerializer : decodeFunctionName] Error when parse json");
            return "";
        }

        if (scanner.refusal)
        {
            util::Log::e("[JsonSerializer : decodeFunctionName] The request was refused because {}", scanner.refusal);

            if (!scanner.repeatedParameter.empty())
            {
                util::Log::d("[JsonSerializer : decodeFunctionName] The repeated parameter is \"{}\"", scanner.repeatedParameter);
            }

            return "";
        }

        if (scanner.name.empty())
        {
            util::Log::e("[JsonSerializer : decodeFunctionName] The request names no function");
        }

        return scanner.name;
    }

    template <typename T>
    static std::optional<T> decodeFunctionReturnValue(const std::string &data)
    {
        // An empty response is the empty value every failing path answers with, not a document that failed to parse.
        if (data.empty())
        {
            return std::nullopt;
        }

        json document;

        try
        {
            document = json::parse(data);
        }
        catch (const std::exception &e)
        {
            util::Log::e("[JsonSerializer : decodeFunctionReturnValue] Error when parse json");
            util::Log::d("[JsonSerializer : decodeFunctionReturnValue] Error when parse json: {}", e.what());
            return std::nullopt;
        }

        // A document that carries no value is the empty value, which is not a failure to report.
        if (!document.contains("r") || document["r"].is_null())
        {
            return std::nullopt;
        }

        try
        {
            return read<T>(document["r"]);
        }
        catch (const std::exception &e)
        {
            util::Log::e("[JsonSerializer : decodeFunctionReturnValue] Value has another type");
            util::Log::d("[JsonSerializer : decodeFunctionReturnValue] Value has another type: {}", e.what());
        }
        catch (...)
        {
            // Filling a consumer type runs code this library did not write, which can raise anything.
            util::Log::e("[JsonSerializer : decodeFunctionReturnValue] Value has another type");
        }

        return std::nullopt;
    }

    template <typename... Args>
    static std::string encodeRequest(const std::string &functionName, Args &&...params)
    {
        // Routing never scans a request built here, so what the scan refuses on every other bridge is refused while it is being built.
        if (functionName.empty())
        {
            util::Log::e("[JsonSerializer : encodeRequest] The request names no function");
            return "";
        }

        std::unordered_set<std::string> names;

        for (const auto *name : std::initializer_list<const std::string *>{&params.n...})
        {
            if (!names.insert(*name).second)
            {
                util::Log::e("[JsonSerializer : encodeRequest] Parameter \"{}\" of function \"{}\" appears more than once", *name, functionName);
                return "";
            }
        }

        // Text that is not utf8 and a consumer type whose to_json raises both fail here, and a request that cannot be written is the empty one routing refuses.
        try
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
        catch (const std::exception &e)
        {
            util::Log::e("[JsonSerializer : encodeRequest] Error when encode data");
            util::Log::d("[JsonSerializer : encodeRequest] Error when encode data: {}", e.what());
        }
        catch (...)
        {
            util::Log::e("[JsonSerializer : encodeRequest] Error when encode data");
        }

        return "";
    }

private:
    template <typename T>
    static T read(const json &value)
    {
        if (!JsonValueGuard::fits<T>(value))
        {
            throw std::invalid_argument("The value does not fit the declared type");
        }

        return value.template get<T>();
    }

    template <typename T>
    static void decodeParam(const json &jsonData, const std::string &name, const std::string &functionName, Message &message)
    {
        if (!jsonData.contains("p") || !jsonData["p"].is_array())
        {
            return;
        }

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
                message.set(name, read<T>(param["v"]));
            }
            catch (const std::exception &e)
            {
                util::Log::e("[JsonSerializer : decodeParam] Value of \"{}\" has another type", name);
                util::Log::d("[JsonSerializer : decodeParam] Value of \"{}\" of function \"{}\" has another type: {}", name, functionName, e.what());
            }
            catch (...)
            {
                // Filling a consumer type runs code this library did not write, which can raise anything.
                util::Log::e("[JsonSerializer : decodeParam] Value of \"{}\" has another type", name);
                util::Log::d("[JsonSerializer : decodeParam] Value of \"{}\" of function \"{}\" raised something that is not an exception", name, functionName);
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
            else if (!response.has_value() || response.type() == typeid(std::nullptr_t))
            {
                // A mapping with nothing to answer says so with nullptr, whatever type it declared.
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
