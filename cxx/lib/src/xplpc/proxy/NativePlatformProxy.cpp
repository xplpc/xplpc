#include "xplpc/proxy/NativePlatformProxy.hpp"
#include "xplpc/core/XPLPC.hpp"
#include "xplpc/data/CallbackList.hpp"
#include "xplpc/map/MappingItem.hpp"
#include "xplpc/serializer/Serializer.hpp"

#include "spdlog/spdlog.h"

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
    XPLPC::initialize();
    initializePlatform();
}

void NativePlatformProxy::finalize()
{
    finalizePlatform();
}

void NativePlatformProxy::finalizePlatform()
{
    MappingList::shared()->clear();
}

void NativePlatformProxy::callProxy(const std::string &key, const std::string &data)
{
    auto functionName = Serializer::decodeFunctionName(data);

    if (functionName.empty())
    {
        spdlog::error("[NativePlatformProxy : callProxy] Function name is empty");
        CallbackList::shared()->execute(key, "");
        return;
    }

    auto mappingItem = MappingList::shared()->find(functionName);

    if (mappingItem)
    {
        mappingItem.value().getExecutor()(key, data);
    }
    else
    {
        spdlog::error("[NativePlatformProxy : call] Mapping not found for function: {}", functionName);
        CallbackList::shared()->execute(key, "");
    }
}

bool NativePlatformProxy::hasMapping(const std::string &name)
{
    auto mappingItem = MappingList::shared()->find(name);

    if (mappingItem)
    {
        return true;
    }

    return false;
}

/*
- TIPS:
- ALL PLATFORMS NEED IMPLEMENT THE "CUSTOM INITIALIZE METHOD".
- IT CAN BE EMPTY, CAN BE REMOVED OR CAN BE IMPLEMENTED IN THIS CLASS.
- ALL MAPPINGS IN C++ SIDE CAN BE INSIDE THE METHOD "NativePlatformProxy::initializePlatform".

> EXAMPLE:

#include "xplpc/custom/Mapping.hpp"

namespace xplpc
{
namespace proxy
{

void NativePlatformProxy::initializePlatform()
{
    xplpc::custom::Mapping::initialize();
}

} // namespace proxy
} // namespace xplpc

> REFERENCE CODE HERE:
cxx/custom/src/xplpc/custom/NativePlatformProxy.cpp
*/

} // namespace proxy
} // namespace xplpc
