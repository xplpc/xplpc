#include "xplpc/objc/support.h"
#include "xplpc/util/Log.hpp"

#include <string>

std::string ObjCSupport::xplpcObjcStringToCppString(NSString *string)
{
    if (!string)
    {
        return "";
    }

    return {[string UTF8String], [string lengthOfBytesUsingEncoding:NSUTF8StringEncoding]};
}

NSString *ObjCSupport::xplpcCppStringToObjcString(const std::string &string)
{
    NSString *text = [[NSString alloc] initWithBytes:string.data()
                                              length:static_cast<NSUInteger>(string.size())
                                            encoding:NSUTF8StringEncoding];

    // A host reached through the c abi can answer with any bytes, and text swift cannot hold is reported rather than read as nothing.
    if (!text)
    {
        xplpc::util::Log::e("[ObjCSupport : xplpcCppStringToObjcString] Unable to decode data");
        return @"";
    }

    return text;
}
