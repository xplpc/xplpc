#include "xplpc/xplpc.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

using namespace xplpc::client;
using namespace xplpc::message;

int main()
{
    const auto answer = Client::callSync<std::string>(Request{"consumer.greeting", Param<std::string>{"name", "consumer"}});

    std::cout << "Returned Value: " << answer.value_or("Empty") << std::endl;

    return answer == "Hello, consumer" ? EXIT_SUCCESS : EXIT_FAILURE;
}
