#ifndef ARROWS_SHADER_TYPES_H
#define ARROWS_SHADER_TYPES_H

#ifdef __METAL_VERSION__
typedef metal::packed_float2 Pack2f;
typedef metal::packed_float3 Pack3f;
typedef metal::float4x4      Mat4f;
typedef metal::float4        Vec4f;
#else
#include <stdint.h>
typedef struct { float u, v; }     Pack2f;
typedef struct { float x, y, z; }  Pack3f;
typedef struct { float m[16]; }    Mat4f;
typedef struct { float v[4]; }     Vec4f;
#endif

typedef struct {
    Pack3f position;
    Pack3f normal;
    Pack3f color;
    Pack2f uv;
} SceneVertex;

typedef struct {
    Mat4f mvp;
    Mat4f model;
    Vec4f eye;
    Vec4f lightDir;
    Vec4f params;
} FrameUniforms;

enum { kBufferVertices = 0, kBufferUniforms = 1, kTextureEarth = 0 };

#endif
