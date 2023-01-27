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
    virtual void callProxy(const std::string &key, const std::string &data) override;
    virtual void finalize() override;

    void initializeNativePlatform(JavaVM *jvm);
    void finalizeNativePlatform();
    JNIEnv *jniGetThreadEnv();
    JNIEnv *jniGetOptThreadEnv();
    jclass jniFindClass(const char *name);

private:
    pthread_key_t threadExitCallbackKey;
    JavaVM *javaVM;
    jobject classLoader;
    jmethodID classLoaderMethodID;
};

} // namespace proxy
} // namespace xplpc
