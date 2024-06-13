#include "xplpc/proxy/JNIPlatformProxy.hpp"
#include <cassert>

namespace xplpc
{
namespace proxy
{

std::shared_ptr<JNIPlatformProxy> JNIPlatformProxy::instance = nullptr;

std::shared_ptr<JNIPlatformProxy> JNIPlatformProxy::shared()
{
    if (instance == nullptr)
    {
        instance = std::make_shared<JNIPlatformProxy>();
    }

    return instance;
}

void JNIPlatformProxy::initialize()
{
    initializePlatform();
}

void JNIPlatformProxy::initializePlatform()
{
    // set jvm from platform jvm
    static ::jni::JvmRef<::jni::kDefaultJvm> jvm{platformJavaVM};

    // call initialize platform method
    ::jni::StaticRef<kPlatformProxy>{}("onInitializePlatform");
}

void JNIPlatformProxy::finalize()
{
    finalizePlatform();
}

void JNIPlatformProxy::finalizePlatform()
{
    ::jni::StaticRef<kPlatformProxy>{}("onFinalizePlatform");

    // reset
    this->platformJavaVM = nullptr;
}

void JNIPlatformProxy::callProxy(const std::string &key, const std::string &data)
{
    ::jni::StaticRef<kPlatformProxy>{}("onNativeProxyCall", key, data);
}

bool JNIPlatformProxy::hasMapping(const std::string &name)
{
    return ::jni::StaticRef<kPlatformProxy>{}("onHasMapping", name);
}

void JNIPlatformProxy::onNativeProxyCallback(const jstring &key, const std::string &data)
{
    ::jni::StaticRef<kPlatformProxy>{}("onNativeProxyCallback", key, data);
}

void JNIPlatformProxy::setPlatformJavaVM(JavaVM *pjvm)
{
    this->platformJavaVM = pjvm;
}

} // namespace proxy
} // namespace xplpc
