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
    static std::shared_ptr<JNIPlatformProxy> shared();

    virtual void initialize() override;
    virtual void initializePlatform() override;
    virtual void finalize() override;
    virtual void finalizePlatform() override;
    virtual void callProxy(const std::string &key, const std::string &data) override;
    virtual bool hasMapping(const std::string &name) override;

    void setJavaVM(JavaVM *jvm);
    JNIEnv *jniGetThreadEnv();
    JNIEnv *jniGetOptThreadEnv();
    jclass jniFindClass(const char *name);

private:
    static std::shared_ptr<JNIPlatformProxy> instance;

    thread_local static JNIEnv *threadEnv;
    JavaVM *javaVM;
    jobject classLoader;
    jmethodID classLoaderMethodID;
};

} // namespace proxy
} // namespace xplpc
