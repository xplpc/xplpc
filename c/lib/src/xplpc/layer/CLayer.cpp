#include "xplpc/layer/CLayer.hpp"
#include "xplpc/core/Router.hpp"
#include "xplpc/core/ThreadScope.hpp"
#include "xplpc/layer/OwnedBytes.hpp"
#include "xplpc/util/Log.hpp"
#include "xplpc/util/NativeBoundary.hpp"

#include <thread>

namespace xplpc
{
namespace layer
{

CLayer::NameTable &CLayer::names()
{
    static auto *table = new NameTable();
    return *table;
}

CLayer::CLayer(const xplpc_host &host)
    : host(host)
{
}

CLayer::~CLayer()
{
    forgetName();

    // A layer the router never attached was refused, and the host that got zero from its attach still owns its context.
    if (self.load(std::memory_order_acquire) == 0 || abandoned.load(std::memory_order_acquire))
    {
        return;
    }

    // A raise leaving a destructor ends the process, so a host raising from its release is stopped here.
    // clang-format off
    util::NativeBoundary::run("CLayer : release", [this] {
        host.post(host.context, XPLPC_POST_RELEASE, 0, nullptr, 0);
    });
    // clang-format on
}

void CLayer::bind(core::Handle handle)
{
    self.store(handle, std::memory_order_release);
}

void CLayer::invoke(core::Handle call, const std::string & /*name*/, const std::string &request)
{
    // An invoke is counted like a post, so an abandon waits for one that is already entering the host.
    Posting counted(posting);

    if (abandoned.load())
    {
        core::Router::shared().answer(call, "");
        return;
    }

    // A host that cannot be entered from this thread gets the call through its post callback, which is what lets an isolate receive it on its own thread.

    if (host.enterableAnywhere || core::ThreadScope::inside(self.load(std::memory_order_acquire)))
    {
        host.invoke(host.context, call, request.data(), request.size());
        return;
    }

    post(XPLPC_POST_INVOKE, call, request);
}

void CLayer::post(xplpc_post_kind kind, std::uint64_t id, const std::string &data) const
{
    // The post is counted before the flag is read, so an abandon either stops it here or waits for it to leave the host.
    Posting counted(posting);

    if (abandoned.load())
    {
        if (kind == XPLPC_POST_INVOKE)
        {
            core::Router::shared().answer(id, "");
        }

        return;
    }

    auto owned = OwnedBytes::copy(data);

    if (!owned)
    {
        util::Log::e("[CLayer : post] There was no memory to hand the data over to the host");

        // A call is answered with the empty value and an answer arrives as one, so neither side is left waiting for bytes that could not be copied.
        if (kind == XPLPC_POST_INVOKE)
        {
            core::Router::shared().answer(id, "");
            return;
        }

        host.post(host.context, kind, id, nullptr, 0);
        return;
    }

    host.post(host.context, kind, id, owned, data.size());
}

void CLayer::forgetName() const
{
    // A name only points at the layer attached under it last, so a layer that was already replaced leaves the entry to the one that replaced it.

    const auto handle = self.load(std::memory_order_acquire);

    if (!handle)
    {
        return;
    }

    auto &table = names();
    std::lock_guard<std::mutex> lock(table.mutex);

    for (auto entry = table.handles.begin(); entry != table.handles.end(); ++entry)
    {
        if (entry->second == handle)
        {
            table.handles.erase(entry);
            return;
        }
    }
}

std::shared_ptr<CLayer> CLayer::attached(core::Handle handle)
{
    return std::dynamic_pointer_cast<CLayer>(core::Router::shared().layer(handle));
}

void CLayer::takePlace(const std::string &name, core::Handle handle)
{
    core::Handle previous = 0;

    {
        auto &table = names();
        std::lock_guard<std::mutex> lock(table.mutex);

        auto &attached = table.handles[name];
        previous = attached;
        attached = handle;
    }

    abandon(previous);
}

void CLayer::abandon(core::Handle handle)
{
    auto abandonedLayer = attached(handle);

    if (!abandonedLayer)
    {
        return;
    }

    // The calls it made are dropped rather than answered, since the callbacks that would receive them no longer exist.
    abandonedLayer->abandoned.store(true);

    // A post that read the flag before it was set is still entering the host, which a runtime that is shutting down deletes once this returns.
    while (abandonedLayer->posting.load() > 0)
    {
        std::this_thread::yield();
    }

    core::Router::shared().detach(handle);
}

CLayer::Posting::Posting(std::atomic<int> &count)
    : count(count)
{
    count.fetch_add(1);
}

CLayer::Posting::~Posting()
{
    count.fetch_sub(1);
}

} // namespace layer
} // namespace xplpc
