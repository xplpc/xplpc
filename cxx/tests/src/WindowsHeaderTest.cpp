// The windows header defines min and max as macros unless told otherwise, and a consumer on windows includes it before this library more often than after.
#if defined(_WIN32)
#include <windows.h>
#endif

#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

TEST(WindowsHeaderTest, ThePublicHeaderCompilesAfterTheWindowsHeader)
{
    EXPECT_EQ(std::optional<int>{42}, xplpc::serializer::Serializer::decodeFunctionReturnValue<int>(R"({"r":42})"));
}
