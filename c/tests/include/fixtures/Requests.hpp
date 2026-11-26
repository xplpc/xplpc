#pragma once

#include <string>

// The documents the suite sends, written once so every test reaches the same mapping the same way.
class Requests
{
public:
    static const std::string &login()
    {
        static const std::string value = R"({"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]})";
        return value;
    }

    static const std::string &loginAnswer()
    {
        static const std::string value = R"({"r":"LOGGED-WITH-REMEMBER"})";
        return value;
    }

    static std::string named(const std::string &name)
    {
        return R"({"f":")" + name + R"(","p":[]})";
    }
};
