#include "xplpc/c/platform.h"
#include "xplpc/xplpc.hpp"

#include <iostream>
#include <string>

using namespace xplpc::core;
using namespace xplpc::map;
using namespace xplpc::data;
using namespace xplpc::client;
using namespace xplpc::message;

void callbackLogin(const Message &m, const Response &r)
{
    auto username = m.get<std::string>("username");
    auto password = m.get<std::string>("password");
    auto remember = m.get<bool>("remember");

    if (username && password && remember)
    {
        if (username.value() == "paulo" && password.value() == "123456")
        {
            if (remember.value())
            {
                r(std::string("LOGGED-WITH-REMEMBER"));
            }
            else
            {
                r(std::string("LOGGED-WITHOUT-REMEMBER"));
            }

            return;
        }
    }

    r(std::string("NOT-LOGGED"));
}

namespace xplpc
{
namespace proxy
{

void NativePlatformProxy::initializePlatform()
{
    MappingList::shared()->add("sample.login", Map::create<std::string, std::string, std::string, bool>({"username", "password", "remember"}, &callbackLogin));
}

} // namespace proxy
} // namespace xplpc

void onNativeProxyCallback(const char *key, size_t keySize, const char *data, size_t dataSize)
{
    // The answer arrives here, which is the only place a host sees what a call produced.

    std::cout << "Returned Value: " << std::string(data, dataSize) << " for key " << std::string(key, keySize) << std::endl;
}

int main()
{
    xplpc_core_initialize(true, nullptr, nullptr, nullptr, &onNativeProxyCallback, nullptr, nullptr);

    const std::string key = "1";
    const std::string data = R"({"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]})";

    xplpc_native_call_proxy(key.c_str(), key.size(), data.c_str(), data.size());

    return EXIT_SUCCESS;
}
