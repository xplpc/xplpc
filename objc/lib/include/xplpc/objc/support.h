#pragma once

#import <Foundation/Foundation.h>

#include <string>

class ObjCSupport
{
public:
    static std::string xplpcObjcStringToCppString(NSString *string);
    static NSString *xplpcCppStringToObjcString(const std::string &string);
};
