#pragma once

#include "xplpc/c/typedefs.h"
#include "xplpc/proxy/PlatformProxy.hpp"

#include <atomic>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_set>

namespace xplpc
{
namespace proxy
{

class CNativePlatformProxy : public PlatformProxy
{
public:
    static std::shared_ptr<CNativePlatformProxy> shared();
    static void registerProxies(bool initializeCxxNativePlatformProxy);

    virtual void initialize() override;
    virtual void initializePlatform() override;
    void finalize();
    void finalizePlatform();
    virtual void callProxy(const std::string &key, const std::string &data) override;
    virtual bool hasMapping(const std::string &name) override;

    void addMapping(const std::string &name);
    void clearMappings();

    void answer(const std::string &key, const std::string &data) const;

    void setFuncPtrToOnInitializePlatform(FuncPtrToOnInitializePlatform value);
    void setFuncPtrToOnFinalizePlatform(FuncPtrToOnFinalizePlatform value);
    void setFuncPtrToOnNativeProxyCall(FuncPtrToOnNativeProxyCall value);
    void setFuncPtrToOnNativeProxyCallback(FuncPtrToOnNativeProxyCallback value);
    void setFuncPtrToOnNativeProxyCallFromThread(FuncPtrToOnNativeProxyCallFromThread value);
    void setFuncPtrToOnNativeProxyCallbackFromThread(FuncPtrToOnNativeProxyCallbackFromThread value);

private:
    static std::shared_ptr<CNativePlatformProxy> instance;
    static std::once_flag initInstanceFlag;
    static std::once_flag proxyRegistrationFlag;

    static bool sendOwned(FuncPtrToOnHostBufferOwner callback, const std::string &key, const std::string &data);

    std::atomic<FuncPtrToOnInitializePlatform> funcPtrToOnInitializePlatform{nullptr};
    std::atomic<FuncPtrToOnFinalizePlatform> funcPtrToOnFinalizePlatform{nullptr};
    std::atomic<FuncPtrToOnNativeProxyCall> funcPtrToOnNativeProxyCall{nullptr};
    std::atomic<FuncPtrToOnNativeProxyCallback> funcPtrToOnNativeProxyCallback{nullptr};
    std::atomic<FuncPtrToOnNativeProxyCallFromThread> funcPtrToOnNativeProxyCallFromThread{nullptr};
    std::atomic<FuncPtrToOnNativeProxyCallbackFromThread> funcPtrToOnNativeProxyCallbackFromThread{nullptr};

    std::unordered_set<std::string> mappings;
    mutable std::shared_mutex mappingsMutex;

    CNativePlatformProxy() = default;
    CNativePlatformProxy(const CNativePlatformProxy &) = delete;
    CNativePlatformProxy &operator=(const CNativePlatformProxy &) = delete;
};

} // namespace proxy
} // namespace xplpc
