#pragma once

#ifdef XPLPC_SERIALIZER_JSON

#include "xplpc/serializer/JsonValueGuard.hpp"
#include "xplpc/type/DataView.hpp"
#include "xplpc/util/Log.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <stdexcept>

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
            value = std::chrono::time_point<Clock, Duration>();
            return;
        }

        // A consumer type reaches this through its own from_json, past the guard the parameters go through, so the range is checked here as well.
        if (!xplpc::serializer::JsonValueGuard::fitsIntegral<std::int64_t>(j) || !xplpc::serializer::JsonValueGuard::holdsMilliseconds<Duration>(j.get<std::int64_t>()))
        {
            throw std::out_of_range("The timestamp does not fit the declared clock");
        }

        const auto milliseconds = std::chrono::milliseconds(j.get<std::int64_t>());
        value = std::chrono::time_point<Clock, Duration>(std::chrono::duration_cast<Duration>(milliseconds));
    }
};

template <>
struct adl_serializer<xplpc::type::DataView>
{
    static void to_json(json &j, const xplpc::type::DataView &dataView)
    {
        j = nlohmann::json{{"buffer", dataView.handle()}};
    }

    static xplpc::type::DataView from_json(const json &j)
    {
        // A handle is read only from an unsigned integer that fits, and anything else names no buffer, which is what every reader then refuses.

        if (!j.is_object() || !j.contains("buffer") || !xplpc::serializer::JsonValueGuard::fitsIntegral<xplpc::core::Handle>(j["buffer"]))
        {
            xplpc::util::Log::e("[DataView : fromJson] Value is not a data view");
            xplpc::util::Log::d("[DataView : fromJson] Value is not a data view: {}", j.type_name());

            return xplpc::type::DataView{};
        }

        return xplpc::type::DataView{j["buffer"].get<xplpc::core::Handle>()};
    }
};

NLOHMANN_JSON_NAMESPACE_END

#endif
