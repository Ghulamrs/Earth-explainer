#import <Cocoa/Cocoa.h>
#import "AppDelegate.h"

static AppDelegate *gDelegate;   // NSApplication holds its delegate weakly

int main(int argc, const char *argv[]) {
    (void)argc; (void)argv;
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        gDelegate = [AppDelegate new];
        app.delegate = gDelegate;
        [app run];
    }
    return 0;
}
