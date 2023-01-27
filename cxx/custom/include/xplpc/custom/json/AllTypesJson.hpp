#ifdef XPLPC_SERIALIZER_JSON

#pragma once

#include "nlohmann/json.hpp"
#include "xplpc/custom/AllTypes.hpp"
#include "xplpc/serializer/JsonSerializerTypes.hpp"

namespace nlohmann
{
template <>
struct adl_serializer<AllTypes>
{
    static AllTypes from_json(const json &j)
    {
        return {
            j.contains("typeInt8") ? j["typeInt8"].get<int8_t>() : static_cast<int8_t>(0),
            j.contains("typeInt16") ? j["typeInt16"].get<int16_t>() : static_cast<int16_t>(0),
            j.contains("typeInt32") ? j["typeInt32"].get<int32_t>() : 0,
            j.contains("typeInt64") ? j["typeInt64"].get<int64_t>() : 0,
            j.contains("typeFloat32") ? j["typeFloat32"].get<float_t>() : 0,
            j.contains("typeFloat64") ? j["typeFloat64"].get<double_t>() : 0,
            j.contains("typeBool") ? j["typeBool"].get<bool>() : false,
            j.contains("typeOptional") ? j["typeOptional"].get<std::optional<bool>>() : std::nullopt,
            j.contains("typeList") ? j["typeList"].get<std::vector<AllTypes>>() : std::vector<AllTypes>{},
            j.contains("typeMap") ? j["typeMap"].get<std::unordered_map<std::string, std::string>>() : std::unordered_map<std::string, std::string>{},
            j.contains("typeDateTime") ? j["typeDateTime"].get<std::chrono::system_clock::time_point>() : std::chrono::system_clock::time_point{},
            j.contains("typeChar") ? j["typeChar"].get<char>() : 'a',
            j.contains("typeChar16") ? j["typeChar16"].get<char16_t>() : u'a',
            j.contains("typeChar32") ? j["typeChar32"].get<char32_t>() : U'a',
            j.contains("typeWchar") ? j["typeWchar"].get<wchar_t>() : L'a',
            j.contains("typeString") ? j["typeString"].get<std::string>() : std::string{},
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
