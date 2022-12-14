#pragma once

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
    static void createDefault();
    static std::shared_ptr<PlatformProxy> shared();
    static bool hasProxy();
    virtual void initialize();
    virtual void initializePlatform();
    virtual void finalize();
    virtual std::string call(const std::string &data);

private:
    static std::shared_ptr<PlatformProxy> proxy;
};

} // namespace proxy
} // namespace xplpc
