#include "fixtures/GeneralTest.hpp"
#include "fixtures/Requests.hpp"
#include "fixtures/TestHost.hpp"
#include "xplpc/core/BufferTable.hpp"
#include "gtest/gtest.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

TEST_F(GeneralTest, ConcurrencyAll)
{
    TestHost host;

    std::atomic<int> answered = 0;
    std::vector<std::thread> threads;

    for (int i = 0; i < 100; i++)
    {
        // clang-format off
        threads.emplace_back([&host, &answered, i] {
            const auto outcome = host.call(static_cast<std::uint64_t>(i), Requests::login());

            if (outcome.answered && outcome.value == Requests::loginAnswer())
            {
                answered++;
            }
        });
        // clang-format on
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    EXPECT_EQ(100, answered);
}

TEST_F(GeneralTest, ConcurrencyEveryThreadAttachesAHostOfItsOwn)
{
    // Attaching and detaching while other layers are calling is the ordinary life of a process with more than one host.

    std::atomic<int> answered = 0;
    std::vector<std::thread> threads;

    for (int i = 0; i < 16; i++)
    {
        // clang-format off
        threads.emplace_back([&answered, i] {
            for (int n = 0; n < 20; n++)
            {
                const auto name = "concurrency.own." + std::to_string(i);
                const auto response = R"({"r":")" + name + R"("})";

                TestHost host(true, TestHost::Serving::Inline, response);
                host.declare(name);

                if (host.call(1, Requests::login()).value == Requests::loginAnswer() && host.call(2, Requests::named(name)).value == response)
                {
                    answered++;
                }
            }
        });
        // clang-format on
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    EXPECT_EQ(16 * 20, answered);
}

TEST_F(GeneralTest, ConcurrencyMappingDeclarations)
{
    // The owners of a name are read by every routing decision while every host is free to keep declaring and retracting it.

    std::vector<std::unique_ptr<TestHost>> hosts;
    std::set<std::string> allowed = {""};

    for (int i = 0; i < 8; i++)
    {
        const auto response = R"({"r":"host-)" + std::to_string(i) + R"("})";

        hosts.push_back(std::make_unique<TestHost>(true, TestHost::Serving::Inline, response));
        allowed.insert(response);
    }

    std::atomic<int> unexpected = 0;
    std::vector<std::thread> threads;

    for (int i = 0; i < 8; i++)
    {
        // clang-format off
        threads.emplace_back([&hosts, &allowed, &unexpected, i] {
            auto &host = *hosts[i];

            for (int n = 0; n < 200; n++)
            {
                const auto name = "concurrency.mapping." + std::to_string(n % 4);

                host.declare(name);

                if (allowed.count(host.call(1, Requests::named(name)).value) == 0)
                {
                    unexpected++;
                }

                if (n % 3 == 0)
                {
                    host.retract(name);
                }
            }
        });
        // clang-format on
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    EXPECT_EQ(0, unexpected);
}

TEST_F(GeneralTest, ConcurrencyCallsRacingADetachAndCancelAreEachAnsweredOnce)
{
    // Allocation, answer, cancel and detach all race here, and every call still resolves exactly once or is cancelled and never resolves.

    auto server = std::make_unique<TestHost>(true, TestHost::Serving::Later, R"({"r":"later"})");
    server->declare("host.race");

    // The answers race the detach below, so they are let go from the start.
    server->releaseLater();

    TestHost caller;

    std::mutex mutex;
    std::set<std::uint64_t> inlineTokens;
    std::set<std::uint64_t> pendingTokens;
    std::set<std::uint64_t> cancelledTokens;
    std::vector<std::thread> threads;

    for (int i = 0; i < 8; i++)
    {
        // clang-format off
        threads.emplace_back([&caller, &mutex, &inlineTokens, &pendingTokens, &cancelledTokens, i] {
            for (int n = 0; n < 50; n++)
            {
                const auto token = static_cast<std::uint64_t>(i * 1000 + n);
                const auto outcome = caller.call(token, Requests::named("host.race"));

                std::unique_lock<std::mutex> lock(mutex);

                if (outcome.answered)
                {
                    inlineTokens.insert(token);
                    continue;
                }

                if (n % 2 == 0 && xplpc_cancel(caller.handle(), outcome.call))
                {
                    cancelledTokens.insert(token);
                    continue;
                }

                pendingTokens.insert(token);
            }
        });
        // clang-format on
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    server->detach();

    for (auto &thread : threads)
    {
        thread.join();
    }

    server.reset();

    ASSERT_TRUE(caller.waitForPosts(XPLPC_POST_ANSWER, pendingTokens.size(), 5000));

    std::multiset<std::uint64_t> posted;

    for (const auto &answer : caller.answers())
    {
        posted.insert(answer.id);
    }

    EXPECT_EQ(400u, inlineTokens.size() + pendingTokens.size() + cancelledTokens.size());
    EXPECT_EQ(pendingTokens.size(), posted.size());

    for (const auto token : pendingTokens)
    {
        EXPECT_EQ(1u, posted.count(token));
    }
}

TEST_F(GeneralTest, ConcurrencyBuffersRegisteredWhileTheHostDetachesAreAllGivenBack)
{
    // A buffer registered while its host detaches is either refused or released with the rest, never left to a layer that is gone.

    const auto before = xplpc::core::BufferTable::shared().count();
    std::vector<std::uint8_t> bytes = {1};

    for (int round = 0; round < 200; round++)
    {
        TestHost host;
        std::atomic<bool> started{false};

        // clang-format off
        std::thread registering([&host, &bytes, &started] {
            started = true;

            while (xplpc_buffer_register(host.handle(), bytes.data(), bytes.size()) != 0)
            {
            }
        });
        // clang-format on

        while (!started)
        {
            std::this_thread::yield();
        }

        host.detach();
        registering.join();
    }

    EXPECT_EQ(before, xplpc::core::BufferTable::shared().count());
}
