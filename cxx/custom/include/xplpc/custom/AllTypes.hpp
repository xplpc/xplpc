#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class AllTypes
{
public:
    int8_t typeInt8;
    int16_t typeInt16;
    int32_t typeInt32;
    int64_t typeInt64;
    float typeFloat32;
    double typeFloat64;
    bool typeBool;
    std::optional<bool> typeOptional;
    std::vector<AllTypes> typeList;
    std::unordered_map<std::string, std::string> typeMap;
    std::chrono::system_clock::time_point typeDateTime;
    char typeChar;
    char16_t typeChar16;
    char32_t typeChar32;
    wchar_t typeWchar;
    std::string typeString;

    AllTypes(
        int8_t typeInt8_,
        int16_t typeInt16_,
        int32_t typeInt32_,
        int64_t typeInt64_,
        float typeFloat32_,
        double typeFloat64_,
        bool typeBool_,
        std::optional<bool> typeOptional_,
        std::vector<AllTypes> typeList_,
        std::unordered_map<std::string, std::string> typeMap_,
        std::chrono::system_clock::time_point typeDateTime_,
        char typeChar_,
        char16_t typeChar16_,
        char32_t typeChar32_,
        wchar_t typeWchar_,
        std::string typeString_)
        : typeInt8(typeInt8_)
        , typeInt16(typeInt16_)
        , typeInt32(typeInt32_)
        , typeInt64(typeInt64_)
        , typeFloat32(typeFloat32_)
        , typeFloat64(typeFloat64_)
        , typeBool(typeBool_)
        , typeOptional(typeOptional_)
        , typeList(std::move(typeList_))
        , typeMap(std::move(typeMap_))
        , typeDateTime(typeDateTime_)
        , typeChar(typeChar_)
        , typeChar16(typeChar16_)
        , typeChar32(typeChar32_)
        , typeWchar(typeWchar_)
        , typeString(std::move(typeString_))
    {
    }
};
