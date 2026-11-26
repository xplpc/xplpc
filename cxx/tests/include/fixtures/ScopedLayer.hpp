#pragma once

#include "fixtures/FakeLayer.hpp"
#include "xplpc/core/Handle.hpp"
#include "xplpc/core/Router.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <utility>

// Attaches a fake layer for the length of a test, so an assertion that fails halfway cannot leave it routed to for the tests after it.
// Only the router holds the layer, which is what lets a test observe when it is destroyed.
class ScopedLayer
{
public:
    explicit ScopedLayer(FakeLayer::Behaviour behaviour, std::shared_ptr<std::atomic<bool>> destroyed = nullptr)
    {
        auto layer = std::make_shared<FakeLayer>(std::move(behaviour), std::move(destroyed));
        auto &raw = *layer;

        handle = xplpc::core::Router::shared().attach(std::move(layer));
        raw.handle.store(handle);
    }

    ~ScopedLayer()
    {
        detach();
    }

    ScopedLayer(const ScopedLayer &) = delete;
    ScopedLayer &operator=(const ScopedLayer &) = delete;

    xplpc::core::Handle get() const
    {
        return handle;
    }

    void declare(const std::string &name) const
    {
        xplpc::core::Router::shared().declare(handle, name);
    }

    void retract(const std::string &name) const
    {
        xplpc::core::Router::shared().retract(handle, name);
    }

    void detach() const
    {
        xplpc::core::Router::shared().detach(handle);
    }

    // Answers with the given value inline, which is what a host whose mapping returns at once does.
    static FakeLayer::Behaviour answering(const std::string &value)
    {
        // clang-format off
        return [value](FakeLayer &, xplpc::core::Handle call, const std::string &, const std::string &) {
            xplpc::core::Router::shared().answer(call, R"({"r":")" + value + R"("})");
        };
        // clang-format on
    }

private:
    xplpc::core::Handle handle = 0;
};
