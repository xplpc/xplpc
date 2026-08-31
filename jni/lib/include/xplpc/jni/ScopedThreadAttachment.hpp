#pragma once

#include <jni.h>

namespace xplpc
{
namespace jni
{

// The thread is detached once it ends, because a native thread that exits while still attached aborts the runtime.
class ScopedThreadAttachment
{
public:
    explicit ScopedThreadAttachment(JavaVM *javaVM)
        : javaVM(javaVM)
    {
#if defined(ANDROID) || defined(__ANDROID__)
        if (javaVM->AttachCurrentThreadAsDaemon(&env, nullptr) != JNI_OK)
#else
        if (javaVM->AttachCurrentThreadAsDaemon(reinterpret_cast<void **>(&env), nullptr) != JNI_OK)
#endif
        {
            env = nullptr;
        }
    }

    ~ScopedThreadAttachment()
    {
        if (env)
        {
            javaVM->DetachCurrentThread();
        }
    }

    ScopedThreadAttachment(const ScopedThreadAttachment &) = delete;
    ScopedThreadAttachment &operator=(const ScopedThreadAttachment &) = delete;

    JNIEnv *get() const { return env; }

private:
    JavaVM *javaVM;
    JNIEnv *env = nullptr;
};

} // namespace jni
} // namespace xplpc
