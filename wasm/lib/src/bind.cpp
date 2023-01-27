#include "xplpc/client/ProxyClient.hpp"
#include "xplpc/core/XPLPC.hpp"
#include "xplpc/proxy/PlatformProxy.hpp"

#include <emscripten.h>
#include <emscripten/bind.h>

namespace em = emscripten;

// BIND: XPLPC

EMSCRIPTEN_BINDINGS(xplpc_core_xplpc)
{
    em::class_<xplpc::core::XPLPC>("XPLPC")
        .class_function("initialize", &xplpc::core::XPLPC::initialize)
        .class_function("isInitialized", &xplpc::core::XPLPC::isInitialized);
}

// BIND: ProxyClient

EMSCRIPTEN_BINDINGS(xplpc_client_proxy_client)
{
    em::class_<xplpc::client::ProxyClient>("ProxyClient")
        .class_function("call", &xplpc::client::ProxyClient::call)
        .class_function("callFromJavascript", &xplpc::client::ProxyClient::callFromJavascript);
}

// BIND: PlatformProxy

struct PlatformProxyWrapper : public em::wrapper<xplpc::proxy::PlatformProxy>
{
    EMSCRIPTEN_WRAPPER(PlatformProxyWrapper);

    void callProxy(const std::string &key, const std::string &data)
    {
        return call<void>("callProxy", key, data);
    }
};

EMSCRIPTEN_BINDINGS(xplpc_proxy_platform_proxy)
{
    em::class_<xplpc::proxy::PlatformProxy>("PlatformProxy")
        .constructor<>()
        .smart_ptr<std::shared_ptr<xplpc::proxy::PlatformProxy>>("shared_ptr<PlatformProxy>")
        .allow_subclass<PlatformProxyWrapper>("PlatformProxyWrapper")
        .class_function("shared", &xplpc::proxy::PlatformProxy::shared)
        .class_function("create", &xplpc::proxy::PlatformProxy::create)
        .class_function("createDefault", &xplpc::proxy::PlatformProxy::createDefault)
        .class_function("createFromPtr", &xplpc::proxy::PlatformProxy::createFromPtr, em::allow_raw_pointer<em::arg<0>>())
        .class_function("hasProxy", &xplpc::proxy::PlatformProxy::hasProxy)
        .function("initialize", &xplpc::proxy::PlatformProxy::initialize)
        .function("callProxy", &xplpc::proxy::PlatformProxy::callProxy, em::pure_virtual())
        .function("callProxyCallback", &xplpc::proxy::PlatformProxy::callProxyCallback);
}
