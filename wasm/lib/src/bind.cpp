#include "xplpc/client/ProxyClient.hpp"
#include "xplpc/core/XPLPC.hpp"
#include "xplpc/proxy/PlatformProxy.hpp"

#include <emscripten.h>
#include <emscripten/bind.h>

namespace em = emscripten;

// Bind: XPLPC
EMSCRIPTEN_BINDINGS(xplpc_core_xplpc)
{
    em::class_<xplpc::core::XPLPC>("XPLPC")
        .class_function("initialize", &xplpc::core::XPLPC::initialize)
        .class_function("isInitialized", &xplpc::core::XPLPC::isInitialized);
}

// Bind: ProxyClient
EMSCRIPTEN_BINDINGS(xplpc_client_proxy_client)
{
    em::class_<xplpc::client::ProxyClient>("ProxyClient")
        .class_function("call", &xplpc::client::ProxyClient::call)
        .class_function("callAsync", &xplpc::client::ProxyClient::callAsync);
}

// Bind: PlatformProxy
struct PlatformProxyWrapper : public em::wrapper<xplpc::proxy::PlatformProxy>
{
    EMSCRIPTEN_WRAPPER(PlatformProxyWrapper);
    std::string doProxyCall(const std::string &data)
    {
        return call<em::val>("onRemoteProxyCall", data).await().as<std::string>();
    }
};

EMSCRIPTEN_BINDINGS(xplpc_proxy_platform_proxy)
{
    em::class_<xplpc::proxy::PlatformProxy>("PlatformProxy")
        .constructor<>()
        .smart_ptr<std::shared_ptr<xplpc::proxy::PlatformProxy>>("PlatformProxy")
        .allow_subclass<PlatformProxyWrapper>("PlatformProxyWrapper")
        .class_function("shared", &xplpc::proxy::PlatformProxy::shared)
        .class_function("create", &xplpc::proxy::PlatformProxy::create)
        .class_function("createDefault", &xplpc::proxy::PlatformProxy::createDefault)
        .class_function("createFromPtr", &xplpc::proxy::PlatformProxy::createFromPtr, em::allow_raw_pointer<em::arg<0>>())
        .class_function("hasProxy", &xplpc::proxy::PlatformProxy::hasProxy)
        .function("initialize", &xplpc::proxy::PlatformProxy::initialize)
        .function("onRemoteProxyCall", &xplpc::proxy::PlatformProxy::doProxyCall, em::pure_virtual());
}

// TODO: XPLPC - REMOVE ALL AFTER TESTS

#include "xplpc/xplpc.hpp"

class HelloClass
{
public:
    static std::string SayHello(const std::string &data)
    {
        spdlog::info("[SayHello 1] {}", data);

        auto request = xplpc::message::Request{
            "platform.battery.level",
            xplpc::message::Param<std::string>{"suffix", "%"},
        };

        spdlog::info("[SayHello 2] {}", request.data());

        auto response = xplpc::client::RemoteClient::call<std::string>(request);
        spdlog::info("[SayHello 3] {}", response.value());

        return "";
    };
};

EMSCRIPTEN_BINDINGS(Hello)
{
    emscripten::class_<HelloClass>("HelloClass")
        .constructor<>()
        .class_function("SayHello", &HelloClass::SayHello);
}
