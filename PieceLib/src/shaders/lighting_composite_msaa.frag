#version 450

layout(location = 0) in vec2 fragUV;

layout(set = 0, binding = 0) uniform sampler2DMS u_WorldPosRoughness;
layout(set = 0, binding = 1) uniform sampler2DMS u_AlbedoAo;
layout(set = 0, binding = 2) uniform sampler2DMS u_NormalAo;
layout(set = 0, binding = 3) uniform sampler2DMS u_Emissive;
layout(set = 0, binding = 4) uniform sampler2D u_EnvDiffuse;
layout(set = 0, binding = 5) uniform sampler2D u_EnvSpecular;

#include "lighting_model.glsl"

layout(location = 0) out vec4 outColor;

vec2 DirectionToLatLongUV(vec3 dir) {
    dir = normalize(dir);
    float phi = atan(dir.z, dir.x);
    float theta = acos(clamp(dir.y, -1.0, 1.0));
    return vec2((phi + PI) / (2.0 * PI), theta / PI);
}

vec3 SampleEquirectangular(sampler2D environment, vec2 uv) {
    return textureLod(environment, vec2(fract(uv.x), uv.y), 0.0).rgb;
}

vec3 SampleEquirectangularLod(sampler2D environment, vec2 uv, float lod) {
    return textureLod(environment, vec2(fract(uv.x), uv.y), 0.0).rgb;
}

vec3 ComputeBackgroundEnvironment() {
    vec2 ndc = fragUV * 2.0 - 1.0;
    vec4 clip = vec4(ndc, 1.0, 1.0);
    vec4 world = u_Lighting.invViewProj * clip;
    vec3 worldPos = abs(world.w) > 1e-6 ? world.xyz / world.w : world.xyz;
    vec3 viewDir = normalize(worldPos - u_Lighting.cameraPosition.xyz);

    vec2 envUV = DirectionToLatLongUV(viewDir);
    vec3 envSpec = SampleEquirectangular(u_EnvSpecular, envUV);
    vec3 envDiff = SampleEquirectangular(u_EnvDiffuse, envUV);
    vec3 envColor = mix(envDiff, envSpec, 0.7);
    return envColor * max(u_Lighting.iblParams.x, 0.0);
}

vec3 ComputeEnvironmentLighting(vec3 albedo, vec3 normal, vec3 viewDir, float roughness, float metallic, float ao) {
    if (u_Lighting.iblParams.x <= 0.0 || metallic <= 0.001) {
        return vec3(0.0);
    }

    vec3 N = normalize(normal);
    vec3 V = normalize(viewDir);
    vec3 R = reflect(-V, N);

    vec2 diffuseUV = DirectionToLatLongUV(N);
    vec3 irradiance = SampleEquirectangular(u_EnvDiffuse, diffuseUV);

    float maxLod = max(u_Lighting.iblParams.w, 0.0);
    float lod = roughness * maxLod;
    vec2 specularUV = DirectionToLatLongUV(R);
    vec3 prefiltered = SampleEquirectangularLod(u_EnvSpecular, specularUV, lod);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    float NdotV = max(dot(N, V), 0.0);
    vec3 F = F_Schlick(NdotV, F0);
    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    vec3 diffuse = irradiance * albedo;
    // The authored HDR is not prefiltered yet, so attenuate its sharp reflection by roughness.
    vec3 specular = prefiltered * F * (1.0 - roughness) * (1.0 - roughness) * metallic;

    vec3 ibl = (kD * diffuse * u_Lighting.iblParams.y)
             + (specular * u_Lighting.iblParams.z);
    return ibl * ao * u_Lighting.iblParams.x;
}

void main() {
    ivec2 pixel = ivec2(fragUV * vec2(textureSize(u_WorldPosRoughness)));
    int sampleCount = textureSamples(u_WorldPosRoughness);

    vec3 accumColor = vec3(0.0);
    float coveredSamples = 0.0;
    vec3 background = (u_Lighting.iblParams.x > 0.0)
        ? ComputeBackgroundEnvironment()
        : vec3(0.1, 0.1, 0.1);

    for (int i = 0; i < sampleCount; ++i) {
        vec4 worldPosRoughness = texelFetch(u_WorldPosRoughness, pixel, i);
        vec4 albedoAo = texelFetch(u_AlbedoAo, pixel, i);
        vec4 normalAo = texelFetch(u_NormalAo, pixel, i);
        vec4 emissiveSample = texelFetch(u_Emissive, pixel, i);

        if (worldPosRoughness.w < -1.5) {
            // Editor billboard icon: unlit passthrough, albedo already holds the final color.
            accumColor += albedoAo.rgb;
            coveredSamples += 1.0;
            continue;
        }

        if (worldPosRoughness.w < 0.0) {
            continue;
        }

        vec3 worldPos = worldPosRoughness.xyz;
        vec3 albedo = albedoAo.rgb;
        float roughness = clamp(worldPosRoughness.w, 0.0, 1.0);
        vec3 normalEncoded = normalAo.rgb;
        vec3 normal = normalize(normalEncoded * 2.0 - 1.0);
        float ao = clamp(albedoAo.a, 0.0, 1.0);
        float metallic = clamp(normalAo.a, 0.0, 1.0);
        vec3 emissive = emissiveSample.rgb;

        vec3 viewDir = normalize(u_Lighting.cameraPosition.xyz - worldPos);
        vec3 litColor = ComputeLighting(albedo, normal, worldPos, viewDir, roughness, metallic, ao);
        vec3 iblColor = ComputeEnvironmentLighting(albedo, normal, viewDir, roughness, metallic, ao);
        accumColor += litColor + iblColor + emissive;
        coveredSamples += 1.0;
    }

    if (coveredSamples <= 0.0) {
        outColor = vec4(background, 1.0);
        return;
    }

    vec3 coveredColor = accumColor / coveredSamples;
    float coverage = coveredSamples / float(sampleCount);
    outColor = vec4(mix(background, coveredColor, coverage), 1.0);
}