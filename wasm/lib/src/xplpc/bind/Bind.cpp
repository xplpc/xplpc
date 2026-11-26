#include "JavascriptBridge.hpp"

#include <emscripten/bind.h>

namespace em = emscripten;

EMSCRIPTEN_BINDINGS(xplpc_bind_javascript_bridge)
{
    em::class_<xplpc::bind::JavascriptBridge>("JavascriptBridge")
        .class_function("attach", &xplpc::bind::JavascriptBridge::attach)
        .class_function("declare", &xplpc::bind::JavascriptBridge::declare)
        .class_function("retract", &xplpc::bind::JavascriptBridge::retract)
        .class_function("call", &xplpc::bind::JavascriptBridge::call)
        .class_function("cancel", &xplpc::bind::JavascriptBridge::cancel)
        .class_function("answer", &xplpc::bind::JavascriptBridge::answer)
        .class_function("registerBuffer", &xplpc::bind::JavascriptBridge::registerBuffer)
        .class_function("releaseBuffer", &xplpc::bind::JavascriptBridge::releaseBuffer)
        .class_function("readBuffer", &xplpc::bind::JavascriptBridge::readBuffer)
        .class_function("pendingCount", &xplpc::bind::JavascriptBridge::pendingCount);
}
