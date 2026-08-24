#include "fixtures/GeneralTest.hpp"
#include "fixtures/VerifiedCall.hpp"
#include "xplpc/custom/Todo.hpp"
#include "xplpc/util/Log.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <cstdint>
#include <thread>
#include <vector>

using namespace xplpc::message;
using namespace xplpc::util;
using namespace xplpc::client;
using namespace xplpc::custom;
using namespace xplpc::proxy;

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
        Log::d("[testLogin] Returned Value: {}", response.value());
    });
    // clang-format on
}

void testTodoSingle()
{
    auto item = Todo{1, "Todo 1", "Body 1", {}, true};
    auto request = Request{"sample.todo.single", Param<Todo>{"item", item}};

    // clang-format off
    VerifiedCall::run<Todo>(request, [](const auto &response) {
        Log::d("[testTodoSingle] Returned Value: {}, {}", response.value().id, response.value().title);
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
        if (response)
        {
            auto list = response.value();

            Log::d("[testTodoList] Returned Value 1: {}, {}", list[0].id, list[0].title);
            Log::d("[testTodoList] Returned Value 2: {}, {}", list[1].id, list[1].title);
        }
    });
    // clang-format on
}

void testEcho()
{
    auto request = Request{"sample.echo", Param<std::string>{"value", "test echo value"}};

    // clang-format off
    VerifiedCall::run<std::string>(request, [](const auto &response) {
        if (response)
        {
            Log::d("[testEcho] Returned Value: {}", response.value());
        }
        else
        {
            Log::d("[testEcho] Returned Value Is Null");
        }
    });
    // clang-format on
}

void testImageToGrayscale()
{
    std::vector<uint8_t> imageData = {
        255, 0, 0, 255, // red pixel
        0, 255, 0, 255, // green pixel
        0, 0, 255, 255, // blue pixel
        0, 0, 0, 0,     // transparent pixel
    };

    int width = 1;
    int height = 1;

    auto request = Request{
        "sample.image.grayscale",
        Param{"image", imageData},
        Param{"width", width},
        Param{"height", height},
    };

    // clang-format off
    VerifiedCall::run<std::vector<uint8_t>>(request, [](const auto &response) {
        EXPECT_EQ(16, response.value().size());
    });
    // clang-format on
}

void testImageToGrayscaleFromDataView()
{
    std::vector<uint8_t> imageData = {
        255, 0, 0, 255, // red pixel
        0, 255, 0, 255, // green pixel
        0, 0, 255, 255, // blue pixel
        0, 0, 0, 0,     // transparent pixel
    };

    auto dataView = DataView{imageData.data(), imageData.size()};

    auto request = Request{
        "sample.image.grayscale.dataview",
        Param{"dataView", dataView},
    };

    // clang-format off
    VerifiedCall::run<std::string>(request, [&dataView](const auto &response) {
        EXPECT_EQ("OK", response.value());

        std::vector<uint8_t> originalVector(dataView.size());

        dataView.copy(originalVector.data());

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

        EXPECT_EQ(16, dataView.size());
        EXPECT_EQ(dataView.ptr()[0], 255);
        EXPECT_EQ(dataView.ptr()[5], 255);
        EXPECT_EQ(dataView.ptr()[10], 255);
        EXPECT_EQ(dataView.ptr()[12], 0);

        auto request = Request{
            "sample.image.grayscale.dataview",
            Param{"dataView", dataView},
        };

        VerifiedCall::run<std::string>(request, [&dataView](const auto &response) {
            EXPECT_EQ("OK", response.value());

            std::vector<uint8_t> originalVector(dataView.size());
            dataView.copy(originalVector.data());

            EXPECT_EQ(16, originalVector.size());
            EXPECT_EQ(originalVector[0], 85);
            EXPECT_EQ(originalVector[4], 85);
            EXPECT_EQ(originalVector[8], 85);
            EXPECT_EQ(originalVector[12], 0);

            EXPECT_EQ(16, dataView.size());
            EXPECT_EQ(dataView.ptr()[0], 85);
            EXPECT_EQ(dataView.ptr()[5], 85);
            EXPECT_EQ(dataView.ptr()[10], 85);
            EXPECT_EQ(dataView.ptr()[12], 0);
        });
    });
    // clang-format on
}

void testLoginFromString()
{
    auto request = R"({"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]})";

    // clang-format off
    VerifiedCall::run(request, [](auto const &response) {
        Log::d("[testLoginFromString] Returned Value: {}", response);
    });
    // clang-format on
}

void testTodoSingleFromString()
{
    auto request = R"({"f":"sample.todo.single","p":[{"n":"item","v":{"body":"Body 1","data":{"data1":"value1","data2":"value2"},"done":true,"id":1,"title":"Title 1"}}]})";

    // clang-format off
    VerifiedCall::run(request, [](auto const &response) {
        Log::d("[testTodoSingleFromString] Returned Value: {}", response);
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
