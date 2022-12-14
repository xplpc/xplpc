#include "xplpc/custom/Todo.hpp"
#include "fixtures/GeneralTest.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <vector>

using namespace xplpc::core;
using namespace xplpc::client;
using namespace xplpc::message;

TEST_F(GeneralTest, TodoTestSingle)
{
    auto item = Todo{1, "Todo 1", "Body 1", {}, true};

    auto request = Request{"sample.todo.single", Param<Todo>{"item", item}};
    auto ret = LocalClient::call<Todo>(request);

    EXPECT_NE(ret, std::nullopt);

    if (ret)
    {
        auto todo = ret.value();

        EXPECT_EQ(1, todo.id);
        EXPECT_EQ("Todo 1", todo.title);
        EXPECT_EQ(true, todo.done);
    }
}

TEST_F(GeneralTest, TodoTestSingleAsync)
{
    auto item = Todo{1, "Todo 1", "Body 1", {}, true};

    auto request = Request{"sample.todo.single", Param<Todo>{"item", item}};
    auto ret = LocalClient::callAsync<Todo>(request).get();

    EXPECT_NE(ret, std::nullopt);

    if (ret)
    {
        auto todo = ret.value();

        EXPECT_EQ(1, todo.id);
        EXPECT_EQ("Todo 1", todo.title);
        EXPECT_EQ(true, todo.done);
    }
}

TEST_F(GeneralTest, TodoMultipleTest)
{
    auto item1 = Todo{1, "Todo 1", "Body 1", {}, true};
    auto item2 = Todo{2, "Todo 2", "Body 2", {}, true};
    auto items = std::vector<Todo>{item1, item2};

    auto request = Request{"sample.todo.list", Param<std::vector<Todo>>{"items", items}};
    auto ret = LocalClient::call<std::vector<Todo>>(request);

    auto total = 0;

    if (ret)
    {
        auto list = ret.value();
        total = list.size();
    }

    EXPECT_EQ(2, total);
}

TEST_F(GeneralTest, TodoMultipleTestAsync)
{
    auto item1 = Todo{1, "Todo 1", "Body 1", {}, true};
    auto item2 = Todo{2, "Todo 2", "Body 2", {}, true};
    auto items = std::vector<Todo>{item1, item2};

    auto request = Request{"sample.todo.list", Param<std::vector<Todo>>{"items", items}};
    auto ret = LocalClient::callAsync<std::vector<Todo>>(request).get();

    auto total = 0;

    if (ret)
    {
        auto list = ret.value();
        total = list.size();
    }

    EXPECT_EQ(2, total);
}
