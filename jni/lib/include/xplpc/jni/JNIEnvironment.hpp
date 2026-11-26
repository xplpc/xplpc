#pragma once

#include <jni.h>

namespace xplpc
{
namespace jni
{

class JNIEnvironment
{
public:
    // Answers the environment of the current thread, attaching a thread the vm did not start for as long as that thread lives.
    static JNIEnv *current(JavaVM *javaVM);
};

} // namespace jni
} // namespace xplpc
