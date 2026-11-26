#pragma once

#include "xplpc/core/Handle.hpp"
#include "xplpc/core/Layer.hpp"
#include "xplpc/jni/ScopedGlobalRef.hpp"

#include <atomic>
#include <jni.h>
#include <memory>
#include <string>

namespace xplpc
{
namespace layer
{

// Kotlin attached to the router, which releases its java references when it goes.
class JNILayer : public core::Layer
{
public:
    // Answers nothing when the kotlin side does not carry every entry point, which is what a release build with broken keep rules produces.
    static std::shared_ptr<JNILayer> create(JavaVM *javaVM, JNIEnv *env);

    // The layer attached by JNI_OnLoad, which the router holds for the life of the process, since its references to the kotlin classes keep the library from ever being unloaded.
    static std::shared_ptr<JNILayer> attached();
    static core::Handle attachedHandle();
    static void publish(core::Handle handle);

    ~JNILayer() override;

    void invoke(core::Handle call, const std::string &name, const std::string &request) override;

    // Hands an answer to the kotlin object waiting on it.
    void answer(jobject pendingCall, const std::string &response) const;

    // Answers a kotlin object that never reached the router, which has no layer to answer through.
    static void answerUnrouted(JNIEnv *env, jobject pendingCall);

    std::shared_ptr<jni::ScopedGlobalRef> keep(JNIEnv *env, jobject object) const;

private:
    static constexpr const char *layerClassName = "com/xplpc/layer/Layer";
    static constexpr const char *pendingCallClassName = "com/xplpc/data/PendingCall";

    static std::atomic<core::Handle> handle;

    JavaVM *javaVM;
    jclass layerClass;
    jmethodID onInvokeMethod;
    jclass pendingCallClass;
    jmethodID onAnswerMethod;

    JNILayer(JavaVM *javaVM, jclass layerClass, jmethodID onInvokeMethod, jclass pendingCallClass, jmethodID onAnswerMethod);

    static bool clearPendingException(JNIEnv *env, const char *source);
};

} // namespace layer
} // namespace xplpc
