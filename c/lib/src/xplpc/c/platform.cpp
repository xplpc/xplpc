#include "xplpc/c/platform.h"
#include "xplpc/core/BufferTable.hpp"
#include "xplpc/core/Router.hpp"
#include "xplpc/layer/CLayer.hpp"
#include "xplpc/layer/OwnedBytes.hpp"
#include "xplpc/util/Log.hpp"
#include "xplpc/util/NativeBoundary.hpp"

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>

using namespace xplpc::core;
using namespace xplpc::layer;
using namespace xplpc::util;

xplpc_handle xplpc_attach(const xplpc_host *host)
{
    if (!host || !host->invoke || !host->post)
    {
        Log::e("[xplpc_attach] The host must carry both an invoke and a post callback");
        return 0;
    }

    // clang-format off
    return NativeBoundary::runAnswering("xplpc_attach", xplpc_handle{0}, [host] {
        auto layer = std::make_shared<CLayer>(*host);
        const auto handle = Router::shared().attach(layer);

        // A layer the router refused is not attached, so it neither takes a name nor replaces the host holding it.
        if (!handle)
        {
            return xplpc_handle{0};
        }

        layer->bind(handle);

        if (host->name)
        {
            CLayer::takePlace(std::string(host->name, host->nameSize), handle);
        }

        return handle;
    });
    // clang-format on
}

void xplpc_detach(xplpc_handle layer)
{
    // clang-format off
    NativeBoundary::run("xplpc_detach", [layer] {
        if (!CLayer::attached(layer))
        {
            Log::e("[xplpc_detach] The layer is not attached");
            return;
        }

        Router::shared().detach(layer);
    });
    // clang-format on
}

void xplpc_abandon(const xplpc_handle *layer)
{
    if (!layer)
    {
        Log::e("[xplpc_abandon] The layer carries no address");
        return;
    }

    // A layer that was already replaced or detached is gone, so abandoning it again has nothing left to do.

    // clang-format off
    NativeBoundary::run("xplpc_abandon", [layer] {
        CLayer::abandon(*layer);
    });
    // clang-format on
}

void xplpc_declare(xplpc_handle layer, const char *name, size_t nameSize)
{
    if (!name)
    {
        Log::e("[xplpc_declare] The name carries no address");
        return;
    }

    // clang-format off
    NativeBoundary::run("xplpc_declare", [&] {
        if (!CLayer::attached(layer))
        {
            Log::e("[xplpc_declare] The layer is not attached");
            return;
        }

        Router::shared().declare(layer, std::string(name, nameSize));
    });
    // clang-format on
}

void xplpc_retract(xplpc_handle layer, const char *name, size_t nameSize)
{
    if (!name)
    {
        Log::e("[xplpc_retract] The name carries no address");
        return;
    }

    // clang-format off
    NativeBoundary::run("xplpc_retract", [&] {
        if (!CLayer::attached(layer))
        {
            Log::e("[xplpc_retract] The layer is not attached");
            return;
        }

        Router::shared().retract(layer, std::string(name, nameSize));
    });
    // clang-format on
}

bool xplpc_call(xplpc_handle layer, uint64_t token, const char *request, size_t requestSize, xplpc_call_result *result)
{
    if (!result)
    {
        Log::e("[xplpc_call] The result carries no address, so the call cannot be answered");
        return false;
    }

    *result = xplpc_call_result{0, nullptr, 0};

    if (!request)
    {
        Log::e("[xplpc_call] The request carries no address");
        result->data = OwnedBytes::copy("");

        return true;
    }

    // clang-format off
    const auto routed = NativeBoundary::runAnswering("xplpc_call", CallResult::answered(""), [&] {
        auto caller = CLayer::attached(layer);

        if (!caller)
        {
            Log::e("[xplpc_call] The layer is not attached");
            return CallResult::answered("");
        }

        return Router::shared().call(layer, std::string(request, requestSize), [caller, token](const std::string &response) {
            caller->post(XPLPC_POST_ANSWER, token, response);
        });
    });
    // clang-format on

    if (!routed.isAnswered())
    {
        result->call = routed.handle();
        return false;
    }

    result->data = OwnedBytes::copy(routed.value());
    result->dataSize = result->data ? routed.value().size() : 0;

    if (!result->data)
    {
        Log::e("[xplpc_call] There was no memory to hand the answer over to the host");
    }

    return true;
}

bool xplpc_cancel(xplpc_handle layer, xplpc_handle call)
{
    // clang-format off
    return NativeBoundary::runAnswering("xplpc_cancel", false, [layer, call] {
        if (!CLayer::attached(layer))
        {
            Log::e("[xplpc_cancel] The layer is not attached");
            return false;
        }

        return Router::shared().cancel(layer, call);
    });
    // clang-format on
}

void xplpc_answer(xplpc_handle call, const char *response, size_t responseSize)
{
    // A response that carries no address still answers the call, since a caller left waiting is worse than one answered with nothing.

    if (!response)
    {
        Log::e("[xplpc_answer] The response carries no address");
    }

    // clang-format off
    NativeBoundary::run("xplpc_answer", [&] {
        Router::shared().answer(call, response ? std::string(response, responseSize) : std::string());
    });
    // clang-format on
}

xplpc_handle xplpc_buffer_register(xplpc_handle layer, void *data, size_t size)
{
    // The host keeps the bytes valid and in place until it releases the handle, which is the only thing that crosses the wire.

    if (!data)
    {
        Log::e("[xplpc_buffer_register] The buffer carries no address");
        return 0;
    }

    // clang-format off
    return NativeBoundary::runAnswering("xplpc_buffer_register", xplpc_handle{0}, [layer, data, size] {
        if (!CLayer::attached(layer))
        {
            Log::e("[xplpc_buffer_register] The layer is not attached");
            return xplpc_handle{0};
        }

        return BufferTable::shared().add(layer, static_cast<std::uint8_t *>(data), size);
    });
    // clang-format on
}

bool xplpc_buffer_read(xplpc_handle buffer, void **data, size_t *size)
{
    if (!data || !size)
    {
        Log::e("[xplpc_buffer_read] The result carries no address");
        return false;
    }

    *data = nullptr;
    *size = 0;

    // clang-format off
    const auto span = NativeBoundary::runAnswering("xplpc_buffer_read", std::optional<BufferSpan>{}, [buffer] {
        return BufferTable::shared().read(buffer);
    });
    // clang-format on

    if (!span)
    {
        return false;
    }

    *data = span->data;
    *size = span->size;

    return true;
}

bool xplpc_buffer_release(xplpc_handle layer, xplpc_handle buffer)
{
    // clang-format off
    return NativeBoundary::runAnswering("xplpc_buffer_release", false, [layer, buffer] {
        if (!CLayer::attached(layer))
        {
            Log::e("[xplpc_buffer_release] The layer is not attached");
            return false;
        }

        return BufferTable::shared().release(layer, buffer);
    });
    // clang-format on
}

size_t xplpc_pending()
{
    // The number of calls still waiting anywhere in the process, which is what lets a host prove it left none behind.

    // clang-format off
    return NativeBoundary::runAnswering("xplpc_pending", size_t{0}, [] {
        return Router::shared().pending();
    });
    // clang-format on
}

void xplpc_free(void *ptr)
{
    std::free(ptr);
}
