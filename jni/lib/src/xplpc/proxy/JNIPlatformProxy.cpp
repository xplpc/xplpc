#include "xplpc/proxy/JNIPlatformProxy.hpp"

namespace xplpc
{
namespace proxy
{

std::string JNIPlatformProxy::call(const std::string &data)
{
    auto env = jniGetThreadEnv();
    jclass clazz = env->FindClass("com/xplpc/proxy/PlatformProxy");
    jmethodID methodId = env->GetStaticMethodID(clazz, "onRemoteClientCall", "(Ljava/lang/String;)Ljava/lang/String;");

    auto ret = (jstring)env->CallStaticObjectMethod(clazz, methodId, xplpc::jni::jniStringFromUTF8(env, data));
    return xplpc::jni::jniUTF8FromString(env, ret);
}

void JNIPlatformProxy::finalize()
{
    PlatformProxy::finalize();
    finalizeNativePlatform();
}

void JNIPlatformProxy::initializeNativePlatform(JavaVM *jvm)
{
    this->javaVM = jvm;
}

void JNIPlatformProxy::finalizeNativePlatform()
{
    this->javaVM = nullptr;
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
        pthread_setspecific(threadExitCallbackKey, env);
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

    // Special case: this allows us to ignore GlobalRef deletions that happen after this
    // thread has been detached. (This is known to happen during process shutdown, when
    // there's no need to release the ref anyway.)
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

} // namespace proxy
} // namespace xplpc
