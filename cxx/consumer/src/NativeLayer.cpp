#include "consumer/Greeting.hpp"
#include "xplpc/xplpc.hpp"

namespace xplpc
{
namespace layer
{

void NativeLayer::initializePlatform()
{
    data::MappingList::shared()->add("consumer.greeting", map::Map::create<std::string, std::string>({"name"}, &consumer::Greeting::target));
}

} // namespace layer
} // namespace xplpc
