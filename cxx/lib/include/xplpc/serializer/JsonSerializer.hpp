#pragma once

#ifdef XPLPC_SERIALIZER_JSON

#include "xplpc/data/CallbackList.hpp"
#include "xplpc/data/MappingList.hpp"
#include "xplpc/message/Message.hpp"
#include "xplpc/serializer/JsonSerializerTypes.hpp"

#include "nlohmann/json.hpp"
using json = nlohmann::json;

#include "spdlog/spdlog.h"

#include <any>
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

class Serializer
{

public:
    template <typename Tr, typename... Ts>
    static void execute(const std::string &key, const std::string &data, const std::vector<std::string> &params)
    {
        json jsonData;

        try
        {
            jsonData = json::parse(data);
        }
        catch (const std::exception &e)
        {
            spdlog::error("[JsonSerializer : execute] Error when parse json: {}", e.what());
            return CallbackList::shared()->execute(key, "");
        }

        std::string functionName;

        if (jsonData.contains("f"))
        {
            functionName = jsonData["f"].get<std::string>();
        }
        else
        {
            spdlog::error("[JsonSerializer : execute] Function name not found in json");
            return CallbackList::shared()->execute(key, "");
        }

        Message message = {};

        auto functionParams = jsonData["p"];
        auto paramsCount = 0;
        std::string lastParamName = "";

        // clang-format off
        ([&](){
            try
            {
                for (const auto & fp: functionParams)
                {
                    lastParamName = fp["n"].get<std::string>();

                    if (lastParamName == params[paramsCount])
                    {
                        if (!fp["v"].is_null()) {
                            message[lastParamName] = fp["v"].get<Ts>();
                        }

                        break;
                    }
                }

                paramsCount++;
            }
            catch (const std::exception &e)
            {
                spdlog::error("[JsonSerializer : execute] Error when get data for parameter \"{}\" and function \"{}\": {}", lastParamName, functionName, e.what());
            }
        }(),...);
        // clang-format on

        // execute target function
        auto mappingItem = MappingList::shared()->find(functionName);

        if (mappingItem)
        {
            try
            {
                // clang-format off
                mappingItem.value().getTarget()(message, [key](auto const &response) {
                    // function return
                    json j;

                    if constexpr (std::is_same<Tr, std::nullopt_t>::value)
                    {
                        j["r"] = nullptr;
                    }
                    else if constexpr (std::is_same<Tr, void>::value)
                    {
                        j["r"] = nullptr;
                    }
                    else if (response.has_value())
                    {
                        j["r"] = std::any_cast<Tr>(response);
                    }
                    else
                    {
                        j["r"] = nullptr;
                    }

                    return CallbackList::shared()->execute(key, j.dump());
                });
                // clang-format on
            }
            catch (const std::exception &e)
            {
                spdlog::error("[JsonSerializer : execute] Error when cast value: {}", e.what());
                return CallbackList::shared()->execute(key, "");
            }
        }
        else
        {
            return CallbackList::shared()->execute(key, "");
        }
    }

    static std::string decodeFunctionName(const std::string &data)
    {
        try
        {
            json j = json::parse(data);

            if (j.contains("f"))
            {
                return j["f"].get<std::string>();
            }
            else
            {
                spdlog::error("[JsonSerializer : decodeFunctionName] Function name not found in json");
            }
        }
        catch (const std::exception &e)
        {
            spdlog::error("[JsonSerializer : decodeFunctionName] Error when parse json: {}", e.what());
        }

        return "";
    }

    template <typename T>
    static std::optional<T> decodeFunctionReturnValue(const std::string &data)
    {
        try
        {
            auto j = json::parse(data);

            if (j["r"].is_null())
            {
                return std::nullopt;
            }
            else
            {
                return j["r"].template get<T>();
            }
        }
        catch (const std::exception &e)
        {
            spdlog::error("[JsonSerializer : decodeFunctionReturnValue] Error when decode data: {}", e.what());
        }

        return std::nullopt;
    }

    template <typename T>
    static std::string encodeResponse(const std::any &data)
    {
        try
        {
            json j;
            j["r"] = std::any_cast<T>(data);
            return j.dump();
        }
        catch (const std::exception &e)
        {
            spdlog::error("[JsonSerializer : encodeResponse] Error when cast value: {}", e.what());
        }

        return "";
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
};

} // namespace serializer
} // namespace xplpc

#endif
