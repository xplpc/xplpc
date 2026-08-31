#pragma once

#include "xplpc/proxy/PlatformProxy.hpp"

#include <atomic>
#include <jni.h>
#include <memory>
#include <mutex>
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
    void finalize();
    void finalizePlatform();
    virtual void callProxy(const std::string &key, const std::string &data) override;
    virtual bool hasMapping(const std::string &name) override;

    void callProxyCallback(const std::string &key, const std::string &data);
    void setJavaVM(JavaVM *jvm);

private:
    static constexpr const char *coreClassName = "com/xplpc/core/XPLPC";
    static constexpr const char *platformProxyClassName = "com/xplpc/proxy/PlatformProxy";

    static std::shared_ptr<JNIPlatformProxy> instance;
    static std::once_flag initInstanceFlag;

    // A mapping may answer from a thread of its own long after the bridge was finalized, so the entry points are published atomically.
    std::atomic<JavaVM *> javaVM{nullptr};
    std::atomic<jclass> platformProxyClass{nullptr};
    std::atomic<jmethodID> onNativeProxyCallMethodID{nullptr};
    std::atomic<jmethodID> onNativeProxyCallbackMethodID{nullptr};
    std::atomic<jmethodID> onHasMappingMethodID{nullptr};

    std::atomic<jobject> classLoader{nullptr};
    std::atomic<jmethodID> classLoaderMethodID{nullptr};
    jmethodID onInitializePlatformMethodID = nullptr;
    jmethodID onFinalizePlatformMethodID = nullptr;

    JNIEnv *jniGetThreadEnv() const;
    jclass jniFindClass(JNIEnv *env, const char *name) const;
    bool cacheClassLoader(JNIEnv *env);
    bool cacheEntryPoints(JNIEnv *env, jclass bridgeClass);
    bool clearPendingException(JNIEnv *env, const char *source) const;
    bool callStaticVoid(jmethodID method, const char *source, const std::string &key, const std::string &data);

    JNIPlatformProxy() = default;
    JNIPlatformProxy(const JNIPlatformProxy &) = delete;
    JNIPlatformProxy &operator=(const JNIPlatformProxy &) = delete;
};

} // namespace proxy
} // namespace xplpc
