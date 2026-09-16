#version 450

layout(location = 0) in vec2 fragUV;

layout(set = 0, binding = 0) uniform sampler2D u_InputColor;
layout(set = 0, binding = 1) uniform sampler2D u_BloomColor;

#include "lighting_model.glsl"

layout(location = 0) out vec4 outColor;

void main() {
    vec3 sceneColor = texture(u_InputColor, fragUV).rgb;
    vec3 bloom = texture(u_BloomColor, fragUV).rgb;
    outColor = vec4(sceneColor + bloom, 1.0);
}