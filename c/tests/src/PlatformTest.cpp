#include "xplpc/c/platform.h"
#include "fixtures/GeneralTest.hpp"
#include "fixtures/Requests.hpp"
#include "fixtures/TestHost.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

class RefusedHost
{
public:
    static void onInvoke(void * /*context*/, xplpc_handle /*call*/, const char * /*request*/, size_t /*requestSize*/)
    {
        ADD_FAILURE() << "A host that was refused was invoked";
    }

    static void onPost(void * /*context*/, xplpc_post_kind /*kind*/, uint64_t /*id*/, char *data, size_t /*dataSize*/)
    {
        xplpc_free(data);
        ADD_FAILURE() << "A host that was refused received a post";
    }
};

class RaisingReleaseHost
{
public:
    static void onInvoke(void * /*context*/, xplpc_handle /*call*/, const char * /*request*/, size_t /*requestSize*/) {}

    static void onPost(void * /*context*/, xplpc_post_kind kind, uint64_t /*id*/, char *data, size_t /*dataSize*/)
    {
        xplpc_free(data);

        if (kind == XPLPC_POST_RELEASE)
        {
            throw std::runtime_error("the host raised while being released");
        }
    }
};

TEST_F(GeneralTest, PlatformRefusesAHostWithoutBothCallbacks)
{
    // A host that cannot receive a late answer or a release would leave something waiting for good, so it is never attached.

    const xplpc_host withoutInvoke{nullptr, nullptr, &RefusedHost::onPost, false, nullptr, 0};
    const xplpc_host withoutPost{nullptr, &RefusedHost::onInvoke, nullptr, false, nullptr, 0};

    EXPECT_EQ(0u, xplpc_attach(nullptr));
    EXPECT_EQ(0u, xplpc_attach(&withoutInvoke));
    EXPECT_EQ(0u, xplpc_attach(&withoutPost));
}

TEST_F(GeneralTest, PlatformRefusesPointersWithoutAnAddress)
{
    // The abi carries a pointer and a size, and building a string from address zero is what takes a host down.

    TestHost server(true, TestHost::Serving::Never);
    server.declare("host.never");

    TestHost host;

    xplpc_declare(host.handle(), nullptr, 8);
    xplpc_retract(host.handle(), nullptr, 8);

    xplpc_call_result result{};

    EXPECT_TRUE(xplpc_call(host.handle(), 1, nullptr, 8, &result));
    EXPECT_EQ(0u, result.call);
    EXPECT_NE(nullptr, result.data);
    EXPECT_EQ(0u, result.dataSize);
    xplpc_free(result.data);

    const auto &request = Requests::login();
    EXPECT_FALSE(xplpc_call(host.handle(), 1, request.data(), request.size(), nullptr));

    const auto pending = host.call(2, Requests::named("host.never"));
    ASSERT_FALSE(pending.answered);

    xplpc_answer(pending.call, nullptr, 8);

    ASSERT_EQ(1u, host.answers().size());
    EXPECT_EQ(2u, host.answers()[0].id);
    EXPECT_EQ("", host.answers()[0].data);

    xplpc_free(nullptr);
}

TEST_F(GeneralTest, PlatformIgnoresHandlesItNeverHandedOut)
{
    // A handle is only ever valid while the router holds it, so zero and a forged one are refused rather than resolved.

    const std::string name = "host.forged";
    const std::string response = R"({"r":"forged"})";
    const auto &request = Requests::login();

    xplpc_detach(0);
    xplpc_declare(0, name.data(), name.size());
    xplpc_retract(0, name.data(), name.size());
    xplpc_answer(0, response.data(), response.size());
    xplpc_answer(0xdeadbeef00000001ull, response.data(), response.size());

    EXPECT_FALSE(xplpc_cancel(0, 0));
    EXPECT_FALSE(xplpc_cancel(0xdeadbeef00000001ull, 0xdeadbeef00000001ull));

    xplpc_call_result result{};

    EXPECT_TRUE(xplpc_call(0xdeadbeef00000001ull, 1, request.data(), request.size(), &result));
    EXPECT_EQ("", std::string(result.data, result.dataSize));
    xplpc_free(result.data);
}

TEST_F(GeneralTest, PlatformStopsAnExceptionRaisedByTheHost)
{
    // A host written in c++ is free to raise from its invoke, and the caller is still answered rather than the exception leaving an extern "C" function.

    TestHost server(true, TestHost::Serving::Raise);
    server.declare("host.raises");

    TestHost caller;

    const auto outcome = caller.call(1, Requests::named("host.raises"));

    EXPECT_TRUE(outcome.answered);
    EXPECT_EQ("", outcome.value);
    EXPECT_EQ(1u, server.invocations().size());
}

TEST_F(GeneralTest, PlatformRefusesTheHandleOfTheNativeLayer)
{
    // Every handle is a small integer and the first one of each kind is the one the c++ layer holds, so a host passing the wrong one must not take c++ routing down with it.

    const auto native = xplpc::core::Router::shared().native();
    const std::string name = "host.native";
    const std::string login = "sample.login";
    std::vector<uint8_t> bytes = {1};
    xplpc::type::Buffer buffer{bytes.data(), bytes.size()};

    // A host holds the name first, so a declare through the native handle that was not refused would take the name from it.
    TestHost server(true, TestHost::Serving::Inline, R"({"r":"from-host"})");
    server.declare(name);

    xplpc_declare(native, name.data(), name.size());
    xplpc_retract(native, login.data(), login.size());
    xplpc_detach(native);

    EXPECT_EQ(0u, xplpc_buffer_register(native, bytes.data(), bytes.size()));
    EXPECT_FALSE(xplpc_buffer_release(native, buffer.view().handle()));
    EXPECT_TRUE(buffer.view().bytes().has_value());

    xplpc_call_result result{};

    EXPECT_TRUE(xplpc_call(native, 1, Requests::login().data(), Requests::login().size(), &result));
    EXPECT_EQ("", std::string(result.data, result.dataSize));
    xplpc_free(result.data);

    TestHost host;

    EXPECT_EQ(Requests::loginAnswer(), host.call(1, Requests::login()).value);
    EXPECT_EQ(R"({"r":"from-host"})", host.call(2, Requests::named(name)).value);
}

TEST_F(GeneralTest, PlatformStopsAnExceptionTheHostRaisesWhileBeingReleased)
{
    // The release is posted from wherever the last reference to the layer goes, and a raise leaving a destructor ends the process.

    const xplpc_host host{nullptr, &RaisingReleaseHost::onInvoke, &RaisingReleaseHost::onPost, true, nullptr, 0};
    const auto layer = xplpc_attach(&host);

    ASSERT_NE(0u, layer);
    EXPECT_NO_THROW(xplpc_detach(layer));
}

extern "C" size_t xplpcCEntryPointCount(void);

TEST_F(GeneralTest, ThePublicHeaderIsValidC)
{
    // A host at this level writes plain c, so the header is compiled as c by the file this reaches rather than only as c++.

    EXPECT_EQ(13u, xplpcCEntryPointCount());
}
