// Shared between the C++/Objective-C++ side and Shaders.metal.
// No matrix library on either side of this boundary: the CPU fills plain
// float arrays from CMatrix<4,4>; only the GPU reinterprets them as float4x4.
#ifndef ARROWS_SHADER_TYPES_H
#define ARROWS_SHADER_TYPES_H

#ifdef __METAL_VERSION__
typedef metal::packed_float3 Pack3f;
typedef metal::float4x4      Mat4f;
typedef metal::float4        Vec4f;
#else
#include <stdint.h>
typedef struct { float x, y, z; }  Pack3f;
typedef struct { float m[16]; }    Mat4f;   // column-major, as Metal expects
typedef struct { float v[4]; }     Vec4f;
#endif

// 36 bytes, identical on both sides. Colour is baked in so the whole triad
// is ONE draw call per mode (wire / solid).
typedef struct {
    Pack3f position;
    Pack3f normal;
    Pack3f color;
} ArrowVertex;

// Every member is a multiple of 16 bytes, so the C layout (align 4) and the
// Metal layout (align 16) agree: 176 bytes, passed with set*Bytes each frame.
typedef struct {
    Mat4f mvp;        // projection * view * model
    Mat4f model;      // rigid: rotation + translation (normal matrix = its 3x3)
    Vec4f eye;        // world-space camera position
    Vec4f lightDir;   // world-space, towards the light
    Vec4f params;     // x = solid alpha, y = wire alpha
} FrameUniforms;

enum { kBufferVertices = 0, kBufferUniforms = 1 };

#endif
