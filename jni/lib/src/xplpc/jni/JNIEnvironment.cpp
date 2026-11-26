#include "xplpc/jni/JNIEnvironment.hpp"
#include "xplpc/jni/ScopedThreadAttachment.hpp"

namespace xplpc
{
namespace jni
{

JNIEnv *JNIEnvironment::current(JavaVM *javaVM)
{
    if (!javaVM)
    {
        return nullptr;
    }

    JNIEnv *env = nullptr;

    if (javaVM->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) == JNI_OK)
    {
        return env;
    }

    // The attachment lives as long as the thread does, so a mapping is free to answer from any thread it creates.
    static thread_local ScopedThreadAttachment attachment{javaVM};

    return attachment.get();
}

} // namespace jni
} // namespace xplpc
