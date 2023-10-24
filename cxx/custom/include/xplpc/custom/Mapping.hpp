#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <thread>

#include "xplpc/custom/AllTypes.hpp"
#include "xplpc/custom/Todo.hpp"
#include "xplpc/xplpc.hpp"

#include "spdlog/spdlog.h"

#ifdef XPLPC_SERIALIZER_JSON
#include "xplpc/custom/json/AllTypesJson.hpp"
#include "xplpc/custom/json/TodoJson.hpp"
#endif

namespace xplpc
{
namespace custom
{

using namespace xplpc::client;
using namespace xplpc::map;
using namespace xplpc::type;

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
        MappingList::shared()->add("sample.image.grayscale.dataview", Map::create<std::string, DataView>({"dataView"}, &callbackImageToGrayscaleFromDataView));
        MappingList::shared()->add("sample.dataview", Map::create<DataView>({}, &callbackDataView));
        MappingList::shared()->add("sample.version", Map::create<std::string>({}, &callbackVersion));
        MappingList::shared()->add("sample.target", Map::create<std::string>({}, &callbackTarget));
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
        Client::call<std::string>(request, [=](const auto &response) {
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

    static void callbackImageToGrayscaleFromDataView(const Message &m, const Response r)
    {
        auto paramDataView = m.get<DataView>("dataView");

        if (paramDataView)
        {
            auto dataView = paramDataView.value();
            auto data = dataView.ptr();

            // process rgba image
            for (auto i = 0; i < dataView.size(); i += 4)
            {
                // skip transparent pixels
                if (data[i + 3] == 0)
                {
                    continue;
                }

                int gray = (data[i] + data[i + 1] + data[i + 2]) / 3;

                // set the red, green and blue values to the grayscale value
                data[i] = gray;
                data[i + 1] = gray;
                data[i + 2] = gray;
            }

            r(std::string{"OK"});
        }
        else
        {
            r(std::string{"INVALID-DATA"});
        }
    }

    static void callbackDataView(const Message &m, const Response r)
    {
        constexpr int size = 16;

        uint8_t *imageData = new uint8_t[size]{
            255, 0, 0, 255, // red pixel
            0, 255, 0, 255, // green pixel
            0, 0, 255, 255, // blue pixel
            0, 0, 0, 0,     // transparent pixel
        };

        auto dataView = DataView{imageData, size};

        r(dataView);
    }

    static void callbackVersion(const Message &m, const Response r)
    {
#ifdef XPLPC_VERSION
#define XPLPC_VERSION_STR XPLPC_VERSION
#else
#define XPLPC_VERSION_STR "0.0.0"
#endif

#ifdef XPLPC_VERSION_CODE
#define XPLPC_VERSION_CODE_STR XPLPC_VERSION_CODE
#else
#define XPLPC_VERSION_CODE_STR "0"
#endif

        r(std::string(XPLPC_VERSION_STR) + " (" + std::string(XPLPC_VERSION_CODE_STR) + ")");
    }

    static void callbackTarget(const Message &m, const Response r)
    {
#ifdef XPLPC_TARGET
#define XPLPC_TARGET_STR XPLPC_TARGET
#else
#define XPLPC_TARGET_STR "unknown"
#endif

        r(std::string(XPLPC_TARGET_STR));
    }
};

} // namespace custom
} // namespace xplpc
