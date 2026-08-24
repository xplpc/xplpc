#include "fixtures/GeneralTest.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace xplpc::client;
using namespace xplpc::core;
using namespace xplpc::data;
using namespace xplpc::map;
using namespace xplpc::message;
using namespace xplpc::proxy;

TEST_F(GeneralTest, CallbackListExecutesOnlyOnce)
{
    auto calls = 0;

    // clang-format off
    CallbackList::shared()->add("callback-once", [&calls](const std::string &) {
        calls++;
    });
    // clang-format on

    CallbackList::shared()->execute("callback-once", "");
    CallbackList::shared()->execute("callback-once", "");

    EXPECT_EQ(1, calls);
}

TEST_F(GeneralTest, CallbackListExecutesUnknownKeyWithoutBlocking)
{
    CallbackList::shared()->execute("callback-unknown", "");
    EXPECT_EQ(0, CallbackList::shared()->count());
}

TEST_F(GeneralTest, CallbackListRemovesPendingEntry)
{
    auto calls = 0;

    // clang-format off
    CallbackList::shared()->add("callback-removed", [&calls](const std::string &) {
        calls++;
    });
    // clang-format on

    EXPECT_EQ(1, CallbackList::shared()->count());

    CallbackList::shared()->remove("callback-removed");
    CallbackList::shared()->execute("callback-removed", "");

    EXPECT_EQ(0, calls);
    EXPECT_EQ(0, CallbackList::shared()->count());
}

TEST_F(GeneralTest, CallbackListIsTakenByASingleThread)
{
    std::atomic<int> calls{0};

    // clang-format off
    CallbackList::shared()->add("callback-concurrent", [&calls](const std::string &) {
        calls.fetch_add(1);
    });
    // clang-format on

    std::vector<std::thread> threads;

    for (auto i = 0; i < 16; ++i)
    {
        // clang-format off
        threads.emplace_back([]() {
            CallbackList::shared()->execute("callback-concurrent", "");
        });
        // clang-format on
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    EXPECT_EQ(1, calls.load());
}

TEST_F(GeneralTest, MappingListSurvivesClear)
{
    EXPECT_GT(MappingList::shared()->count(), 0);

    MappingList::shared()->clear();

    EXPECT_NE(nullptr, MappingList::shared());
    EXPECT_EQ(0, MappingList::shared()->count());
    EXPECT_FALSE(MappingList::shared()->has("sample.login"));

    // The mappings are restored so the remaining tests keep working.
    xplpc::custom::Mapping::initialize();

    EXPECT_TRUE(MappingList::shared()->has("sample.login"));
}

TEST_F(GeneralTest, PlatformProxyListPrependsToTheFront)
{
    auto first = std::make_shared<NativePlatformProxy>();
    auto second = std::make_shared<NativePlatformProxy>();

    PlatformProxyList::shared()->prepend(first);
    PlatformProxyList::shared()->prepend(second);

    std::vector<std::shared_ptr<PlatformProxy>> visited;

    // clang-format off
    PlatformProxyList::shared()->forEach([&visited](const std::shared_ptr<PlatformProxy> &proxy) {
        visited.push_back(proxy);
        return false;
    });
    // clang-format on

    ASSERT_GE(visited.size(), 2u);
    EXPECT_EQ(second, visited[0]);
    EXPECT_EQ(first, visited[1]);
}

TEST_F(GeneralTest, PlatformProxyListIgnoresEmptyItem)
{
    auto initialCount = PlatformProxyList::shared()->count();

    PlatformProxyList::shared()->append(nullptr);

    EXPECT_EQ(initialCount, PlatformProxyList::shared()->count());
}

TEST_F(GeneralTest, ClientDropsCallbackForUnknownFunction)
{
    auto pendingBefore = CallbackList::shared()->count();
    auto called = false;

    auto request = Request{"sample.function.that.does.not.exist"};

    // clang-format off
    Client::call<std::string>(request, [&called](const auto &response) {
        called = true;
        EXPECT_FALSE(response.has_value());
    });
    // clang-format on

    EXPECT_TRUE(called);
    EXPECT_EQ(pendingBefore, CallbackList::shared()->count());
}

TEST_F(GeneralTest, ClientAcceptsEmptyCallbackForUnknownFunction)
{
    auto request = Request{"sample.function.that.does.not.exist"};
    Client::call<std::string>(request, nullptr);

    EXPECT_EQ(0, CallbackList::shared()->count());
}

TEST_F(GeneralTest, CoreInitializeIsIdempotent)
{
    XPLPC::initialize();
    XPLPC::initialize();

    EXPECT_TRUE(XPLPC::isInitialized());
}

TEST_F(GeneralTest, DataViewWithoutAddressCopiesNothing)
{
    // A view that carries no address describes nothing to read, and a decoded one can arrive empty.

    std::vector<uint8_t> target(4, 7);

    xplpc::type::DataView{nullptr, 4}.copy(target.data());

    EXPECT_EQ(7, target[0]);
    EXPECT_EQ(7, target[3]);
}
