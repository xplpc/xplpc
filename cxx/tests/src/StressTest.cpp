#include "fixtures/GeneralTest.hpp"
#include "fixtures/ScopedLayer.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace xplpc::client;
using namespace xplpc::data;
using namespace xplpc::map;
using namespace xplpc::message;

constexpr int threadCount = 16;
constexpr int iterationCount = 400;

TEST_F(GeneralTest, StressMappingListUnderContention)
{
    std::vector<std::thread> threads;

    for (int t = 0; t < threadCount; ++t)
    {
        // clang-format off
        threads.emplace_back([t]() {
            for (int i = 0; i < iterationCount; ++i)
            {
                const auto name = "stress.mapping." + std::to_string(i % 16);

                MappingList::shared()->add(name, Map::create<std::string, std::string>({"value"}, [](const Message &, const Response &r) {
                    r(std::string{"ok"});
                }));

                MappingList::shared()->find(name);
                MappingList::shared()->has(name);
                MappingList::shared()->count();

                if (t == 0 && i % 64 == 0)
                {
                    MappingList::shared()->clear();
                }
            }
        });
        // clang-format on
    }

    for (auto &thread : threads)
    {
        thread.join();
    }
}

TEST_F(GeneralTest, StressAClearRacingAnAddNeverLeavesANameRoutedToNothing)
{
    // A host owns the name below c++, so a call reaches either the c++ mapping or the host, and an empty answer means the router still routes it to a mapping c++ no longer holds.

    ScopedLayer host(ScopedLayer::answering("from-host"));
    host.declare("stress.race");

    for (int round = 0; round < 2000; ++round)
    {
        std::atomic<bool> start{false};

        // clang-format off
        std::thread adding([&start]() {
            while (!start.load())
            {
            }

            MappingList::shared()->add("stress.race", Map::create<std::string>({}, [](const Message &, const Response &r) {
                r(std::string{"from-cxx"});
            }));
        });

        std::thread clearing([&start]() {
            while (!start.load())
            {
            }

            MappingList::shared()->clear();
        });
        // clang-format on

        start.store(true);
        adding.join();
        clearing.join();

        const auto expected = MappingList::shared()->has("stress.race") ? "from-cxx" : "from-host";
        const auto response = Client::callSync<std::string>(Request{"stress.race"});

        ASSERT_EQ(expected, response.value_or("nothing")) << "Round " << round;
    }

    MappingList::shared()->clear();
}

TEST_F(GeneralTest, StressClientCallsFromManyThreads)
{
    // A client call reaches the registries from every thread at once, which is the real usage pattern.

    std::atomic<int> answered{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < threadCount; ++t)
    {
        // clang-format off
        threads.emplace_back([&answered, t]() {
            for (int i = 0; i < 64; ++i)
            {
                // Every call sends a value of its own, so an answer handed to the wrong caller is counted as missing.
                const auto value = "stress-" + std::to_string(t) + "-" + std::to_string(i);
                auto request = Request{"sample.echo", Param<std::string>{"value", value}};

                Client::call<std::string>(request, [&answered, value](const auto &response) {
                    if (response && response.value() == value)
                    {
                        answered.fetch_add(1, std::memory_order_relaxed);
                    }
                });
            }
        });
        // clang-format on
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    EXPECT_EQ(threadCount * 64, answered.load());
    EXPECT_EQ(0u, Router::shared().pending());
}

TEST_F(GeneralTest, StressClientSyncAgainstAMappingRacingToAnswer)
{
    // The responder is handed to a worker that is already running, so the answer lands inside the window between the call being dispatched and it being cancelled.
    // Starting a thread inside the mapping instead makes that window unreachable, since creating one costs more than cancelling does.
    // What the answer writes into has to survive the frame that asked for it and be safe to read while it is being written.

    std::mutex queueGuard;
    std::vector<Response> queue;
    std::atomic<bool> running{true};

    // clang-format off
    std::thread worker([&queueGuard, &queue, &running]() {
        while (running.load(std::memory_order_relaxed))
        {
            Response responder;

            {
                std::lock_guard<std::mutex> lock(queueGuard);

                if (!queue.empty())
                {
                    responder = std::move(queue.back());
                    queue.pop_back();
                }
            }

            if (responder)
            {
                responder(std::string{"racing"});
            }
        }
    });

    MappingList::shared()->add("test.sync.racing", Map::create<std::string>({}, [&queueGuard, &queue](const Message &, const Response &r) {
        std::lock_guard<std::mutex> lock(queueGuard);
        queue.push_back(r);
    }));
    // clang-format on

    std::vector<std::thread> threads;

    for (int t = 0; t < threadCount; ++t)
    {
        // clang-format off
        threads.emplace_back([]() {
            for (int i = 0; i < 256; ++i)
            {
                const auto response = Client::callSync<std::string>(Request{"test.sync.racing"});

                EXPECT_TRUE(!response.has_value() || response.value() == "racing");
            }
        });
        // clang-format on
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    running.store(false, std::memory_order_relaxed);
    worker.join();

    // How many callers read the answer in time is decided by the race, and what has to hold whichever side takes the call is that nothing is left waiting.
    EXPECT_EQ(0u, Router::shared().pending());
}
