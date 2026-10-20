#include <metal_stdlib>
#include "ShaderTypes.h"
using namespace metal;

struct VOut {
    float4 position [[position]];
    float3 worldPos;
    float3 worldNormal;
    float3 color;
    float2 uv;
};

vertex VOut sceneVertex(uint vid [[vertex_id]],
                        const device SceneVertex* verts [[buffer(kBufferVertices)]],
                        constant FrameUniforms&   u     [[buffer(kBufferUniforms)]]) {
    SceneVertex v = verts[vid];
    float4 p = float4(float3(v.position), 1.0);
    VOut o;
    o.position    = u.mvp * p;
    o.worldPos    = (u.model * p).xyz;
    o.worldNormal = (u.model * float4(float3(v.normal), 0.0)).xyz;
    o.color       = float3(v.color);
    o.uv          = float2(v.uv);
    return o;
}

fragment float4 wireFragment(VOut in [[stage_in]],
                             constant FrameUniforms& u [[buffer(kBufferUniforms)]]) {
    return float4(in.color, u.params.y);
}

static float3 environment(float3 r) {
    float3 env = mix(float3(0.03, 0.03, 0.035), float3(0.60, 0.63, 0.70), smoothstep(-0.25, 0.6, r.x));
    env += float3(3.0) * pow(saturate(dot(r, normalize(float3(0.8, -0.5, -0.6)))), 48.0);
    env += float3(1.2) * pow(saturate(dot(r, normalize(float3(0.3,  0.8,  0.5)))), 24.0);
    return env;
}

fragment float4 metalFragment(VOut in [[stage_in]],
                              constant FrameUniforms& u [[buffer(kBufferUniforms)]]) {
    float3 N = normalize(in.worldNormal);
    float3 V = normalize(u.eye.xyz - in.worldPos);
    float3 L = normalize(u.lightDir.xyz);
    float3 H = normalize(L + V);
    float3 R = reflect(-V, N);

    float  NdotV = saturate(dot(N, V));
    float3 F0    = max(in.color, float3(0.10));
    float3 F     = F0 + (1.0 - F0) * pow(1.0 - NdotV, 5.0);

    float  diffuse  = saturate(dot(N, L));
    float  specular = pow(saturate(dot(N, H)), 120.0);

    float3 c = in.color * (0.06 + 0.22 * diffuse)
             + F * environment(R) * 0.9
             + F * specular * 2.5;
    return float4(c, u.params.x);
}

fragment float4 earthFragment(VOut in [[stage_in]],
                              constant FrameUniforms& u [[buffer(kBufferUniforms)]],
                              texture2d<float> earth [[texture(kTextureEarth)]]) {
    constexpr sampler s(filter::linear, mip_filter::linear, max_anisotropy(8),
                        s_address::repeat, t_address::clamp_to_edge);
    float3 albedo = earth.sample(s, in.uv).rgb;

    float3 N = normalize(in.worldNormal);
    float3 V = normalize(u.eye.xyz - in.worldPos);
    float3 L = normalize(u.lightDir.xyz);
    float3 H = normalize(L + V);
    float3 R = reflect(-V, N);
    float  NdotV = saturate(dot(N, V));

    float ocean    = saturate((albedo.b - max(albedo.r, albedo.g)) * 6.0);
    float diffuse  = saturate(dot(N, L));
    float specular = pow(saturate(dot(N, H)), mix(24.0, 160.0, ocean)) * mix(0.15, 1.6, ocean);
    float fresnel  = pow(1.0 - NdotV, 5.0);

    float3 c = albedo * (0.16 + 0.95 * diffuse)
             + specular * float3(1.0, 0.97, 0.90)
             + environment(R) * fresnel * 0.35
             + float3(0.30, 0.55, 1.0) * pow(1.0 - NdotV, 3.0) * 0.5;
    return float4(c, u.params.x);
}
