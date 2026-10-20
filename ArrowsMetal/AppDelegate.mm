#import "AppDelegate.h"
#import "ArrowView.h"
#import "Renderer.h"

@implementation AppDelegate {
    NSWindow    *_window;
    ArrowView   *_view;
    Renderer    *_renderer;
    NSTextField *_status;
}

static NSTextField *label(NSString *text, CGFloat size, NSColor *color) {
    NSTextField *t = [NSTextField labelWithString:text];
    t.font = [NSFont monospacedSystemFontOfSize:size weight:NSFontWeightRegular];
    t.textColor = color;
    t.translatesAutoresizingMaskIntoConstraints = NO;
    return t;
}

- (void)buildMenu {
    NSMenu *bar = [NSMenu new];
    NSMenuItem *appItem = [NSMenuItem new];
    [bar addItem:appItem];
    NSMenu *appMenu = [NSMenu new];
    [appMenu addItemWithTitle:@"Quit Arrows" action:@selector(terminate:) keyEquivalent:@"q"];
    appItem.submenu = appMenu;
    NSApp.mainMenu = bar;
}

- (void)applicationDidFinishLaunching:(NSNotification *)note {
    [self buildMenu];

    _window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 1100, 760)
                                          styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                                                    NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable
                                            backing:NSBackingStoreBuffered defer:NO];
    _window.title = @"Arrows — CVector / CMatrix / CQuat on Metal";
    [_window center];
    _window.delegate = self;

    _view = [[ArrowView alloc] initWithFrame:_window.contentView.bounds device:MTLCreateSystemDefaultDevice()];
    _renderer = [[Renderer alloc] initWithView:_view];
    if (!_renderer) {
        NSAlert *a = [NSAlert new];
        a.messageText = @"Metal could not be initialised.";
        [a runModal];
        [NSApp terminate:nil];
        return;
    }
    _view.delegate = _renderer;
    _view.renderer = _renderer;
    _window.contentView = _view;

    // HUD: updated only when the mode changes, never per frame.
    NSColor *dim = [NSColor colorWithWhite:0.75 alpha:1.0];
    _status = label(_renderer.statusText, 13, NSColor.whiteColor);
    NSTextField *help = label(@"Enter  wireframe ⇄ solid metal\n"
                              @"Q/A  W/S  E/D   rotate ±X ±Y ±Z  (hold several to combine)\n"
                              @"←/→  ↑/↓  R/F   move   ±X ±Y ±Z\n"
                              @"L  world / local axes    drag  free rotate    scroll  zoom    Space  reset",
                              11, dim);
    help.maximumNumberOfLines = 0;
    [_view addSubview:_status];
    [_view addSubview:help];
    [NSLayoutConstraint activateConstraints:@[
        [_status.leadingAnchor constraintEqualToAnchor:_view.leadingAnchor constant:14],
        [_status.topAnchor     constraintEqualToAnchor:_view.topAnchor     constant:12],
        [help.leadingAnchor    constraintEqualToAnchor:_view.leadingAnchor constant:14],
        [help.bottomAnchor     constraintEqualToAnchor:_view.bottomAnchor  constant:-12],
    ]];
    __weak NSTextField *weakStatus = _status;
    _renderer.statusChanged = ^(NSString *text) { weakStatus.stringValue = text; };

    [_window makeKeyAndOrderFront:nil];
    [_window makeFirstResponder:_view];
    if (@available(macOS 14.0, *)) [NSApp activate];
    else [NSApp activateIgnoringOtherApps:YES];
}

// Cmd-Tab away with a key held would otherwise leave it "down" forever.
- (void)windowDidResignKey:(NSNotification *)note { [_renderer clearControls]; }

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app { return YES; }

@end
