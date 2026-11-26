#pragma once

#include "xplpc/c/typedefs.h"
#include "xplpc/core/Handle.hpp"
#include "xplpc/core/Layer.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace xplpc
{
namespace layer
{

// A host reached through the c abi, which is entered directly only where it said it can be and receives everything else through its post callback.
class CLayer : public core::Layer
{
public:
    explicit CLayer(const xplpc_host &host);

    // Nothing can reach the host once this runs, which is why the release is the last thing its post callback carries.
    ~CLayer() override;

    CLayer(const CLayer &) = delete;
    CLayer &operator=(const CLayer &) = delete;

    void bind(core::Handle handle);
    void invoke(core::Handle call, const std::string &name, const std::string &request) override;
    void post(xplpc_post_kind kind, std::uint64_t id, const std::string &data) const;

    // Answers the layer only when the handle names one a host attached through the abi, since every other kind of handle is a small integer a host can pass by mistake.
    static std::shared_ptr<CLayer> attached(core::Handle handle);

    // Makes the layer the one attached under the name and detaches the one it replaces without ever reaching that host again, since the runtime behind it is gone.
    static void takePlace(const std::string &name, core::Handle handle);

    // Detaches the layer without reaching its host again, and returns only once no post that started earlier is still entering it.
    static void abandon(core::Handle handle);

private:
    // Counts a post for as long as it runs, so an abandon can wait until nothing is entering a host that is about to disappear.
    class Posting
    {
    public:
        explicit Posting(std::atomic<int> &count);
        ~Posting();

        Posting(const Posting &) = delete;
        Posting &operator=(const Posting &) = delete;

    private:
        std::atomic<int> &count;
    };

    xplpc_host host;
    std::atomic<core::Handle> self{0};
    std::atomic<bool> abandoned{false};
    mutable std::atomic<int> posting{0};

    struct NameTable
    {
        std::mutex mutex;
        std::unordered_map<std::string, core::Handle> handles;
    };

    void forgetName() const;

    // A layer can be released on a mapping's own thread while the process exits, so the table it reaches is never destroyed.
    static NameTable &names();
};

} // namespace layer
} // namespace xplpc
