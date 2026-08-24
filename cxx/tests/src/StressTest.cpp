#include "fixtures/GeneralTest.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

using namespace xplpc::client;
using namespace xplpc::data;
using namespace xplpc::map;
using namespace xplpc::message;
using namespace xplpc::proxy;

constexpr int threadCount = 16;
constexpr int iterationCount = 400;

TEST_F(GeneralTest, StressUniqueIDIsDistinctUnderContention)
{
    // The counter is the one piece of shared state every call touches before anything else.

    std::mutex guard;
    std::set<std::string> keys;
    std::vector<std::thread> threads;

    for (int t = 0; t < threadCount; ++t)
    {
        // clang-format off
        threads.emplace_back([&guard, &keys]() {
            for (int i = 0; i < iterationCount; ++i)
            {
                auto key = UniqueID::generate();

                std::lock_guard<std::mutex> lock(guard);
                keys.insert(std::move(key));
            }
        });
        // clang-format on
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    EXPECT_EQ(static_cast<size_t>(threadCount * iterationCount), keys.size());
}

TEST_F(GeneralTest, StressCallbackListUnderContention)
{
    // Every thread races on the same small key space, so add and take overlap constantly.

    std::atomic<int> executed{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < threadCount; ++t)
    {
        // clang-format off
        threads.emplace_back([&executed, t]() {
            for (int i = 0; i < iterationCount; ++i)
            {
                const auto key = "stress-" + std::to_string(i % 32);

                CallbackList::shared()->add(key, [&executed](const std::string &) {
                    executed.fetch_add(1, std::memory_order_relaxed);
                });

                CallbackList::shared()->execute(key, "data");
                CallbackList::shared()->count();

                if (t % 2 == 0)
                {
                    CallbackList::shared()->remove(key);
                }
            }
        });
        // clang-format on
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    EXPECT_GT(executed.load(), 0);
}

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

TEST_F(GeneralTest, StressPlatformProxyListUnderContention)
{
    std::vector<std::thread> threads;

    for (int t = 0; t < threadCount; ++t)
    {
        // clang-format off
        threads.emplace_back([]() {
            for (int i = 0; i < 64; ++i)
            {
                PlatformProxyList::shared()->append(std::make_shared<NativePlatformProxy>());
                PlatformProxyList::shared()->count();

                PlatformProxyList::shared()->forEach([](const std::shared_ptr<PlatformProxy> &proxy) {
                    return proxy == nullptr;
                });
            }
        });
        // clang-format on
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    EXPECT_GT(PlatformProxyList::shared()->count(), 0);
}

TEST_F(GeneralTest, PlatformProxyListToleratesReentrantAppend)
{
    // The iteration runs over a snapshot, so a proxy is free to grow the list from inside the callback.

    auto initialCount = PlatformProxyList::shared()->count();
    auto visited = 0;

    // clang-format off
    PlatformProxyList::shared()->forEach([&visited](const std::shared_ptr<PlatformProxy> &) {
        visited++;
        PlatformProxyList::shared()->append(std::make_shared<NativePlatformProxy>());
        return false;
    });
    // clang-format on

    EXPECT_EQ(initialCount, visited);
    EXPECT_EQ(initialCount * 2, PlatformProxyList::shared()->count());
}

TEST_F(GeneralTest, StressClientCallsFromManyThreads)
{
    // A client call reaches the registries from every thread at once, which is the real usage pattern.

    std::atomic<int> answered{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < threadCount; ++t)
    {
        // clang-format off
        threads.emplace_back([&answered]() {
            for (int i = 0; i < 64; ++i)
            {
                auto request = Request{"sample.echo", Param<std::string>{"value", "stress"}};

                Client::call<std::string>(request, [&answered](const auto &response) {
                    if (response && response.value() == "stress")
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
    EXPECT_EQ(0, CallbackList::shared()->count());
}

TEST_F(GeneralTest, StressClientSyncAgainstAMappingRacingToAnswer)
{
    // The responder is handed to a worker that is already running, so the answer lands inside the window between the call being dispatched and its key being taken back.
    // Starting a thread inside the mapping instead makes that window unreachable, since creating one costs more than taking the key back does.
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

    // How many callers read the answer in time is decided by the race, and what has to hold whichever side wins the key is that nothing is left registered.
    EXPECT_EQ(0, CallbackList::shared()->count());
}
