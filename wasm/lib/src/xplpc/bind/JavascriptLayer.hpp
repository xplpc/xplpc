#pragma once

#include "xplpc/core/Handle.hpp"
#include "xplpc/core/Layer.hpp"

#include <emscripten/val.h>

#include <string>

namespace xplpc
{
namespace bind
{

// The page as one layer of the router, which holds a counted handle to the javascript object so the page dropping its own cannot free it.
class JavascriptLayer : public core::Layer
{
public:
    explicit JavascriptLayer(emscripten::val host);

    void invoke(core::Handle call, const std::string &name, const std::string &request) override;

private:
    emscripten::val host;
};

} // namespace bind
} // namespace xplpc
