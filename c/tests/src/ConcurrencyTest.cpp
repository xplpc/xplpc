#include "fixtures/GeneralTest.hpp"
#include "xplpc/c/platform.h"
#include "gtest/gtest.h"

#include <string>
#include <thread>
#include <vector>

void testLogin()
{
    std::string key = "1";
    std::string data = R"({"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]})";

    xplpc_native_call_proxy(const_cast<char *>(key.c_str()), key.size(), const_cast<char *>(data.c_str()), data.size());
}

TEST_F(GeneralTest, ConcurrencyAll)
{
    std::vector<std::thread> threads;

    for (int i = 0; i < 100; ++i)
    {
        threads.push_back(std::thread(testLogin));
    }

    for (auto &thread : threads)
    {
        thread.join();
    }
}

TEST_F(GeneralTest, ConcurrencyMappingDeclarations)
{
    // The declared names are read by every routing decision while the host is free to keep declaring and dropping them.

    xplpc_native_clear_mappings();

    std::vector<std::thread> threads;

    for (int i = 0; i < 16; ++i)
    {
        // clang-format off
        threads.emplace_back([i] {
            for (int n = 0; n < 200; ++n)
            {
                const auto name = "concurrency.mapping." + std::to_string(n % 8);
                xplpc_native_add_mapping(name.c_str(), name.size());

                testLogin();

                if (i == 0 && n % 64 == 0)
                {
                    xplpc_native_clear_mappings();
                }
            }
        });
        // clang-format on
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    xplpc_native_clear_mappings();
}
