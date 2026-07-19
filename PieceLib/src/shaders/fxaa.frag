#version 450

layout(location = 0) in vec2 fragUV;

layout(set = 0, binding = 0) uniform sampler2D u_InputColor;
layout(set = 0, binding = 1) uniform sampler2D u_GBufferWorldPosRoughness;

layout(location = 0) out vec4 outColor;

const float FXAA_SPAN_MAX = 12.0;
const float FXAA_REDUCE_MUL = 1.0 / 8.0;
const float FXAA_REDUCE_MIN = 1.0 / 128.0;
const float FXAA_EDGE_THRESHOLD = 1.0 / 32.0;
const float FXAA_EDGE_THRESHOLD_MIN = 1.0 / 64.0;
const float FXAA_SUBPIX_TRIM = 0.25;

float Luma(vec3 color) {
    // FXAA should operate on perceptual luma, not raw HDR linear radiance.
    vec3 perceptual = sqrt(max(color, vec3(0.0)));
    return dot(perceptual, vec3(0.299, 0.587, 0.114));
}

void main() {
    vec2 texel = 1.0 / vec2(textureSize(u_InputColor, 0));

    vec3 rgbM  = texture(u_InputColor, fragUV).rgb;
    vec3 worldPosM = texture(u_GBufferWorldPosRoughness, fragUV).xyz;
    vec3 rgbNW = texture(u_InputColor, fragUV + vec2(-1.0, -1.0) * texel).rgb;
    vec3 rgbNE = texture(u_InputColor, fragUV + vec2( 1.0, -1.0) * texel).rgb;
    vec3 rgbSW = texture(u_InputColor, fragUV + vec2(-1.0,  1.0) * texel).rgb;
    vec3 rgbSE = texture(u_InputColor, fragUV + vec2( 1.0,  1.0) * texel).rgb;

    vec3 worldPosNW = texture(u_GBufferWorldPosRoughness, fragUV + vec2(-1.0, -1.0) * texel).xyz;
    vec3 worldPosNE = texture(u_GBufferWorldPosRoughness, fragUV + vec2( 1.0, -1.0) * texel).xyz;
    vec3 worldPosSW = texture(u_GBufferWorldPosRoughness, fragUV + vec2(-1.0,  1.0) * texel).xyz;
    vec3 worldPosSE = texture(u_GBufferWorldPosRoughness, fragUV + vec2( 1.0,  1.0) * texel).xyz;

    float lumaM  = Luma(rgbM);
    float lumaNW = Luma(rgbNW);
    float lumaNE = Luma(rgbNE);
    float lumaSW = Luma(rgbSW);
    float lumaSE = Luma(rgbSE);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    float lumaRange = lumaMax - lumaMin;

    if (lumaRange < max(FXAA_EDGE_THRESHOLD_MIN, lumaMax * FXAA_EDGE_THRESHOLD)) {
        outColor = vec4(rgbM, 1.0);
        return;
    }

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max(
        (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL),
        FXAA_REDUCE_MIN);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = clamp(dir * rcpDirMin, vec2(-FXAA_SPAN_MAX), vec2(FXAA_SPAN_MAX)) * texel;

    vec3 rgbA = 0.5 * (
        texture(u_InputColor, fragUV + dir * (1.0 / 3.0 - 0.5)).rgb +
        texture(u_InputColor, fragUV + dir * (2.0 / 3.0 - 0.5)).rgb);

    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(u_InputColor, fragUV + dir * -0.5).rgb +
        texture(u_InputColor, fragUV + dir *  0.5).rgb);

    float lumaB = Luma(rgbB);
    vec3 filtered = (lumaB < lumaMin || lumaB > lumaMax) ? rgbA : rgbB;

    float lumaAvg = (lumaNW + lumaNE + lumaSW + lumaSE) * 0.25;
    float subpix = clamp(abs(lumaAvg - lumaM) / max(lumaRange, 1e-6), 0.0, 1.0);
    subpix = clamp((subpix - FXAA_SUBPIX_TRIM) / (1.0 - FXAA_SUBPIX_TRIM), 0.0, 1.0);
    subpix = subpix * subpix;

    float geometryEdge = max(
        max(length(worldPosNW - worldPosM), length(worldPosNE - worldPosM)),
        max(length(worldPosSW - worldPosM), length(worldPosSE - worldPosM)));
    geometryEdge = smoothstep(0.02, 0.20, geometryEdge);

    float edgeBlend = clamp(max(subpix, geometryEdge), 0.0, 1.0);

    outColor = vec4(mix(rgbM, filtered, edgeBlend), 1.0);
}