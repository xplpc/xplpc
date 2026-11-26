#pragma once

#include <jni.h>

namespace xplpc
{
namespace jni
{

// The local reference is deleted as soon as it leaves scope, which keeps the local reference table flat on long lived native threads.
class ScopedLocalRef
{
public:
    ScopedLocalRef(JNIEnv *env, jobject ref)
        : env(env)
        , ref(ref)
    {
    }

    ~ScopedLocalRef()
    {
        if (ref)
        {
            env->DeleteLocalRef(ref);
        }
    }

    ScopedLocalRef(const ScopedLocalRef &) = delete;
    ScopedLocalRef &operator=(const ScopedLocalRef &) = delete;

    jobject get() const { return ref; }
    jclass asClass() const { return static_cast<jclass>(ref); }

private:
    JNIEnv *env;
    jobject ref;
};

} // namespace jni
} // namespace xplpc
