#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragWorldPos;

layout(set = 0, binding = 0) uniform sampler2D u_Albedo;
layout(set = 0, binding = 1) uniform sampler2D u_Roughness;
layout(set = 0, binding = 2) uniform sampler2D u_AO;

#include "material_model.glsl"

layout(location = 0) out vec4 outWorldPosRoughness;
layout(location = 1) out vec4 outAlbedoAo;
layout(location = 2) out vec4 outNormalAo;

vec3 ComputeGeometryNormal(vec3 worldPos) {
    vec3 dx = dFdx(worldPos);
    vec3 dy = dFdy(worldPos);
    vec3 normal = normalize(cross(dx, dy));

    if (!gl_FrontFacing) {
        normal = -normal;
    }

    return normal;
}

void main() {
    MaterialSample material = SampleMaterial(fragUV);
    vec3 normal = ComputeGeometryNormal(fragWorldPos);

    outWorldPosRoughness = vec4(fragWorldPos, clamp(material.roughness, 0.0, 1.0));
    // Use alpha as coverage mask for correct deferred MSAA edge blending.
    outAlbedoAo = vec4(material.albedo.rgb, 1.0);
    outNormalAo = vec4(normal * 0.5 + 0.5, clamp(material.ao, 0.0, 1.0));
}
