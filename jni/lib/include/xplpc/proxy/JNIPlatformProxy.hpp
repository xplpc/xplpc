#pragma once

#include "xplpc/jni/support.hpp"
#include "xplpc/proxy/PlatformProxy.hpp"

#include <memory>
#include <string>

namespace xplpc
{
namespace proxy
{

class JNIPlatformProxy : public PlatformProxy
{
public:
    virtual std::string call(const std::string &data) override;
    virtual void finalize() override;

    void initializeNativePlatform(JavaVM *jvm);
    void finalizeNativePlatform();
    JNIEnv *jniGetThreadEnv();
    JNIEnv *jniGetOptThreadEnv();

private:
    pthread_key_t threadExitCallbackKey;
    JavaVM *javaVM;
};

} // namespace proxy
} // namespace xplpc
