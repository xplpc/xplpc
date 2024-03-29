#pragma once

#include "xplpc/proxy/PlatformProxy.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace xplpc
{
namespace data
{

using namespace xplpc::proxy;

class PlatformProxyList
{
public:
    PlatformProxyList() = default;
    void append(const std::shared_ptr<PlatformProxy> &item);
    void appendFromPtr(PlatformProxy *item);
    void insert(size_t index, const std::shared_ptr<PlatformProxy> &item);
    void insertFromPtr(size_t index, PlatformProxy *item);
    size_t count() const noexcept;
    static std::shared_ptr<PlatformProxyList> shared();
    std::vector<std::shared_ptr<PlatformProxy>> list;

#if defined(__EMSCRIPTEN__)
    static void appendFromJavascript(PlatformProxy *item);
    static void insertFromJavascript(size_t index, PlatformProxy *item);
#endif

private:
    static std::shared_ptr<PlatformProxyList> instance;
};

} // namespace data
} // namespace xplpc
