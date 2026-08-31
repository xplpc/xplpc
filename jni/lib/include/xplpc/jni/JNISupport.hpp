#pragma once

#include <cstddef>
#include <jni.h>
#include <string>

namespace xplpc
{
namespace jni
{

class JNISupport
{
public:
    static std::string xplpcJniStringToCppString(JNIEnv *env, jstring value);
    static jstring xplpcCppStringToJniString(JNIEnv *env, const std::string &value);

private:
    static void appendUTF8(std::string &out, char32_t codePoint);
    static void appendUTF16(std::u16string &out, char32_t codePoint);
    static char32_t nextUTF8CodePoint(const std::string &input, size_t &index);
    static std::string utf16ToUTF8(const char16_t *input, size_t length);
    static std::u16string utf8ToUTF16(const std::string &input);
};

} // namespace jni
} // namespace xplpc
