#pragma once

#include "xplpc/jni/JNIEnvironment.hpp"

#include <jni.h>

namespace xplpc
{
namespace jni
{

// Keeps a java object alive across threads and deletes the reference on whichever thread lets it go last.
class ScopedGlobalRef
{
public:
    ScopedGlobalRef(JavaVM *javaVM, JNIEnv *env, jobject local)
        : javaVM(javaVM)
        , ref(env->NewGlobalRef(local))
    {
    }

    ~ScopedGlobalRef()
    {
        auto env = JNIEnvironment::current(javaVM);

        if (env && ref)
        {
            env->DeleteGlobalRef(ref);
        }
    }

    ScopedGlobalRef(const ScopedGlobalRef &) = delete;
    ScopedGlobalRef &operator=(const ScopedGlobalRef &) = delete;

    jobject get() const { return ref; }

private:
    JavaVM *javaVM;
    jobject ref;
};

} // namespace jni
} // namespace xplpc
