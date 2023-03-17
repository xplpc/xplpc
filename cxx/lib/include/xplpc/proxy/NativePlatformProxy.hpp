#pragma once

#include "xplpc/proxy/PlatformProxy.hpp"

#include <string>

namespace xplpc
{
namespace proxy
{

class NativePlatformProxy : public PlatformProxy
{
public:
    virtual void initialize() override;
    virtual void initializePlatform() override;
    virtual void finalize() override;
    virtual void finalizePlatform() override;
    virtual void callProxy(const std::string &key, const std::string &data) override;
    virtual bool hasMapping(const std::string &name) override;
};

} // namespace proxy
} // namespace xplpc
