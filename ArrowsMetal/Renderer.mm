#import "Renderer.h"
#import <QuartzCore/QuartzCore.h>

#include "ShaderTypes.h"
#include "Core/Scene.h"
#include "Core/ArrowMesh.h"

@implementation Renderer {
    __weak MTKView              *_view;
    id<MTLDevice>                _device;
    id<MTLCommandQueue>          _queue;
    id<MTLRenderPipelineState>   _wirePipeline;
    id<MTLRenderPipelineState>   _solidPipeline;
    id<MTLDepthStencilState>     _depthState;
    id<MTLBuffer>                _vertexBuffer;
    id<MTLBuffer>                _triangleIndices;
    id<MTLBuffer>                _lineIndices;
    NSUInteger                   _triangleIndexCount;
    NSUInteger                   _lineIndexCount;

    Scene                        _scene;
    CFTimeInterval               _lastTime;
}

- (instancetype)initWithView:(MTKView *)view {
    if (!(self = [super init])) return nil;
    _view   = view;
    _device = view.device;
    _queue  = [_device newCommandQueue];

    view.colorPixelFormat        = MTLPixelFormatBGRA8Unorm_sRGB;   // shade in linear, store sRGB
    view.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    view.sampleCount             = 4;                                // MSAA: smooth wire lines
    view.clearColor              = MTLClearColorMake(0.035, 0.037, 0.045, 1.0);
    // Efficiency: no display-link while nothing moves; frames are requested on input.
    view.paused                  = YES;
    view.enableSetNeedsDisplay   = YES;

    if (![self buildPipelines:view]) return nil;
    [self buildGeometry];

    MTLDepthStencilDescriptor *dd = [MTLDepthStencilDescriptor new];
    dd.depthCompareFunction = MTLCompareFunctionLess;
    dd.depthWriteEnabled    = YES;
    _depthState = [_device newDepthStencilStateWithDescriptor:dd];
    return self;
}

- (BOOL)buildPipelines:(MTKView *)view {
    id<MTLLibrary> lib = [_device newDefaultLibrary];
    if (!lib) { NSLog(@"No default Metal library"); return NO; }

    MTLRenderPipelineDescriptor *pd = [MTLRenderPipelineDescriptor new];
    pd.vertexFunction                  = [lib newFunctionWithName:@"arrowVertex"];
    pd.rasterSampleCount               = view.sampleCount;
    pd.depthAttachmentPixelFormat      = view.depthStencilPixelFormat;
    MTLRenderPipelineColorAttachmentDescriptor *ca = pd.colorAttachments[0];
    ca.pixelFormat                 = view.colorPixelFormat;
    ca.blendingEnabled             = YES;      // used only during the cross-fade
    ca.sourceRGBBlendFactor        = MTLBlendFactorSourceAlpha;
    ca.destinationRGBBlendFactor   = MTLBlendFactorOneMinusSourceAlpha;
    ca.sourceAlphaBlendFactor      = MTLBlendFactorOne;
    ca.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

    NSError *err = nil;
    pd.fragmentFunction = [lib newFunctionWithName:@"wireFragment"];
    _wirePipeline = [_device newRenderPipelineStateWithDescriptor:pd error:&err];
    pd.fragmentFunction = [lib newFunctionWithName:@"metalFragment"];
    _solidPipeline = [_device newRenderPipelineStateWithDescriptor:pd error:&err];
    if (!_wirePipeline || !_solidPipeline) { NSLog(@"Pipeline error: %@", err); return NO; }
    return YES;
}

// Built once on the CPU with CVector/CRotrix, uploaded once, never touched again.
- (void)buildGeometry {
    TriadMesh mesh = buildTriad();
    _vertexBuffer = [_device newBufferWithBytes:mesh.vertices.data()
                                         length:mesh.vertices.size() * sizeof(ArrowVertex)
                                        options:MTLResourceStorageModeShared | MTLResourceCPUCacheModeWriteCombined];
    _triangleIndices = [_device newBufferWithBytes:mesh.triangles.data()
                                            length:mesh.triangles.size() * sizeof(uint16_t)
                                           options:MTLResourceStorageModeShared | MTLResourceCPUCacheModeWriteCombined];
    _lineIndices = [_device newBufferWithBytes:mesh.lines.data()
                                        length:mesh.lines.size() * sizeof(uint16_t)
                                       options:MTLResourceStorageModeShared | MTLResourceCPUCacheModeWriteCombined];
    _triangleIndexCount = mesh.triangles.size();
    _lineIndexCount     = mesh.lines.size();
}

#pragma mark - Input (from ArrowView)

- (void)wake {
    MTKView *v = _view;
    if (_scene.needsFrames()) {
        if (v.paused) { _lastTime = 0; v.paused = NO; }   // run at display rate
    } else {
        [v setNeedsDisplay:YES];                           // one frame only
    }
}

- (void)setControl:(int)control down:(BOOL)down {
    if (control < 0 || control >= ControlCount) return;
    _scene.setControl((Control)control, down);
    [self wake];
}
- (void)clearControls       { _scene.clearControls(); [self wake]; }
- (void)toggleSolid         { _scene.toggleSolid();     [self statusDidChange]; [self wake]; }
- (void)toggleAxesFrame     { _scene.toggleAxesFrame(); [self statusDidChange]; [self wake]; }
- (void)reset               { _scene.reset();           [self wake]; }
- (void)zoomBy:(double)f    { _scene.zoom(f);           [self wake]; }
- (void)dragByX:(double)dx y:(double)dy { _scene.dragRotate(dx, dy); [self wake]; }

- (NSString *)statusText {
    return [NSString stringWithFormat:@"%@   ·   rotate about %@ axes",
            _scene.isSolid() ? @"Solid metal" : @"Wireframe",
            _scene.isLocal() ? @"arrow (local)" : @"world"];
}

- (void)statusDidChange { if (self.statusChanged) self.statusChanged(self.statusText); }

#pragma mark - MTKViewDelegate

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    if (size.height > 0) _scene.setAspect(size.width / size.height);
    [view setNeedsDisplay:YES];
}

- (void)drawInMTKView:(MTKView *)view {
    CFTimeInterval now = CACurrentMediaTime();
    double dt = _lastTime > 0 ? now - _lastTime : 0.0;
    if (dt > 0.05) dt = 0.05;
    _lastTime = now;
    bool more = _scene.update(dt);

    // Per-frame CPU maths: one 4x4 product, done with CMatrix.
    CMatrix<4, 4> model = _scene.modelMatrix();
    CMatrix<4, 4> mvp   = _scene.viewProjection() * model;
    CVector<3>    light = CVector<3>(-0.4, 0.9, 0.6).normalized();

    FrameUniforms u;
    toColumnMajor(mvp,   u.mvp.m);
    toColumnMajor(model, u.model.m);
    const CVector<3>& eye = _scene.eye();
    for (int i = 0; i < 3; ++i) { u.eye.v[i] = (float)eye[i]; u.lightDir.v[i] = (float)light[i]; }
    u.eye.v[3] = 1.0f; u.lightDir.v[3] = 0.0f;
    u.params.v[0] = (float)_scene.solidAlpha();
    u.params.v[1] = (float)_scene.wireAlpha();
    u.params.v[2] = u.params.v[3] = 0.0f;

    MTLRenderPassDescriptor *pass = view.currentRenderPassDescriptor;
    id<MTLCommandBuffer> cmd = [_queue commandBuffer];
    if (pass) {
        id<MTLRenderCommandEncoder> enc = [cmd renderCommandEncoderWithDescriptor:pass];
        [enc setDepthStencilState:_depthState];
        [enc setFrontFacingWinding:MTLWindingCounterClockwise];
        [enc setVertexBuffer:_vertexBuffer offset:0 atIndex:kBufferVertices];
        [enc setVertexBytes:&u length:sizeof(u) atIndex:kBufferUniforms];
        [enc setFragmentBytes:&u length:sizeof(u) atIndex:kBufferUniforms];

        if (u.params.v[1] > 0.0f) {               // wireframe: one draw for all three arrows
            [enc setRenderPipelineState:_wirePipeline];
            [enc setCullMode:MTLCullModeNone];
            [enc drawIndexedPrimitives:MTLPrimitiveTypeLine indexCount:_lineIndexCount
                             indexType:MTLIndexTypeUInt16 indexBuffer:_lineIndices indexBufferOffset:0];
        }
        if (u.params.v[0] > 0.0f) {               // solid metal: one draw for all three arrows
            [enc setRenderPipelineState:_solidPipeline];
            [enc setCullMode:MTLCullModeBack];
            [enc drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:_triangleIndexCount
                             indexType:MTLIndexTypeUInt16 indexBuffer:_triangleIndices indexBufferOffset:0];
        }
        [enc endEncoding];
        [cmd presentDrawable:view.currentDrawable];
    }
    [cmd commit];

    if (!more && !view.paused) { view.paused = YES; _lastTime = 0; }   // idle: stop the display-link
}

@end
