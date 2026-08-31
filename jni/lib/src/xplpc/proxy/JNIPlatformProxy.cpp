#include "xplpc/proxy/JNIPlatformProxy.hpp"
#include "xplpc/data/CallbackList.hpp"
#include "xplpc/jni/JNISupport.hpp"
#include "xplpc/jni/ScopedLocalRef.hpp"
#include "xplpc/jni/ScopedThreadAttachment.hpp"
#include "xplpc/util/Log.hpp"

namespace xplpc
{
namespace proxy
{

using namespace xplpc::data;
using namespace xplpc::jni;

std::shared_ptr<JNIPlatformProxy> JNIPlatformProxy::instance = nullptr;
std::once_flag JNIPlatformProxy::initInstanceFlag;

std::shared_ptr<JNIPlatformProxy> JNIPlatformProxy::shared()
{
    // clang-format off
    std::call_once(initInstanceFlag, []() {
        instance = std::shared_ptr<JNIPlatformProxy>(new JNIPlatformProxy());
    });
    // clang-format on

    return instance;
}

void JNIPlatformProxy::initialize()
{
    initializePlatform();
}

void JNIPlatformProxy::initializePlatform()
{
    auto env = jniGetThreadEnv();

    if (!env)
    {
        util::Log::e("[JNIPlatformProxy : initializePlatform] No java vm was set");
        return;
    }

    if (!cacheClassLoader(env))
    {
        return;
    }

    ScopedLocalRef proxyClass{env, jniFindClass(env, platformProxyClassName)};

    if (!proxyClass.get())
    {
        clearPendingException(env, "initializePlatform");
        util::Log::e("[JNIPlatformProxy : initializePlatform] Class {} was not found", platformProxyClassName);
        return;
    }

    auto cachedClass = static_cast<jclass>(env->NewGlobalRef(proxyClass.get()));

    if (!cacheEntryPoints(env, cachedClass))
    {
        env->DeleteGlobalRef(cachedClass);
        util::Log::e("[JNIPlatformProxy : initializePlatform] Class {} does not carry every entry point", platformProxyClassName);
        return;
    }

    // The class is published last, so no thread can reach an entry point that is not resolved yet.
    platformProxyClass.store(cachedClass);

    env->CallStaticVoidMethod(cachedClass, onInitializePlatformMethodID);
    clearPendingException(env, "onInitializePlatform");
}

bool JNIPlatformProxy::cacheClassLoader(JNIEnv *env)
{
    // The class loader is cached here because a thread the vm did not create cannot resolve our classes through FindClass.

    ScopedLocalRef ourClass{env, env->FindClass(coreClassName)};

    if (clearPendingException(env, "cacheClassLoader") || !ourClass.get())
    {
        util::Log::e("[JNIPlatformProxy : cacheClassLoader] Class {} was not found", coreClassName);
        return false;
    }

    ScopedLocalRef classClass{env, env->GetObjectClass(ourClass.get())};
    auto getClassLoader = env->GetMethodID(classClass.asClass(), "getClassLoader", "()Ljava/lang/ClassLoader;");

    if (clearPendingException(env, "cacheClassLoader") || !getClassLoader)
    {
        util::Log::e("[JNIPlatformProxy : cacheClassLoader] Class does not carry getClassLoader");
        return false;
    }

    ScopedLocalRef loader{env, env->CallObjectMethod(ourClass.get(), getClassLoader)};

    if (clearPendingException(env, "cacheClassLoader") || !loader.get())
    {
        util::Log::e("[JNIPlatformProxy : cacheClassLoader] Class {} carries no class loader", coreClassName);
        return false;
    }

    ScopedLocalRef classLoaderClass{env, env->FindClass("java/lang/ClassLoader")};

    if (clearPendingException(env, "cacheClassLoader") || !classLoaderClass.get())
    {
        util::Log::e("[JNIPlatformProxy : cacheClassLoader] Class java/lang/ClassLoader was not found");
        return false;
    }

    auto loadClass = env->GetMethodID(classLoaderClass.asClass(), "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");

    if (clearPendingException(env, "cacheClassLoader") || !loadClass)
    {
        util::Log::e("[JNIPlatformProxy : cacheClassLoader] ClassLoader does not carry loadClass");
        return false;
    }

    // The method id is published before the loader, so a thread that sees the loader can already reach through it.
    classLoaderMethodID.store(loadClass);
    classLoader.store(env->NewGlobalRef(loader.get()));

    return true;
}

bool JNIPlatformProxy::cacheEntryPoints(JNIEnv *env, jclass bridgeClass)
{
    // Nothing is published unless every entry point resolved, since calling through a null method id aborts the runtime.

    auto call = env->GetStaticMethodID(bridgeClass, "onNativeProxyCall", "(Ljava/lang/String;Ljava/lang/String;)V");
    auto callback = env->GetStaticMethodID(bridgeClass, "onNativeProxyCallback", "(Ljava/lang/String;Ljava/lang/String;)V");
    auto mapping = env->GetStaticMethodID(bridgeClass, "onHasMapping", "(Ljava/lang/String;)Z");
    auto platformInitialize = env->GetStaticMethodID(bridgeClass, "onInitializePlatform", "()V");
    auto platformFinalize = env->GetStaticMethodID(bridgeClass, "onFinalizePlatform", "()V");

    clearPendingException(env, "cacheEntryPoints");

    if (!call || !callback || !mapping || !platformInitialize || !platformFinalize)
    {
        return false;
    }

    onNativeProxyCallMethodID.store(call);
    onNativeProxyCallbackMethodID.store(callback);
    onHasMappingMethodID.store(mapping);
    onInitializePlatformMethodID = platformInitialize;
    onFinalizePlatformMethodID = platformFinalize;

    return true;
}

void JNIPlatformProxy::finalize()
{
    finalizePlatform();
}

void JNIPlatformProxy::finalizePlatform()
{
    auto cachedClass = platformProxyClass.exchange(nullptr);

    if (!cachedClass)
    {
        return;
    }

    auto env = jniGetThreadEnv();

    if (!env)
    {
        // The class was already taken above, so the java side is never notified and the references it held cannot be released from here.
        util::Log::e("[JNIPlatformProxy : finalizePlatform] There is no environment on this thread, so the bridge cannot be finalized");
        return;
    }

    // The java side is notified before any cached reference is dropped, otherwise there is no way left to reach it.
    env->CallStaticVoidMethod(cachedClass, onFinalizePlatformMethodID);
    clearPendingException(env, "onFinalizePlatform");

    // The entry points stop resolving before the references they point at are released.
    onNativeProxyCallMethodID.store(nullptr);
    onNativeProxyCallbackMethodID.store(nullptr);
    onHasMappingMethodID.store(nullptr);
    javaVM.store(nullptr);

    auto cachedClassLoader = classLoader.exchange(nullptr);
    classLoaderMethodID.store(nullptr);

    env->DeleteGlobalRef(cachedClass);

    if (cachedClassLoader)
    {
        env->DeleteGlobalRef(cachedClassLoader);
    }

    onInitializePlatformMethodID = nullptr;
    onFinalizePlatformMethodID = nullptr;
}

void JNIPlatformProxy::callProxy(const std::string &key, const std::string &data)
{
    if (callStaticVoid(onNativeProxyCallMethodID.load(), "onNativeProxyCall", key, data))
    {
        return;
    }

    // The caller is answered with the empty value rather than left waiting for a response that can no longer arrive.
    CallbackList::shared()->execute(key, "");
}

void JNIPlatformProxy::callProxyCallback(const std::string &key, const std::string &data)
{
    callStaticVoid(onNativeProxyCallbackMethodID.load(), "onNativeProxyCallback", key, data);
}

bool JNIPlatformProxy::callStaticVoid(jmethodID method, const char *source, const std::string &key, const std::string &data)
{
    auto cachedClass = platformProxyClass.load();
    auto env = jniGetThreadEnv();

    if (!cachedClass || !method || !env)
    {
        util::Log::e("[JNIPlatformProxy : {}] The bridge is gone, so this call cannot reach java", source);
        return false;
    }

    ScopedLocalRef jniKey{env, JNISupport::xplpcCppStringToJniString(env, key)};
    ScopedLocalRef jniData{env, JNISupport::xplpcCppStringToJniString(env, data)};

    env->CallStaticVoidMethod(cachedClass, method, jniKey.get(), jniData.get());

    // A java side that raised did not deliver the call, so the caller is answered rather than left waiting.
    return !clearPendingException(env, source);
}

bool JNIPlatformProxy::hasMapping(const std::string &name)
{
    auto cachedClass = platformProxyClass.load();
    auto method = onHasMappingMethodID.load();
    auto env = jniGetThreadEnv();

    if (!cachedClass || !method || !env)
    {
        return false;
    }

    ScopedLocalRef jniName{env, JNISupport::xplpcCppStringToJniString(env, name)};

    const auto result = env->CallStaticBooleanMethod(cachedClass, method, jniName.get());

    if (clearPendingException(env, "onHasMapping"))
    {
        return false;
    }

    return result;
}

bool JNIPlatformProxy::clearPendingException(JNIEnv *env, const char *source) const
{
    // A java exception left pending makes every later jni call undefined, so it is reported and cleared at the boundary.

    if (!env->ExceptionCheck())
    {
        return false;
    }

    util::Log::e("[JNIPlatformProxy : {}] The java side raised an exception", source);

    // Describing it writes the message and the stack to logcat, which is where the request would end up, so it is kept to the level a release build does not reach.
    if (util::Log::logger()->should_log(spdlog::level::debug))
    {
        env->ExceptionDescribe();
    }

    env->ExceptionClear();

    return true;
}

void JNIPlatformProxy::setJavaVM(JavaVM *jvm)
{
    javaVM.store(jvm);
}

JNIEnv *JNIPlatformProxy::jniGetThreadEnv() const
{
    auto vm = javaVM.load();

    if (!vm)
    {
        return nullptr;
    }

    JNIEnv *env = nullptr;

    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) == JNI_OK)
    {
        return env;
    }

    // The attachment lives as long as the thread does, so a mapping is free to answer from any thread it creates.
    static thread_local ScopedThreadAttachment attachment{vm};

    return attachment.get();
}

jclass JNIPlatformProxy::jniFindClass(JNIEnv *env, const char *name) const
{
    if (auto clazz = env->FindClass(name))
    {
        return clazz;
    }

    env->ExceptionClear();

    auto cachedClassLoader = classLoader.load();
    auto loadClass = classLoaderMethodID.load();

    if (!cachedClassLoader || !loadClass)
    {
        return nullptr;
    }

    ScopedLocalRef jniName{env, env->NewStringUTF(name)};

    return static_cast<jclass>(env->CallObjectMethod(cachedClassLoader, loadClass, jniName.get()));
}

} // namespace proxy
} // namespace xplpc
