#include "xplpc/data/PlatformProxyList.hpp"

#include <algorithm>

namespace xplpc
{
namespace data
{

std::shared_ptr<PlatformProxyList> PlatformProxyList::instance = nullptr;
std::once_flag PlatformProxyList::initInstanceFlag;

void PlatformProxyList::append(const std::shared_ptr<PlatformProxy> &item)
{
    if (!item)
    {
        return;
    }

    std::unique_lock<std::shared_mutex> lock(mutex);
    list.push_back(item);
}

void PlatformProxyList::prepend(const std::shared_ptr<PlatformProxy> &item)
{
    // A host proxy is registered after the native one and has to be asked first, since the first that owns the name receives the call.

    if (!item)
    {
        return;
    }

    std::unique_lock<std::shared_mutex> lock(mutex);
    list.insert(list.begin(), item);
}

size_t PlatformProxyList::count() const
{
    std::shared_lock<std::shared_mutex> lock(mutex);
    return list.size();
}

void PlatformProxyList::clear()
{
    std::unique_lock<std::shared_mutex> lock(mutex);
    list.clear();
}

bool PlatformProxyList::forEach(const std::function<bool(const std::shared_ptr<PlatformProxy> &)> &func) const
{
    // The iteration runs over a snapshot, so a proxy is free to call back into this list without deadlocking.
    std::vector<std::shared_ptr<PlatformProxy>> snapshot;

    {
        std::shared_lock<std::shared_mutex> lock(mutex);
        snapshot = list;
    }

    for (const auto &proxy : snapshot)
    {
        if (func(proxy))
        {
            return true;
        }
    }

    return false;
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
void PlatformProxyList::prependFromJavascript(PlatformProxy *item)
{
    // The binding layer already owns the object, so a second owner here would delete it twice.

    // clang-format off
    shared()->prepend(std::shared_ptr<PlatformProxy>(item, [](PlatformProxy *) {}));
    // clang-format on
}
#endif

} // namespace data
} // namespace xplpc
