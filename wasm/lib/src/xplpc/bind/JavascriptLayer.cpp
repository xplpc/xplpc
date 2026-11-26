#include "JavascriptLayer.hpp"

#include <utility>

namespace xplpc
{
namespace bind
{

JavascriptLayer::JavascriptLayer(emscripten::val host)
    : host(std::move(host))
{
}

void JavascriptLayer::invoke(core::Handle call, const std::string & /*name*/, const std::string &request)
{
    host.call<void>("invoke", call, request);
}

} // namespace bind
} // namespace xplpc
