#version 450

layout(location = 0) in vec2 fragUV;

layout(set = 0, binding = 0) uniform sampler2D u_CurrentColor;
layout(set = 0, binding = 1) uniform sampler2D u_HistoryColor;

layout(location = 0) out vec4 outColor;

float Luma(vec3 color) {
    vec3 perceptual = sqrt(max(color, vec3(0.0)));
    return dot(perceptual, vec3(0.299, 0.587, 0.114));
}

void main() {
    vec3 currentColor = texture(u_CurrentColor, fragUV).rgb;
    vec3 historyColor = texture(u_HistoryColor, fragUV).rgb;

    float currentLuma = Luma(currentColor);
    float historyLuma = Luma(historyColor);
    float delta = abs(currentLuma - historyLuma);
    float rejection = smoothstep(0.03, 0.18, delta);
    float historyWeight = mix(0.18, 0.02, rejection);

    outColor = vec4(mix(currentColor, historyColor, historyWeight), 1.0);
}