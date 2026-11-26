#pragma once

#include "xplpc/core/CallResult.hpp"
#include "xplpc/core/CallState.hpp"
#include "xplpc/core/Handle.hpp"
#include "xplpc/core/Layer.hpp"
#include "xplpc/core/SlotTable.hpp"

#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace xplpc
{
namespace core
{

// Routes every call in the process to the layer that registered its name last, and is never reset or destroyed.
class Router
{
public:
    static Router &shared();

    Handle attach(std::shared_ptr<Layer> layer);

    // Stops routing to the layer and from it, answers every call it was serving or waiting on with the empty value, and lets it go once nothing is inside it.
    void detach(Handle layer);

    // Makes the layer the owner of the name until it retracts it or another layer declares it after.
    void declare(Handle layer, const std::string &name);

    // Gives the name back to the layer that declared it before this one, if any.
    void retract(Handle layer, const std::string &name);

    CallResult call(Handle caller, const std::string &request, Completion completion);
    CallResult call(Handle caller, const std::string &name, const std::string &request, Completion completion);

    // Answers true when the call the caller made was still waiting, which guarantees its completion never runs, and refuses a call another layer made.
    bool cancel(Handle caller, Handle call);

    void answer(Handle call, const std::string &response);

    // Answers the attached layer, which is what a bridge needs to hand an answer back to the host that made the call.
    std::shared_ptr<Layer> layer(Handle layer) const;

    bool attached(Handle layer) const;

    // The layer serving the mappings c++ registered, which is attached before any other.
    Handle native() const;

    size_t pending() const;

private:
    struct AttachedLayer
    {
        std::shared_ptr<Layer> layer;
        std::vector<std::string> names;
    };

    struct PendingCall
    {
        std::shared_ptr<CallState> state;
        Handle caller;
        Handle owner;
    };

    enum class Refusal
    {
        None,
        CallerNotAttached,
        FunctionNotFound,
        NoCallHandle,
    };

    mutable std::mutex mutex;
    SlotTable<AttachedLayer> layers;
    SlotTable<PendingCall> calls;
    std::unordered_map<std::string, std::vector<Handle>> owners;
    Handle nativeLayer;
    std::once_flag platformFlag;

    Router();
    Router(const Router &) = delete;
    Router &operator=(const Router &) = delete;

    void initializePlatform();
    static void report(Refusal refusal, const std::string &name);
    void removeOwner(Handle layer, const std::string &name);
    Handle ownerOf(const std::string &name) const;
};

} // namespace core
} // namespace xplpc
