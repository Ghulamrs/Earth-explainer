#import "ArrowView.h"
#import "Renderer.h"
#include "Core/Scene.h"

@implementation ArrowView

- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)acceptsFirstMouse:(NSEvent *)event { return YES; }

static int controlFor(NSEvent *e) {
    NSString *s = e.charactersIgnoringModifiers.lowercaseString;
    if (s.length == 0) return -1;
    switch ([s characterAtIndex:0]) {
        case 'q': return RotXPos;   case 'a': return RotXNeg;
        case 'w': return RotYPos;   case 's': return RotYNeg;
        case 'e': return RotZPos;   case 'd': return RotZNeg;
        case NSRightArrowFunctionKey: return MoveXPos;
        case NSLeftArrowFunctionKey:  return MoveXNeg;
        case NSUpArrowFunctionKey:    return MoveYPos;
        case NSDownArrowFunctionKey:  return MoveYNeg;
        case 'r': return MoveZPos;  case 'f': return MoveZNeg;
        default:  return -1;
    }
}

- (void)keyDown:(NSEvent *)e {
    if (e.modifierFlags & NSEventModifierFlagCommand) { [super keyDown:e]; return; }
    if (e.keyCode == 36 || e.keyCode == 76) {
        if (!e.isARepeat) [self.renderer toggleSolid];
        return;
    }
    NSString *s = e.charactersIgnoringModifiers.lowercaseString;
    unichar c = s.length ? [s characterAtIndex:0] : 0;
    if (c == ' ') { [self.renderer reset];           return; }
    if (c == 'l') { if (!e.isARepeat) [self.renderer toggleAxesFrame]; return; }

    int control = controlFor(e);
    if (control >= 0) { if (!e.isARepeat) [self.renderer setControl:control down:YES]; return; }
    [super keyDown:e];
}

- (void)keyUp:(NSEvent *)e {
    int control = controlFor(e);
    if (control >= 0) [self.renderer setControl:control down:NO];
    else [super keyUp:e];
}

- (BOOL)resignFirstResponder {
    [self.renderer clearControls];
    return [super resignFirstResponder];
}

- (void)mouseDragged:(NSEvent *)e {
    [self.renderer dragByX:e.deltaX y:-e.deltaY];
}

- (void)scrollWheel:(NSEvent *)e {
    double dy = e.hasPreciseScrollingDeltas ? e.scrollingDeltaY * 0.01 : e.scrollingDeltaY * 0.1;
    [self.renderer zoomBy:exp(-dy)];
}

- (void)magnifyWithEvent:(NSEvent *)e {
    [self.renderer zoomBy:1.0 / (1.0 + e.magnification)];
}

@end
