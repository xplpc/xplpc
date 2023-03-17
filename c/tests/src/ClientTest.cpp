#include "fixtures/GeneralTest.hpp"
#include "xplpc/c/platform.h"
#include "gtest/gtest.h"

#include <string>
#include <thread>

TEST_F(GeneralTest, ClientTestLogin)
{
    std::string key = "1";
    std::string data = R"({"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]})";

    xplpc_native_call_proxy(const_cast<char *>(key.c_str()), key.size(), const_cast<char *>(data.c_str()), data.size());
}

TEST_F(GeneralTest, ClientTestLoginAsync)
{
    std::string key = "1";
    std::string data = R"({"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]})";

    // clang-format off
    std::thread([=] {
        xplpc_native_call_proxy(const_cast<char *>(key.c_str()), key.size(), const_cast<char *>(data.c_str()), data.size());
    }).join();
    // clang-format on
}
