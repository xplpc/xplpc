#include "xplpc/proxy/NativePlatformProxy.hpp"
#include "xplpc/core/XPLPC.hpp"
#include "xplpc/data/CallbackList.hpp"
#include "xplpc/map/MappingItem.hpp"
#include "xplpc/serializer/Serializer.hpp"
#include "xplpc/util/Log.hpp"

namespace xplpc
{
namespace proxy
{

using namespace xplpc::core;
using namespace xplpc::data;
using namespace xplpc::map;
using namespace xplpc::serializer;

void NativePlatformProxy::initialize()
{
    initializePlatform();
}

void NativePlatformProxy::callProxy(const std::string &key, const std::string &data)
{
    auto functionName = Serializer::decodeFunctionName(data);

    if (functionName.empty())
    {
        CallbackList::shared()->execute(key, "");
        return;
    }

    const auto mappingItem = MappingList::shared()->find(functionName);

    if (!mappingItem)
    {
        util::Log::e("[NativePlatformProxy : callProxy] Mapping not found for function: {}", functionName);
        CallbackList::shared()->execute(key, "");
        return;
    }

    mappingItem.value().getExecutor()(key, data, functionName);
}

bool NativePlatformProxy::hasMapping(const std::string &name)
{
    return MappingList::shared()->has(name);
}

} // namespace proxy
} // namespace xplpc
