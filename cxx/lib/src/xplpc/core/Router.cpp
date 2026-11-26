#include "xplpc/core/Router.hpp"
#include "xplpc/core/BufferTable.hpp"
#include "xplpc/core/ThreadScope.hpp"
#include "xplpc/layer/NativeLayer.hpp"
#include "xplpc/serializer/Serializer.hpp"
#include "xplpc/util/Log.hpp"

#include <algorithm>
#include <exception>
#include <optional>
#include <utility>

namespace xplpc
{
namespace core
{

Router &Router::shared()
{
    // The router is never destroyed, since destroying it at exit would release layers whose hosts may already be gone, such as an interpreter that finalized first.
    static Router *router = new Router();
    return *router;
}

Router::Router()
{
    // The logger is published first, so a host can redirect this library before anything it does writes a line.
    util::Log::logger();

    nativeLayer = layers.insert(AttachedLayer{std::make_shared<layer::NativeLayer>(), {}});
}

Handle Router::attach(std::shared_ptr<Layer> layer)
{
    if (!layer)
    {
        util::Log::e("[Router : attach] A layer that does not exist cannot be attached");
        return 0;
    }

    // The mappings c++ registers are declared before any host attaches, so a host declaring the same name afterwards is the one that receives it.
    initializePlatform();

    Handle handle = 0;

    {
        std::lock_guard<std::mutex> lock(mutex);
        handle = layers.insert(AttachedLayer{std::move(layer), {}});
    }

    if (!handle)
    {
        util::Log::e("[Router : attach] Every handle a layer can have is taken");
    }

    return handle;
}

void Router::detach(Handle layer)
{
    std::optional<AttachedLayer> detached;
    std::vector<std::shared_ptr<CallState>> orphaned;

    {
        std::lock_guard<std::mutex> lock(mutex);

        detached = layers.take(layer);

        if (!detached)
        {
            return;
        }

        for (const auto &name : detached->names)
        {
            removeOwner(layer, name);
        }

        // clang-format off
        const auto involved = calls.select([layer](const PendingCall &call) {
            return call.caller == layer || call.owner == layer;
        });
        // clang-format on

        for (const auto handle : involved)
        {
            orphaned.push_back(calls.take(handle)->state);
        }
    }

    // The layer is still held here, so the answers that go back to it while it detaches still find it.
    for (const auto &state : orphaned)
    {
        state->resolve("");
    }

    BufferTable::shared().releaseOwnedBy(layer);
}

void Router::declare(Handle layer, const std::string &name)
{
    {
        std::lock_guard<std::mutex> lock(mutex);

        auto attached = layers.find(layer);

        if (attached)
        {
            removeOwner(layer, name);
            owners[name].push_back(layer);

            if (std::find(attached->names.begin(), attached->names.end(), name) == attached->names.end())
            {
                attached->names.push_back(name);
            }

            return;
        }
    }

    util::Log::e("[Router : declare] A layer that is not attached declared function \"{}\"", name);
}

void Router::retract(Handle layer, const std::string &name)
{
    std::lock_guard<std::mutex> lock(mutex);

    auto attached = layers.find(layer);

    if (!attached)
    {
        return;
    }

    removeOwner(layer, name);
    attached->names.erase(std::remove(attached->names.begin(), attached->names.end(), name), attached->names.end());
}

CallResult Router::call(Handle caller, const std::string &request, Completion completion)
{
    // An empty request is what an encoder that failed answers with, and that encoder already said why, so it is answered without being read as a document.
    if (request.empty())
    {
        return CallResult::answered("");
    }

    return call(caller, serializer::Serializer::decodeFunctionName(request), request, std::move(completion));
}

CallResult Router::call(Handle caller, const std::string &name, const std::string &request, Completion completion)
{
    initializePlatform();

    if (name.empty())
    {
        // The decoder is the one that knows whether the document was unreadable or merely carried no name, so it reports and this answers.
        return CallResult::answered("");
    }

    auto state = std::make_shared<CallState>(std::move(completion));
    std::shared_ptr<Layer> target;
    Handle call = 0;
    auto refusal = Refusal::None;

    {
        std::lock_guard<std::mutex> lock(mutex);

        const auto owner = ownerOf(name);

        if (!layers.find(caller))
        {
            refusal = Refusal::CallerNotAttached;
        }
        else if (!owner)
        {
            refusal = Refusal::FunctionNotFound;
        }
        else
        {
            target = layers.find(owner)->layer;
            call = calls.insert(PendingCall{state, caller, owner});
            refusal = call ? Refusal::None : Refusal::NoCallHandle;
        }
    }

    // A sink of the logger is code of the host's own, which may reach back into the router, so a refusal is reported once the lock is free.
    if (refusal != Refusal::None)
    {
        report(refusal, name);
        return CallResult::answered("");
    }

    {
        ThreadScope scope(caller);

        try
        {
            target->invoke(call, name, request);
        }
        catch (const std::exception &e)
        {
            util::Log::e("[Router : call] The layer raised while serving function \"{}\"", name);
            util::Log::d("[Router : call] The layer raised while serving function \"{}\": {}", name, e.what());
            answer(call, "");
        }
        catch (...)
        {
            util::Log::e("[Router : call] The layer raised something that is not an exception while serving function \"{}\"", name);
            answer(call, "");
        }
    }

    if (state->settle())
    {
        return CallResult::pending(call);
    }

    return CallResult::answered(state->takeValue());
}

bool Router::cancel(Handle caller, Handle call)
{
    // The call is let go outside the lock, since destroying its completion can release a reference the host holds and reach back into the router.

    std::optional<PendingCall> cancelled;
    bool foreign = false;

    {
        std::lock_guard<std::mutex> lock(mutex);

        const auto *pending = calls.find(call);

        if (!pending)
        {
            return false;
        }

        foreign = pending->caller != caller;

        if (!foreign)
        {
            cancelled = calls.take(call);
        }
    }

    if (foreign)
    {
        util::Log::e("[Router : cancel] The call was made by another layer");
        return false;
    }

    return cancelled.has_value();
}

void Router::answer(Handle call, const std::string &response)
{
    // A call that was cancelled, already answered or dropped by a detach is no longer here, and answering it is a no-op.

    std::optional<PendingCall> answered;

    {
        std::lock_guard<std::mutex> lock(mutex);
        answered = calls.take(call);
    }

    if (answered)
    {
        answered->state->resolve(response);
    }
}

std::shared_ptr<Layer> Router::layer(Handle layer) const
{
    std::lock_guard<std::mutex> lock(mutex);

    auto attached = layers.find(layer);
    return attached ? attached->layer : nullptr;
}

bool Router::attached(Handle layer) const
{
    std::lock_guard<std::mutex> lock(mutex);
    return layers.find(layer) != nullptr;
}

Handle Router::native() const
{
    return nativeLayer;
}

size_t Router::pending() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return calls.count();
}

void Router::initializePlatform()
{
    // clang-format off
    std::call_once(platformFlag, []() {
        layer::NativeLayer::initializePlatform();
    });
    // clang-format on
}

void Router::report(Refusal refusal, const std::string &name)
{
    switch (refusal)
    {
    case Refusal::CallerNotAttached:
        util::Log::e("[Router : call] A layer that is not attached called function \"{}\"", name);
        break;
    case Refusal::FunctionNotFound:
        util::Log::e("[Router : call] Function not found: {}", name);
        break;
    case Refusal::NoCallHandle:
        util::Log::e("[Router : call] Every handle a call can have is taken, so function \"{}\" is not routed", name);
        break;
    case Refusal::None:
        break;
    }
}

void Router::removeOwner(Handle layer, const std::string &name)
{
    auto found = owners.find(name);

    if (found == owners.end())
    {
        return;
    }

    auto &stack = found->second;
    stack.erase(std::remove(stack.begin(), stack.end(), layer), stack.end());

    if (stack.empty())
    {
        owners.erase(found);
    }
}

Handle Router::ownerOf(const std::string &name) const
{
    const auto found = owners.find(name);
    return found == owners.end() ? 0 : found->second.back();
}

} // namespace core
} // namespace xplpc
