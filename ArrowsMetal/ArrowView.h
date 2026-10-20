#import <MetalKit/MetalKit.h>
@class Renderer;

// MTKView that turns keyboard / mouse / trackpad events into Scene input.
@interface ArrowView : MTKView
@property (nonatomic, weak) Renderer *renderer;
@end
