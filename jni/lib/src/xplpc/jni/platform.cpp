#include "xplpc/client/ProxyClient.hpp"
#include "xplpc/jni/support.hpp"
#include "xplpc/proxy/JNIPlatformProxy.hpp"
#include <memory>

using namespace xplpc::client;
using namespace xplpc::proxy;
using namespace xplpc::jni;

extern "C"
{
    jint JNICALL JNI_OnLoad(JavaVM *jvm, void * /*reserved*/)
    {
        auto proxy = std::make_shared<JNIPlatformProxy>();
        proxy->initializeNativePlatform(jvm);

        PlatformProxy::create(proxy);
        PlatformProxy::shared()->initialize();

        return JNI_VERSION_1_6;
    }

    JNIEXPORT void JNICALL JNI_OnUnload(JavaVM * /*jvm*/, void * /*reserved*/)
    {
        PlatformProxy::shared()->finalize();
    }

    JNIEXPORT jstring JNICALL
    Java_com_xplpc_proxy_PlatformProxy_call(JNIEnv *env, jclass thiz, jstring data)
    {
        return jniStringFromUTF8(env, ProxyClient::call(jniUTF8FromString(env, data)));
    }
}
