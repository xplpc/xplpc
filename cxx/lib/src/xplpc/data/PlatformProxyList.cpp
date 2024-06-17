#include "xplpc/data/PlatformProxyList.hpp"

namespace xplpc
{
namespace data
{

std::shared_ptr<PlatformProxyList> PlatformProxyList::instance = nullptr;
std::once_flag PlatformProxyList::initInstanceFlag;

void PlatformProxyList::append(const std::shared_ptr<PlatformProxy> &item)
{
    list.push_back(item);
}

void PlatformProxyList::appendFromPtr(PlatformProxy *item)
{
    list.push_back(std::shared_ptr<PlatformProxy>(item));
}

void PlatformProxyList::insert(size_t index, const std::shared_ptr<PlatformProxy> &item)
{
    list.insert(list.begin() + index, item);
}

void PlatformProxyList::insertFromPtr(size_t index, PlatformProxy *item)
{
    list.insert(list.begin() + index, std::shared_ptr<PlatformProxy>(item));
}

size_t PlatformProxyList::count() const noexcept
{
    return list.size();
}

std::shared_ptr<PlatformProxyList> PlatformProxyList::shared()
{
    // clang-format off
    std::call_once(initInstanceFlag, []() {
        instance = std::shared_ptr<PlatformProxyList>(new PlatformProxyList());
    });
    // clang-format on

    return instance;
}

#if defined(__EMSCRIPTEN__)
void PlatformProxyList::appendFromJavascript(PlatformProxy *item)
{
    shared()->appendFromPtr(item);
}

void PlatformProxyList::insertFromJavascript(size_t index, PlatformProxy *item)
{
    shared()->insertFromPtr(index, item);
}
#endif

} // namespace data
} // namespace xplpc
