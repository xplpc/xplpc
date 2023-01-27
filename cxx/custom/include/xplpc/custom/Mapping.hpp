#pragma once

#include <chrono>
#include <thread>

#include "xplpc/custom/AllTypes.hpp"
#include "xplpc/custom/Todo.hpp"
#include "xplpc/xplpc.hpp"

#ifdef XPLPC_SERIALIZER_JSON
#include "xplpc/custom/json/AllTypesJson.hpp"
#include "xplpc/custom/json/TodoJson.hpp"
#endif

namespace xplpc
{
namespace custom
{

using namespace xplpc::map;
using namespace xplpc::client;

class Mapping
{
public:
    static void initialize()
    {
        // mapping data (function name, map<return value, params types>(params names), function ref)
        MappingList::shared()->add("sample.login", Map::create<std::string, std::string, std::string, bool>({"username", "password", "remember"}, &callbackLogin));
        MappingList::shared()->add("sample.todo.single", Map::create<Todo, Todo>({"item"}, &callbackTodoSingle));
        MappingList::shared()->add("sample.todo.list", Map::create<std::vector<Todo>, std::vector<Todo>>({"items"}, &callbackTodoList));
        MappingList::shared()->add("sample.echo", Map::create<std::string, std::string>({"value"}, &callbackEcho));
        MappingList::shared()->add("sample.alltypes.single", Map::create<AllTypes, AllTypes>({"item"}, &callbackAllTypesSingle));
        MappingList::shared()->add("sample.alltypes.list", Map::create<std::vector<AllTypes>, std::vector<AllTypes>>({"items"}, &callbackAllTypesList));
        MappingList::shared()->add("sample.async", Map::create<void>({}, &callbackAsync));
        MappingList::shared()->add("sample.reverse", Map::create<std::string>({}, &callbackReverse));
    }

    static void callbackLogin(const Message &m, const Response r)
    {
        auto username = m.get<std::string>("username");
        auto password = m.get<std::string>("password");
        auto remember = m.get<bool>("remember");

        if (username && password && remember)
        {
            if (username.value() == "paulo" && password.value() == "123456")
            {
                spdlog::debug("[callbackLogin] Logged");

                if (remember.value())
                {
                    r(std::string("LOGGED-WITH-REMEMBER"));
                }
                else
                {
                    r(std::string("LOGGED-WITHOUT-REMEMBER"));
                }

                return;
            }
        }

        spdlog::debug("[callbackLogin] Not logged");
        r(std::string("NOT-LOGGED"));
    }

    static void callbackTodoSingle(const Message &m, const Response r)
    {
        auto item = m.get<Todo>("item");

        if (item)
        {
            spdlog::debug("[callbackTodoSingle] Received Item: {}, {}", item.value().id, item.value().title);
            r(item.value());
        }
        else
        {
            r(nullptr);
        }
    }

    static void callbackTodoList(const Message &m, const Response r)
    {
        auto items = m.get<std::vector<Todo>>("items");

        if (items)
        {
            spdlog::debug("[callbackTodoList] Received Item 1: {}, {}", items.value()[0].id, items.value()[0].title);
            spdlog::debug("[callbackTodoList] Received Item 2: {}, {}", items.value()[1].id, items.value()[1].title);

            r(items.value());
        }
        else
        {
            spdlog::debug("[callbackTodoList] List is empty");
            r(nullptr);
        }
    }

    static void callbackEcho(const Message &m, const Response r)
    {
        auto value = m.get<std::string>("value");

        if (value)
        {
            spdlog::debug("[callbackEcho] Received Value: {}", value.value());
            r(value.value());
        }
        else
        {
            spdlog::debug("[callbackEcho] Received Value Is Empty");
            r(std::string("<EMPTY>"));
        }
    }

    static void callbackAllTypesSingle(const Message &m, const Response r)
    {
        auto item = m.get<AllTypes>("item");

        if (item)
        {
            spdlog::debug("[callbackAllTypesSingle] Received Item: {}", item.value().typeString);
            r(item.value());
        }
        else
        {
            r(nullptr);
        }
    }

    static void callbackAllTypesList(const Message &m, const Response r)
    {
        auto items = m.get<std::vector<AllTypes>>("items");

        if (items)
        {
            spdlog::debug("[callbackAllTypesList] Received Item 1: {}", items.value()[0].typeString);
            spdlog::debug("[callbackAllTypesList] Received Item 2: {}", items.value()[1].typeString);

            r(items.value());
        }
        else
        {
            spdlog::debug("[callbackAllTypesList] List is empty");
            r(nullptr);
        }
    }

    static void callbackAsync(const Message &m, const Response r)
    {
        // clang-format off
        std::thread([=] {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            r(nullptr);
        }).join();
        // clang-format on
    }

    static void callbackReverse(const Message &m, const Response r)
    {
        auto request = Request{"platform.reverse.response"};

        // clang-format off
        RemoteClient::call<std::string>(request, [=](const auto &response) {
            if (response) {
                r(std::string{"response-is-"} + response.value());
            } else {
                r(std::string{"response-is-empty"});
            }
        });
        // clang-format on
    }
};

} // namespace custom
} // namespace xplpc
