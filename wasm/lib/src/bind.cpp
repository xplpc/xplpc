// Bind: XPLPC
#include "xplpc/core/XPLPC.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/bind.h>
namespace em = emscripten;

EMSCRIPTEN_BINDINGS(xplpc_core_xplpc)
{
    em::class_<xplpc::core::XPLPC>("XPLPC")
        .class_function("initialize", &xplpc::core::XPLPC::initialize)
        .class_function("isInitialized", &xplpc::core::XPLPC::isInitialized);
}
#endif

// Bind: ProxyClient
#include "xplpc/client/ProxyClient.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/bind.h>
namespace em = emscripten;

EMSCRIPTEN_BINDINGS(xplpc_client_proxy_client)
{
    em::class_<xplpc::client::ProxyClient>("ProxyClient")
        .class_function("call", &xplpc::client::ProxyClient::call)
        .class_function("callAsync", &xplpc::client::ProxyClient::callAsync);
}
#endif

// Bind: PlatformProxy
#include "xplpc/proxy/PlatformProxy.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/bind.h>
namespace em = emscripten;

EMSCRIPTEN_BINDINGS(xplpc_proxy_platform_proxy)
{
    em::class_<xplpc::proxy::PlatformProxy>("PlatformProxy")
        .smart_ptr<std::shared_ptr<xplpc::proxy::PlatformProxy>>("PlatformProxy")
        .class_function("shared", &xplpc::proxy::PlatformProxy::shared)
        .class_function("createDefault", &xplpc::proxy::PlatformProxy::createDefault)
        .class_function("create", &xplpc::proxy::PlatformProxy::create)
        .class_function("hasProxy", &xplpc::proxy::PlatformProxy::hasProxy)
        .function("initialize", &xplpc::proxy::PlatformProxy::initialize)
        .function("call", &xplpc::proxy::PlatformProxy::call);
}
#endif
