#version 450

layout(location = 0) in vec2 fragUV;
layout(set = 0, binding = 0) uniform sampler2D u_InputColor;

#include "lighting_model.glsl"

layout(location = 0) out vec4 outColor;

void main() {
    vec3 color = texture(u_InputColor, fragUV).rgb;
    float brightness = max(max(color.r, color.g), color.b);
    float threshold = u_Lighting.bloomParams.x;
    float knee = max(threshold * 0.25, 0.001);
    float soft = clamp((brightness - threshold + knee) / (2.0 * knee), 0.0, 1.0);
    soft = soft * soft * (3.0 - 2.0 * soft);
    float contribution = max(brightness - threshold, 0.0) + soft * knee;
    outColor = vec4(color * (contribution / max(brightness, 0.0001)), 1.0);
}
