#pragma once

#include <string>

namespace xplpc
{
namespace proxy
{

class PlatformProxy
{
public:
    virtual ~PlatformProxy() = default;
    virtual void initialize() = 0;
    virtual void initializePlatform() = 0;
    virtual void finalize() = 0;
    virtual void finalizePlatform() = 0;
    virtual void callProxy(const std::string &key, const std::string &data) = 0;
    virtual bool hasMapping(const std::string &name) = 0;
};

} // namespace proxy
} // namespace xplpc
