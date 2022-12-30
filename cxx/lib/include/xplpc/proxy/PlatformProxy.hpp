#pragma once

#include <future>
#include <memory>
#include <string>

namespace xplpc
{
namespace proxy
{

class PlatformProxy
{
public:
    static void create(std::shared_ptr<PlatformProxy> proxy);
    static void createFromPtr(PlatformProxy *proxy);
    static void createDefault();
    static std::shared_ptr<PlatformProxy> shared();
    static bool hasProxy();
    virtual void initialize();
    virtual void initializePlatform();
    virtual void finalize();
    virtual std::string callProxy(const std::string &data);

    // TODO: XPLPC - WHAT I NEED USE FOR WASM CALLBACK?
    virtual void callProxyAsync(const std::string &data, std::function<void(const std::string &)> callback);

private:
    static std::shared_ptr<PlatformProxy> proxy;
};

} // namespace proxy
} // namespace xplpc
