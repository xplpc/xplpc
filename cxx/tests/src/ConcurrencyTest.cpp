#include "fixtures/GeneralTest.hpp"
#include "spdlog/spdlog.h"
#include "xplpc/custom/Todo.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <cstdint>
#include <thread>
#include <vector>

using namespace xplpc::message;
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
    LocalClient::call<std::string>(request, [](const auto &response) {
        spdlog::debug("[testLogin] Returned Value: {}", response.value());
    });
    // clang-format on
}

void testTodoSingle()
{
    auto item = Todo{1, "Todo 1", "Body 1", {}, true};
    auto request = Request{"sample.todo.single", Param<Todo>{"item", item}};

    // clang-format off
    LocalClient::call<Todo>(request, [](const auto &response) {
        spdlog::debug("[testTodoSingle] Returned Value: {}, {}", response.value().id, response.value().title);
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
    LocalClient::call<std::vector<Todo>>(request, [](const auto &response) {
        if (response)
        {
            auto list = response.value();

            spdlog::debug("[testTodoList] Returned Value 1: {}, {}", list[0].id, list[0].title);
            spdlog::debug("[testTodoList] Returned Value 2: {}, {}", list[1].id, list[1].title);
        }
    });
    // clang-format on
}

void testEcho()
{
    auto request = Request{"sample.echo", Param<std::string>{"value", "test echo value"}};

    // clang-format off
    LocalClient::call<std::string>(request, [](const auto &response) {
        if (response)
        {
            spdlog::debug("[testEcho] Returned Value: {}", response.value());
        }
        else
        {
            spdlog::debug("[testEcho] Returned Value Is Null");
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
    LocalClient::call<std::vector<uint8_t>>(request, [](const auto &response) {
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
    LocalClient::call<std::string>(request, [&dataView](const auto &response) {
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
    LocalClient::call<DataView>(request, [](const auto &response) {
        auto dataView = response.value();

        // check current values
        EXPECT_EQ(16, dataView.size());
        EXPECT_EQ(dataView.ptr()[0], 255);
        EXPECT_EQ(dataView.ptr()[5], 255);
        EXPECT_EQ(dataView.ptr()[10], 255);
        EXPECT_EQ(dataView.ptr()[12], 0);

        auto request = Request{
            "sample.image.grayscale.dataview",
            Param{"dataView", dataView},
        };

        LocalClient::call<std::string>(request, [&dataView](const auto &response) {
            EXPECT_EQ("OK", response.value());

            std::vector<uint8_t> originalVector(dataView.size());
            dataView.copy(originalVector.data());

            // check copied values
            EXPECT_EQ(16, originalVector.size());
            EXPECT_EQ(originalVector[0], 85);
            EXPECT_EQ(originalVector[4], 85);
            EXPECT_EQ(originalVector[8], 85);
            EXPECT_EQ(originalVector[12], 0);

            // current original values again
            EXPECT_EQ(16, dataView.size());
            EXPECT_EQ(dataView.ptr()[0], 85);
            EXPECT_EQ(dataView.ptr()[5], 85);
            EXPECT_EQ(dataView.ptr()[10], 85);
            EXPECT_EQ(dataView.ptr()[12], 0);
        });
    });
    // clang-format on
}

void testLoginWithProxyClient()
{
    auto request = R"({"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]})";

    // clang-format off
    ProxyClient::call(request, [](auto const &response) {
        spdlog::debug("[testLoginWithProxyClient] Returned Value: {}", response);
    });
    // clang-format on
}

void testTodoSingleWithProxyClient()
{
    auto request = R"({"f":"sample.todo.single","p":[{"n":"item","v":{"body":"Body 1","data":{"data1":"value1","data2":"value2"},"done":true,"id":1,"title":"Title 1"}}]})";

    // clang-format off
    ProxyClient::call(request, [](auto const &response) {
        spdlog::debug("[testTodoSingleWithProxyClient] Returned Value: {}", response);
    });
    // clang-format on
}

void testRemoteClient()
{
    auto request = Request{
        "platform.battery.level",
        Param<std::string>{"suffix", "%"},
    };

    // clang-format off
    RemoteClient::call<std::string>(request, [](const auto &response) {
        if (response)
        {
            spdlog::debug("[testRemoteClient] Returned Value: {}", response.value());
        }
        else
        {
            spdlog::debug("[testRemoteClient] Returned Value: Empty");
        }
    });
    // clang-format on
}

TEST_F(GeneralTest, ConcurrencyAll)
{
    std::vector<std::thread> threads;

    for (int i = 0; i < 1000; ++i)
    {
        threads.push_back(std::thread(testLogin));
        threads.push_back(std::thread(testTodoSingle));
        threads.push_back(std::thread(testTodoList));
        threads.push_back(std::thread(testEcho));
        threads.push_back(std::thread(testImageToGrayscale));
        threads.push_back(std::thread(testImageToGrayscaleFromDataView));
        threads.push_back(std::thread(testDataView));
        threads.push_back(std::thread(testLoginWithProxyClient));
        threads.push_back(std::thread(testTodoSingleWithProxyClient));
        threads.push_back(std::thread(testRemoteClient));
    }

    for (auto &thread : threads)
    {
        thread.join();
    }
}
