#include "xplpc/layer/NativeLayer.hpp"
#include "xplpc/core/Router.hpp"
#include "xplpc/data/MappingList.hpp"
#include "xplpc/util/Log.hpp"

namespace xplpc
{
namespace layer
{

void NativeLayer::invoke(core::Handle call, const std::string &name, const std::string &request)
{
    const auto mappingItem = data::MappingList::shared()->find(name);

    if (!mappingItem)
    {
        util::Log::e("[NativeLayer : invoke] Mapping not found for function: {}", name);
        core::Router::shared().answer(call, "");
        return;
    }

    mappingItem.value().getExecutor()(call, request, name);
}

} // namespace layer
} // namespace xplpc
