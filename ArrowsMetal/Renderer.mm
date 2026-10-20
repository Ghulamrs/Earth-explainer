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
    id<MTLRenderPipelineState>   _metalPipeline;
    id<MTLRenderPipelineState>   _earthPipeline;
    id<MTLDepthStencilState>     _depthState;
    id<MTLBuffer>                _vertexBuffer;
    id<MTLBuffer>                _arrowIndices;
    id<MTLBuffer>                _earthIndices;
    id<MTLBuffer>                _lineIndices;
    NSUInteger                   _arrowIndexCount;
    NSUInteger                   _earthIndexCount;
    NSUInteger                   _lineIndexCount;
    NSUInteger                   _arrowLineIndexCount;
    id<MTLTexture>               _earthTexture;
    BOOL                         _realTexture;
    NSArray<NSView *>           *_labels;

    Scene                        _scene;
    CFTimeInterval               _lastTime;
}

- (instancetype)initWithView:(MTKView *)view {
    if (!(self = [super init])) return nil;
    _view   = view;
    _device = view.device;
    _queue  = [_device newCommandQueue];

    view.colorPixelFormat        = MTLPixelFormatBGRA8Unorm_sRGB;
    view.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    view.sampleCount             = 4;
    view.clearColor              = MTLClearColorMake(0.20, 0.22, 0.26, 1.0);
    view.paused                  = YES;
    view.enableSetNeedsDisplay   = YES;

    if (![self buildPipelines:view]) return nil;
    [self buildGeometry];
    [self loadEarthTexture];

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
    pd.vertexFunction                  = [lib newFunctionWithName:@"sceneVertex"];
    pd.rasterSampleCount               = view.sampleCount;
    pd.depthAttachmentPixelFormat      = view.depthStencilPixelFormat;
    MTLRenderPipelineColorAttachmentDescriptor *ca = pd.colorAttachments[0];
    ca.pixelFormat                 = view.colorPixelFormat;
    ca.blendingEnabled             = YES;
    ca.sourceRGBBlendFactor        = MTLBlendFactorSourceAlpha;
    ca.destinationRGBBlendFactor   = MTLBlendFactorOneMinusSourceAlpha;
    ca.sourceAlphaBlendFactor      = MTLBlendFactorOne;
    ca.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

    NSError *err = nil;
    pd.fragmentFunction = [lib newFunctionWithName:@"wireFragment"];
    _wirePipeline  = [_device newRenderPipelineStateWithDescriptor:pd error:&err];
    pd.fragmentFunction = [lib newFunctionWithName:@"metalFragment"];
    _metalPipeline = [_device newRenderPipelineStateWithDescriptor:pd error:&err];
    pd.fragmentFunction = [lib newFunctionWithName:@"earthFragment"];
    _earthPipeline = [_device newRenderPipelineStateWithDescriptor:pd error:&err];
    if (!_wirePipeline || !_metalPipeline || !_earthPipeline) { NSLog(@"Pipeline error: %@", err); return NO; }
    return YES;
}

static id<MTLBuffer> makeBuffer(id<MTLDevice> d, const void *bytes, size_t length) {
    return [d newBufferWithBytes:bytes length:length
                         options:MTLResourceStorageModeShared | MTLResourceCPUCacheModeWriteCombined];
}

- (void)buildGeometry {
    SceneMesh mesh = buildScene();
    _vertexBuffer = makeBuffer(_device, mesh.vertices.data(),       mesh.vertices.size()       * sizeof(SceneVertex));
    _arrowIndices = makeBuffer(_device, mesh.arrowTriangles.data(), mesh.arrowTriangles.size() * sizeof(uint16_t));
    _earthIndices = makeBuffer(_device, mesh.earthTriangles.data(), mesh.earthTriangles.size() * sizeof(uint16_t));
    _lineIndices  = makeBuffer(_device, mesh.lines.data(),          mesh.lines.size()          * sizeof(uint16_t));
    _arrowIndexCount = mesh.arrowTriangles.size();
    _earthIndexCount = mesh.earthTriangles.size();
    _lineIndexCount  = mesh.lines.size();
    _arrowLineIndexCount = mesh.arrowLineIndexCount;
}

- (void)loadEarthTexture {
    NSBundle *b = NSBundle.mainBundle;
    NSURL *url = [b URLForResource:@"earth" withExtension:@"jpg"];
    if (!url) url = [b URLForResource:@"earth" withExtension:@"png"];
    if (url) {
        MTKTextureLoader *loader = [[MTKTextureLoader alloc] initWithDevice:_device];
        NSError *err = nil;
        _earthTexture = [loader newTextureWithContentsOfURL:url
                                                    options:@{ MTKTextureLoaderOptionSRGB: @YES,
                                                               MTKTextureLoaderOptionGenerateMipmaps: @YES,
                                                               MTKTextureLoaderOptionTextureStorageMode: @(MTLStorageModePrivate) }
                                                      error:&err];
        if (!_earthTexture) NSLog(@"Earth texture %@ failed: %@", url.lastPathComponent, err);
    }
    _realTexture = _earthTexture != nil;
    if (_realTexture) return;

    const int W = 256, H = 128;
    std::vector<uint8_t> px(W * H * 4);
    for (int y = 0; y < H; ++y) {
        double lat = 90.0 - 180.0 * (y + 0.5) / H;
        bool ice = std::fabs(lat) > 72.0;
        for (int x = 0; x < W; ++x) {
            uint8_t *p = &px[(y * W + x) * 4];
            if (ice) { p[0] = 235; p[1] = 240; p[2] = 245; }
            else     { p[0] = 20;  p[1] = (uint8_t)(70 + std::fabs(lat)); p[2] = 160; }
            p[3] = 255;
        }
    }
    MTLTextureDescriptor *td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm_sRGB
                                                                                  width:W height:H mipmapped:NO];
    _earthTexture = [_device newTextureWithDescriptor:td];
    [_earthTexture replaceRegion:MTLRegionMake2D(0, 0, W, H) mipmapLevel:0 withBytes:px.data() bytesPerRow:W * 4];
}

#pragma mark - Input (from ArrowView)

- (void)wake {
    MTKView *v = _view;
    if (_scene.needsFrames()) {
        if (v.paused) { _lastTime = 0; v.paused = NO; }
    } else {
        [v setNeedsDisplay:YES];
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
- (void)turnFrame2          { if (_scene.turnFrame2()) [self wake]; }
- (void)reset               { _scene.reset();           [self wake]; }
- (void)zoomBy:(double)f    { _scene.zoom(f);           [self wake]; }
- (void)dragByX:(double)dx y:(double)dy { _scene.dragRotate(dx, dy); [self wake]; }
- (void)setAxisLabels:(NSArray<NSView *> *)labels { _labels = [labels copy]; [self wake]; }

- (NSString *)statusText {
    return [NSString stringWithFormat:@"%@   ·   rotate about %@ axes%@",
            _scene.isSolid() ? @"Solid" : @"Wireframe",
            _scene.isLocal() ? @"body (local)" : @"world",
            _realTexture ? @"" : @"   ·   placeholder Earth: add Resources/earth.jpg"];
}

- (void)statusDidChange { if (self.statusChanged) self.statusChanged(self.statusText); }

#pragma mark - MTKViewDelegate

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    if (size.height > 0) _scene.setAspect(size.width / size.height);
    [view setNeedsDisplay:YES];
}

- (void)placeLabels:(MTKView *)view {
    if (_labels.count != 3) return;
    NSSize size = view.bounds.size;
    bool earthOpaque = _scene.solidAlpha() > 0.5;
    for (int i = 0; i < 3; ++i) {
        AxisLabel a = _scene.projectAxisLabel(i, size.width, size.height, earthOpaque);
        NSView *label = _labels[i];
        label.hidden = !a.visible;
        if (!a.visible) continue;
        NSSize ls = label.frame.size;
        [label setFrameOrigin:NSMakePoint(round(a.x - ls.width * 0.5), round(a.y - ls.height * 0.5))];
        label.alphaValue = a.occluded ? 0.3 : 1.0;
    }
}

- (FrameUniforms)uniformsFor:(const CMatrix<4, 4>&)model light:(const CVector<3>&)light {
    FrameUniforms u;
    toColumnMajor(_scene.viewProjection() * model, u.mvp.m);
    toColumnMajor(model, u.model.m);
    const CVector<3>& eye = _scene.eye();
    for (int i = 0; i < 3; ++i) { u.eye.v[i] = (float)eye[i]; u.lightDir.v[i] = (float)light[i]; }
    u.eye.v[3] = 1.0f; u.lightDir.v[3] = 0.0f;
    u.params.v[0] = (float)_scene.solidAlpha();
    u.params.v[1] = (float)_scene.wireAlpha();
    u.params.v[2] = u.params.v[3] = 0.0f;
    return u;
}

- (void)bind:(const FrameUniforms&)u to:(id<MTLRenderCommandEncoder>)enc {
    [enc setVertexBytes:&u length:sizeof(u) atIndex:kBufferUniforms];
    [enc setFragmentBytes:&u length:sizeof(u) atIndex:kBufferUniforms];
}

- (void)drawInMTKView:(MTKView *)view {
    CFTimeInterval now = CACurrentMediaTime();
    double dt = _lastTime > 0 ? now - _lastTime : 0.0;
    if (dt > 0.05) dt = 0.05;
    _lastTime = now;
    bool more = _scene.update(dt);

    const ViewBasis& cam = _scene.cameraBasis();
    CVector<3> light = (cam.up * 0.6 - cam.right * 0.45 - cam.forward * 0.75).normalized();
    FrameUniforms u1 = [self uniformsFor:_scene.frame1().drawMatrix() light:light];
    FrameUniforms u2 = [self uniformsFor:_scene.frame2().drawMatrix() light:light];
    float solid = u1.params.v[0], wire = u1.params.v[1];

    MTLRenderPassDescriptor *pass = view.currentRenderPassDescriptor;
    id<MTLCommandBuffer> cmd = [_queue commandBuffer];
    if (pass) {
        id<MTLRenderCommandEncoder> enc = [cmd renderCommandEncoderWithDescriptor:pass];
        [enc setDepthStencilState:_depthState];
        [enc setFrontFacingWinding:MTLWindingCounterClockwise];
        [enc setVertexBuffer:_vertexBuffer offset:0 atIndex:kBufferVertices];

        if (wire > 0.0f) {
            [enc setRenderPipelineState:_wirePipeline];
            [enc setCullMode:MTLCullModeNone];
            [self bind:u1 to:enc];
            [enc drawIndexedPrimitives:MTLPrimitiveTypeLine indexCount:_lineIndexCount
                             indexType:MTLIndexTypeUInt16 indexBuffer:_lineIndices indexBufferOffset:0];
            [self bind:u2 to:enc];
            [enc drawIndexedPrimitives:MTLPrimitiveTypeLine indexCount:_arrowLineIndexCount
                             indexType:MTLIndexTypeUInt16 indexBuffer:_lineIndices indexBufferOffset:0];
        }
        if (solid > 0.0f) {
            [enc setCullMode:MTLCullModeBack];
            [self bind:u1 to:enc];
            [enc setRenderPipelineState:_earthPipeline];
            [enc setFragmentTexture:_earthTexture atIndex:kTextureEarth];
            [enc drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:_earthIndexCount
                             indexType:MTLIndexTypeUInt16 indexBuffer:_earthIndices indexBufferOffset:0];
            [enc setRenderPipelineState:_metalPipeline];
            [enc drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:_arrowIndexCount
                             indexType:MTLIndexTypeUInt16 indexBuffer:_arrowIndices indexBufferOffset:0];
            [self bind:u2 to:enc];
            [enc drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:_arrowIndexCount
                             indexType:MTLIndexTypeUInt16 indexBuffer:_arrowIndices indexBufferOffset:0];
        }
        [enc endEncoding];
        [cmd presentDrawable:view.currentDrawable];
    }
    [cmd commit];
    [self placeLabels:view];

    if (!more && !view.paused) { view.paused = YES; _lastTime = 0; }
}

@end
