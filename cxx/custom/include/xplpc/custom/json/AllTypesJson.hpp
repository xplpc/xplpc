#pragma once

#ifdef XPLPC_SERIALIZER_JSON

#include "nlohmann/json.hpp"
#include "xplpc/custom/AllTypes.hpp"
#include "xplpc/custom/json/JsonField.hpp"
#include "xplpc/serializer/JsonSerializerTypes.hpp"

#include <stdexcept>

namespace nlohmann
{
template <>
struct adl_serializer<AllTypes>
{
    static constexpr int maxDepth = 64;

    static AllTypes from_json(const json &j)
    {
        // A list of items carrying lists of their own is decoded by recursion, so the depth a caller sends is bounded before it can exhaust the stack.
        static thread_local int depth = 0;

        struct Level
        {
            Level()
            {
                if (depth >= maxDepth)
                {
                    throw std::invalid_argument("Field typeList is nested too deep");
                }

                ++depth;
            }

            ~Level()
            {
                --depth;
            }
        };

        const Level level;

        return {
            JsonField::read<int8_t>(j, "typeInt8", 0),
            JsonField::read<int16_t>(j, "typeInt16", 0),
            JsonField::read<int32_t>(j, "typeInt32", 0),
            JsonField::read<int64_t>(j, "typeInt64", 0),
            JsonField::read<float_t>(j, "typeFloat32", 0),
            JsonField::read<double_t>(j, "typeFloat64", 0),
            JsonField::read<bool>(j, "typeBool", false),
            JsonField::read<std::optional<bool>>(j, "typeOptional", std::nullopt),
            JsonField::read<std::vector<AllTypes>>(j, "typeList", {}),
            JsonField::read<std::unordered_map<std::string, std::string>>(j, "typeMap", {}),
            JsonField::read<std::chrono::system_clock::time_point>(j, "typeDateTime", {}),
            JsonField::read<char>(j, "typeChar", 'a'),
            JsonField::read<char16_t>(j, "typeChar16", u'a'),
            JsonField::read<char32_t>(j, "typeChar32", U'a'),
            JsonField::read<wchar_t>(j, "typeWchar", L'a'),
            JsonField::read<std::string>(j, "typeString", {}),
        };
    }

    static void to_json(json &j, AllTypes o)
    {
        j = nlohmann::json{
            {"typeInt8", o.typeInt8},
            {"typeInt16", o.typeInt16},
            {"typeInt32", o.typeInt32},
            {"typeInt64", o.typeInt64},
            {"typeFloat32", o.typeFloat32},
            {"typeFloat64", o.typeFloat64},
            {"typeBool", o.typeBool},
            {"typeOptional", o.typeOptional},
            {"typeList", o.typeList},
            {"typeMap", o.typeMap},
            {"typeDateTime", o.typeDateTime},
            {"typeChar", o.typeChar},
            {"typeChar16", o.typeChar16},
            {"typeChar32", o.typeChar32},
            {"typeWchar", o.typeWchar},
            {"typeString", o.typeString},
        };
    }
};
} // namespace nlohmann

#endif
