#include "fixtures/GeneralTest.hpp"
#include "fixtures/VerifiedCall.hpp"
#include "fixtures/ViewBytes.hpp"
#include "xplpc/custom/Todo.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <cstdint>
#include <thread>
#include <vector>

using namespace xplpc::message;
using namespace xplpc::client;
using namespace xplpc::custom;

void testLogin()
{
    auto request = Request{
        "sample.login",
        Param<std::string>{"username", "paulo"},
        Param<std::string>{"password", "123456"},
        Param<bool>{"remember", true},
    };

    // clang-format off
    VerifiedCall::run<std::string>(request, [](const auto &response) {
        EXPECT_EQ("LOGGED-WITH-REMEMBER", response.value());
    });
    // clang-format on
}

void testTodoSingle()
{
    auto item = Todo{1, "Todo 1", "Body 1", {}, true};
    auto request = Request{"sample.todo.single", Param<Todo>{"item", item}};

    // clang-format off
    VerifiedCall::run<Todo>(request, [](const auto &response) {
        EXPECT_EQ(1, response.value().id);
        EXPECT_EQ("Todo 1", response.value().title);
    });
    // clang-format on
}

void testTodoList()
{
    auto item1 = Todo{1, "Todo 1", "Body 1", {}, true};
    auto item2 = Todo{2, "Todo 2", "Body 2", {}, true};
    auto items = std::vector<Todo>{item1, item2};

    auto request = Request{"sample.todo.list", Param<std::vector<Todo>>{"items", items}};

    // clang-format off
    VerifiedCall::run<std::vector<Todo>>(request, [](const auto &response) {
        ASSERT_TRUE(response.has_value());
        ASSERT_EQ(2u, response.value().size());
        EXPECT_EQ("Todo 1", response.value()[0].title);
        EXPECT_EQ("Todo 2", response.value()[1].title);
    });
    // clang-format on
}

void testEcho()
{
    auto request = Request{"sample.echo", Param<std::string>{"value", "test echo value"}};

    // clang-format off
    VerifiedCall::run<std::string>(request, [](const auto &response) {
        EXPECT_EQ("test echo value", response.value());
    });
    // clang-format on
}

void testImageToGrayscale()
{
    // The four pixels are red, green, blue and transparent.
    // clang-format off
    std::vector<uint8_t> imageData = {
        255, 0, 0, 255,
        0, 255, 0, 255,
        0, 0, 255, 255,
        0, 0, 0, 0,
    };
    // clang-format on

    // Four pixels of four bytes are a two by two image, and declaring anything else is refused by the mapping.
    int width = 2;
    int height = 2;

    auto request = Request{
        "sample.image.grayscale",
        Param{"image", imageData},
        Param{"width", width},
        Param{"height", height},
    };

    // clang-format off
    VerifiedCall::run<std::vector<uint8_t>>(request, [](const auto &response) {
        ASSERT_TRUE(response.has_value());
        EXPECT_EQ(std::vector<uint8_t>({85, 85, 85, 255, 85, 85, 85, 255, 85, 85, 85, 255, 0, 0, 0, 0}), response.value());
    });
    // clang-format on
}

void testImageToGrayscaleFromDataView()
{
    // The four pixels are red, green, blue and transparent.
    // clang-format off
    std::vector<uint8_t> imageData = {
        255, 0, 0, 255,
        0, 255, 0, 255,
        0, 0, 255, 255,
        0, 0, 0, 0,
    };
    // clang-format on

    Buffer buffer{imageData.data(), imageData.size()};
    auto dataView = buffer.view();

    auto request = Request{
        "sample.image.grayscale.dataview",
        Param{"dataView", dataView},
    };

    // clang-format off
    VerifiedCall::run<std::string>(request, [&dataView](const auto &response) {
        EXPECT_EQ("OK", response.value());

        const auto originalVector = ViewBytes::of(dataView);

        EXPECT_EQ(16, originalVector.size());
        EXPECT_EQ(originalVector[0], 85);
        EXPECT_EQ(originalVector[4], 85);
        EXPECT_EQ(originalVector[8], 85);
        EXPECT_EQ(originalVector[12], 0);
    });
    // clang-format on
}

void testDataView()
{
    auto request = Request{"sample.dataview"};

    // clang-format off
    VerifiedCall::run<DataView>(request, [](const auto &response) {
        auto dataView = response.value();

        EXPECT_EQ(16, ViewBytes::of(dataView).size());
        EXPECT_EQ(ViewBytes::of(dataView)[0], 255);
        EXPECT_EQ(ViewBytes::of(dataView)[5], 255);
        EXPECT_EQ(ViewBytes::of(dataView)[10], 255);
        EXPECT_EQ(ViewBytes::of(dataView)[12], 0);

        auto request = Request{
            "sample.image.grayscale.dataview",
            Param{"dataView", dataView},
        };

        VerifiedCall::run<std::string>(request, [&dataView](const auto &response) {
            EXPECT_EQ("OK", response.value());

            const auto originalVector = ViewBytes::of(dataView);

            EXPECT_EQ(16, originalVector.size());
            EXPECT_EQ(originalVector[0], 85);
            EXPECT_EQ(originalVector[4], 85);
            EXPECT_EQ(originalVector[8], 85);
            EXPECT_EQ(originalVector[12], 0);

            EXPECT_EQ(16, ViewBytes::of(dataView).size());
            EXPECT_EQ(ViewBytes::of(dataView)[0], 85);
            EXPECT_EQ(ViewBytes::of(dataView)[5], 85);
            EXPECT_EQ(ViewBytes::of(dataView)[10], 85);
            EXPECT_EQ(ViewBytes::of(dataView)[12], 0);
        });
    });
    // clang-format on
}

void testLoginFromString()
{
    auto request = R"({"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]})";

    // clang-format off
    VerifiedCall::run(request, [](auto const &response) {
        EXPECT_EQ(R"({"r":"LOGGED-WITH-REMEMBER"})", response);
    });
    // clang-format on
}

void testTodoSingleFromString()
{
    auto request = R"({"f":"sample.todo.single","p":[{"n":"item","v":{"body":"Body 1","data":{"data1":"value1","data2":"value2"},"done":true,"id":1,"title":"Title 1"}}]})";

    // clang-format off
    VerifiedCall::run(request, [](auto const &response) {
        EXPECT_EQ(R"({"r":{"body":"Body 1","data":{"data1":"value1","data2":"value2"},"done":true,"id":1,"title":"Title 1"}})", response);
    });
    // clang-format on
}

TEST_F(GeneralTest, ConcurrencyAll)
{
    std::vector<std::thread> threads;

    for (int i = 0; i < 100; ++i)
    {
        threads.push_back(std::thread(testLogin));
        threads.push_back(std::thread(testTodoSingle));
        threads.push_back(std::thread(testTodoList));
        threads.push_back(std::thread(testEcho));
        threads.push_back(std::thread(testImageToGrayscale));
        threads.push_back(std::thread(testImageToGrayscaleFromDataView));
        threads.push_back(std::thread(testDataView));
        threads.push_back(std::thread(testLoginFromString));
        threads.push_back(std::thread(testTodoSingleFromString));
    }

    for (auto &thread : threads)
    {
        thread.join();
    }
}
