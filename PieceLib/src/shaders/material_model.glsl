struct MaterialSample {
    vec4 albedo;
    vec3 normal;
    float metallic;
    float roughness;
    float ao;
    vec3 emissive;
};

MaterialSample SampleMaterial(vec2 uv) {
    MaterialSample material;
    material.albedo = texture(u_Albedo, uv);
    material.normal = texture(u_Normal, uv).rgb;
    vec4 metallicRoughness = texture(u_Roughness, uv);
    // glTF metallicRoughness uses G=roughness and B=metallic. Grayscale roughness maps still work via R fallback.
    material.roughness = max(metallicRoughness.g, metallicRoughness.r);
    material.metallic = metallicRoughness.b;
    material.ao = texture(u_AO, uv).r;
    material.emissive = texture(u_Emissive, uv).rgb;
    return material;
}
