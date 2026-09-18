#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 fragWorldPos;

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    vec4 worldPositionSize; // xyz = center, w = size
    vec4 cameraRight;
    vec4 cameraUp;
    vec4 tint;
    ivec4 entityData; // x/y = UUID halves
} pushConstants;

void main() {
    vec3 worldPos = pushConstants.worldPositionSize.xyz
        + pushConstants.cameraRight.xyz * inPosition.x * pushConstants.worldPositionSize.w
        + pushConstants.cameraUp.xyz * inPosition.y * pushConstants.worldPositionSize.w;

    gl_Position = pushConstants.viewProj * vec4(worldPos, 1.0);
    fragUV = inUV;
    fragWorldPos = worldPos;
}
