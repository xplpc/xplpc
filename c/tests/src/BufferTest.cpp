#include "fixtures/GeneralTest.hpp"
#include "fixtures/TestHost.hpp"
#include "xplpc/c/platform.h"
#include "gtest/gtest.h"

#include <cstdint>
#include <string>
#include <vector>

TEST_F(GeneralTest, BufferRegisteredByAHostIsReadThroughItsHandle)
{
    TestHost host;
    std::vector<uint8_t> bytes = {1, 2, 3, 4};

    const auto buffer = xplpc_buffer_register(host.handle(), bytes.data(), bytes.size());

    ASSERT_NE(0u, buffer);

    void *data = nullptr;
    size_t size = 0;

    ASSERT_TRUE(xplpc_buffer_read(buffer, &data, &size));
    EXPECT_EQ(bytes.data(), data);
    EXPECT_EQ(4u, size);

    EXPECT_TRUE(xplpc_buffer_release(host.handle(), buffer));
}

TEST_F(GeneralTest, BufferReleasedReadsNothing)
{
    TestHost host;
    std::vector<uint8_t> bytes = {1};

    const auto buffer = xplpc_buffer_register(host.handle(), bytes.data(), bytes.size());

    ASSERT_TRUE(xplpc_buffer_release(host.handle(), buffer));

    void *data = &bytes;
    size_t size = 9;

    EXPECT_FALSE(xplpc_buffer_read(buffer, &data, &size));
    EXPECT_EQ(nullptr, data);
    EXPECT_EQ(0u, size);
    EXPECT_FALSE(xplpc_buffer_release(host.handle(), buffer));
}

TEST_F(GeneralTest, BufferIsReleasedOnlyByTheHostThatRegisteredIt)
{
    TestHost owner;
    TestHost other;
    std::vector<uint8_t> bytes = {1};

    const auto buffer = xplpc_buffer_register(owner.handle(), bytes.data(), bytes.size());

    EXPECT_FALSE(xplpc_buffer_release(other.handle(), buffer));
    EXPECT_TRUE(xplpc_buffer_release(owner.handle(), buffer));
}

TEST_F(GeneralTest, BufferRegisteredByAHostGoesWhenItDetaches)
{
    std::vector<uint8_t> bytes = {1};
    xplpc_handle buffer = 0;

    {
        TestHost host;
        buffer = xplpc_buffer_register(host.handle(), bytes.data(), bytes.size());

        ASSERT_NE(0u, buffer);
    }

    void *data = nullptr;
    size_t size = 0;

    EXPECT_FALSE(xplpc_buffer_read(buffer, &data, &size));
}

TEST_F(GeneralTest, BufferRefusesWhatItCannotRegisterOrAnswer)
{
    TestHost host;
    std::vector<uint8_t> bytes = {1};

    EXPECT_EQ(0u, xplpc_buffer_register(host.handle(), nullptr, 1));
    EXPECT_EQ(0u, xplpc_buffer_register(0, bytes.data(), bytes.size()));

    size_t size = 0;
    void *data = nullptr;

    EXPECT_FALSE(xplpc_buffer_read(1, nullptr, &size));
    EXPECT_FALSE(xplpc_buffer_read(1, &data, nullptr));
}

TEST_F(GeneralTest, BufferCrossesTheWireAsItsHandleAndReachesACxxMapping)
{
    // The c++ mapping reads the bytes through the handle the host put in the request, and changes them where they are.

    TestHost host;
    std::vector<uint8_t> bytes = {255, 0, 0, 255, 0, 255, 0, 255};

    const auto buffer = xplpc_buffer_register(host.handle(), bytes.data(), bytes.size());
    const auto request = R"({"f":"sample.image.grayscale.dataview","p":[{"n":"dataView","v":{"buffer":)" + std::to_string(buffer) + "}}]}";

    const auto outcome = host.call(1, request);

    ASSERT_TRUE(outcome.answered);
    EXPECT_EQ(R"({"r":"OK"})", outcome.value);
    EXPECT_EQ(85, bytes[0]);
    EXPECT_EQ(85, bytes[5]);

    EXPECT_TRUE(xplpc_buffer_release(host.handle(), buffer));
}
