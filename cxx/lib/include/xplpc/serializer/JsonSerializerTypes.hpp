#ifdef XPLPC_SERIALIZER_JSON

#pragma once

#include "xplpc/type/DataView.hpp"

#include <chrono>
#include <optional>

#include "nlohmann/json.hpp"
using json = nlohmann::json;

NLOHMANN_JSON_NAMESPACE_BEGIN

// OPTIONAL TYPE

template <typename T>
struct adl_serializer<std::optional<T>>
{
    static void to_json(json &j, const std::optional<T> &opt)
    {
        if (opt == std::nullopt)
        {
            j = nullptr;
        }
        else
        {
            j = *opt;
        }
    }

    static void from_json(const json &j, std::optional<T> &opt)
    {
        if (j.is_null())
        {
            opt = std::nullopt;
        }
        else
        {
            opt = j.get<T>();
        }
    }
};

// CHRONO (DATE AND TIME) TYPE

template <typename Clock, typename Duration>
struct adl_serializer<std::chrono::time_point<Clock, Duration>>
{
    static void to_json(json &j, const std::chrono::time_point<Clock, Duration> &tp)
    {
        j = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
    }

    static void from_json(const json &j, std::chrono::time_point<Clock, Duration> &value)
    {
        if (j.is_null())
        {
            auto dur = std::chrono::milliseconds(0);
            value = std::chrono::time_point<std::chrono::system_clock>(dur);
        }
        else
        {
            auto dur = std::chrono::milliseconds(j);
            value = std::chrono::time_point<std::chrono::system_clock>(dur);
        }
    }
};

// DATAVIEW TYPE

template <>
struct adl_serializer<xplpc::type::DataView>
{
    static void to_json(json &j, const xplpc::type::DataView &dataView)
    {
        j = nlohmann::json{
            {"ptr", reinterpret_cast<std::uintptr_t>(dataView.ptr())},
            {"size", dataView.size()},
        };
    };

    static xplpc::type::DataView from_json(const json &j)
    {
        if (j.is_null())
        {
            return xplpc::type::DataView{0, 0};
        }
        else
        {
            return xplpc::type::DataView{
                reinterpret_cast<uint8_t *>(j["ptr"].get<std::uintptr_t>()),
                j["size"].get<size_t>(),
            };
        }
    }
};

NLOHMANN_JSON_NAMESPACE_END

#endif
