#pragma once

#ifdef XPLPC_SERIALIZER_JSON

#include "xplpc/type/DataView.hpp"

#include <chrono>
#include <cstddef>
#include <optional>

#include "nlohmann/json.hpp"
using json = nlohmann::json;

NLOHMANN_JSON_NAMESPACE_BEGIN

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

template <>
struct adl_serializer<xplpc::type::DataView>
{
    static void to_json(json &j, const xplpc::type::DataView &dataView)
    {
        j = nlohmann::json{
            {"ptr", reinterpret_cast<std::uintptr_t>(dataView.ptr())},
            {"size", dataView.size()},
        };
    }

    static xplpc::type::DataView from_json(const json &j)
    {
        if (j.is_null() || !j.contains("ptr") || !j.contains("size"))
        {
            return xplpc::type::DataView{nullptr, 0};
        }

        auto ptr = reinterpret_cast<uint8_t *>(j["ptr"].get<std::uintptr_t>());

        // The address comes from another process boundary, and a null one can never carry bytes.
        if (!ptr)
        {
            return xplpc::type::DataView{nullptr, 0};
        }

        return xplpc::type::DataView{ptr, j["size"].get<size_t>()};
    }
};

NLOHMANN_JSON_NAMESPACE_END

#endif
