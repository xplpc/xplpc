#include "fixtures/GeneralTest.hpp"
#include "fixtures/ProxyCallbackRecorder.hpp"
#include "xplpc/c/platform.h"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <stdexcept>
#include <string>
#include <thread>

TEST_F(GeneralTest, ClientTestLogin)
{
    xplpc_core_initialize(true, nullptr, nullptr, nullptr, &ProxyCallbackRecorder::onNativeProxyCallback, nullptr, nullptr);
    ProxyCallbackRecorder::reset();

    const std::string key = "1";
    const std::string data = R"({"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]})";

    xplpc_native_call_proxy(key.c_str(), key.size(), data.c_str(), data.size());

    EXPECT_EQ(key, ProxyCallbackRecorder::key);
    EXPECT_EQ(R"({"r":"LOGGED-WITH-REMEMBER"})", ProxyCallbackRecorder::data);
}

TEST_F(GeneralTest, ClientTestLoginAsync)
{
    xplpc_core_initialize(true, nullptr, nullptr, nullptr, &ProxyCallbackRecorder::onNativeProxyCallback, nullptr, nullptr);
    ProxyCallbackRecorder::reset();

    const std::string key = "1";
    const std::string data = R"({"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]})";

    // clang-format off
    std::thread([&key, &data] {
        xplpc_native_call_proxy(key.c_str(), key.size(), data.c_str(), data.size());
    }).join();
    // clang-format on

    EXPECT_EQ(key, ProxyCallbackRecorder::key);
    EXPECT_EQ(R"({"r":"LOGGED-WITH-REMEMBER"})", ProxyCallbackRecorder::data);
}

TEST_F(GeneralTest, ClientCarriesEveryUtf8WidthThroughTheAbi)
{
    // The abi carries a pointer and a size and never a terminator, so a payload wider than ascii has to survive byte for byte.

    xplpc_core_initialize(true, nullptr, nullptr, nullptr, &ProxyCallbackRecorder::onNativeProxyCallback, nullptr, nullptr);
    ProxyCallbackRecorder::reset();

    const std::string key = "1";
    const std::string value = "acentuacao \u00e7 \u65e5\u672c\u8a9e \U0001f600";
    const std::string data = R"({"f":"sample.echo","p":[{"n":"value","v":")" + value + R"("}]})";

    xplpc_native_call_proxy(key.c_str(), key.size(), data.c_str(), data.size());

    EXPECT_EQ(key, ProxyCallbackRecorder::key);
    EXPECT_EQ(R"({"r":")" + value + R"("})", ProxyCallbackRecorder::data);
}

TEST_F(GeneralTest, ClientAnswersEmptyForAFunctionNothingOwns)
{
    // Nothing owns this name, so the caller is answered with the empty value rather than left waiting.

    xplpc_core_initialize(true, nullptr, nullptr, nullptr, &ProxyCallbackRecorder::onNativeProxyCallback, nullptr, nullptr);
    xplpc_native_clear_mappings();
    ProxyCallbackRecorder::reset();

    const std::string key = "1";
    const std::string data = R"({"f":"not.found","p":[]})";

    xplpc_native_call_proxy(key.c_str(), key.size(), data.c_str(), data.size());

    EXPECT_EQ(key, ProxyCallbackRecorder::key);
    EXPECT_EQ("", ProxyCallbackRecorder::data);
}

TEST_F(GeneralTest, ClientAnswersWhenAMappingRaises)
{
    // A mapping is free to raise something that does not derive from std::exception, and the boundary that stops it is not the one holding the key, so the caller would otherwise wait for the life of the process.

    xplpc_core_initialize(true, nullptr, nullptr, nullptr, &ProxyCallbackRecorder::onNativeProxyCallback, nullptr, nullptr);
    ProxyCallbackRecorder::reset();

    // clang-format off
    xplpc::data::MappingList::shared()->add("test.raises", xplpc::map::Map::create<std::string>({}, [](const xplpc::message::Message &, const xplpc::message::Response &) {
        throw 42;
    }));
    // clang-format on

    const std::string key = "1";
    const std::string data = R"({"f":"test.raises","p":[]})";

    xplpc_native_call_proxy(key.c_str(), key.size(), data.c_str(), data.size());

    EXPECT_EQ(key, ProxyCallbackRecorder::key);
    EXPECT_EQ("", ProxyCallbackRecorder::data);
}

TEST_F(GeneralTest, ClientAnswersEveryPendingCallWhenTheLibraryIsFinalized)
{
    // A mapping that never answers leaves a call pending, and after finalize nothing can ever resolve it, so the caller is answered while the host can still be reached.

    xplpc_core_initialize(
        true,
        nullptr,
        nullptr,
        nullptr,
        &ProxyCallbackRecorder::onNativeProxyCallback,
        nullptr,
        &ProxyCallbackRecorder::onNativeProxyCallbackFromThread);

    ProxyCallbackRecorder::reset();

    // clang-format off
    xplpc::data::MappingList::shared()->add("test.never.answers", xplpc::map::Map::create<std::string>({}, [](const xplpc::message::Message &, const xplpc::message::Response &) {
    }));
    // clang-format on

    const std::string key = "pending";
    const std::string data = R"({"f":"test.never.answers","p":[]})";

    xplpc_native_call_proxy(key.c_str(), key.size(), data.c_str(), data.size());

    EXPECT_EQ(1u, xplpc::data::CallbackList::shared()->count());

    xplpc_core_finalize();

    ASSERT_TRUE(ProxyCallbackRecorder::waitForAnswerCountFromThread(5000, 1));
    EXPECT_EQ(key, ProxyCallbackRecorder::takeKeyFromThread());
    EXPECT_EQ("", ProxyCallbackRecorder::takeDataFromThread());
    EXPECT_EQ(0u, xplpc::data::CallbackList::shared()->count());
}

TEST_F(GeneralTest, ClientRunsNoMappingAfterTheLibraryIsFinalized)
{
    // Routing happens inside the core, so a call arriving after finalize would still reach a c++ mapping and run whatever it does, with the answer dropped afterwards.

    xplpc_core_initialize(true, nullptr, nullptr, nullptr, &ProxyCallbackRecorder::onNativeProxyCallback, nullptr, nullptr);
    ProxyCallbackRecorder::reset();

    static bool executed = false;
    executed = false;

    // clang-format off
    xplpc::data::MappingList::shared()->add("test.after.finalize", xplpc::map::Map::create<std::string>({}, [](const xplpc::message::Message &, const xplpc::message::Response &r) {
        executed = true;
        r(std::string("ran"));
    }));
    // clang-format on

    xplpc_core_finalize();

    const std::string key = "after-finalize";
    const std::string data = R"({"f":"test.after.finalize","p":[]})";

    xplpc_native_call_proxy(key.c_str(), key.size(), data.c_str(), data.size());

    EXPECT_FALSE(executed);
}
