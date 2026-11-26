#pragma once

#include "xplpc/core/Handle.hpp"

#include <emscripten/val.h>

#include <atomic>
#include <cstddef>
#include <string>

namespace xplpc
{
namespace bind
{

class JavascriptBridge
{
public:
    // Attaches the page once, and a second call keeps the layer the first one attached.
    static void attach(emscripten::val host);

    static void declare(const std::string &name);
    static void retract(const std::string &name);

    // Answers the handle of a call still waiting, or zero once the callback was answered, which happens before this returns when the mapping answered inline.
    static core::Handle call(const std::string &requestData, emscripten::val callback);

    static bool cancel(core::Handle call);
    static void answer(core::Handle call, const std::string &response);

    // Registers heap bytes the page keeps in place until it releases the handle, which is the only thing that crosses the wire.
    static core::Handle registerBuffer(size_t address, size_t size);
    static bool releaseBuffer(core::Handle buffer);

    // Answers a view of the heap bytes a handle names, or null, and the view is detached the moment the heap grows.
    static emscripten::val readBuffer(core::Handle buffer);

    // The number of calls still waiting anywhere in the process, which is what lets every test prove it left none behind.
    static size_t pendingCount();

private:
    static std::atomic<core::Handle> layer;
};

} // namespace bind
} // namespace xplpc
