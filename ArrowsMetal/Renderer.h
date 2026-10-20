#import <MetalKit/MetalKit.h>

@interface Renderer : NSObject <MTKViewDelegate>

- (nullable instancetype)initWithView:(nonnull MTKView *)view;

- (void)setControl:(int)control down:(BOOL)down;
- (void)clearControls;
- (void)toggleSolid;
- (void)toggleAxesFrame;
- (void)turnFrame2;
- (void)reset;
- (void)zoomBy:(double)factor;
- (void)dragByX:(double)dx y:(double)dyUp;

- (void)setAxisLabels:(nonnull NSArray<NSView *> *)labels;

@property (nonatomic, readonly, nonnull) NSString *statusText;
@property (nonatomic, copy, nullable) void (^statusChanged)(NSString * _Nonnull text);

@end
