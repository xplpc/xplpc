#include "fixtures/GeneralTest.hpp"
#include "spdlog/spdlog.h"
#include "xplpc/custom/Todo.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

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

    auto ret = LocalClient::call<std::string>(request).value();
    spdlog::debug("[testLogin] Returned Value: {}", ret);
}

void testLoginAsync()
{
    auto request = Request{
        "sample.login",
        Param<std::string>{"username", "paulo"},
        Param<std::string>{"password", "123456"},
        Param<bool>{"remember", true},
    };

    auto ret = LocalClient::callAsync<std::string>(request).get().value();
    spdlog::debug("[testLogin] Returned Value: {}", ret);
}

void testTodoSingle()
{
    auto item = Todo{1, "Todo 1", "Body 1", {}, true};

    auto request = Request{"sample.todo.single", Param<Todo>{"item", item}};
    auto ret = LocalClient::call<Todo>(request);

    if (ret)
    {
        spdlog::debug("[testTodoSingle] Returned Value: {}, {}", ret.value().id, ret.value().title);
    }
}

void testTodoSingleAsync()
{
    auto item = Todo{1, "Todo 1", "Body 1", {}, true};

    auto request = Request{"sample.todo.single", Param<Todo>{"item", item}};
    auto ret = LocalClient::callAsync<Todo>(request).get();

    if (ret)
    {
        spdlog::debug("[testTodoSingle] Returned Value: {}, {}", ret.value().id, ret.value().title);
    }
}

void testTodoList()
{
    auto item1 = Todo{1, "Todo 1", "Body 1", {}, true};
    auto item2 = Todo{2, "Todo 2", "Body 2", {}, true};
    auto items = std::vector<Todo>{item1, item2};

    auto request = Request{"sample.todo.list", Param<std::vector<Todo>>{"items", items}};
    auto ret = LocalClient::call<std::vector<Todo>>(request);

    if (ret)
    {
        auto list = ret.value();

        spdlog::debug("[testTodoList] Returned Value 1: {}, {}", list[0].id, list[0].title);
        spdlog::debug("[testTodoList] Returned Value 2: {}, {}", list[1].id, list[1].title);
    }
}

void testTodoListAsync()
{
    auto item1 = Todo{1, "Todo 1", "Body 1", {}, true};
    auto item2 = Todo{2, "Todo 2", "Body 2", {}, true};
    auto items = std::vector<Todo>{item1, item2};

    auto request = Request{"sample.todo.list", Param<std::vector<Todo>>{"items", items}};
    auto ret = LocalClient::callAsync<std::vector<Todo>>(request).get();

    if (ret)
    {
        auto list = ret.value();

        spdlog::debug("[testTodoList] Returned Value 1: {}, {}", list[0].id, list[0].title);
        spdlog::debug("[testTodoList] Returned Value 2: {}, {}", list[1].id, list[1].title);
    }
}

void testEcho()
{
    auto request = Request{"sample.echo", Param<std::string>{"value", "test echo value"}};
    auto ret = LocalClient::call<std::string>(request);

    if (ret)
    {
        spdlog::debug("[testEcho] Returned Value: {}", ret.value());
    }
    else
    {
        spdlog::debug("[testEcho] Returned Value Is Null");
    }
}

void testEchoAsync()
{
    auto request = Request{"sample.echo", Param<std::string>{"value", "test echo value"}};
    auto ret = LocalClient::callAsync<std::string>(request).get();

    if (ret)
    {
        spdlog::debug("[testEcho] Returned Value: {}", ret.value());
    }
    else
    {
        spdlog::debug("[testEcho] Returned Value Is Null");
    }
}

void testLoginWithProxyClient()
{
    auto request = R"({"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]})";
    auto ret = ProxyClient::call(request);

    spdlog::debug("[testLoginWithProxyClient] Returned Value: {}", ret);
}

void testLoginWithProxyClientAsync()
{
    auto request = R"({"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]})";
    auto ret = ProxyClient::callAsync(request).get();

    spdlog::debug("[testLoginWithProxyClient] Returned Value: {}", ret);
}

void testTodoSingleWithProxyClient()
{
    auto request = R"({"f":"sample.todo.single","p":[{"n":"item","v":{"body":"Body 1","data":{"data1":"value1","data2":"value2"},"done":true,"id":1,"title":"Title 1"}}]})";
    auto ret = ProxyClient::call(request);

    spdlog::debug("[testTodoSingleWithProxyClient] Returned Value: {}", ret);
}

void testTodoSingleWithProxyClientAsync()
{
    auto request = R"({"f":"sample.todo.single","p":[{"n":"item","v":{"body":"Body 1","data":{"data1":"value1","data2":"value2"},"done":true,"id":1,"title":"Title 1"}}]})";
    auto ret = ProxyClient::callAsync(request).get();

    spdlog::debug("[testTodoSingleWithProxyClient] Returned Value: {}", ret);
}

void testRemoteClient()
{
    auto request = Request{
        "platform.battery.level",
        Param<std::string>{"suffix", "%"},
    };

    auto ret = RemoteClient::call<std::string>(request);

    if (ret)
    {
        spdlog::debug("[testRemoteClient] Returned Value: {}", ret.value());
    }
    else
    {
        spdlog::debug("[testRemoteClient] Returned Value Is Null");
    }
}

void testRemoteClientAsync()
{
    auto request = Request{
        "platform.battery.level",
        Param<std::string>{"suffix", "%"},
    };

    auto ret = RemoteClient::callAsync<std::string>(request).get();

    if (ret)
    {
        spdlog::debug("[testRemoteClient] Returned Value: {}", ret.value());
    }
    else
    {
        spdlog::debug("[testRemoteClient] Returned Value Is Null");
    }
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
        threads.push_back(std::thread(testLoginWithProxyClient));
        threads.push_back(std::thread(testTodoSingleWithProxyClient));
        threads.push_back(std::thread(testRemoteClient));

        threads.push_back(std::thread(testLoginAsync));
        threads.push_back(std::thread(testTodoSingleAsync));
        threads.push_back(std::thread(testTodoListAsync));
        threads.push_back(std::thread(testEchoAsync));
        threads.push_back(std::thread(testLoginWithProxyClientAsync));
        threads.push_back(std::thread(testTodoSingleWithProxyClientAsync));
        threads.push_back(std::thread(testRemoteClientAsync));
    }

    for (auto &thread : threads)
    {
        thread.join();
    }
}
