#version 450

layout(location = 0) in vec2 fragUV;
layout(set = 0, binding = 0) uniform sampler2D u_InputBloom;
layout(set = 0, binding = 1) uniform sampler2D u_BloomParams;

#include "lighting_model.glsl"

layout(location = 0) out vec4 outColor;

void main() {
    vec2 texel = 1.0 / vec2(textureSize(u_InputBloom, 0));
    float radius = max(texture(u_BloomParams, fragUV).z, 0.0);
    vec2 stepSize = texel * max(radius, 1.0) * vec2(0.0, 1.0);

    vec3 color = texture(u_InputBloom, fragUV).rgb * 0.227027;
    color += texture(u_InputBloom, fragUV + stepSize * 1.384615).rgb * 0.316216;
    color += texture(u_InputBloom, fragUV - stepSize * 1.384615).rgb * 0.316216;
    color += texture(u_InputBloom, fragUV + stepSize * 3.230769).rgb * 0.070270;
    color += texture(u_InputBloom, fragUV - stepSize * 3.230769).rgb * 0.070270;
    outColor = vec4(color, 1.0);
}
