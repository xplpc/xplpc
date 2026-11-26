#pragma once

#ifdef XPLPC_SERIALIZER_JSON

#include "nlohmann/json.hpp"
#include "xplpc/serializer/JsonValueGuard.hpp"

#include <stdexcept>
#include <string>

// Reads one field of a consumer type through the check the library applies to a parameter, since nlohmann alone converts whatever number it finds.
class JsonField
{
public:
    template <typename T>
    static T read(const nlohmann::json &j, const char *key, T fallback)
    {
        // A value that is not an object carries none of the fields, so it is refused rather than read as one holding every default.
        if (!j.is_object())
        {
            throw std::invalid_argument("The value is not an object");
        }

        if (!j.contains(key))
        {
            return fallback;
        }

        const auto &value = j[key];

        if (!xplpc::serializer::JsonValueGuard::fits<T>(value))
        {
            throw std::invalid_argument(std::string{"Field "} + key + " has another type");
        }

        return value.get<T>();
    }
};

#endif
