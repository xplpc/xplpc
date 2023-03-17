#include "xplpc/client/Client.hpp"
#include "xplpc/core/XPLPC.hpp"
#include "xplpc/data/CallbackList.hpp"
#include "xplpc/data/PlatformProxyList.hpp"
#include "xplpc/proxy/NativePlatformProxy.hpp"

#include <emscripten.h>
#include <emscripten/bind.h>

#include <memory>

namespace em = emscripten;

// BIND: XPLPC

EMSCRIPTEN_BINDINGS(xplpc_core_xplpc)
{
    em::class_<xplpc::core::XPLPC>("XPLPC")
        .class_function("initialize", &xplpc::core::XPLPC::initialize)
        .class_function("isInitialized", &xplpc::core::XPLPC::isInitialized);
}

// BIND: Client

EMSCRIPTEN_BINDINGS(xplpc_client_client)
{
    em::class_<xplpc::client::Client>("Client")
        .class_function("call", select_overload<void(const std::string &, em::val)>(&xplpc::client::Client::call));
}

// BIND: PlatformProxyList

EMSCRIPTEN_BINDINGS(xplpc_data_platform_proxy_list)
{
    em::class_<xplpc::data::PlatformProxyList>("PlatformProxyList")
        .class_function("appendFromJavascript", &xplpc::data::PlatformProxyList::appendFromJavascript, em::allow_raw_pointer<em::arg<0>>())
        .class_function("insertFromJavascript", &xplpc::data::PlatformProxyList::insertFromJavascript, em::allow_raw_pointer<em::arg<1>>());
}

// BIND: PlatformProxy

struct PlatformProxyWrapper : public em::wrapper<xplpc::proxy::PlatformProxy>
{
    EMSCRIPTEN_WRAPPER(PlatformProxyWrapper);

    void initialize()
    {
        return call<void>("initialize");
    }

    void initializePlatform()
    {
        return call<void>("initializePlatform");
    }

    void finalize()
    {
        return call<void>("finalize");
    }

    void finalizePlatform()
    {
        return call<void>("finalizePlatform");
    }

    void callProxy(const std::string &key, const std::string &data)
    {
        return call<void>("callProxy", key, data);
    }

    bool hasMapping(const std::string &name)
    {
        return call<bool>("hasMapping", name);
    }
};

EMSCRIPTEN_BINDINGS(xplpc_proxy_platform_proxy)
{
    em::class_<xplpc::proxy::PlatformProxy>("PlatformProxy")
        .allow_subclass<PlatformProxyWrapper>("PlatformProxyWrapper")
        .function("initialize", &xplpc::proxy::PlatformProxy::initialize, em::pure_virtual())
        .function("initializePlatform", &xplpc::proxy::PlatformProxy::initializePlatform, em::pure_virtual())
        .function("finalize", &xplpc::proxy::PlatformProxy::finalize, em::pure_virtual())
        .function("finalizePlatform", &xplpc::proxy::PlatformProxy::finalizePlatform, em::pure_virtual())
        .function("callProxy", &xplpc::proxy::PlatformProxy::callProxy, em::pure_virtual())
        .function("hasMapping", &xplpc::proxy::PlatformProxy::hasMapping, em::pure_virtual());
}

// BIND: NativePlatformProxy

EMSCRIPTEN_BINDINGS(xplpc_proxy_native_platform_proxy)
{
    em::class_<xplpc::proxy::NativePlatformProxy, em::base<xplpc::proxy::PlatformProxy>>("NativePlatformProxy")
        .constructor<>()
        .smart_ptr<std::shared_ptr<xplpc::proxy::NativePlatformProxy>>("NativePlatformProxy")
        .function("initialize", &xplpc::proxy::NativePlatformProxy::initialize);
}

// BIND: CallbackList

EMSCRIPTEN_BINDINGS(xplpc_data_callback_list)
{
    em::class_<xplpc::data::CallbackList>("CallbackList")
        .class_function("executeFromJavascript", &xplpc::data::CallbackList::executeFromJavascript);
}
