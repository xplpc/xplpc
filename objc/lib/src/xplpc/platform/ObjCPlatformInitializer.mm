#import "xplpc/xplpc-Swift.h"

__attribute__((constructor)) static void ModuleInitializer()
{
    // The constructor attribute is how a framework runs code when it loads, as documented at https://developer.apple.com/library/archive/documentation/MacOSX/Conceptual/BPFrameworks/Tasks/InitializingFrameworks.html.

    PlatformInitializer *platformInitializer = [[PlatformInitializer alloc] init];
    [platformInitializer initialize];
}
