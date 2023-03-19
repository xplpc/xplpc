#import "xplpc/xplpc-Swift.h"

// https://developer.apple.com/library/archive/documentation/MacOSX/Conceptual/BPFrameworks/Tasks/InitializingFrameworks.html
__attribute__((constructor)) static void ModuleInitializer()
{
    NSLog(@"XPLPC MODULE INITIALIZED");
    PlatformInitializer *platformInitializer = [[PlatformInitializer alloc] init];
    [platformInitializer initialize];
}
