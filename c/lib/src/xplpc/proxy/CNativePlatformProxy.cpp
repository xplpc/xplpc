#include "xplpc/proxy/CNativePlatformProxy.hpp"
#include "xplpc/core/XPLPC.hpp"
#include "xplpc/data/CallbackList.hpp"
#include "xplpc/data/PlatformProxyList.hpp"
#include "xplpc/proxy/HostCallScope.hpp"
#include "xplpc/proxy/NativePlatformProxy.hpp"
#include "xplpc/util/Log.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace xplpc
{
namespace proxy
{

using namespace xplpc::data;

std::shared_ptr<CNativePlatformProxy> CNativePlatformProxy::instance = nullptr;
std::once_flag CNativePlatformProxy::initInstanceFlag;
std::once_flag CNativePlatformProxy::proxyRegistrationFlag;

std::shared_ptr<CNativePlatformProxy> CNativePlatformProxy::shared()
{
    // clang-format off
    std::call_once(initInstanceFlag, []() {
        instance = std::shared_ptr<CNativePlatformProxy>(new CNativePlatformProxy());
    });
    // clang-format on

    return instance;
}

void CNativePlatformProxy::registerProxies(bool initializeCxxNativePlatformProxy)
{
    // The proxies are registered only on the first call, so re-initializing rebinds the host callbacks instead of stacking duplicates.

    // clang-format off
    std::call_once(proxyRegistrationFlag, [initializeCxxNativePlatformProxy]() {
        if (initializeCxxNativePlatformProxy)
        {
            auto nativePlatformProxy = std::make_shared<NativePlatformProxy>();
            nativePlatformProxy->initialize();
            PlatformProxyList::shared()->prepend(nativePlatformProxy);
        }

        PlatformProxyList::shared()->prepend(shared());
    });
    // clang-format on
}

void CNativePlatformProxy::initialize()
{
    initializePlatform();
}

void CNativePlatformProxy::initializePlatform()
{
    if (auto callback = funcPtrToOnInitializePlatform.load())
    {
        callback();
    }
}

void CNativePlatformProxy::finalize()
{
    finalizePlatform();
}

void CNativePlatformProxy::finalizePlatform()
{
    // A call still waiting is answered while the host can still be reached, since every channel that could carry an answer is dropped below.
    core::XPLPC::finalize();

    if (auto callback = funcPtrToOnFinalizePlatform.exchange(nullptr))
    {
        callback();
    }

    funcPtrToOnInitializePlatform.store(nullptr);
    funcPtrToOnNativeProxyCall.store(nullptr);
    funcPtrToOnNativeProxyCallback.store(nullptr);
    funcPtrToOnNativeProxyCallFromThread.store(nullptr);
    funcPtrToOnNativeProxyCallbackFromThread.store(nullptr);

    clearMappings();
}

bool CNativePlatformProxy::hasMapping(const std::string &name)
{
    // The host declares what it owns, so this is answered without crossing the bridge and stays valid on any thread.

    std::shared_lock<std::shared_mutex> lock(mappingsMutex);
    return mappings.find(name) != mappings.end();
}

void CNativePlatformProxy::addMapping(const std::string &name)
{
    std::unique_lock<std::shared_mutex> lock(mappingsMutex);
    mappings.insert(name);
}

void CNativePlatformProxy::clearMappings()
{
    std::unique_lock<std::shared_mutex> lock(mappingsMutex);
    mappings.clear();
}

void CNativePlatformProxy::callProxy(const std::string &key, const std::string &data)
{
    if (HostCallScope::active())
    {
        if (auto callback = funcPtrToOnNativeProxyCall.load())
        {
            callback(key.c_str(), key.size(), data.c_str(), data.size());
            return;
        }
    }
    else if (auto callback = funcPtrToOnNativeProxyCallFromThread.load())
    {
        if (sendOwned(callback, key, data))
        {
            return;
        }
    }

    // The caller is answered with the empty value rather than left waiting for a response that can no longer arrive.
    util::Log::e("[CNativePlatformProxy : callProxy] The host cannot be reached from this thread");
    CallbackList::shared()->execute(key, "");
}

void CNativePlatformProxy::answer(const std::string &key, const std::string &data) const
{
    if (HostCallScope::active())
    {
        if (auto callback = funcPtrToOnNativeProxyCallback.load())
        {
            callback(key.c_str(), key.size(), data.c_str(), data.size());
            return;
        }

        util::Log::e("[CNativePlatformProxy : answer] The bridge is gone, so this answer is lost");

        return;
    }

    auto callback = funcPtrToOnNativeProxyCallbackFromThread.load();

    if (!callback)
    {
        util::Log::e("[CNativePlatformProxy : answer] The host declared no way to be answered from another thread, so this answer is lost");
        return;
    }

    if (!sendOwned(callback, key, data))
    {
        util::Log::e("[CNativePlatformProxy : answer] There was no memory to hand the answer over, so it is lost");
    }
}

bool CNativePlatformProxy::sendOwned(FuncPtrToOnHostBufferOwner callback, const std::string &key, const std::string &data)
{
    // The host reads the buffers after this frame is gone, so it receives copies it owns, and it receives both or neither.

    // An empty answer is what every failing path sends, and asking for zero bytes may answer null, which would read here as a failure to allocate.
    auto ownedKey = static_cast<char *>(std::malloc(std::max<size_t>(key.size(), 1)));
    auto ownedData = static_cast<char *>(std::malloc(std::max<size_t>(data.size(), 1)));

    if (!ownedKey || !ownedData)
    {
        std::free(ownedKey);
        std::free(ownedData);

        return false;
    }

    std::memcpy(ownedKey, key.data(), key.size());
    std::memcpy(ownedData, data.data(), data.size());

    callback(ownedKey, key.size(), ownedData, data.size());

    return true;
}

void CNativePlatformProxy::setFuncPtrToOnInitializePlatform(FuncPtrToOnInitializePlatform value)
{
    funcPtrToOnInitializePlatform.store(value);
}

void CNativePlatformProxy::setFuncPtrToOnFinalizePlatform(FuncPtrToOnFinalizePlatform value)
{
    funcPtrToOnFinalizePlatform.store(value);
}

void CNativePlatformProxy::setFuncPtrToOnNativeProxyCall(FuncPtrToOnNativeProxyCall value)
{
    funcPtrToOnNativeProxyCall.store(value);
}

void CNativePlatformProxy::setFuncPtrToOnNativeProxyCallback(FuncPtrToOnNativeProxyCallback value)
{
    funcPtrToOnNativeProxyCallback.store(value);
}

void CNativePlatformProxy::setFuncPtrToOnNativeProxyCallFromThread(FuncPtrToOnNativeProxyCallFromThread value)
{
    funcPtrToOnNativeProxyCallFromThread.store(value);
}

void CNativePlatformProxy::setFuncPtrToOnNativeProxyCallbackFromThread(FuncPtrToOnNativeProxyCallbackFromThread value)
{
    funcPtrToOnNativeProxyCallbackFromThread.store(value);
}

} // namespace proxy
} // namespace xplpc
