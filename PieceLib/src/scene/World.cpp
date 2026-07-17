#include <PiecePCH.h>

#include <scene/World.h>

#include <scene/Components.h>
#include <scene/Entity.h>
#include <scene/Scene.h>

namespace Piece {

namespace World {

namespace {

constexpr glm::vec3 kDefaultDirectionalDirection{-0.4f, -1.0f, -0.2f};
constexpr glm::vec3 kDefaultDirectionalColor{1.0f, 0.98f, 0.9f};
constexpr float kDefaultDirectionalIntensity = 1.2f;
constexpr uint32_t kMaxPointLights = 4;

struct MaterialRecord {
    uint32_t id{0};
    std::string name;
    MaterialTextures textures{};
};

Ref<Scene> s_ActiveScene = CreateRef<Scene>();
float s_SpecularStrength = 1.0f;
float s_SpecularShininessMin = 8.0f;
float s_SpecularShininessMax = 128.0f;
uint32_t s_NextMaterialId = 1;
std::vector<MaterialRecord> s_Materials;

Ref<Scene> EnsureScene() {
    if (!s_ActiveScene) {
        s_ActiveScene = CreateRef<Scene>();
    }
    return s_ActiveScene;
}

Entity FindDirectionalLightEntity(Scene& scene) {
    auto view = scene.GetAllEntitiesViewWith<DirectionalLightComponent>();
    for (auto handle : view) {
        return Entity{handle, &scene};
    }
    return {};
}

std::vector<Entity> CollectPointLights(Scene& scene) {
    std::vector<Entity> lights;
    auto view = scene.GetAllEntitiesViewWith<PointLightComponent>();
    for (auto handle : view) {
        lights.emplace_back(handle, &scene);
    }
    return lights;
}

} // namespace

Ref<Scene> GetActiveScene() {
    return EnsureScene();
}

void SetActiveScene(const Ref<Scene>& scene) {
    s_ActiveScene = scene ? scene : CreateRef<Scene>();
}

uint32_t SpawnPrimitive(PrimitiveType primitiveType, const SpawnTransform& transform, const std::string& name) {
    Ref<Scene> scene = EnsureScene();
    Entity entity;
    switch (primitiveType) {
    case PrimitiveType::Quad:
        entity = scene->CreateQuad(name.empty() ? "Quad" : name);
        break;
    case PrimitiveType::Cube:
        entity = scene->CreateCube(name.empty() ? "Cube" : name);
        break;
    case PrimitiveType::Sphere:
        entity = scene->CreateSphere(name.empty() ? "Sphere" : name);
        break;
    default:
        return 0;
    }

    auto& transformComponent = entity.GetComponent<TransformComponent>();
    transformComponent.position = transform.position;
    transformComponent.rotation = transform.rotation;
    transformComponent.scale = transform.scale;
    return static_cast<uint32_t>(entity);
}

uint32_t SpawnQuad(const SpawnTransform& transform) {
    return SpawnPrimitive(PrimitiveType::Quad, transform, "Quad");
}

uint32_t SpawnCube(const SpawnTransform& transform) {
    return SpawnPrimitive(PrimitiveType::Cube, transform, "Cube");
}

uint32_t SpawnSphere(const SpawnTransform& transform) {
    return SpawnPrimitive(PrimitiveType::Sphere, transform, "Sphere");
}

std::vector<RenderEntityView> GetRenderEntities() {
    std::vector<RenderEntityView> entities;
    Ref<Scene> scene = EnsureScene();
    auto view = scene->GetAllEntitiesViewWith<TagComponent, MeshRendererComponent>();
    for (auto handle : view) {
        const auto& tag = view.get<TagComponent>(handle);
        const auto& meshRenderer = view.get<MeshRendererComponent>(handle);
        const auto& transform = Entity{handle, scene.get()}.GetComponent<TransformComponent>();

        RenderEntityView entity{};
        entity.id = static_cast<uint32_t>(handle);
        entity.name = tag.tag;
        entity.primitiveType = meshRenderer.primitiveType;
        entity.materialId = meshRenderer.materialId;
        entity.transform.position = transform.position;
        entity.transform.rotation = transform.rotation;
        entity.transform.scale = transform.scale;
        entities.push_back(std::move(entity));
    }

    return entities;
}

bool SetEntityTransform(uint32_t entityId, const SpawnTransform& transform) {
    Ref<Scene> scene = EnsureScene();
    auto view = scene->GetAllEntitiesViewWith<TransformComponent>();
    for (auto handle : view) {
        if (static_cast<uint32_t>(handle) != entityId) {
            continue;
        }

        auto& component = view.get<TransformComponent>(handle);
        component.position = transform.position;
        component.rotation = transform.rotation;
        component.scale = transform.scale;
        return true;
    }

    return false;
}

bool SetEntityMaterial(uint32_t entityId, uint32_t materialId) {
    Ref<Scene> scene = EnsureScene();
    auto view = scene->GetAllEntitiesViewWith<MeshRendererComponent>();
    for (auto handle : view) {
        if (static_cast<uint32_t>(handle) != entityId) {
            continue;
        }

        view.get<MeshRendererComponent>(handle).materialId = materialId;
        return true;
    }

    return false;
}

uint32_t CreateMaterial(const std::string& name) {
    MaterialRecord material{};
    material.id = s_NextMaterialId++;
    material.name = name.empty() ? "Material " + std::to_string(material.id) : name;
    s_Materials.push_back(material);
    return material.id;
}

std::vector<MaterialView> GetMaterials() {
    std::vector<MaterialView> materials;
    materials.reserve(s_Materials.size());
    for (const auto& material : s_Materials) {
        MaterialView view{};
        view.id = material.id;
        view.name = material.name;
        view.textures = material.textures;
        materials.push_back(std::move(view));
    }
    return materials;
}

bool SetMaterialTexturePath(uint32_t materialId, TextureSlot slot, const std::string& path) {
    for (auto& material : s_Materials) {
        if (material.id != materialId) {
            continue;
        }

        switch (slot) {
        case TextureSlot::Albedo:
            material.textures.albedoPath = path;
            return true;
        case TextureSlot::Normal:
            material.textures.normalPath = path;
            return true;
        case TextureSlot::Height:
            material.textures.heightPath = path;
            return true;
        case TextureSlot::Roughness:
            material.textures.roughnessPath = path;
            return true;
        case TextureSlot::AmbientOcclusion:
            material.textures.ambientOcclusionPath = path;
            return true;
        default:
            return false;
        }
    }

    return false;
}

MaterialTextures ResolveMaterialTextures(uint32_t materialId, const MaterialTextures& fallback) {
    for (const auto& material : s_Materials) {
        if (material.id == materialId) {
            return material.textures;
        }
    }

    return fallback;
}

LightingSettings GetLightingSettings() {
    LightingSettings settings{};
    settings.specularStrength = s_SpecularStrength;
    settings.specularShininessMin = s_SpecularShininessMin;
    settings.specularShininessMax = s_SpecularShininessMax;

    Ref<Scene> scene = EnsureScene();

    Entity directional = FindDirectionalLightEntity(*scene);
    if (directional) {
        const auto& directionalLight = directional.GetComponent<DirectionalLightComponent>();
        settings.directionalEnabled = true;
        settings.directionalDirection = directionalLight.direction;
        settings.directionalColor = directionalLight.color;
        settings.directionalIntensity = directionalLight.intensity;
    } else {
        settings.directionalDirection = kDefaultDirectionalDirection;
        settings.directionalColor = kDefaultDirectionalColor;
        settings.directionalIntensity = kDefaultDirectionalIntensity;
    }

    std::vector<Entity> pointLights = CollectPointLights(*scene);
    settings.pointLightCount = std::min<uint32_t>(static_cast<uint32_t>(pointLights.size()), kMaxPointLights);
    for (uint32_t i = 0; i < settings.pointLightCount; ++i) {
        const auto& light = pointLights[i].GetComponent<PointLightComponent>();
        const auto& transform = pointLights[i].GetComponent<TransformComponent>();
        settings.pointLights[i].position = transform.position;
        settings.pointLights[i].radius = light.radius;
        settings.pointLights[i].color = light.color;
        settings.pointLights[i].intensity = light.intensity;
    }

    return settings;
}

void SetLightingSettings(const LightingSettings& settings) {
    Ref<Scene> scene = EnsureScene();

    s_SpecularStrength = std::max(0.0f, settings.specularStrength);
    s_SpecularShininessMin = std::max(1.0f, settings.specularShininessMin);
    s_SpecularShininessMax = std::max(s_SpecularShininessMin, settings.specularShininessMax);

    Entity directional = FindDirectionalLightEntity(*scene);
    if (settings.directionalEnabled) {
        if (!directional) {
            directional = scene->CreateDirectionalLight();
        }
        auto& light = directional.GetComponent<DirectionalLightComponent>();
        light.direction = settings.directionalDirection;
        light.color = settings.directionalColor;
        light.intensity = std::max(0.0f, settings.directionalIntensity);
    } else if (directional) {
        scene->DestroyEntity(directional);
    }

    std::vector<Entity> pointLights = CollectPointLights(*scene);
    const uint32_t targetCount = std::min<uint32_t>(settings.pointLightCount, kMaxPointLights);

    while (pointLights.size() < targetCount) {
        pointLights.emplace_back(scene->CreatePointLight());
    }
    while (pointLights.size() > targetCount) {
        scene->DestroyEntity(pointLights.back());
        pointLights.pop_back();
    }

    for (uint32_t i = 0; i < targetCount; ++i) {
        auto& light = pointLights[i].GetComponent<PointLightComponent>();
        auto& transform = pointLights[i].GetComponent<TransformComponent>();
        transform.position = settings.pointLights[i].position;
        light.radius = std::max(0.01f, settings.pointLights[i].radius);
        light.color = settings.pointLights[i].color;
        light.intensity = std::max(0.0f, settings.pointLights[i].intensity);
    }
}

} // namespace World

} // namespace Piece
