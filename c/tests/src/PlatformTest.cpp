#include "xplpc/c/platform.h"
#include "fixtures/DetachedAnswerMapping.hpp"
#include "fixtures/GeneralTest.hpp"
#include "fixtures/ProxyCallbackRecorder.hpp"
#include "gtest/gtest.h"

#include "xplpc/client/Client.hpp"
#include "xplpc/data/CallbackList.hpp"
#include "xplpc/data/MappingList.hpp"
#include "xplpc/map/Map.hpp"

#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

std::string ProxyCallbackRecorder::key;
std::string ProxyCallbackRecorder::data;
std::string ProxyCallbackRecorder::callKey;
std::string ProxyCallbackRecorder::callData;
std::string ProxyCallbackRecorder::callKeyFromThread;
std::string ProxyCallbackRecorder::callDataFromThread;
std::string ProxyCallbackRecorder::keyFromThread;
std::string ProxyCallbackRecorder::dataFromThread;
size_t ProxyCallbackRecorder::answersFromThread = 0;
std::mutex ProxyCallbackRecorder::mutex;
std::condition_variable ProxyCallbackRecorder::condition;

TEST_F(GeneralTest, PlatformInitializeIsIdempotent)
{
    xplpc_core_initialize(true, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    xplpc_core_initialize(true, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

    EXPECT_TRUE(xplpc_core_is_initialized());
}

TEST_F(GeneralTest, PlatformCallProxyRespectsBufferSize)
{
    xplpc_core_initialize(true, nullptr, nullptr, nullptr, &ProxyCallbackRecorder::onNativeProxyCallback, nullptr, nullptr);
    ProxyCallbackRecorder::reset();

    // The buffers are deliberately not null terminated, so any use of strlen would read past the end.
    const std::string key = "call-key";
    const std::string data = R"({"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]})";

    std::vector<char> keyBuffer(key.begin(), key.end());
    std::vector<char> dataBuffer(data.begin(), data.end());

    xplpc_native_call_proxy(keyBuffer.data(), keyBuffer.size(), dataBuffer.data(), dataBuffer.size());

    EXPECT_EQ(key, ProxyCallbackRecorder::key);
    EXPECT_EQ(R"({"r":"LOGGED-WITH-REMEMBER"})", ProxyCallbackRecorder::data);
}

TEST_F(GeneralTest, PlatformRefusesBuffersWithoutAnAddress)
{
    // The abi carries a pointer and a size, and building a string from address zero is what takes a host down.

    xplpc_core_initialize(true, nullptr, nullptr, nullptr, &ProxyCallbackRecorder::onNativeProxyCallback, nullptr, nullptr);
    ProxyCallbackRecorder::reset();

    const std::string key = "call-key";
    std::vector<char> keyBuffer(key.begin(), key.end());

    xplpc_native_call_proxy(nullptr, 8, keyBuffer.data(), keyBuffer.size());
    xplpc_native_call_proxy(keyBuffer.data(), keyBuffer.size(), nullptr, 8);
    xplpc_native_call_proxy_callback(nullptr, 8, keyBuffer.data(), keyBuffer.size());
    xplpc_native_call_proxy_callback(keyBuffer.data(), keyBuffer.size(), nullptr, 8);
    xplpc_native_add_mapping(nullptr, 8);

    EXPECT_TRUE(ProxyCallbackRecorder::key.empty());
    EXPECT_TRUE(ProxyCallbackRecorder::data.empty());
    EXPECT_TRUE(xplpc_core_is_initialized());
}

TEST_F(GeneralTest, PlatformStopsAnExceptionAtTheNativeBoundary)
{
    // A c++ exception leaving an extern "C" function is undefined in the abi, so a callback that raises has to stop here.

    xplpc_core_initialize(true, nullptr, nullptr, nullptr, &ProxyCallbackRecorder::onNativeProxyCallback, nullptr, nullptr);

    const std::string key = "raising-key";
    const std::string data = R"({"r":"anything"})";

    // clang-format off
    xplpc::data::CallbackList::shared()->add(key, [](const std::string &) {
        throw std::runtime_error("the consumer callback raised");
    });
    // clang-format on

    std::vector<char> keyBuffer(key.begin(), key.end());
    std::vector<char> dataBuffer(data.begin(), data.end());

    xplpc_native_call_proxy_callback(keyBuffer.data(), keyBuffer.size(), dataBuffer.data(), dataBuffer.size());

    EXPECT_TRUE(xplpc_core_is_initialized());
    EXPECT_EQ(0, xplpc::data::CallbackList::shared()->count());
}

TEST_F(GeneralTest, PlatformCallProxyAnswersEmptyForUnknownFunction)
{
    xplpc_core_initialize(true, nullptr, nullptr, nullptr, &ProxyCallbackRecorder::onNativeProxyCallback, nullptr, nullptr);
    xplpc_native_clear_mappings();
    ProxyCallbackRecorder::reset();

    const std::string key = "missing-key";
    const std::string data = R"({"f":"sample.function.that.does.not.exist","p":[]})";

    xplpc_native_call_proxy(key.c_str(), key.size(), data.c_str(), data.size());

    EXPECT_EQ(key, ProxyCallbackRecorder::key);
    EXPECT_EQ("", ProxyCallbackRecorder::data);
}

TEST_F(GeneralTest, PlatformRoutesDeclaredMappingToHost)
{
    // A name the host declared is resolved without asking it, so the call reaches the host on any thread.

    xplpc_core_initialize(true, nullptr, nullptr, &ProxyCallbackRecorder::onNativeProxyCall, &ProxyCallbackRecorder::onNativeProxyCallback, nullptr, nullptr);
    xplpc_native_clear_mappings();
    ProxyCallbackRecorder::reset();

    const std::string name = "host.function";
    xplpc_native_add_mapping(name.c_str(), name.size());

    const std::string key = "host-key";
    const std::string data = R"({"f":"host.function","p":[]})";

    xplpc_native_call_proxy(key.c_str(), key.size(), data.c_str(), data.size());

    EXPECT_EQ(data, ProxyCallbackRecorder::callData);
    EXPECT_EQ(key, ProxyCallbackRecorder::key);
    EXPECT_EQ(R"({"r":"from-host"})", ProxyCallbackRecorder::data);
}

TEST_F(GeneralTest, PlatformRoutesAContestedNameToTheHostRatherThanTheCore)
{
    // The host proxy is prepended, so a name both sides claim is answered by the host and the core mapping never runs.

    xplpc_core_initialize(true, nullptr, nullptr, &ProxyCallbackRecorder::onNativeProxyCall, &ProxyCallbackRecorder::onNativeProxyCallback, nullptr, nullptr);
    xplpc_native_clear_mappings();
    ProxyCallbackRecorder::reset();

    const std::string name = "sample.login";
    xplpc_native_add_mapping(name.c_str(), name.size());

    const std::string key = "contested-key";
    const std::string data = R"({"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]})";

    xplpc_native_call_proxy(key.c_str(), key.size(), data.c_str(), data.size());

    EXPECT_EQ(data, ProxyCallbackRecorder::callData);
    EXPECT_EQ(R"({"r":"from-host"})", ProxyCallbackRecorder::data);
}

TEST_F(GeneralTest, NestedCoreMappingReachesTheHostAndAnswersTheOuterCaller)
{
    // A core mapping needs a value only the host has, so the outer answer is built from an inner call that crossed the abi.

    xplpc_core_initialize(true, nullptr, nullptr, &ProxyCallbackRecorder::onNativeProxyCall, &ProxyCallbackRecorder::onNativeProxyCallback, nullptr, nullptr);
    xplpc_native_clear_mappings();
    ProxyCallbackRecorder::reset();

    const std::string name = "platform.reverse.response";
    xplpc_native_add_mapping(name.c_str(), name.size());

    const std::string key = "nested-key";
    const std::string data = R"({"f":"sample.reverse","p":[]})";

    xplpc_native_call_proxy(key.c_str(), key.size(), data.c_str(), data.size());

    EXPECT_EQ(key, ProxyCallbackRecorder::takeKey());
    EXPECT_EQ(R"({"r":"response-is-from-host"})", ProxyCallbackRecorder::takeData());
}

TEST_F(GeneralTest, NestedCoreMappingReachesAHostThatAnswersLater)
{
    // The host takes time over the inner call, and the outer answer still reaches the caller once it arrives.

    xplpc_core_initialize(true, nullptr, nullptr, &ProxyCallbackRecorder::onNativeProxyCallDeferred, &ProxyCallbackRecorder::onNativeProxyCallback, nullptr, nullptr);
    xplpc_native_clear_mappings();
    ProxyCallbackRecorder::reset();

    const std::string name = "platform.reverse.response";
    xplpc_native_add_mapping(name.c_str(), name.size());

    const std::string key = "nested-late-key";
    const std::string data = R"({"f":"sample.reverse","p":[]})";

    xplpc_native_call_proxy(key.c_str(), key.size(), data.c_str(), data.size());

    EXPECT_TRUE(ProxyCallbackRecorder::takeData().empty());

    ASSERT_TRUE(ProxyCallbackRecorder::waitForAnswer(5000));
    EXPECT_EQ(key, ProxyCallbackRecorder::takeKey());
    EXPECT_EQ(R"({"r":"response-is-from-host-later"})", ProxyCallbackRecorder::takeData());
}

TEST_F(GeneralTest, CoreReachesTheHostFromAThreadTheHostDidNotStart)
{
    // The host cannot be entered directly from here, so the call travels through the channel that hands it owned buffers.

    xplpc_core_initialize(true, nullptr, nullptr, &ProxyCallbackRecorder::onNativeProxyCall, &ProxyCallbackRecorder::onNativeProxyCallback, &ProxyCallbackRecorder::onNativeProxyCallFromThread, nullptr);
    xplpc_native_clear_mappings();
    ProxyCallbackRecorder::reset();

    const std::string name = "host.off.thread";
    xplpc_native_add_mapping(name.c_str(), name.size());

    std::string answer;

    // clang-format off
    std::thread([&answer] {
        xplpc::client::Client::call(R"({"f":"host.off.thread","p":[]})", [&answer](const std::string &response) {
            answer = response;
        });
    }).join();
    // clang-format on

    EXPECT_EQ(R"({"f":"host.off.thread","p":[]})", ProxyCallbackRecorder::callDataFromThread);
    EXPECT_EQ(R"({"r":"from-host-off-thread"})", answer);
}

TEST_F(GeneralTest, HostMappingAnswersAfterTheCallReturned)
{
    // The host owns the name and needs time to answer it, which is the shape a network request takes.

    xplpc_core_initialize(true, nullptr, nullptr, &ProxyCallbackRecorder::onNativeProxyCallDeferred, &ProxyCallbackRecorder::onNativeProxyCallback, nullptr, nullptr);
    xplpc_native_clear_mappings();
    ProxyCallbackRecorder::reset();

    const std::string name = "host.deferred";
    xplpc_native_add_mapping(name.c_str(), name.size());

    const std::string key = "deferred-host-key";
    const std::string data = R"({"f":"host.deferred","p":[]})";

    xplpc_native_call_proxy(key.c_str(), key.size(), data.c_str(), data.size());

    EXPECT_TRUE(ProxyCallbackRecorder::takeData().empty());

    ASSERT_TRUE(ProxyCallbackRecorder::waitForAnswer(5000));
    EXPECT_EQ(key, ProxyCallbackRecorder::takeKey());
    EXPECT_EQ(R"({"r":"from-host-later"})", ProxyCallbackRecorder::takeData());
}

TEST_F(GeneralTest, PlatformAnswersAnEmptyKeyFromAnotherThreadThroughOwnedBuffers)
{
    // The key the host called with is carried back whatever it holds, and an empty one is the length the owned buffers have to survive.

    xplpc_core_initialize(true, nullptr, nullptr, nullptr, &ProxyCallbackRecorder::onNativeProxyCallback, nullptr, &ProxyCallbackRecorder::onNativeProxyCallbackFromThread);
    ProxyCallbackRecorder::reset();

    const std::string key = "";
    const std::string data = R"({"f":"sample.async","p":[]})";

    xplpc_native_call_proxy(key.c_str(), key.size(), data.c_str(), data.size());

    ASSERT_TRUE(ProxyCallbackRecorder::waitForAnswerFromThread(5000));
    EXPECT_EQ("", ProxyCallbackRecorder::takeKeyFromThread());
    EXPECT_EQ(R"({"r":null})", ProxyCallbackRecorder::takeDataFromThread());
}

TEST_F(GeneralTest, PlatformAnswersFromAnotherThreadThroughOwnedBuffers)
{
    // The mapping answers from a thread it created, which the host can only be reached on through the owned buffers.

    xplpc_core_initialize(true, nullptr, nullptr, nullptr, &ProxyCallbackRecorder::onNativeProxyCallback, nullptr, &ProxyCallbackRecorder::onNativeProxyCallbackFromThread);
    ProxyCallbackRecorder::reset();

    const std::string key = "async-key";
    const std::string data = R"({"f":"sample.async","p":[]})";

    xplpc_native_call_proxy(key.c_str(), key.size(), data.c_str(), data.size());

    ASSERT_TRUE(ProxyCallbackRecorder::waitForAnswerFromThread(5000));
    EXPECT_EQ("", ProxyCallbackRecorder::key);
    EXPECT_EQ(R"({"r":null})", ProxyCallbackRecorder::takeDataFromThread());
}

TEST_F(GeneralTest, PlatformDoesNotBlockOnAMappingThatAnswersLater)
{
    // A mapping that answers after the call has returned is the genuinely asynchronous case, and it must not block the caller.

    xplpc_core_initialize(true, nullptr, nullptr, nullptr, &ProxyCallbackRecorder::onNativeProxyCallback, nullptr, &ProxyCallbackRecorder::onNativeProxyCallbackFromThread);
    ProxyCallbackRecorder::reset();

    xplpc::data::MappingList::shared()->add("test.detached", xplpc::map::Map::create<std::string>({}, &DetachedAnswerMapping::target));

    const std::string key = "detached-key";
    const std::string data = R"({"f":"test.detached","p":[]})";

    xplpc_native_call_proxy(key.c_str(), key.size(), data.c_str(), data.size());

    // The call returned while the mapping was still working, so nothing has been answered yet.
    EXPECT_EQ("", ProxyCallbackRecorder::key);

    EXPECT_TRUE(ProxyCallbackRecorder::waitForAnswerFromThread(5000));
    EXPECT_EQ(R"({"r":"detached"})", ProxyCallbackRecorder::takeDataFromThread());
}

TEST_F(GeneralTest, PlatformAnswersEmptyWhenTheHostCannotBeReached)
{
    // The host owns the name but declared no way to be reached from another thread, and the caller still has to be answered.

    xplpc_core_initialize(true, nullptr, nullptr, &ProxyCallbackRecorder::onNativeProxyCall, &ProxyCallbackRecorder::onNativeProxyCallback, nullptr, nullptr);
    xplpc_native_clear_mappings();
    ProxyCallbackRecorder::reset();

    const std::string name = "host.unreachable";
    xplpc_native_add_mapping(name.c_str(), name.size());

    std::string answer = "never answered";

    // clang-format off
    std::thread([&answer] {
        xplpc::client::Client::call(R"({"f":"host.unreachable","p":[]})", [&answer](const std::string &response) {
            answer = response;
        });
    }).join();
    // clang-format on

    EXPECT_EQ("", answer);
}

TEST_F(GeneralTest, PlatformSurvivesACallAfterFinalize)
{
    // Finalizing takes away the way back to the host, and a call arriving after it must not reach anything that was released.

    xplpc_core_initialize(true, nullptr, nullptr, nullptr, &ProxyCallbackRecorder::onNativeProxyCallback, nullptr, nullptr);
    ProxyCallbackRecorder::reset();

    xplpc_core_finalize();

    const std::string key = "after-finalize";
    const std::string data = R"({"f":"sample.login","p":[]})";

    xplpc_native_call_proxy(key.c_str(), key.size(), data.c_str(), data.size());
    xplpc_native_call_proxy_callback(key.c_str(), key.size(), data.c_str(), data.size());

    EXPECT_EQ("", ProxyCallbackRecorder::key);
}

extern "C" size_t xplpcCEntryPointCount(void);

TEST_F(GeneralTest, ThePublicHeaderIsValidC)
{
    // A host at this level writes plain c, so the header is compiled as c by the file this reaches rather than only as c++.

    EXPECT_EQ(8u, xplpcCEntryPointCount());
}
