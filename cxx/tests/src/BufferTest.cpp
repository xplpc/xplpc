#include "fixtures/GeneralTest.hpp"
#include "fixtures/LogRecorder.hpp"
#include "fixtures/ReenteringSink.hpp"
#include "fixtures/ScopedLayer.hpp"
#include "fixtures/VerifiedCall.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

using namespace xplpc::client;
using namespace xplpc::core;
using namespace xplpc::message;
using namespace xplpc::type;

TEST_F(GeneralTest, BufferIsReadThroughTheViewItHandsOut)
{
    std::vector<uint8_t> bytes = {1, 2, 3, 4};
    Buffer buffer{bytes.data(), bytes.size()};

    const auto span = buffer.view().bytes();

    ASSERT_TRUE(span.has_value());
    EXPECT_EQ(bytes.data(), span->data);
    EXPECT_EQ(4u, span->size);
}

TEST_F(GeneralTest, BufferReleasedIsRefusedInsteadOfRead)
{
    // A view that outlives the bytes it named is the use after free a raw address made possible, and here it reads nothing.

    std::vector<uint8_t> bytes = {255, 0, 0, 255};
    DataView stale;

    {
        Buffer buffer{bytes.data(), bytes.size()};
        stale = buffer.view();
    }

    LogRecorder recorder;

    EXPECT_FALSE(stale.bytes().has_value());
    EXPECT_EQ(1u, recorder.errors().size());

    auto request = Request{
        "sample.image.grayscale.dataview",
        Param{"dataView", stale},
    };

    // clang-format off
    VerifiedCall::run<std::string>(request, [](const auto &response) {
        EXPECT_EQ("INVALID-DATA", response.value());
    });
    // clang-format on

    EXPECT_EQ(255, bytes[0]);
}

TEST_F(GeneralTest, BufferWhoseSlotWasReusedRefusesTheOldView)
{
    std::vector<uint8_t> first = {1};
    std::vector<uint8_t> second = {2};
    DataView stale;

    {
        Buffer buffer{first.data(), first.size()};
        stale = buffer.view();
    }

    Buffer reused{second.data(), second.size()};

    ASSERT_EQ(HandleParts::slot(stale.handle()), HandleParts::slot(reused.view().handle()));
    EXPECT_FALSE(stale.bytes().has_value());
    EXPECT_EQ(2, reused.view().bytes()->data[0]);
}

TEST_F(GeneralTest, BufferIsReleasedOnlyByTheLayerThatRegisteredIt)
{
    std::vector<uint8_t> bytes = {7};
    Buffer buffer{bytes.data(), bytes.size()};

    ScopedLayer other(ScopedLayer::answering("unused"));

    EXPECT_FALSE(BufferTable::shared().release(other.get(), buffer.view().handle()));
    EXPECT_TRUE(buffer.view().bytes().has_value());
}

TEST_F(GeneralTest, BufferRegisteredByALayerGoesWhenTheLayerDetaches)
{
    std::vector<uint8_t> bytes = {9};

    ScopedLayer layer(ScopedLayer::answering("unused"));
    const auto handle = BufferTable::shared().add(layer.get(), bytes.data(), bytes.size());

    ASSERT_TRUE(DataView{handle}.bytes().has_value());

    layer.detach();

    EXPECT_FALSE(DataView{handle}.bytes().has_value());
}

TEST_F(GeneralTest, BufferIsRefusedToALayerThatDetached)
{
    // A layer gives back every buffer it holds when it detaches, so one registered for it afterwards would be held by nobody for good.

    std::vector<uint8_t> bytes = {9};
    const auto before = BufferTable::shared().count();

    ScopedLayer layer(ScopedLayer::answering("unused"));
    layer.detach();

    EXPECT_EQ(0u, BufferTable::shared().add(layer.get(), bytes.data(), bytes.size()));
    EXPECT_EQ(before, BufferTable::shared().count());
}

TEST_F(GeneralTest, BufferIsRefusedWhenItsSizeDoesNotFitTheMemoryItNames)
{
    // A negative length that crossed as unsigned is the size a host gets wrong, and every reader would walk past the buffer with it.

    std::vector<uint8_t> bytes = {9};
    const auto before = BufferTable::shared().count();
    const auto native = Router::shared().native();

    EXPECT_EQ(0u, BufferTable::shared().add(native, bytes.data(), static_cast<size_t>(-1)));
    EXPECT_EQ(0u, BufferTable::shared().add(native, bytes.data(), (std::numeric_limits<size_t>::max)() / 2 + 1));
    EXPECT_EQ(before, BufferTable::shared().count());
}

TEST_F(GeneralTest, BufferReportsEveryRefusalWithItsTableFree)
{
    // A sink that reads the table from the line it is handed would wait forever on a lock the reporting thread already holds.

    std::vector<uint8_t> bytes = {9};

    ScopedLayer gone(ScopedLayer::answering("unused"));
    gone.detach();

    // clang-format off
    ReenteringSink sink([] {
        BufferTable::shared().count();
    });
    // clang-format on

    EXPECT_EQ(0u, BufferTable::shared().add(gone.get(), bytes.data(), bytes.size()));
    EXPECT_FALSE(BufferTable::shared().read(Handle{12345}).has_value());

    EXPECT_EQ(2, sink.count());
}
