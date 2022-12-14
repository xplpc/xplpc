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
Response callbackForLogin(const Message &data)
{
    auto username = data.get<std::string>("username");
    auto password = data.get<std::string>("password");
    auto remember = data.get<bool>("remember");

    if (username && password && remember)
    {
        if (username.value() == "paulo" && password.value() == "123456")
        {
            if (remember.value())
            {
                return Response::str("LOGGED-WITH-REMEMBER");
            }
            else
            {
                return Response::str("LOGGED-WITHOUT-REMEMBER");
            }
        }
    }

    return Response::str("NOT-LOGGED");
}

// custom platform initializer
namespace xplpc
{
namespace proxy
{

void PlatformProxy::initializePlatform()
{
    // add mapping (return value, params types, function name, params names, function ref)
    MappingData::add(Map::create<std::string, std::string, std::string, bool>("sample.login", {"username", "password", "remember"}, &callbackForLogin));
}

} // namespace proxy
} // namespace xplpc

// sample
int main()
{
    // initialize
    PlatformProxy::createDefault();
    PlatformProxy::shared()->initialize();

    // call local method
    {
        auto request = Request{
            "sample.login",
            Param<std::string>{"username", "paulo"},
            Param<std::string>{"password", "123456"},
            Param<bool>{"remember", true},
        };

        auto ret = LocalClient::call<std::string>(request);
        std::cout << "Returned Value: " << (ret ? ret.value() : "empty") << std::endl;
    }

    // call remote method
    {
        auto request = Request{
            "platform.battery.level",
            Param<std::string>{"suffix", "%"},
        };

        auto ret = RemoteClient::call<std::string>(request);
        std::cout << "Returned Value: " << (ret ? ret.value() : "empty") << std::endl;
    }

    return EXIT_SUCCESS;
}
