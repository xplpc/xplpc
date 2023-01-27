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
        : typeInt8(std::move(typeInt8_))
        , typeInt16(std::move(typeInt16_))
        , typeInt32(std::move(typeInt32_))
        , typeInt64(std::move(typeInt64_))
        , typeFloat32(std::move(typeFloat32_))
        , typeFloat64(std::move(typeFloat64_))
        , typeBool(std::move(typeBool_))
        , typeOptional(std::move(typeOptional_))
        , typeList(std::move(typeList_))
        , typeMap(std::move(typeMap_))
        , typeDateTime(std::move(typeDateTime_))
        , typeChar(std::move(typeChar_))
        , typeChar16(std::move(typeChar16_))
        , typeChar32(std::move(typeChar32_))
        , typeWchar(std::move(typeWchar_))
        , typeString(std::move(typeString_))
    {
    }
};
