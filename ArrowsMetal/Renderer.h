#import <MetalKit/MetalKit.h>

// Owns the GPU objects and the C++ Scene. Draws only when something changes.
@interface Renderer : NSObject <MTKViewDelegate>

- (nullable instancetype)initWithView:(nonnull MTKView *)view;

- (void)setControl:(int)control down:(BOOL)down;   // Control enum from Scene.h
- (void)clearControls;
- (void)toggleSolid;
- (void)toggleAxesFrame;
- (void)reset;
- (void)zoomBy:(double)factor;
- (void)dragByX:(double)dx y:(double)dyUp;

@property (nonatomic, readonly, nonnull) NSString *statusText;
@property (nonatomic, copy, nullable) void (^statusChanged)(NSString * _Nonnull text);

@end
