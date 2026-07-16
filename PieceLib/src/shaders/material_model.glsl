struct MaterialSample {
    vec4 albedo;
    float roughness;
    float ao;
};

MaterialSample SampleMaterial(vec2 uv) {
    MaterialSample material;
    material.albedo = texture(u_Albedo, uv);
    material.roughness = texture(u_Roughness, uv).r;
    material.ao = texture(u_AO, uv).r;
    return material;
}

vec3 ComputeSurfaceNormal(vec3 worldPos, vec3 viewDir) {
    vec3 dx = dFdx(worldPos);
    vec3 dy = dFdy(worldPos);
    vec3 normal = normalize(cross(dx, dy));

    if (dot(normal, viewDir) < 0.0) {
        normal = -normal;
    }

    return normal;
}
