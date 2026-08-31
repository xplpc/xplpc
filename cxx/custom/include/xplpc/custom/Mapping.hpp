#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <thread>

#include "xplpc/custom/AllTypes.hpp"
#include "xplpc/custom/Todo.hpp"
#include "xplpc/xplpc.hpp"

#ifdef XPLPC_SERIALIZER_JSON
#include "xplpc/custom/json/AllTypesJson.hpp"
#include "xplpc/custom/json/TodoJson.hpp"
#include "xplpc/util/Log.hpp"
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

    static void callbackLogin(const Message &m, const Response &r)
    {
        auto username = m.get<std::string>("username");
        auto password = m.get<std::string>("password");
        auto remember = m.get<bool>("remember");

        if (username && password && remember)
        {
            if (username.value() == "paulo" && password.value() == "123456")
            {
                util::Log::d("[callbackLogin] Logged");

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

        util::Log::d("[callbackLogin] Not logged");
        r(std::string("NOT-LOGGED"));
    }

    static void callbackTodoSingle(const Message &m, const Response &r)
    {
        auto item = m.get<Todo>("item");

        if (item)
        {
            util::Log::d("[callbackTodoSingle] Received Item: {}, {}", item.value().id, item.value().title);
            r(item.value());
        }
        else
        {
            r(nullptr);
        }
    }

    static void callbackTodoList(const Message &m, const Response &r)
    {
        auto items = m.get<std::vector<Todo>>("items");

        if (items)
        {
            // How many items arrive is decided by the caller, so the list is walked rather than indexed.
            for (const auto &item : items.value())
            {
                util::Log::d("[callbackTodoList] Received Item: {}, {}", item.id, item.title);
            }

            r(items.value());
        }
        else
        {
            util::Log::d("[callbackTodoList] List is empty");
            r(nullptr);
        }
    }

    static void callbackEcho(const Message &m, const Response &r)
    {
        auto value = m.get<std::string>("value");

        if (value)
        {
            util::Log::d("[callbackEcho] Received Value: {}", value.value());
            r(value.value());
        }
        else
        {
            util::Log::d("[callbackEcho] Received Value Is Empty");
            r(std::string("<EMPTY>"));
        }
    }

    static void callbackAllTypesSingle(const Message &m, const Response &r)
    {
        auto item = m.get<AllTypes>("item");

        if (item)
        {
            util::Log::d("[callbackAllTypesSingle] Received Item: {}", item.value().typeString);
            r(item.value());
        }
        else
        {
            r(nullptr);
        }
    }

    static void callbackAllTypesList(const Message &m, const Response &r)
    {
        auto items = m.get<std::vector<AllTypes>>("items");

        if (items)
        {
            for (const auto &item : items.value())
            {
                util::Log::d("[callbackAllTypesList] Received Item: {}", item.typeString);
            }

            r(items.value());
        }
        else
        {
            util::Log::d("[callbackAllTypesList] List is empty");
            r(nullptr);
        }
    }

    static void callbackAsync(const Message & /*m*/, const Response &r)
    {
        // The work is left running and the call returns at once, which is what a mapping must do to never hold the bridge.
        // clang-format off
        std::thread([r] {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            r(nullptr);
        }).detach();
        // clang-format on
    }

    static void callbackReverse(const Message & /*m*/, const Response &r)
    {
        auto request = Request{"platform.reverse.response"};

        // The response is copied because the nested call answers after this frame is gone.
        // clang-format off
        Client::call<std::string>(request, [r](const auto &response) {
            if (response) {
                r(std::string{"response-is-"} + response.value());
            } else {
                r(std::string{"response-is-empty"});
            }
        });
        // clang-format on
    }

    static void callbackImageToGrayscale(const Message &m, const Response &r)
    {
        auto imageDataRaw = m.get<std::vector<uint8_t>>("image");
        auto imageWidth = m.get<int>("width");
        auto imageHeight = m.get<int>("height");

        if (imageDataRaw && imageWidth && imageHeight)
        {
            auto imageData = imageDataRaw.value();
            auto width = imageWidth.value();
            auto height = imageHeight.value();

            // The dimensions arrive from the caller, so the buffer is the authority on how far the walk can go.
            if (width <= 0 || height <= 0 || static_cast<size_t>(width) * static_cast<size_t>(height) * 4 != imageData.size())
            {
                util::Log::e("[callbackImageToGrayscale] The image does not match the size it declares");
                r({});
                return;
            }

            // The walk stops where the last whole pixel ends, so the reads and writes below stay inside the buffer.
            const size_t pixelBytes = imageData.size() - (imageData.size() % 4);

            for (size_t i = 0; i < pixelBytes; i += 4)
            {
                if (imageData[i + 3] == 0)
                {
                    continue;
                }

                int gray = (imageData[i] + imageData[i + 1] + imageData[i + 2]) / 3;

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

    static void callbackImageToGrayscaleFromDataView(const Message &m, const Response &r)
    {
        auto paramDataView = m.get<DataView>("dataView");

        if (paramDataView)
        {
            auto dataView = paramDataView.value();
            auto data = dataView.ptr();

            // A view decoded from the wire can carry no address, and reading one is a fault rather than an empty answer.
            if (!data)
            {
                util::Log::e("[callbackImageToGrayscaleFromDataView] The view carries no address");
                r(std::string{"INVALID-DATA"});
                return;
            }

            // The walk stops where the last whole pixel ends, so a size the caller rounded badly cannot reach past the buffer.
            const size_t pixelBytes = dataView.size() - (dataView.size() % 4);

            for (size_t i = 0; i < pixelBytes; i += 4)
            {
                if (data[i + 3] == 0)
                {
                    continue;
                }

                int gray = (data[i] + data[i + 1] + data[i + 2]) / 3;

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

    static void callbackDataView(const Message & /*m*/, const Response &r)
    {
        // The caller reads the buffer after the call returns, and every thread owning its own keeps a concurrent caller out of it.
        static thread_local std::array<uint8_t, 16> imageData;

        imageData = {
            255, 0, 0, 255, // red pixel
            0, 255, 0, 255, // green pixel
            0, 0, 255, 255, // blue pixel
            0, 0, 0, 0,     // transparent pixel
        };

        r(DataView{imageData.data(), imageData.size()});
    }

    static void callbackVersion(const Message & /*m*/, const Response &r)
    {
        r(std::string(XPLPC_VERSION) + " (" + std::string(XPLPC_VERSION_CODE) + ")");
    }

    static void callbackTarget(const Message & /*m*/, const Response &r)
    {
        r(std::string(XPLPC_TARGET));
    }
};

} // namespace custom
} // namespace xplpc
