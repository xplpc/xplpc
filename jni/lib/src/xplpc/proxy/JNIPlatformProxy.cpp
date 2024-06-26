#include "xplpc/proxy/JNIPlatformProxy.hpp"
#include <cassert>

namespace xplpc
{
namespace proxy
{

std::shared_ptr<JNIPlatformProxy> JNIPlatformProxy::instance = nullptr;
std::once_flag JNIPlatformProxy::initInstanceFlag;
thread_local JNIEnv *JNIPlatformProxy::threadEnv = nullptr;

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

    jclass ourClass = env->FindClass("com/xplpc/core/XPLPC");
    jclass classClass = env->GetObjectClass(ourClass);
    jmethodID getClassLoaderMethod = env->GetMethodID(classClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject tmp = env->CallObjectMethod(ourClass, getClassLoaderMethod);
    classLoader = (jobject)env->NewGlobalRef(tmp);

    jclass classLoaderClass = env->FindClass("java/lang/ClassLoader");
    classLoaderMethodID = env->GetMethodID(classLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");

    // call initialize platform method
    jclass clazz = jniFindClass("com/xplpc/proxy/PlatformProxy");
    jmethodID methodID = env->GetStaticMethodID(clazz, "onInitializePlatform", "()V");
    env->CallStaticVoidMethod(clazz, methodID);
}

void JNIPlatformProxy::finalize()
{
    finalizePlatform();
}

void JNIPlatformProxy::finalizePlatform()
{
    this->javaVM = nullptr;
    this->classLoader = nullptr;
    this->classLoaderMethodID = nullptr;

    // call finalize platform method
    auto env = jniGetThreadEnv();
    jclass clazz = jniFindClass("com/xplpc/proxy/PlatformProxy");
    jmethodID methodID = env->GetStaticMethodID(clazz, "onFinalizePlatform", "()V");
    env->CallStaticVoidMethod(clazz, methodID);
}

void JNIPlatformProxy::callProxy(const std::string &key, const std::string &data)
{
    auto env = jniGetThreadEnv();
    jclass clazz = jniFindClass("com/xplpc/proxy/PlatformProxy");
    jmethodID methodID = env->GetStaticMethodID(clazz, "onNativeProxyCall", "(Ljava/lang/String;Ljava/lang/String;)V");

    env->CallStaticVoidMethod(clazz, methodID, xplpc::jni::jniStringFromUTF8(env, key), xplpc::jni::jniStringFromUTF8(env, data));
}

bool JNIPlatformProxy::hasMapping(const std::string &name)
{
    auto env = jniGetThreadEnv();
    jclass clazz = jniFindClass("com/xplpc/proxy/PlatformProxy");
    jmethodID methodID = env->GetStaticMethodID(clazz, "onHasMapping", "(Ljava/lang/String;)Z");
    return env->CallStaticBooleanMethod(clazz, methodID, xplpc::jni::jniStringFromUTF8(env, name));
}

void JNIPlatformProxy::setJavaVM(JavaVM *jvm)
{
    this->javaVM = jvm;
}

JNIEnv *JNIPlatformProxy::jniGetThreadEnv()
{
    assert(javaVM);

    JNIEnv *env = nullptr;
    jint res = javaVM->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);

    if (res == JNI_EDETACHED)
    {
#if defined(ANDROID) || defined(__ANDROID__)
        res = javaVM->AttachCurrentThread(&env, nullptr);
#else
        res = javaVM->AttachCurrentThread(reinterpret_cast<void **>(&env), nullptr);
#endif
        threadEnv = env;
    }

    if (res != 0 || !env)
    {
        std::abort();
    }

    return env;
}

JNIEnv *JNIPlatformProxy::jniGetOptThreadEnv()
{
    if (!javaVM)
    {
        return nullptr;
    }

    // special case: this allows us to ignore global-ref deletions that happen after this thread has been detached
    // this is known to happen during process shutdown, when there's no need to release the ref anyway
    JNIEnv *env = nullptr;
    const jint res = javaVM->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);

    if (res == JNI_EDETACHED)
    {
        return nullptr;
    }

    // still bail on any other error
    if (res != 0 || !env)
    {
        std::abort();
    }

    return env;
}

jclass JNIPlatformProxy::jniFindClass(const char *name)
{
    JNIEnv *env = jniGetThreadEnv();
    jclass clazz = env->FindClass(name);

    if (!clazz)
    {
        env->ExceptionClear();

        // use cached class loader, needed for our classes on non-java thread
        jstring jname = env->NewStringUTF(name);
        clazz = static_cast<jclass>(env->CallObjectMethod(classLoader, classLoaderMethodID, jname));
    }

    return clazz;
}

} // namespace proxy
} // namespace xplpc
