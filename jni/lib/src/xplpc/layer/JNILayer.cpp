#include "xplpc/layer/JNILayer.hpp"
#include "xplpc/core/Router.hpp"
#include "xplpc/jni/JNIEnvironment.hpp"
#include "xplpc/jni/JNISupport.hpp"
#include "xplpc/jni/ScopedLocalRef.hpp"
#include "xplpc/util/Log.hpp"

namespace xplpc
{
namespace layer
{

using namespace xplpc::jni;

std::atomic<core::Handle> JNILayer::handle{0};

std::shared_ptr<JNILayer> JNILayer::create(JavaVM *javaVM, JNIEnv *env)
{
    // The bridge is resolved only inside JNI_OnLoad, on the thread loading the library, where FindClass sees the classes of the loader that asked.
    // A lookup that fails leaves an exception pending, and no other lookup may be made until it is cleared, so each one is checked before the next.

    ScopedLocalRef layerClass{env, env->FindClass(layerClassName)};

    if (!layerClass.get())
    {
        env->ExceptionClear();
        util::Log::e("[JNILayer : create] Class {} was not found", layerClassName);
        return nullptr;
    }

    ScopedLocalRef pendingCallClass{env, env->FindClass(pendingCallClassName)};

    if (!pendingCallClass.get())
    {
        env->ExceptionClear();
        util::Log::e("[JNILayer : create] Class {} was not found", pendingCallClassName);
        return nullptr;
    }

    // Calling through a null method id aborts the runtime, so both entry points are resolved before either is used.
    auto onInvoke = env->GetStaticMethodID(layerClass.asClass(), "onInvoke", "(JLjava/lang/String;)V");

    if (!onInvoke)
    {
        env->ExceptionClear();
        util::Log::e("[JNILayer : create] Class {} does not carry onInvoke", layerClassName);
        return nullptr;
    }

    auto onAnswer = env->GetMethodID(pendingCallClass.asClass(), "onAnswer", "(Ljava/lang/String;)V");

    if (!onAnswer)
    {
        env->ExceptionClear();
        util::Log::e("[JNILayer : create] Class {} does not carry onAnswer", pendingCallClassName);
        return nullptr;
    }

    auto cachedLayerClass = static_cast<jclass>(env->NewGlobalRef(layerClass.get()));
    auto cachedPendingCallClass = static_cast<jclass>(env->NewGlobalRef(pendingCallClass.get()));

    if (!cachedLayerClass || !cachedPendingCallClass)
    {
        util::Log::e("[JNILayer : create] There was no memory to keep the kotlin classes");

        if (cachedLayerClass)
        {
            env->DeleteGlobalRef(cachedLayerClass);
        }

        if (cachedPendingCallClass)
        {
            env->DeleteGlobalRef(cachedPendingCallClass);
        }

        return nullptr;
    }

    return std::shared_ptr<JNILayer>(new JNILayer(javaVM, cachedLayerClass, onInvoke, cachedPendingCallClass, onAnswer));
}

JNILayer::JNILayer(JavaVM *javaVM, jclass layerClass, jmethodID onInvokeMethod, jclass pendingCallClass, jmethodID onAnswerMethod)
    : javaVM(javaVM)
    , layerClass(layerClass)
    , onInvokeMethod(onInvokeMethod)
    , pendingCallClass(pendingCallClass)
    , onAnswerMethod(onAnswerMethod)
{
}

JNILayer::~JNILayer()
{
    // The layer goes only when attaching it was refused, since the router holds an attached one for the life of the process.

    if (auto env = JNIEnvironment::current(javaVM))
    {
        env->DeleteGlobalRef(layerClass);
        env->DeleteGlobalRef(pendingCallClass);
    }
    else
    {
        util::Log::e("[JNILayer : destroy] There is no environment on this thread, so the kotlin classes cannot be released");
    }
}

std::shared_ptr<JNILayer> JNILayer::attached()
{
    return std::dynamic_pointer_cast<JNILayer>(core::Router::shared().layer(attachedHandle()));
}

core::Handle JNILayer::attachedHandle()
{
    return handle.load(std::memory_order_acquire);
}

void JNILayer::publish(core::Handle attachedHandle)
{
    handle.store(attachedHandle, std::memory_order_release);
}

void JNILayer::invoke(core::Handle call, const std::string & /*name*/, const std::string &request)
{
    auto env = JNIEnvironment::current(javaVM);

    if (!env)
    {
        util::Log::e("[JNILayer : invoke] There is no environment on this thread, so the call cannot reach kotlin");
        core::Router::shared().answer(call, "");
        return;
    }

    ScopedLocalRef jniRequest{env, JNISupport::xplpcCppStringToJniString(env, request)};

    env->CallStaticVoidMethod(layerClass, onInvokeMethod, static_cast<jlong>(call), jniRequest.get());

    // Kotlin answers every call it receives, so a raise here means it never got to, and the caller is answered rather than left waiting.
    if (clearPendingException(env, "invoke"))
    {
        core::Router::shared().answer(call, "");
    }
}

void JNILayer::answer(jobject pendingCall, const std::string &response) const
{
    auto env = JNIEnvironment::current(javaVM);

    if (!env)
    {
        util::Log::e("[JNILayer : answer] There is no environment on this thread, so this answer is lost");
        return;
    }

    ScopedLocalRef jniResponse{env, JNISupport::xplpcCppStringToJniString(env, response)};

    env->CallVoidMethod(pendingCall, onAnswerMethod, jniResponse.get());
    clearPendingException(env, "answer");
}

void JNILayer::answerUnrouted(JNIEnv *env, jobject pendingCall)
{
    ScopedLocalRef pendingCallClass{env, env->GetObjectClass(pendingCall)};
    auto onAnswer = env->GetMethodID(pendingCallClass.asClass(), "onAnswer", "(Ljava/lang/String;)V");

    if (!onAnswer)
    {
        clearPendingException(env, "answerUnrouted");
        util::Log::e("[JNILayer : answerUnrouted] The kotlin side does not carry onAnswer, so this call cannot be answered");
        return;
    }

    ScopedLocalRef jniResponse{env, JNISupport::xplpcCppStringToJniString(env, "")};

    env->CallVoidMethod(pendingCall, onAnswer, jniResponse.get());
    clearPendingException(env, "answerUnrouted");
}

std::shared_ptr<ScopedGlobalRef> JNILayer::keep(JNIEnv *env, jobject object) const
{
    return std::make_shared<ScopedGlobalRef>(javaVM, env, object);
}

bool JNILayer::clearPendingException(JNIEnv *env, const char *source)
{
    // A java exception left pending makes every later jni call undefined, so it is reported and cleared at the boundary.

    if (!env->ExceptionCheck())
    {
        return false;
    }

    util::Log::e("[JNILayer : {}] The kotlin side raised an exception", source);

    // Describing it writes the message and the stack to logcat, which is where the request would end up, so it is kept to the level a release build does not reach.
    if (util::Log::logger()->should_log(spdlog::level::debug))
    {
        env->ExceptionDescribe();
    }

    env->ExceptionClear();

    return true;
}

} // namespace layer
} // namespace xplpc
