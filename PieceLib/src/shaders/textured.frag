#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 2) in vec3 fragWorldNormal;
layout(location = 3) flat in int fragNormalSource;
layout(location = 4) flat in int fragMaterialFlags;

layout(set = 0, binding = 0) uniform sampler2D u_Albedo;
layout(set = 0, binding = 1) uniform sampler2D u_Normal;
layout(set = 0, binding = 2) uniform sampler2D u_Roughness;
layout(set = 0, binding = 3) uniform sampler2D u_Metallic;
layout(set = 0, binding = 4) uniform sampler2D u_AO;
layout(set = 0, binding = 5) uniform sampler2D u_Emissive;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    mat4 model;
    vec4 materialFactors;
    vec4 baseColor;
    vec4 emissiveColor;
    vec4 bloomParams;
    vec4 bloomFlags;
    ivec4 materialData;
} pushConstants;

#include "material_model.glsl"

layout(location = 0) out vec4 outWorldPosRoughness;
layout(location = 1) out vec4 outAlbedoAo;
layout(location = 2) out vec4 outNormalAo;
layout(location = 3) out vec4 outEmissive;
layout(location = 4) out uvec2 outEntityId;
layout(location = 5) out vec4 outBloomParams;

const int MATERIAL_FLAG_HAS_NORMAL_MAP = 1 << 0;
const int MATERIAL_FLAG_HAS_EMISSIVE_MAP = 1 << 1;
const int MATERIAL_FLAG_HAS_METALLIC_MAP = 1 << 2;

vec3 ComputeTangentSpaceNormal(vec3 baseNormal, vec3 worldPos, vec2 uv, vec3 encodedNormal) {
    vec3 tangentNormal = encodedNormal * 2.0 - 1.0;

    // Empty/missing normal maps currently resolve to white fallback texels.
    if (all(greaterThan(encodedNormal, vec3(0.99)))) {
        tangentNormal = vec3(0.0, 0.0, 1.0);
    }

    vec3 dp1 = dFdx(worldPos);
    vec3 dp2 = dFdy(worldPos);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);

    float det = duv1.x * duv2.y - duv1.y * duv2.x;
    if (abs(det) < 1e-8) {
        return normalize(baseNormal);
    }

    vec3 T = normalize(dp1 * duv2.y - dp2 * duv1.y);
    vec3 B = normalize(-dp1 * duv2.x + dp2 * duv1.x);
    vec3 N = normalize(baseNormal);
    mat3 TBN = mat3(T, B, N);
    return normalize(TBN * tangentNormal);
}

vec3 ComputeGeometryNormal(vec3 worldPos) {
    vec3 dx = dFdx(worldPos);
    vec3 dy = dFdy(worldPos);
    // In Vulkan screen space Y increases downward, so cross(dx, dy) points
    // into the surface on front faces. Swapping the operands gives the correct
    // outward-facing normal.
    vec3 normal = normalize(cross(dy, dx));

    if (!gl_FrontFacing) {
        normal = -normal;
    }

    return normal;
}

void main() {
    MaterialSample material = SampleMaterial(fragUV, (fragMaterialFlags & MATERIAL_FLAG_HAS_METALLIC_MAP) != 0, u_Metallic);
    material.albedo.rgb *= pushConstants.baseColor.rgb;
    material.roughness = clamp(material.roughness * pushConstants.materialFactors.x, 0.0, 1.0);
    material.metallic = clamp(material.metallic * pushConstants.materialFactors.y, 0.0, 1.0);

    vec3 normal = ComputeGeometryNormal(fragWorldPos);
    if (fragNormalSource == 1) {
        normal = normalize(fragWorldNormal);
        if (!gl_FrontFacing) {
            normal = -normal;
        }
    }

    if ((fragMaterialFlags & MATERIAL_FLAG_HAS_NORMAL_MAP) != 0) {
        normal = ComputeTangentSpaceNormal(normalize(normal), fragWorldPos, fragUV, material.normal);
        if (!gl_FrontFacing) {
            normal = -normal;
        }
    }

    outWorldPosRoughness = vec4(fragWorldPos, clamp(material.roughness, 0.0, 1.0));
    outAlbedoAo = vec4(material.albedo.rgb, clamp(material.ao, 0.0, 1.0));
    outNormalAo = vec4(normal * 0.5 + 0.5, clamp(material.metallic, 0.0, 1.0));
    vec3 emissive = (pushConstants.emissiveColor.a > 0.5)
        ? material.emissive * pushConstants.emissiveColor.rgb
        : vec3(0.0);
    outEmissive = vec4(emissive, pushConstants.bloomFlags.x);
    outEntityId = uvec2(
        uint(pushConstants.materialData.z),
        uint(pushConstants.materialData.w));
    outBloomParams = pushConstants.bloomParams;
}
