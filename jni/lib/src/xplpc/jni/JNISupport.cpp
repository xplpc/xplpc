#include "xplpc/jni/JNISupport.hpp"

#include <cstdint>

namespace xplpc
{
namespace jni
{

constexpr char32_t replacementCharacter = 0xFFFD;
constexpr char32_t maxCodePoint = 0x10FFFF;
constexpr char32_t supplementaryPlaneStart = 0x10000;
constexpr char16_t highSurrogateStart = 0xD800;
constexpr char16_t lowSurrogateStart = 0xDC00;
constexpr char16_t surrogateEnd = 0xE000;

std::string JNISupport::xplpcJniStringToCppString(JNIEnv *env, jstring value)
{
    if (!value)
    {
        return "";
    }

    const auto length = env->GetStringLength(value);
    const jchar *chars = env->GetStringChars(value, nullptr);

    if (!chars)
    {
        return "";
    }

    auto out = utf16ToUTF8(reinterpret_cast<const char16_t *>(chars), static_cast<size_t>(length));
    env->ReleaseStringChars(value, chars);

    return out;
}

jstring JNISupport::xplpcCppStringToJniString(JNIEnv *env, const std::string &value)
{
    const auto chars = utf8ToUTF16(value);
    return env->NewString(reinterpret_cast<const jchar *>(chars.data()), static_cast<jsize>(chars.size()));
}

void JNISupport::appendUTF8(std::string &out, char32_t codePoint)
{
    if (codePoint < 0x80)
    {
        out.push_back(static_cast<char>(codePoint));
    }
    else if (codePoint < 0x800)
    {
        out.push_back(static_cast<char>(0xC0 | (codePoint >> 6)));
        out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
    else if (codePoint < supplementaryPlaneStart)
    {
        out.push_back(static_cast<char>(0xE0 | (codePoint >> 12)));
        out.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
    else
    {
        out.push_back(static_cast<char>(0xF0 | (codePoint >> 18)));
        out.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
}

void JNISupport::appendUTF16(std::u16string &out, char32_t codePoint)
{
    if (codePoint < supplementaryPlaneStart)
    {
        out.push_back(static_cast<char16_t>(codePoint));
        return;
    }

    const auto value = codePoint - supplementaryPlaneStart;

    out.push_back(static_cast<char16_t>(highSurrogateStart + (value >> 10)));
    out.push_back(static_cast<char16_t>(lowSurrogateStart + (value & 0x3FF)));
}

char32_t JNISupport::nextUTF8CodePoint(const std::string &input, size_t &index)
{
    // One code point is read and the index advances, yielding the replacement character for any malformed sequence.

    const auto lead = static_cast<uint8_t>(input[index++]);

    if (lead < 0x80)
    {
        return lead;
    }

    size_t continuationCount = 0;
    char32_t codePoint = 0;
    char32_t smallestForLength = 0;

    if ((lead & 0xE0) == 0xC0)
    {
        continuationCount = 1;
        codePoint = lead & 0x1F;
        smallestForLength = 0x80;
    }
    else if ((lead & 0xF0) == 0xE0)
    {
        continuationCount = 2;
        codePoint = lead & 0x0F;
        smallestForLength = 0x800;
    }
    else if ((lead & 0xF8) == 0xF0)
    {
        continuationCount = 3;
        codePoint = lead & 0x07;
        smallestForLength = supplementaryPlaneStart;
    }
    else
    {
        return replacementCharacter;
    }

    if (index + continuationCount > input.size())
    {
        index = input.size();
        return replacementCharacter;
    }

    for (size_t i = 0; i < continuationCount; ++i)
    {
        const auto continuation = static_cast<uint8_t>(input[index]);

        if ((continuation & 0xC0) != 0x80)
        {
            return replacementCharacter;
        }

        codePoint = (codePoint << 6) | (continuation & 0x3F);
        ++index;
    }

    // A code point written in more bytes than it needs is a different sequence carrying the same value, and it has to be refused rather than decoded.
    if (codePoint < smallestForLength || codePoint > maxCodePoint || (codePoint >= highSurrogateStart && codePoint < surrogateEnd))
    {
        return replacementCharacter;
    }

    return codePoint;
}

std::string JNISupport::utf16ToUTF8(const char16_t *input, size_t length)
{
    std::string out;
    out.reserve(length);

    for (size_t i = 0; i < length; ++i)
    {
        char32_t codePoint = input[i];

        if (codePoint >= highSurrogateStart && codePoint < lowSurrogateStart)
        {
            const auto hasLowSurrogate = (i + 1 < length) && input[i + 1] >= lowSurrogateStart && input[i + 1] < surrogateEnd;

            codePoint = hasLowSurrogate
                            ? supplementaryPlaneStart + ((codePoint - highSurrogateStart) << 10) + (input[++i] - lowSurrogateStart)
                            : replacementCharacter;
        }
        else if (codePoint >= lowSurrogateStart && codePoint < surrogateEnd)
        {
            codePoint = replacementCharacter;
        }

        appendUTF8(out, codePoint);
    }

    return out;
}

std::u16string JNISupport::utf8ToUTF16(const std::string &input)
{
    std::u16string out;
    out.reserve(input.size());

    size_t index = 0;

    while (index < input.size())
    {
        appendUTF16(out, nextUTF8CodePoint(input, index));
    }

    return out;
}

} // namespace jni
} // namespace xplpc
