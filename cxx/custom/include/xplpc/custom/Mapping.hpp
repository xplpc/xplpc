#pragma once

#include <chrono>
#include <cstdint>
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
        MappingList::shared()->add("sample.image.grayscale", Map::create<std::vector<uint8_t>, std::vector<uint8_t>, int, int>({"image", "width", "height"}, &callbackImageToGrayscale));
        MappingList::shared()->add("sample.image.grayscale.pointer", Map::create<std::string, std::uintptr_t, int, int>({"pointer", "width", "height"}, &callbackImageToGrayscaleFromPointer));
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

    static void callbackImageToGrayscale(const Message &m, const Response r)
    {
        auto imageDataRaw = m.get<std::vector<uint8_t>>("image");
        auto imageWidth = m.get<int>("width");
        auto imageHeight = m.get<int>("height");

        if (imageDataRaw && imageWidth && imageHeight)
        {
            auto imageData = imageDataRaw.value();
            auto width = imageWidth.value();
            auto height = imageHeight.value();

            // process rgba image
            for (auto i = 0; i < width * height * 4; i += 4)
            {
                // skip transparent pixels
                if (imageData[i + 3] == 0)
                {
                    continue;
                }

                int gray = (imageData[i] + imageData[i + 1] + imageData[i + 2]) / 3;

                // set the red, green, and blue values to the grayscale value
                imageData[i] = gray;
                imageData[i + 1] = gray;
                imageData[i + 2] = gray;
            }

            r(imageData);
        }
        else
        {
            r({});
        }
    }

    static void callbackImageToGrayscaleFromPointer(const Message &m, const Response r)
    {
        auto pointer = m.get<std::uintptr_t>("pointer");
        auto imageWidth = m.get<int>("width");
        auto imageHeight = m.get<int>("height");

        if (pointer && imageWidth && imageHeight)
        {
            uint8_t *imageData = reinterpret_cast<uint8_t *>(pointer.value());
            auto width = imageWidth.value();
            auto height = imageHeight.value();

            // proccess rgba image
            for (auto i = 0; i < width * height * 4; i += 4)
            {
                // skip transparent pixels
                if (imageData[i + 3] == 0)
                {
                    continue;
                }

                int gray = (imageData[i] + imageData[i + 1] + imageData[i + 2]) / 3;

                // set the red, green, and blue values to the grayscale value
                imageData[i] = gray;
                imageData[i + 1] = gray;
                imageData[i + 2] = gray;
            }

            r(std::string{"OK"});
        }
        else
        {
            r(std::string{"INVALID-DATA"});
        }
    }
};

} // namespace custom
} // namespace xplpc
