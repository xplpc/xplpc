#pragma once

#include <cassert>
#include <codecvt>
#include <cstring>
#include <locale>

#include <jni.h>

namespace xplpc
{
namespace jni
{

std::string jniUTF8FromString(JNIEnv *env, const jstring jstr);
jstring jniStringFromUTF8(JNIEnv *env, const std::string &str);

} // namespace jni
} // namespace xplpc
