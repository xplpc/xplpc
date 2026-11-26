#include "xplpc/c/platform.h"
#include "xplpc/xplpc.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

using namespace xplpc::map;
using namespace xplpc::data;
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
namespace layer
{

void NativeLayer::initializePlatform()
{
    MappingList::shared()->add("sample.login", Map::create<std::string, std::string, std::string, bool>({"username", "password", "remember"}, &callbackLogin));
}

} // namespace layer
} // namespace xplpc

void onInvoke(void * /*context*/, xplpc_handle call, const char * /*request*/, size_t /*requestSize*/)
{
    // This host declares no name of its own, so nothing is ever routed here and the call is answered with the empty value.

    xplpc_answer(call, "", 0);
}

void onPost(void * /*context*/, xplpc_post_kind kind, uint64_t token, char *data, size_t dataSize)
{
    // Whatever post carries belongs to the host once it arrives.

    if (kind == XPLPC_POST_ANSWER)
    {
        std::cout << "Returned Value: " << std::string(data, dataSize) << " for token " << token << std::endl;
    }

    // A call that arrives on a thread this host cannot be entered on comes here instead of through invoke, and it is answered the same way.
    if (kind == XPLPC_POST_INVOKE)
    {
        xplpc_answer(token, "", 0);
    }

    xplpc_free(data);
}

int main()
{
    const xplpc_host host{nullptr, &onInvoke, &onPost, false, nullptr, 0};
    const auto layer = xplpc_attach(&host);

    if (layer == 0)
    {
        return EXIT_FAILURE;
    }

    const std::string request = R"({"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]})";
    xplpc_call_result result{};

    if (xplpc_call(layer, 1, request.data(), request.size(), &result))
    {
        std::cout << "Returned Value: " << std::string(result.data, result.dataSize) << " for token 1" << std::endl;
        xplpc_free(result.data);
    }

    xplpc_detach(layer);

    return EXIT_SUCCESS;
}
