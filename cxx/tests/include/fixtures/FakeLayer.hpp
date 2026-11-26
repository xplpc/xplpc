#pragma once

#include "xplpc/core/Handle.hpp"
#include "xplpc/core/Layer.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <utility>

// A layer whose behaviour each test chooses, standing for a host this library calls and did not write.
class FakeLayer : public xplpc::core::Layer
{
public:
    using Behaviour = std::function<void(FakeLayer &layer, xplpc::core::Handle call, const std::string &name, const std::string &request)>;

    explicit FakeLayer(Behaviour behaviour, std::shared_ptr<std::atomic<bool>> destroyed = nullptr)
        : behaviour(std::move(behaviour))
        , destroyed(std::move(destroyed))
    {
    }

    ~FakeLayer() override
    {
        if (destroyed)
        {
            destroyed->store(true);
        }
    }

    void invoke(xplpc::core::Handle call, const std::string &name, const std::string &request) override
    {
        behaviour(*this, call, name, request);
    }

    // The handle the router gave this layer, which a behaviour needs to detach it from inside its own invoke.
    std::atomic<xplpc::core::Handle> handle{0};

private:
    Behaviour behaviour;
    std::shared_ptr<std::atomic<bool>> destroyed;
};
