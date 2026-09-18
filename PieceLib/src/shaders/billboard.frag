#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragWorldPos;

layout(set = 0, binding = 0) uniform sampler2D u_Icon;

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    vec4 worldPositionSize;
    vec4 cameraRight;
    vec4 cameraUp;
    vec4 tint;
    ivec4 entityData;
} pushConstants;

layout(location = 0) out vec4 outWorldPosRoughness;
layout(location = 1) out vec4 outAlbedoAo;
layout(location = 2) out vec4 outNormalAo;
layout(location = 3) out vec4 outEmissive;
layout(location = 4) out uvec2 outEntityId;
layout(location = 5) out vec4 outBloomParams;

void main() {
    vec4 iconSample = texture(u_Icon, fragUV);
    if (iconSample.a < 0.5) {
        discard;
    }

    // Roughness < -1.5 tells the lighting pass this pixel is an unlit icon, not scene geometry.
    outWorldPosRoughness = vec4(fragWorldPos, -2.0);
    outAlbedoAo = vec4(iconSample.rgb * pushConstants.tint.rgb, 1.0);
    outNormalAo = vec4(0.5, 0.5, 1.0, 0.0);
    outEmissive = vec4(0.0);
    outEntityId = uvec2(uint(pushConstants.entityData.x), uint(pushConstants.entityData.y));
    outBloomParams = vec4(0.0);
}
