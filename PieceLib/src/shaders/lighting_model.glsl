struct PointLight {
    vec4 positionRadius;
    vec4 colorIntensity;
};

layout(set = 1, binding = 0) uniform LightingUbo {
    vec4 cameraPosition;
    mat4 invViewProj;
    vec4 dirLightDirection;
    vec4 dirLightColorIntensity;
    PointLight pointLights[4];
    ivec4 pointLightCount;
    vec4 specularParams;
    vec4 iblParams; // x = intensity, y = diffuseStrength, z = specularStrength, w = maxSpecularLod
} u_Lighting;

// ----------------------------------------------------------------------------
// Cook-Torrance BRDF helpers
// ----------------------------------------------------------------------------

const float PI = 3.14159265359;

// GGX / Trowbridge-Reitz normal distribution.
// roughness is perceptually remapped: a = roughness^2.
float D_GGX(float NdotH, float roughness) {
    float a  = roughness * roughness;
    float a2 = a * a;
    float d  = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

// Schlick-GGX single-term geometry factor.
float G_SchlickGGX(float NdotX, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotX / (NdotX * (1.0 - k) + k);
}

// Smith geometry term (view + light).
float G_Smith(float NdotV, float NdotL, float roughness) {
    return G_SchlickGGX(max(NdotV, 0.0001), roughness)
         * G_SchlickGGX(max(NdotL, 0.0001), roughness);
}

// Fresnel-Schlick approximation.
vec3 F_Schlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float PointLightAttenuation(float distance, float radius) {
    radius = max(radius, 0.001);

    // Keep the light finite at its origin while retaining inverse-square behavior.
    float inverseSquare = 1.0 / max(distance * distance, 1.0);
    float softCutoff = 1.0 - smoothstep(radius * 0.75, radius, distance);
    return inverseSquare * softCutoff;
}

// ----------------------------------------------------------------------------
// Per-light Cook-Torrance evaluation
// ----------------------------------------------------------------------------
vec3 CookTorrance(
    vec3  albedo,
    float metallic,
    float roughness,
    vec3  normal,
    vec3  viewDir,
    vec3  lightDir,
    vec3  lightColor)
{
    float NdotL = max(dot(normal, lightDir), 0.0);
    if (NdotL <= 0.0) return vec3(0.0);

    vec3  H     = normalize(viewDir + lightDir);
    float NdotV = max(dot(normal, viewDir), 0.0001);
    float NdotH = max(dot(normal, H),       0.0);
    float HdotV = max(dot(H,      viewDir), 0.0);

    // Reflectance at normal incidence: 0.04 for dielectrics, albedo for metals.
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    float D   = D_GGX(NdotH, roughness);
    float G   = G_Smith(NdotV, NdotL, roughness);
    vec3  F   = F_Schlick(HdotV, F0);

    vec3 specular = (D * G * F) / (4.0 * NdotV * NdotL);

    // Energy conservation: metals have no diffuse term.
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

    return (kD * albedo / PI + specular) * lightColor * NdotL;
}

// ----------------------------------------------------------------------------
// Scene lighting accumulation
// ----------------------------------------------------------------------------
vec3 ComputeLighting(vec3 baseColor, vec3 normal, vec3 worldPos, vec3 viewDir, float roughness, float ao) {
    // Clamp roughness to avoid numerical issues in the GGX denominator.
    roughness = clamp(roughness, 0.05, 1.0);

    // metallic = 0 (dielectric) until the G-buffer gains a metallic channel.
    float metallic = 0.0;

    vec3 lighting = vec3(0.0);

    // Directional light
    vec3 dirLightDir   = normalize(-u_Lighting.dirLightDirection.xyz);
    vec3 dirLightColor = u_Lighting.dirLightColorIntensity.rgb
                       * u_Lighting.dirLightColorIntensity.a;
    lighting += CookTorrance(baseColor, metallic, roughness, normal, viewDir, dirLightDir, dirLightColor);

    // Point lights
    for (int i = 0; i < u_Lighting.pointLightCount.x; ++i) {
        vec3  lightToFrag = worldPos - u_Lighting.pointLights[i].positionRadius.xyz;
        float distance    = length(lightToFrag);
        float radius      = u_Lighting.pointLights[i].positionRadius.w;
        float attenuation = PointLightAttenuation(distance, radius);

        if (attenuation <= 0.0) continue;

        vec3 pointLightDir   = normalize(-lightToFrag);
        vec3 pointLightColor = u_Lighting.pointLights[i].colorIntensity.rgb
                             * u_Lighting.pointLights[i].colorIntensity.a
                             * attenuation;
        lighting += CookTorrance(baseColor, metallic, roughness, normal, viewDir, pointLightDir, pointLightColor);
    }

    vec3 ambient = max(u_Lighting.specularParams.w, 0.0) * baseColor * ao;

    return ambient + lighting;
}

vec3 ComputeLighting(vec3 baseColor, vec3 normal, vec3 worldPos, vec3 viewDir, float roughness, float metallic, float ao) {
    roughness = clamp(roughness, 0.05, 1.0);
    metallic = clamp(metallic, 0.0, 1.0);

    vec3 lighting = vec3(0.0);

    vec3 dirLightDir   = normalize(-u_Lighting.dirLightDirection.xyz);
    vec3 dirLightColor = u_Lighting.dirLightColorIntensity.rgb
                       * u_Lighting.dirLightColorIntensity.a;
    lighting += CookTorrance(baseColor, metallic, roughness, normal, viewDir, dirLightDir, dirLightColor);

    for (int i = 0; i < u_Lighting.pointLightCount.x; ++i) {
        vec3  lightToFrag = worldPos - u_Lighting.pointLights[i].positionRadius.xyz;
        float distance    = length(lightToFrag);
        float radius      = u_Lighting.pointLights[i].positionRadius.w;
        float attenuation = PointLightAttenuation(distance, radius);

        if (attenuation <= 0.0) continue;

        vec3 pointLightDir   = normalize(-lightToFrag);
        vec3 pointLightColor = u_Lighting.pointLights[i].colorIntensity.rgb
                             * u_Lighting.pointLights[i].colorIntensity.a
                             * attenuation;
        lighting += CookTorrance(baseColor, metallic, roughness, normal, viewDir, pointLightDir, pointLightColor);
    }

    vec3 ambient = max(u_Lighting.specularParams.w, 0.0) * baseColor * ao;
    return ambient + lighting;
}
