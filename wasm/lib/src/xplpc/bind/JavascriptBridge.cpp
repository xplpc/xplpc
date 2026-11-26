#include "JavascriptBridge.hpp"
#include "JavascriptLayer.hpp"

#include "xplpc/core/BufferTable.hpp"
#include "xplpc/core/CallState.hpp"
#include "xplpc/core/Router.hpp"

#include <cstdint>
#include <memory>

namespace xplpc
{
namespace bind
{

using namespace xplpc::core;

std::atomic<Handle> JavascriptBridge::layer{0};

void JavascriptBridge::attach(emscripten::val host)
{
    if (layer.load())
    {
        return;
    }

    layer.store(Router::shared().attach(std::make_shared<JavascriptLayer>(host)));
}

void JavascriptBridge::declare(const std::string &name)
{
    Router::shared().declare(layer.load(), name);
}

void JavascriptBridge::retract(const std::string &name)
{
    Router::shared().retract(layer.load(), name);
}

Handle JavascriptBridge::call(const std::string &requestData, emscripten::val callback)
{
    // clang-format off
    const Completion completion = [callback](const std::string &response) {
        callback(response);
    };
    // clang-format on

    const auto result = Router::shared().call(layer.load(), requestData, completion);

    if (result.isAnswered())
    {
        CallState::run(completion, result.value());
    }

    return result.handle();
}

bool JavascriptBridge::cancel(Handle call)
{
    return Router::shared().cancel(layer.load(), call);
}

void JavascriptBridge::answer(Handle call, const std::string &response)
{
    Router::shared().answer(call, response);
}

Handle JavascriptBridge::registerBuffer(size_t address, size_t size)
{
    return BufferTable::shared().add(layer.load(), reinterpret_cast<std::uint8_t *>(address), size);
}

bool JavascriptBridge::releaseBuffer(Handle buffer)
{
    return BufferTable::shared().release(layer.load(), buffer);
}

emscripten::val JavascriptBridge::readBuffer(Handle buffer)
{
    const auto span = BufferTable::shared().read(buffer);

    if (!span)
    {
        return emscripten::val::null();
    }

    return emscripten::val(emscripten::typed_memory_view(span->size, span->data));
}

size_t JavascriptBridge::pendingCount()
{
    return Router::shared().pending();
}

} // namespace bind
} // namespace xplpc
