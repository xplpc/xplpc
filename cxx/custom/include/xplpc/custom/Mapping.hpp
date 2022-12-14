#pragma once

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

class Mapping
{
public:
    static void initialize()
    {
        // mapping data
        MappingData::add(Map::create<std::string, std::string, std::string, bool>("sample.login", {"username", "password", "remember"}, &callbackLogin));
        MappingData::add(Map::create<Todo, Todo>("sample.todo.single", {"item"}, &callbackTodoSingle));
        MappingData::add(Map::create<std::vector<Todo>, std::vector<Todo>>("sample.todo.list", {"items"}, &callbackTodoList));
        MappingData::add(Map::create<std::string, std::string>("sample.echo", {"value"}, &callbackEcho));
        MappingData::add(Map::create<AllTypes, AllTypes>("sample.alltypes.single", {"item"}, &callbackAllTypesSingle));
        MappingData::add(Map::create<std::vector<AllTypes>, std::vector<AllTypes>>("sample.alltypes.list", {"items"}, &callbackAllTypesList));
    }

    static Response callbackLogin(const Message &data)
    {
        auto username = data.get<std::string>("username");
        auto password = data.get<std::string>("password");
        auto remember = data.get<bool>("remember");

        if (username && password && remember)
        {
            if (username.value() == "paulo" && password.value() == "123456")
            {
                spdlog::debug("[callbackLogin] Logged");

                if (remember.value())
                {
                    return Response::str("LOGGED-WITH-REMEMBER");
                }
                else
                {
                    return Response::str("LOGGED-WITHOUT-REMEMBER");
                }
            }
        }

        spdlog::debug("[callbackLogin] Not logged");

        return Response::str("NOT-LOGGED");
    }

    static Response callbackTodoSingle(const Message &data)
    {
        auto item = data.get<Todo>("item");

        if (item)
        {
            spdlog::debug("[callbackTodoSingle] Received Item: {}, {}", item.value().id, item.value().title);
            return Response(item.value());
        }

        return Response::none();
    }

    static Response callbackTodoList(const Message &data)
    {
        auto items = data.get<std::vector<Todo>>("items");

        if (items)
        {
            spdlog::debug("[callbackTodoList] Received Item 1: {}, {}", items.value()[0].id, items.value()[0].title);
            spdlog::debug("[callbackTodoList] Received Item 2: {}, {}", items.value()[1].id, items.value()[1].title);

            return Response(items.value());
        }

        spdlog::debug("[callbackTodoList] List is empty");

        return Response::none();
    }

    static Response callbackEcho(const Message &data)
    {
        auto value = data.get<std::string>("value");

        if (value)
        {
            spdlog::debug("[callbackEcho] Received Value: {}", value.value());
            return Response::str(value.value());
        }
        else
        {
            spdlog::debug("[callbackEcho] Received Value Is Empty");
            return Response::str("<EMPTY>");
        }
    }

    static Response callbackAllTypesSingle(const Message &data)
    {
        auto item = data.get<AllTypes>("item");

        if (item)
        {
            spdlog::debug("[callbackAllTypesSingle] Received Item: {}", item.value().typeString);
            return Response(item.value());
        }

        return Response::none();
    }

    static Response callbackAllTypesList(const Message &data)
    {
        auto items = data.get<std::vector<AllTypes>>("items");

        if (items)
        {
            spdlog::debug("[callbackAllTypesList] Received Item 1: {}", items.value()[0].typeString);
            spdlog::debug("[callbackAllTypesList] Received Item 2: {}", items.value()[1].typeString);

            return Response(items.value());
        }

        spdlog::debug("[callbackAllTypesList] List is empty");

        return Response::none();
    }
};

} // namespace custom
} // namespace xplpc
