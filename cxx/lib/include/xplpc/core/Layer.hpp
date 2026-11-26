#pragma once

#include "xplpc/core/Handle.hpp"

#include <string>

namespace xplpc
{
namespace core
{

// One language attached to the router, which serves the names it declared.
class Layer
{
public:
    virtual ~Layer() = default;

    // Serves a call to a name this layer declared and answers it through the router with the handle it received, at once or later and from any thread.
    virtual void invoke(Handle call, const std::string &name, const std::string &request) = 0;
};

} // namespace core
} // namespace xplpc
