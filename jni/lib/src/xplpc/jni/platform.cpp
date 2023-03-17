#include "xplpc/client/Client.hpp"
#include "xplpc/data/CallbackList.hpp"
#include "xplpc/data/PlatformProxyList.hpp"
#include "xplpc/jni/support.hpp"
#include "xplpc/proxy/JNIPlatformProxy.hpp"
#include "xplpc/proxy/NativePlatformProxy.hpp"

#include <cstdint>
#include <memory>

using namespace xplpc::client;
using namespace xplpc::data;
using namespace xplpc::proxy;
using namespace xplpc::jni;

extern "C"
{
    JNIEXPORT jint JNICALL
    JNI_OnLoad(JavaVM *jvm, void * /*reserved*/)
    {
        // initialize native platform proxy
        auto nativePlatformProxy = std::make_shared<NativePlatformProxy>();
        nativePlatformProxy->initialize();
        PlatformProxyList::shared()->insert(0, nativePlatformProxy);

        // jni platform proxy
        auto jniPlatformProxy = JNIPlatformProxy::shared();
        jniPlatformProxy->setJavaVM(jvm);
        jniPlatformProxy->initialize();

        PlatformProxyList::shared()->insert(0, jniPlatformProxy);

        return JNI_VERSION_1_6;
    }

    JNIEXPORT void JNICALL
    JNI_OnUnload(JavaVM * /*jvm*/, void * /*reserved*/)
    {
        JNIPlatformProxy::shared()->finalize();
    }

    JNIEXPORT void JNICALL
    Java_com_xplpc_proxy_PlatformProxy_nativeProxyCall(JNIEnv *env, jclass /*clazz*/, jstring key, jstring data)
    {
        // clang-format off
        Client::call(jniUTF8FromString(env, data), [key](const auto &response) {
            auto proxy = JNIPlatformProxy::shared();
            auto env = proxy->jniGetThreadEnv();

            jclass clazz = proxy->jniFindClass("com/xplpc/proxy/PlatformProxy");
            jmethodID methodID = env->GetStaticMethodID(clazz, "onCallProxyCallback", "(Ljava/lang/String;Ljava/lang/String;)V");

            env->CallStaticVoidMethod(clazz, methodID, key, jniStringFromUTF8(env, response));
        });
        // clang-format on
    }

    JNIEXPORT void JNICALL
    Java_com_xplpc_proxy_PlatformProxy_nativeCallProxyCallback(JNIEnv *env, jclass /*clazz*/, jstring key, jstring data)
    {
        CallbackList::shared()->execute(jniUTF8FromString(env, key), jniUTF8FromString(env, data));
    }

    JNIEXPORT jlong JNICALL
    Java_com_xplpc_helper_ByteBufferHelper_getPtrAddress(JNIEnv *env, jobject /*thiz*/, jobject data)
    {
        auto pointer = reinterpret_cast<uint8_t *>(env->GetDirectBufferAddress(data));
        auto address = reinterpret_cast<std::uintptr_t>(pointer);
        return static_cast<jlong>(address);
    }

    JNIEXPORT jlong JNICALL
    Java_com_xplpc_helper_ByteArrayHelper_getPtrAddress(JNIEnv *env, jobject /*thiz*/, jbyteArray data)
    {
        auto pointer = (jbyte *)env->GetPrimitiveArrayCritical(data, nullptr);
        auto address = reinterpret_cast<std::uintptr_t>(pointer);
        env->ReleasePrimitiveArrayCritical(data, pointer, JNI_ABORT);
        return static_cast<jlong>(address);
    }

    JNIEXPORT jbyteArray JNICALL
    Java_com_xplpc_helper_ByteArrayHelper_createFromPtr(JNIEnv *env, jobject thiz, jlong ptr, jint size)
    {
        auto byteArray = env->NewByteArray(static_cast<jsize>(size));
        auto dataPtr = reinterpret_cast<uint8_t *>(ptr);
        env->SetByteArrayRegion(byteArray, 0, size, reinterpret_cast<const jbyte *>(dataPtr));
        return byteArray;
    }
}
