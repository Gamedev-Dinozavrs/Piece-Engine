#version 450

layout(location = 0) in vec2 fragUV;
layout(set = 0, binding = 0) uniform sampler2D u_InputColor;
layout(set = 0, binding = 1) uniform sampler2D u_BloomParams;
layout(set = 0, binding = 2) uniform sampler2D u_EmissiveBloom;

#include "lighting_model.glsl"

layout(location = 0) out vec4 outColor;

void main() {
    vec4 bloomInput = texture(u_InputColor, fragUV);
    vec4 bloomParams = texture(u_BloomParams, fragUV);
    vec4 emissiveBloom = texture(u_EmissiveBloom, fragUV);
    vec3 hdrColor = max(bloomInput.rgb - emissiveBloom.rgb, vec3(0.0)) * bloomParams.w;
    vec3 color = hdrColor + emissiveBloom.rgb * emissiveBloom.a;
    float brightness = max(max(color.r, color.g), color.b);
    float threshold = max(bloomParams.x, 0.0);
    float knee = max(threshold * 0.25, 0.001);
    float soft = clamp((brightness - threshold + knee) / (2.0 * knee), 0.0, 1.0);
    soft = soft * soft * (3.0 - 2.0 * soft);
    float contribution = max(brightness - threshold, 0.0) + soft * knee;
    outColor = vec4(color * (contribution / max(brightness, 0.0001)) * max(bloomParams.y, 0.0), 1.0);
}
