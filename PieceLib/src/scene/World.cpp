#include <PiecePCH.h>

#include <scene/World.h>

#include <scene/Components.h>
#include <scene/Entity.h>
#include <scene/Scene.h>
#include <renderer/Renderer.h>

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
    MaterialSurfaceFactors surfaceFactors{};
};

Ref<Scene> s_ActiveScene = nullptr;
float s_SpecularStrength = 1.0f;
float s_SpecularShininessMin = 8.0f;
float s_SpecularShininessMax = 128.0f;
EnvironmentSettings s_EnvironmentSettings{};
uint32_t s_NextMaterialId = 1;
std::vector<MaterialRecord> s_Materials;
uint32_t s_DefaultMaterialId = 0;

uint32_t EnsureDefaultMaterialId() {
    if (s_DefaultMaterialId != 0) {
        for (const auto& material : s_Materials) {
            if (material.id == s_DefaultMaterialId) {
                return s_DefaultMaterialId;
            }
        }
    }

    MaterialRecord material{};
    material.id = s_NextMaterialId++;
    material.name = "Default Material";
    s_Materials.push_back(material);
    s_DefaultMaterialId = material.id;
    return s_DefaultMaterialId;
}

Ref<Scene> EnsureScene() {
    if (!s_ActiveScene) {
        s_ActiveScene = CreateRef<Scene>();
    }
    return s_ActiveScene;
}

void DestroyActiveSceneIfEmpty() {
    if (s_ActiveScene && s_ActiveScene->IsEmpty()) {
        s_ActiveScene.reset();
    }
}

Entity SpawnPrimitiveEntity(Scene& scene, PrimitiveType primitiveType, const std::string& name) {
    const char* defaultName = "Entity";
    switch (primitiveType) {
    case PrimitiveType::Quad:
        defaultName = "Quad";
        break;
    case PrimitiveType::Cube:
        defaultName = "Cube";
        break;
    case PrimitiveType::Sphere:
        defaultName = "Sphere";
        break;
    default:
        break;
    }

    Entity entity = scene.CreateEntity(name.empty() ? defaultName : name);
        const uint32_t defaultMaterialId = EnsureDefaultMaterialId();
        auto& meshRenderer = entity.AddComponent<MeshRendererComponent>(primitiveType, NormalSource::Vertex);
        meshRenderer.materialId = defaultMaterialId;
        entity.AddComponent<MaterialComponent>(defaultMaterialId);
    return entity;
}

Entity FindEntityByUUID(Scene& scene, UUID uuid) {
    auto view = scene.GetAllEntitiesViewWith<TagComponent>();
    for (auto handle : view) {
        if (scene.GetAllEntitiesViewWith<TagComponent>().get<TagComponent>(handle).id == uuid) {
            return Entity{handle, &scene};
        }
    }

    return {};
}

void AddChildLink(Scene& scene, const UUID& parentUuid, const UUID& childUuid) {
    Entity parent = FindEntityByUUID(scene, parentUuid);
    if (!parent || !parent.HasComponent<HierarchyComponent>()) {
        return;
    }

    auto& children = parent.GetComponent<HierarchyComponent>().children;
    if (std::find(children.begin(), children.end(), childUuid) == children.end()) {
        children.push_back(childUuid);
    }
}

void RemoveChildLink(Scene& scene, const UUID& parentUuid, const UUID& childUuid) {
    Entity parent = FindEntityByUUID(scene, parentUuid);
    if (!parent || !parent.HasComponent<HierarchyComponent>()) {
        return;
    }

    auto& children = parent.GetComponent<HierarchyComponent>().children;
    children.erase(std::remove(children.begin(), children.end(), childUuid), children.end());
}

Entity SpawnDirectionalLightEntity(Scene& scene) {
    auto existing = scene.GetAllEntitiesViewWith<DirectionalLightComponent>();
    uint32_t count = 0;
    for (auto handle : existing) {
        (void)handle;
        ++count;
    }
    Entity entity = scene.CreateEntity("Directional Light " + std::to_string(count));
    entity.AddComponent<DirectionalLightComponent>();
    return entity;
}

Entity SpawnPointLightEntity(Scene& scene) {
    auto existing = scene.GetAllEntitiesViewWith<PointLightComponent>();
    uint32_t count = 0;
    for (auto handle : existing) {
        (void)handle;
        ++count;
    }
    Entity entity = scene.CreateEntity("Point Light " + std::to_string(count));
    entity.AddComponent<PointLightComponent>();
    return entity;
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
    return s_ActiveScene;
}

void SetActiveScene(const Ref<Scene>& scene) {
    s_ActiveScene = scene;
}

uint32_t SpawnPrimitive(PrimitiveType primitiveType, const SpawnTransform& transform, const std::string& name) {
    Ref<Scene> scene = EnsureScene();
    if (primitiveType == PrimitiveType::Unknown) {
        return 0;
    }

    Entity entity = SpawnPrimitiveEntity(*scene, primitiveType, name);

    auto& transformComponent = entity.GetComponent<TransformComponent>();
    transformComponent.position = transform.position;
    transformComponent.rotation = transform.rotation;
    transformComponent.scale = transform.scale;
    return static_cast<uint32_t>(entity);
}

bool DestroyEntity(uint32_t entityId) {
    if (!s_ActiveScene) {
        return false;
    }

    auto view = s_ActiveScene->GetAllEntitiesViewWith<TagComponent>();
    for (auto handle : view) {
        if (static_cast<uint32_t>(handle) != entityId) {
            continue;
        }

        Renderer::WaitIdle();
        s_ActiveScene->DestroyEntity(Entity{handle, s_ActiveScene.get()});
        DestroyActiveSceneIfEmpty();
        return true;
    }

    return false;
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

uint32_t SpawnMesh(const Ref<Mesh>& mesh, const SpawnTransform& transform, const std::string& name, bool hasVertexNormals) {
    if (!mesh) {
        return 0;
    }

    Ref<Scene> scene = EnsureScene();
    Entity entity = scene->CreateEntity(name.empty() ? "Imported Mesh" : name);
    auto& renderer = entity.AddComponent<MeshRendererComponent>(
        PrimitiveType::Unknown,
        hasVertexNormals ? NormalSource::Vertex : NormalSource::Derivative);
    renderer.mesh = mesh;
    const uint32_t defaultMaterialId = EnsureDefaultMaterialId();
    renderer.materialId = defaultMaterialId;
    entity.AddComponent<MaterialComponent>(defaultMaterialId);

    auto& transformComponent = entity.GetComponent<TransformComponent>();
    transformComponent.position = transform.position;
    transformComponent.rotation = transform.rotation;
    transformComponent.scale = transform.scale;
    return static_cast<uint32_t>(entity);
}

uint32_t CreateEmptyObject(const std::string& name) {
    Ref<Scene> scene = EnsureScene();
    Entity entity = scene->CreateEntity(name.empty() ? "Empty Object" : name);
    return static_cast<uint32_t>(entity);
}

bool SetEntityParent(uint32_t childEntityId, uint32_t parentEntityId) {
    if (!s_ActiveScene) {
        return false;
    }

    Ref<Scene> scene = s_ActiveScene;
    Entity child;
    Entity parent;
    auto view = scene->GetAllEntitiesViewWith<TagComponent>();
    for (auto handle : view) {
        if (static_cast<uint32_t>(handle) == childEntityId) {
            child = Entity{handle, scene.get()};
        } else if (static_cast<uint32_t>(handle) == parentEntityId) {
            parent = Entity{handle, scene.get()};
        }
    }

    if (!child || !child.HasComponent<HierarchyComponent>()) {
        return false;
    }

    if (parentEntityId == childEntityId) {
        return false;
    }

    const bool hasParent = parentEntityId != 0;
    if (hasParent && (!parent || !parent.HasComponent<HierarchyComponent>())) {
        return false;
    }

    const UUID childUuid = child.GetComponent<TagComponent>().id;
    UUID parentUuid{0};
    if (hasParent) {
        parentUuid = parent.GetComponent<TagComponent>().id;

        // Prevent cycles: a node cannot be parented to any of its descendants.
        Entity cursor = parent;
        while (cursor && cursor.HasComponent<HierarchyComponent>()) {
            const UUID cursorParentUuid = cursor.GetComponent<HierarchyComponent>().parent;
            if (cursorParentUuid == childUuid) {
                return false;
            }
            if (static_cast<uint64_t>(cursorParentUuid) == 0) {
                break;
            }
            cursor = FindEntityByUUID(*scene, cursorParentUuid);
        }
    }

    if (child.GetComponent<HierarchyComponent>().parent == parentUuid) {
        return true;
    }

    const UUID oldParentUuid = child.GetComponent<HierarchyComponent>().parent;
    if (static_cast<uint64_t>(oldParentUuid) != 0) {
        RemoveChildLink(*scene, oldParentUuid, childUuid);
    }

    child.GetComponent<HierarchyComponent>().parent = parentUuid;
    if (hasParent) {
        AddChildLink(*scene, parentUuid, childUuid);
    }
    return true;
}

std::vector<RenderEntityView> GetRenderEntities() {
    std::vector<RenderEntityView> entities;
    if (!s_ActiveScene) {
        return entities;
    }

    Ref<Scene> scene = s_ActiveScene;
    auto view = scene->GetAllEntitiesViewWith<TagComponent, MeshRendererComponent>();
    for (auto handle : view) {
        const auto& tag = view.get<TagComponent>(handle);
        const auto& meshRenderer = view.get<MeshRendererComponent>(handle);
        Entity currentEntity{handle, scene.get()};
        const auto& transform = currentEntity.GetComponent<TransformComponent>();

        RenderEntityView entity{};
        entity.id = static_cast<uint32_t>(handle);
        entity.name = tag.tag;
        entity.primitiveType = meshRenderer.primitiveType;
        entity.materialId = meshRenderer.materialId;
        if (currentEntity.HasComponent<MaterialComponent>()) {
            entity.materialId = currentEntity.GetComponent<MaterialComponent>().materialId;
        }
        entity.transform.position = transform.position;
        entity.transform.rotation = transform.rotation;
        entity.transform.scale = transform.scale;
        entities.push_back(std::move(entity));
    }

    return entities;
}

bool SetEntityTransform(uint32_t entityId, const SpawnTransform& transform) {
    if (!s_ActiveScene) {
        return false;
    }

    Ref<Scene> scene = s_ActiveScene;
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
    if (!s_ActiveScene) {
        return false;
    }

    Ref<Scene> scene = s_ActiveScene;
    auto view = scene->GetAllEntitiesViewWith<MeshRendererComponent>();
    for (auto handle : view) {
        if (static_cast<uint32_t>(handle) != entityId) {
            continue;
        }

        auto& meshRenderer = view.get<MeshRendererComponent>(handle);
        meshRenderer.materialId = materialId;
        Entity entity{handle, scene.get()};
        if (entity.HasComponent<MaterialComponent>()) {
            entity.GetComponent<MaterialComponent>().materialId = materialId;
        } else {
            entity.AddComponent<MaterialComponent>(materialId);
        }
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
        view.surfaceFactors = material.surfaceFactors;
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
        case TextureSlot::Emissive:
            material.textures.emissivePath = path;
            return true;
        default:
            return false;
        }
    }

    return false;
}

bool SetMaterialSurfaceFactors(uint32_t materialId, float roughnessFactor, float metallicFactor) {
    for (auto& material : s_Materials) {
        if (material.id != materialId) {
            continue;
        }

        material.surfaceFactors.roughnessFactor = std::clamp(roughnessFactor, 0.0f, 1.0f);
        material.surfaceFactors.metallicFactor = std::clamp(metallicFactor, 0.0f, 1.0f);
        return true;
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

MaterialSurfaceFactors ResolveMaterialSurfaceFactors(uint32_t materialId, const MaterialSurfaceFactors& fallback) {
    for (const auto& material : s_Materials) {
        if (material.id == materialId) {
            return material.surfaceFactors;
        }
    }

    return fallback;
}

LightingSettings GetLightingSettings() {
    LightingSettings settings{};
    settings.specularStrength = s_SpecularStrength;
    settings.specularShininessMin = s_SpecularShininessMin;
    settings.specularShininessMax = s_SpecularShininessMax;

    if (!s_ActiveScene) {
        settings.directionalDirection = kDefaultDirectionalDirection;
        settings.directionalColor = kDefaultDirectionalColor;
        settings.directionalIntensity = kDefaultDirectionalIntensity;
        return settings;
    }

    Ref<Scene> scene = s_ActiveScene;

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
    s_SpecularStrength = std::max(0.0f, settings.specularStrength);
    s_SpecularShininessMin = std::max(1.0f, settings.specularShininessMin);
    s_SpecularShininessMax = std::max(s_SpecularShininessMin, settings.specularShininessMax);

    const uint32_t targetCount = std::min<uint32_t>(settings.pointLightCount, kMaxPointLights);
    const bool needsScene = settings.directionalEnabled || targetCount > 0;
    if (!needsScene && !s_ActiveScene) {
        return;
    }

    Ref<Scene> scene = EnsureScene();

    Entity directional = FindDirectionalLightEntity(*scene);
    if (settings.directionalEnabled) {
        if (!directional) {
            directional = SpawnDirectionalLightEntity(*scene);
        }
        auto& light = directional.GetComponent<DirectionalLightComponent>();
        light.direction = settings.directionalDirection;
        light.color = settings.directionalColor;
        light.intensity = std::max(0.0f, settings.directionalIntensity);
    } else if (directional) {
        Renderer::WaitIdle();
        scene->DestroyEntity(directional);
    }

    std::vector<Entity> pointLights = CollectPointLights(*scene);

    while (pointLights.size() < targetCount) {
        pointLights.emplace_back(SpawnPointLightEntity(*scene));
    }
    while (pointLights.size() > targetCount) {
        Renderer::WaitIdle();
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

    DestroyActiveSceneIfEmpty();
}

EnvironmentSettings GetEnvironmentSettings() {
    return s_EnvironmentSettings;
}

void SetEnvironmentSettings(const EnvironmentSettings& settings) {
    s_EnvironmentSettings.enabled = settings.enabled;
    s_EnvironmentSettings.diffuseMapPath = settings.diffuseMapPath;
    s_EnvironmentSettings.specularMapPath = settings.specularMapPath;
    s_EnvironmentSettings.intensity = std::max(0.0f, settings.intensity);
    s_EnvironmentSettings.diffuseStrength = std::max(0.0f, settings.diffuseStrength);
    s_EnvironmentSettings.specularStrength = std::max(0.0f, settings.specularStrength);
    s_EnvironmentSettings.aaTechnique = settings.aaTechnique;
    s_EnvironmentSettings.msaaSampleCount = settings.msaaSampleCount;
}

void ClearScene() {
    if (s_ActiveScene) {
        Renderer::WaitIdle();
        s_ActiveScene->Clear();
        s_ActiveScene.reset();
    }
    s_Materials.clear();
    s_NextMaterialId = 1;
    s_DefaultMaterialId = 0;
    s_EnvironmentSettings = EnvironmentSettings{};
}

} // namespace World

} // namespace Piece
