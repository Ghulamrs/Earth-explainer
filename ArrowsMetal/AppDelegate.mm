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
    _window.title = @"Earth Explainer — CVector / CMatrix / CQuat on Metal";
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

    NSColor *dim = [NSColor colorWithWhite:0.92 alpha:1.0];
    _status = label(_renderer.statusText, 13, NSColor.whiteColor);
    NSTextField *help = label(@"Enter  wireframe ⇄ solid metal\n"
                              @"Q/A  W/S  E/D   rotate ±X ±Y ±Z  (hold several to combine)\n"
                              @"hold X Y Z + ←/→   rotate about the held axis (or their sum)\n"
                              @"←/→  ↑/↓  R/F   move   ±X ±Y ±Z\n"
                              @"Tab  Frame2 turns: X +74°, Y −34°, own Z +35° (one per press)\n"
                              @"L  own / world axes    drag  free rotate    scroll  zoom    Space  reset",
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
    NSArray<NSString *> *names = @[ @"X", @"Y", @"Z" ];
    NSArray<NSColor *> *colors = @[ [NSColor colorWithSRGBRed:0.95 green:0.15 blue:0.12 alpha:1],
                                    [NSColor colorWithSRGBRed:0.20 green:0.85 blue:0.25 alpha:1],
                                    NSColor.blackColor ];
    NSMutableArray<NSView *> *axisLabels = [NSMutableArray array];
    for (NSUInteger i = 0; i < 3; ++i) {
        NSTextField *t = [NSTextField labelWithString:names[i]];
        t.font = [NSFont systemFontOfSize:22 weight:NSFontWeightBold];
        t.textColor = colors[i];
        [t sizeToFit];
        [_view addSubview:t];
        [axisLabels addObject:t];
    }
    [_renderer setAxisLabels:axisLabels];

    __weak NSTextField *weakStatus = _status;
    _renderer.statusChanged = ^(NSString *text) { weakStatus.stringValue = text; };

    [_window makeKeyAndOrderFront:nil];
    [_window makeFirstResponder:_view];
    if (@available(macOS 14.0, *)) [NSApp activate];
    else [NSApp activateIgnoringOtherApps:YES];
}

- (void)windowDidResignKey:(NSNotification *)note { [_renderer clearControls]; }

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app { return YES; }

@end
