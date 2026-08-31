#include "xplpc/objc/support.h"

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
    return [[NSString alloc] initWithBytes:string.data()
                                    length:static_cast<NSUInteger>(string.size())
                                  encoding:NSUTF8StringEncoding];
}
