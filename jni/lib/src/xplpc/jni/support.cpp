#include "xplpc/jni/support.hpp"
#include <codecvt>
#include <locale>

namespace xplpc
{
namespace jni
{

// clang-format: off
using Utf8Converter = std::wstring_convert<std::codecvt_utf8_utf16<char16_t, 0x10ffff, std::codecvt_mode::little_endian>, char16_t>;
// clang-format: on

std::string jniUTF8FromString(JNIEnv *env, const jstring jstr)
{
    auto length = env->GetStringLength(jstr);
    const jchar *u16 = env->GetStringChars(jstr, nullptr);
    auto *p = reinterpret_cast<const char16_t *>(u16);
    std::string out = Utf8Converter{}.to_bytes(p, p + length);
    env->ReleaseStringChars(jstr, u16);
    return out;
}

jstring jniStringFromUTF8(JNIEnv *env, const std::string &str)
{
    std::u16string u16 = Utf8Converter{}.from_bytes(str);
    jstring res = env->NewString(reinterpret_cast<const jchar *>(u16.data()), u16.size());
    return res;
}

} // namespace jni
} // namespace xplpc
