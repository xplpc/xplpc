// add imports
#include "xplpc/c/platform.h"
#include "xplpc/xplpc.hpp"
#include <iostream>
#include <string>

// add namespaces
using namespace xplpc::core;
using namespace xplpc::map;
using namespace xplpc::data;
using namespace xplpc::client;
using namespace xplpc::message;

// callback method
void callbackLogin(const Message &m, const Response r)
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

// custom platform initializer
namespace xplpc
{
namespace proxy
{

void NativePlatformProxy::initializePlatform()
{
    // mapping data (function name, map<return value, params types>(params names), function ref)
    MappingList::shared()->add("sample.login", Map::create<std::string, std::string, std::string, bool>({"username", "password", "remember"}, &callbackLogin));
}

} // namespace proxy
} // namespace xplpc

// sample
int main()
{
    xplpc_core_initialize(true, nullptr, nullptr, nullptr, nullptr, nullptr);

    std::string key = "1";
    std::string data = R"({"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]})";

    xplpc_native_call_proxy(const_cast<char *>(key.c_str()), key.size(), const_cast<char *>(data.c_str()), data.size());

    return EXIT_SUCCESS;
}
