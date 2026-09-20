#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in uvec4 inJointIds;
layout(location = 5) in vec4 inJointWeights;

layout(set = 2, binding = 0) uniform BonePalette {
    mat4 bones[128];
} bonePalette;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 fragWorldPos;
layout(location = 2) out vec3 fragWorldNormal;
layout(location = 3) flat out int fragNormalSource;
layout(location = 4) flat out int fragMaterialFlags;

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

void main() {
    float weightSum = inJointWeights.x + inJointWeights.y + inJointWeights.z + inJointWeights.w;
    mat4 skinMatrix = mat4(1.0);
    if (weightSum > 0.0001) {
        skinMatrix =
            inJointWeights.x * bonePalette.bones[min(inJointIds.x, 127u)] +
            inJointWeights.y * bonePalette.bones[min(inJointIds.y, 127u)] +
            inJointWeights.z * bonePalette.bones[min(inJointIds.z, 127u)] +
            inJointWeights.w * bonePalette.bones[min(inJointIds.w, 127u)];
    }

    vec4 skinnedPosition = skinMatrix * vec4(inPosition, 1.0);
    vec3 skinnedNormal = mat3(skinMatrix) * inNormal;
    gl_Position = pushConstants.mvp * skinnedPosition;

    fragUV = inUV;

    mat3 normalMatrix = transpose(inverse(mat3(pushConstants.model)));
    fragWorldNormal = normalize(normalMatrix * skinnedNormal);
    fragNormalSource = pushConstants.materialData.x;
    fragMaterialFlags = pushConstants.materialData.y;

    vec4 worldPos = pushConstants.model * skinnedPosition;
    fragWorldPos = worldPos.xyz;
}
