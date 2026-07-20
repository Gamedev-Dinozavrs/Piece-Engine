#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 fragWorldPos;
layout(location = 2) out vec3 fragWorldNormal;
layout(location = 3) flat out int fragNormalSource;
layout(location = 4) flat out int fragMaterialFlags;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    mat4 model;
    vec4 materialFactors;
    ivec4 materialData;
} pushConstants;

void main() {
    gl_Position = pushConstants.mvp * vec4(inPosition, 1.0);

    fragUV = inUV;

    mat3 normalMatrix = transpose(inverse(mat3(pushConstants.model)));
    fragWorldNormal = normalize(normalMatrix * inNormal);
    fragNormalSource = pushConstants.materialData.x;
    fragMaterialFlags = pushConstants.materialData.y;

    vec4 worldPos = pushConstants.model * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;
}
