#include "xplpc/core/BufferTable.hpp"
#include "xplpc/core/CallState.hpp"
#include "xplpc/core/Router.hpp"
#include "xplpc/jni/JNISupport.hpp"
#include "xplpc/layer/JNILayer.hpp"
#include "xplpc/util/Log.hpp"
#include "xplpc/util/NativeBoundary.hpp"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>

using namespace xplpc::core;
using namespace xplpc::jni;
using namespace xplpc::layer;
using namespace xplpc::util;

extern "C"
{
    JNIEXPORT jint JNICALL
    JNI_OnLoad(JavaVM *jvm, void * /*reserved*/)
    {
        // A bridge whose kotlin side did not resolve is refused here, so loading the library fails loudly instead of attaching a layer that cannot reach kotlin.

        // clang-format off
        return NativeBoundary::runAnswering("JNI_OnLoad", jint{JNI_ERR}, [jvm] {
            JNIEnv *env = nullptr;

            if (jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK)
            {
                Log::e("[JNI_OnLoad] The loading thread carries no environment");
                return jint{JNI_ERR};
            }

            auto layer = JNILayer::create(jvm, env);

            if (!layer)
            {
                return jint{JNI_ERR};
            }

            JNILayer::publish(Router::shared().attach(layer));

            return jint{JNI_VERSION_1_6};
        });
        // clang-format on
    }

    JNIEXPORT void JNICALL
    Java_com_xplpc_layer_Layer_declare(JNIEnv *env, jclass /*clazz*/, jstring name)
    {
        // clang-format off
        NativeBoundary::run("declare", [env, name] {
            Router::shared().declare(JNILayer::attachedHandle(), JNISupport::xplpcJniStringToCppString(env, name));
        });
        // clang-format on
    }

    JNIEXPORT void JNICALL
    Java_com_xplpc_layer_Layer_retract(JNIEnv *env, jclass /*clazz*/, jstring name)
    {
        // clang-format off
        NativeBoundary::run("retract", [env, name] {
            Router::shared().retract(JNILayer::attachedHandle(), JNISupport::xplpcJniStringToCppString(env, name));
        });
        // clang-format on
    }

    JNIEXPORT jlong JNICALL
    Java_com_xplpc_layer_Layer_call(JNIEnv *env, jclass /*clazz*/, jstring request, jobject pendingCall)
    {
        // Answers the handle of a call still waiting, or zero once the kotlin object was answered, which happens before this returns when the mapping answered inline.

        // clang-format off
        const auto routed = NativeBoundary::runAnswering("call", std::optional<jlong>{}, [env, request, pendingCall] {
            auto layer = JNILayer::attached();

            if (!layer)
            {
                Log::e("[call] The bridge is gone, so this call cannot be routed");
                return std::optional<jlong>{};
            }

            auto kept = layer->keep(env, pendingCall);

            const Completion completion = [layer, kept](const std::string &response) {
                layer->answer(kept->get(), response);
            };

            const auto result = Router::shared().call(JNILayer::attachedHandle(), JNISupport::xplpcJniStringToCppString(env, request), completion);

            if (result.isAnswered())
            {
                CallState::run(completion, result.value());
            }

            return std::optional<jlong>{static_cast<jlong>(result.handle())};
        });
        // clang-format on

        if (!routed)
        {
            JNILayer::answerUnrouted(env, pendingCall);
            return 0;
        }

        return routed.value();
    }

    JNIEXPORT jboolean JNICALL
    Java_com_xplpc_layer_Layer_cancel(JNIEnv * /*env*/, jclass /*clazz*/, jlong call)
    {
        // clang-format off
        return NativeBoundary::runAnswering("cancel", jboolean{JNI_FALSE}, [call] {
            return Router::shared().cancel(JNILayer::attachedHandle(), static_cast<Handle>(call)) ? jboolean{JNI_TRUE} : jboolean{JNI_FALSE};
        });
        // clang-format on
    }

    JNIEXPORT jlong JNICALL
    Java_com_xplpc_layer_Layer_pending(JNIEnv * /*env*/, jclass /*clazz*/)
    {
        // clang-format off
        return NativeBoundary::runAnswering("pending", jlong{0}, [] {
            return static_cast<jlong>(Router::shared().pending());
        });
        // clang-format on
    }

    JNIEXPORT void JNICALL
    Java_com_xplpc_layer_Layer_answer(JNIEnv *env, jclass /*clazz*/, jlong call, jstring response)
    {
        // clang-format off
        NativeBoundary::run("answer", [env, call, response] {
            Router::shared().answer(static_cast<Handle>(call), JNISupport::xplpcJniStringToCppString(env, response));
        });
        // clang-format on
    }

    JNIEXPORT jlong JNICALL
    Java_com_xplpc_layer_Layer_registerBuffer(JNIEnv *env, jclass /*clazz*/, jobject data)
    {
        // The buffer is held through a global reference for as long as it is registered, so forgetting to release it leaks the bytes rather than freeing them under a reader.

        // clang-format off
        return NativeBoundary::runAnswering("registerBuffer", jlong{0}, [env, data] {
            auto layer = JNILayer::attached();
            auto address = static_cast<std::uint8_t *>(env->GetDirectBufferAddress(data));

            if (!layer)
            {
                Log::e("[registerBuffer] The bridge is gone, so the buffer cannot be registered");
                return jlong{0};
            }

            if (!address)
            {
                Log::e("[registerBuffer] The buffer has no address native code can use");
                return jlong{0};
            }

            const auto size = static_cast<size_t>(env->GetDirectBufferCapacity(data));

            return static_cast<jlong>(BufferTable::shared().add(JNILayer::attachedHandle(), address, size, layer->keep(env, data)));
        });
        // clang-format on
    }

    JNIEXPORT jboolean JNICALL
    Java_com_xplpc_layer_Layer_releaseBuffer(JNIEnv * /*env*/, jclass /*clazz*/, jlong buffer)
    {
        // clang-format off
        return NativeBoundary::runAnswering("releaseBuffer", jboolean{JNI_FALSE}, [buffer] {
            return BufferTable::shared().release(JNILayer::attachedHandle(), static_cast<Handle>(buffer)) ? jboolean{JNI_TRUE} : jboolean{JNI_FALSE};
        });
        // clang-format on
    }

    JNIEXPORT jbyteArray JNICALL
    Java_com_xplpc_layer_Layer_readBuffer(JNIEnv *env, jclass /*clazz*/, jlong buffer)
    {
        // clang-format off
        return NativeBoundary::runAnswering("readBuffer", jbyteArray{nullptr}, [env, buffer] {
            const auto span = BufferTable::shared().read(static_cast<Handle>(buffer));

            if (!span)
            {
                return jbyteArray{nullptr};
            }

            // A java array is indexed by a signed 32 bit length, so a larger buffer would be read short or not at all.
            if (span->size > static_cast<size_t>((std::numeric_limits<jsize>::max)()))
            {
                Log::e("[Layer : readBuffer] The buffer is larger than a java array can hold");
                return jbyteArray{nullptr};
            }

            auto bytes = env->NewByteArray(static_cast<jsize>(span->size));

            if (!bytes)
            {
                return jbyteArray{nullptr};
            }

            env->SetByteArrayRegion(bytes, 0, static_cast<jsize>(span->size), reinterpret_cast<const jbyte *>(span->data));

            return bytes;
        });
        // clang-format on
    }
}
