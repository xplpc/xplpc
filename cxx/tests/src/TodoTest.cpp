#include "xplpc/custom/Todo.hpp"
#include "fixtures/GeneralTest.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <thread>
#include <vector>

using namespace xplpc::core;
using namespace xplpc::client;
using namespace xplpc::message;

TEST_F(GeneralTest, TodoTestSingle)
{
    auto item = Todo{1, "Todo 1", "Body 1", {}, true};

    auto request = Request{"sample.todo.single", Param<Todo>{"item", item}};

    // clang-format off
    Client::call<Todo>(request, [](const auto &response) {
        EXPECT_NE(response, std::nullopt);

        if (response)
        {
            auto todo = response.value();

            EXPECT_EQ(1, todo.id);
            EXPECT_EQ("Todo 1", todo.title);
            EXPECT_EQ(true, todo.done);
        }
    });
    // clang-format on
}

TEST_F(GeneralTest, TodoTestSingleAsync)
{
    auto item = Todo{1, "Todo 1", "Body 1", {}, true};

    auto request = Request{"sample.todo.single", Param<Todo>{"item", item}};

    // clang-format off
    std::thread([=] {
        Client::call<Todo>(request, [](const auto &response) {
            EXPECT_NE(response, std::nullopt);

            if (response)
            {
                auto todo = response.value();

                EXPECT_EQ(1, todo.id);
                EXPECT_EQ("Todo 1", todo.title);
                EXPECT_EQ(true, todo.done);
            }
        });
    }).join();
    // clang-format on
}

TEST_F(GeneralTest, TodoMultipleTest)
{
    auto item1 = Todo{1, "Todo 1", "Body 1", {}, true};
    auto item2 = Todo{2, "Todo 2", "Body 2", {}, true};
    auto items = std::vector<Todo>{item1, item2};

    auto request = Request{"sample.todo.list", Param<std::vector<Todo>>{"items", items}};

    // clang-format off
    Client::call<std::vector<Todo>>(request, [](const auto &response) {
        auto total = 0;

        if (response)
        {
            auto list = response.value();
            total = list.size();
        }

        EXPECT_EQ(2, total);
    });
    // clang-format on
}

TEST_F(GeneralTest, TodoMultipleTestAsync)
{
    auto item1 = Todo{1, "Todo 1", "Body 1", {}, true};
    auto item2 = Todo{2, "Todo 2", "Body 2", {}, true};
    auto items = std::vector<Todo>{item1, item2};

    auto request = Request{"sample.todo.list", Param<std::vector<Todo>>{"items", items}};

    // clang-format off
    std::thread([=] {
        Client::call<std::vector<Todo>>(request, [](const auto &response) {
            auto total = 0;

            if (response)
            {
                auto list = response.value();
                total = list.size();
            }

            EXPECT_EQ(2, total);
        });
    }).join();
    // clang-format on
}
