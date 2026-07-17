struct PointLight {
    vec4 positionRadius;
    vec4 colorIntensity;
};

layout(set = 1, binding = 0) uniform LightingUbo {
    vec4 cameraPosition;
    vec4 dirLightDirection;
    vec4 dirLightColorIntensity;
    PointLight pointLights[4];
    ivec4 pointLightCount;
    vec4 specularParams;
} u_Lighting;

float ComputeSpecular(vec3 normal, vec3 lightDir, vec3 viewDir, float roughness) {
    float ndotl = max(dot(normal, lightDir), 0.0);
    if (ndotl <= 0.0) {
        return 0.0;
    }

    vec3 halfVector = lightDir + viewDir;
    float halfVectorLength = length(halfVector);
    if (halfVectorLength <= 0.0) {
        return 0.0;
    }

    halfVector /= halfVectorLength;
    float shininess = mix(u_Lighting.specularParams.z, u_Lighting.specularParams.y, clamp(roughness, 0.0, 1.0));
    return pow(max(dot(normal, halfVector), 0.0), shininess) * u_Lighting.specularParams.x;
}

vec3 ComputeLighting(vec3 baseColor, vec3 normal, vec3 worldPos, vec3 viewDir, float roughness, float ao) {
    vec3 ambient = 0.08 * baseColor * ao;
    vec3 lighting = vec3(0.0);

    vec3 dirLightDir = normalize(-u_Lighting.dirLightDirection.xyz);
    float dirDiffuse = max(dot(normal, dirLightDir), 0.0);
    float dirSpecular = ComputeSpecular(normal, dirLightDir, viewDir, roughness);
    vec3 dirLightColor = u_Lighting.dirLightColorIntensity.rgb * u_Lighting.dirLightColorIntensity.a;
    lighting += baseColor * dirLightColor * dirDiffuse;
    lighting += dirLightColor * dirSpecular;

    for (int i = 0; i < u_Lighting.pointLightCount.x; ++i) {
        vec3 lightToFrag = worldPos - u_Lighting.pointLights[i].positionRadius.xyz;
        float distance = length(lightToFrag);
        float radius = max(u_Lighting.pointLights[i].positionRadius.w, 0.001);
        float attenuation = clamp(1.0 - distance / radius, 0.0, 1.0);
        attenuation *= attenuation;

        if (attenuation <= 0.0) {
            continue;
        }

        vec3 pointLightDir = normalize(-lightToFrag);
        float pointDiffuse = max(dot(normal, pointLightDir), 0.0);
        float pointSpecular = ComputeSpecular(normal, pointLightDir, viewDir, roughness);
        vec3 pointColor = u_Lighting.pointLights[i].colorIntensity.rgb * u_Lighting.pointLights[i].colorIntensity.a * attenuation;

        lighting += baseColor * pointColor * pointDiffuse;
        lighting += pointColor * pointSpecular;
    }

    return ambient + lighting;
}
