#version 450

layout(location = 0) in vec2 fragUV;

layout(set = 0, binding = 0) uniform sampler2D u_WorldPosRoughness;
layout(set = 0, binding = 1) uniform sampler2D u_AlbedoAo;
layout(set = 0, binding = 2) uniform sampler2D u_NormalAo;

#include "lighting_model.glsl"

layout(location = 0) out vec4 outColor;

void main() {
    vec4 worldPosRoughness = texture(u_WorldPosRoughness, fragUV);
    vec4 albedoAo = texture(u_AlbedoAo, fragUV);
    vec4 normalAo = texture(u_NormalAo, fragUV);

    const vec3 kBackground = vec3(0.1, 0.1, 0.1);
    const vec3 kClearNormalEncoded = vec3(0.5, 0.5, 1.0);

    float coverage = clamp(albedoAo.a, 0.0, 1.0);
    if (coverage <= 0.001) {
        outColor = vec4(kBackground, 1.0);
        return;
    }

    float invCoverage = 1.0 / max(coverage, 1e-4);

    vec3 worldPos = worldPosRoughness.xyz * invCoverage;
    vec3 albedo = albedoAo.rgb * invCoverage;
    float roughness = clamp((worldPosRoughness.w + 1.0 - coverage) * invCoverage, 0.0, 1.0);
    vec3 normalEncoded = (normalAo.rgb - (1.0 - coverage) * kClearNormalEncoded) * invCoverage;
    vec3 normal = normalize(normalEncoded * 2.0 - 1.0);
    float ao = clamp(normalAo.a * invCoverage, 0.0, 1.0);

    vec3 viewDir = normalize(u_Lighting.cameraPosition.xyz - worldPos);
    vec3 litColor = ComputeLighting(albedo, normal, worldPos, viewDir, roughness, ao);

    vec3 finalColor = mix(kBackground, litColor, coverage);
    outColor = vec4(finalColor, 1.0);
}
