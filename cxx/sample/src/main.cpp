// add imports
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
    // initialize
    auto proxy = std::make_shared<NativePlatformProxy>();
    proxy->initialize();

    PlatformProxyList::shared()->append(proxy);

    // call function
    {
        auto request = Request{
            "sample.login",
            Param<std::string>{"username", "paulo"},
            Param<std::string>{"password", "123456"},
            Param<bool>{"remember", true},
        };

        // clang-format off
        Client::call<std::string>(request, [](const auto &response) {
            std::cout << "Returned Value: " << (response ? response.value() : "Empty") << std::endl;
        });
        // clang-format on
    }

    return EXIT_SUCCESS;
}
