#pragma once

#ifdef XPLPC_SERIALIZER_JSON

#include "xplpc/data/MappingData.hpp"
#include "xplpc/message/Message.hpp"
#include "xplpc/serializer/JsonSerializerTypes.hpp"

#include "nlohmann/json.hpp"
using json = nlohmann::json;

#include "spdlog/spdlog.h"

#include <any>
#include <exception>
#include <map>
#include <memory>
#include <string>
#include <utility>
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
    static std::string execute(const std::string &data, const std::vector<std::string> &params)
    {
        json jsonResponse;
        json jsonData;

        try
        {
            jsonData = json::parse(data);
        }
        catch (std::exception &e)
        {
            spdlog::error("[JsonSerializer : execute] Error when parse json: {}", e.what());
            return std::move(jsonResponse.dump());
        }

        std::string functionName;

        if (jsonData.contains("f"))
        {
            functionName = jsonData["f"].get<std::string>();
        }
        else
        {
            return std::move(jsonResponse.dump());
            spdlog::error("[JsonSerializer : execute] Function name not found in json");
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
            catch (std::exception &e)
            {
                spdlog::error("[JsonSerializer : execute] Error when get data for parameter \"{}\" and function \"{}\": {}", lastParamName, functionName, e.what());
            }
        }(),...);
        // clang-format on

        auto mappingItem = MappingData::find(functionName);

        if (mappingItem)
        {
            try
            {
                // execute target function
                auto ret = mappingItem.value().getTarget()(message);

                // convert target function return
                if (ret.hasValue())
                {
                    jsonResponse["r"] = std::any_cast<Tr>(ret.getValue());
                }
                else
                {
                    jsonResponse["r"] = nullptr;
                }
            }
            catch (std::exception &e)
            {
                spdlog::error("[JsonSerializer : execute] Error when try to cast type and create return data: {}", e.what());
            }
        }

        return std::move(jsonResponse.dump());
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
        catch (std::exception &e)
        {
            spdlog::error("[JsonSerializer : decodeFunctionName] Error when parse json: {}", e.what());
            return "";
        }

        return "";
    }

    template <typename T>
    static T decodeFunctionReturnValue(const std::string &data)
    {
        auto j = json::parse(data);
        return j["r"].template get<T>();
    }

    template <typename T>
    static std::string encodeResponse(const std::any &data)
    {
        json j;

        try
        {
            j["r"] = std::any_cast<T>(data);
        }
        catch (std::exception &e)
        {
            spdlog::error("[JsonSerializer : encodeResponse] Error when try to cast type and create return data: {}", e.what());
        }

        return std::move(j.dump());
    }

    template <typename... Args>
    static std::string encodeRequest(const std::string &functionName, Args &&...params)
    {
        auto mappingItem = MappingData::find(functionName);

        json j;

        if (mappingItem)
        {
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

            return std::move(j.dump());
        }

        return std::move(j.dump());
    }
};

} // namespace serializer
} // namespace xplpc

#endif
