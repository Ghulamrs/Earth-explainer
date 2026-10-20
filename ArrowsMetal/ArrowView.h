#import <MetalKit/MetalKit.h>
@class Renderer;

@interface ArrowView : MTKView
@property (nonatomic, weak) Renderer *renderer;
@end
