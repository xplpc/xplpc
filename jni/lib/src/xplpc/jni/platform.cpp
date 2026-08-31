#include "xplpc/client/Client.hpp"
#include "xplpc/core/XPLPC.hpp"
#include "xplpc/data/CallbackList.hpp"
#include "xplpc/data/PlatformProxyList.hpp"
#include "xplpc/jni/JNISupport.hpp"
#include "xplpc/proxy/JNIPlatformProxy.hpp"
#include "xplpc/proxy/NativePlatformProxy.hpp"
#include "xplpc/util/NativeBoundary.hpp"

#include <cstdint>
#include <memory>
#include <string>

using namespace xplpc::client;
using namespace xplpc::data;
using namespace xplpc::jni;
using namespace xplpc::proxy;
using namespace xplpc::util;

extern "C"
{
    JNIEXPORT jint JNICALL
    JNI_OnLoad(JavaVM *jvm, void * /*reserved*/)
    {
        // clang-format off
        NativeBoundary::run("JNI_OnLoad", [jvm] {
            auto nativePlatformProxy = std::make_shared<NativePlatformProxy>();
            nativePlatformProxy->initialize();
            PlatformProxyList::shared()->prepend(nativePlatformProxy);

            auto jniPlatformProxy = JNIPlatformProxy::shared();
            jniPlatformProxy->setJavaVM(jvm);
            jniPlatformProxy->initialize();

            PlatformProxyList::shared()->prepend(jniPlatformProxy);

            // The flag is set once both proxies are registered, so nothing reports itself as ready while a call would find no host.
            xplpc::core::XPLPC::initialize();
        });
        // clang-format on

        return JNI_VERSION_1_6;
    }

    JNIEXPORT void JNICALL
    JNI_OnUnload(JavaVM * /*jvm*/, void * /*reserved*/)
    {
        // clang-format off
        NativeBoundary::run("JNI_OnUnload", [] {
            JNIPlatformProxy::shared()->finalize();
        });
        // clang-format on
    }

    JNIEXPORT void JNICALL
    Java_com_xplpc_proxy_PlatformProxy_callNativeProxy(JNIEnv *env, jclass /*clazz*/, jstring key, jstring data)
    {
        // clang-format off
        NativeBoundary::run("callNativeProxy", [env, key, data] {
            // The key is a local reference bound to this frame, so it is materialized before the callback can outlive the call.
            const auto ownedKey = JNISupport::xplpcJniStringToCppString(env, key);

            Client::call(JNISupport::xplpcJniStringToCppString(env, data), [ownedKey](const std::string &response) {
                JNIPlatformProxy::shared()->callProxyCallback(ownedKey, response);
            });
        });
        // clang-format on
    }

    JNIEXPORT void JNICALL
    Java_com_xplpc_proxy_PlatformProxy_callNativeProxyCallback(JNIEnv *env, jclass /*clazz*/, jstring key, jstring data)
    {
        // clang-format off
        NativeBoundary::run("callNativeProxyCallback", [env, key, data] {
            CallbackList::shared()->execute(
                JNISupport::xplpcJniStringToCppString(env, key),
                JNISupport::xplpcJniStringToCppString(env, data));
        });
        // clang-format on
    }

    JNIEXPORT jlong JNICALL
    Java_com_xplpc_helper_ByteBufferHelper_getPtrAddress(JNIEnv *env, jobject /*thiz*/, jobject data)
    {
        // clang-format off
        return NativeBoundary::runAnswering("getPtrAddress", jlong{0}, [env, data] {
            auto pointer = reinterpret_cast<uint8_t *>(env->GetDirectBufferAddress(data));
            auto address = reinterpret_cast<std::uintptr_t>(pointer);
            return static_cast<jlong>(address);
        });
        // clang-format on
    }

    JNIEXPORT jbyteArray JNICALL
    Java_com_xplpc_helper_ByteArrayHelper_createFromPtr(JNIEnv *env, jobject /*thiz*/, jlong ptr, jint size)
    {
        // clang-format off
        return NativeBoundary::runAnswering("createFromPtr", jbyteArray{nullptr}, [env, ptr, size] {
            // A view carrying no address describes nothing to read, and reading it anyway is a fault rather than an empty answer.
            if (ptr == 0 || size <= 0)
            {
                return env->NewByteArray(0);
            }

            auto byteArray = env->NewByteArray(static_cast<jsize>(size));

            if (!byteArray)
            {
                return jbyteArray{nullptr};
            }

            env->SetByteArrayRegion(byteArray, 0, size, reinterpret_cast<const jbyte *>(ptr));

            return byteArray;
        });
        // clang-format on
    }
}
