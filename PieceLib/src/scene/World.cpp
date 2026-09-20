#include <PiecePCH.h>

#include <scene/World.h>

#include <scene/Components.h>
#include <scene/Entity.h>
#include <scene/Scene.h>
#include <renderer/Renderer.h>
#include <assets/MaterialAssetSerializer.h>

#include <cctype>
#include <algorithm>

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
    std::string assetPath;
    MaterialTextures textures{};
    MaterialSurfaceFactors surfaceFactors{};
    MaterialColors colors{};
    MaterialRenderSettings renderSettings{};
};

Ref<Scene> s_ActiveScene = nullptr;
float s_SpecularStrength = 1.0f;
float s_SpecularShininessMin = 8.0f;
float s_SpecularShininessMax = 128.0f;
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

std::string ToLowerMaterialName(const std::string& name) {
    std::string lower = name;
    for (char& character : lower) {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }
    return lower;
}

std::string MakeUniqueMaterialName(const std::string& requestedName, uint32_t ignoredMaterialId = 0) {
    const std::string baseName = requestedName.empty() ? "Material" : requestedName;
    auto nameExists = [ignoredMaterialId](const std::string& candidate) {
        const std::string lowerCandidate = ToLowerMaterialName(candidate);
        for (const auto& material : s_Materials) {
            if (material.id == ignoredMaterialId) {
                continue;
            }
            if (ToLowerMaterialName(material.name) == lowerCandidate) {
                return true;
            }
        }
        return false;
    };

    if (!nameExists(baseName)) {
        return baseName;
    }

    for (uint32_t suffix = 2; ; ++suffix) {
        const std::string candidate = baseName + " (" + std::to_string(suffix) + ")";
        if (!nameExists(candidate)) {
            return candidate;
        }
    }
}

Ref<Scene> EnsureScene() {
    if (!s_ActiveScene) {
        s_ActiveScene = CreateRef<Scene>();
        Entity light = s_ActiveScene->CreateEntity("Directional Light");
        light.AddComponent<DirectionalLightComponent>();
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
        auto& material = entity.AddComponent<MaterialComponent>(defaultMaterialId);
        material.colors = ResolveMaterialColors(defaultMaterialId);
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

Entity FindHierarchyRoot(Scene& scene, Entity entity) {
    while (entity && entity.HasComponent<HierarchyComponent>()) {
        const UUID parentUuid = entity.GetComponent<HierarchyComponent>().parent;
        if (static_cast<uint64_t>(parentUuid) == 0) {
            break;
        }
        Entity parent = FindEntityByUUID(scene, parentUuid);
        if (!parent) {
            break;
        }
        entity = parent;
    }
    return entity;
}

void ForEachAnimatorInHierarchy(Scene& scene, Entity entity, const std::function<void(AnimatorComponent&)>& callback) {
    if (!entity) {
        return;
    }
    if (entity.HasComponent<AnimatorComponent>()) {
        callback(entity.GetComponent<AnimatorComponent>());
    }
    if (!entity.HasComponent<HierarchyComponent>()) {
        return;
    }
    const auto children = entity.GetComponent<HierarchyComponent>().children;
    for (const UUID& childUuid : children) {
        ForEachAnimatorInHierarchy(scene, FindEntityByUUID(scene, childUuid), callback);
    }
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

Entity FindEnvironmentEntity(Scene& scene) {
    auto view = scene.GetAllEntitiesViewWith<EnvironmentComponent>();
    for (auto handle : view) {
        return Entity{handle, &scene};
    }
    return {};
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
    auto& material = entity.AddComponent<MaterialComponent>(defaultMaterialId);
    material.colors = ResolveMaterialColors(defaultMaterialId);

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

uint32_t CreateEnvironmentObject(const std::string& name) {
    Ref<Scene> scene = EnsureScene();
    Entity entity = FindEnvironmentEntity(*scene);
    if (entity) {
        return static_cast<uint32_t>(entity);
    }

    entity = scene->CreateEntity(name.empty() ? "Environment" : name);
    entity.AddComponent<EnvironmentComponent>();
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
            auto& material = entity.GetComponent<MaterialComponent>();
            material.materialId = materialId;
            material.colors = ResolveMaterialColors(materialId);
        } else {
            auto& material = entity.AddComponent<MaterialComponent>(materialId);
            material.colors = ResolveMaterialColors(materialId);
        }
        return true;
    }

    return false;
}

bool SetEntityImportedModelInfo(uint32_t entityId, const std::string& sourcePath, const std::string& meshName) {
    if (!s_ActiveScene) {
        return false;
    }

    Ref<Scene> scene = s_ActiveScene;
    auto view = scene->GetAllEntitiesViewWith<TagComponent>();
    for (auto handle : view) {
        if (static_cast<uint32_t>(handle) != entityId) {
            continue;
        }

        Entity entity{handle, scene.get()};
        if (entity.HasComponent<ImportedModelComponent>()) {
            auto& imported = entity.GetComponent<ImportedModelComponent>();
            imported.sourcePath = sourcePath;
            imported.meshName = meshName;
        } else {
            auto& imported = entity.AddComponent<ImportedModelComponent>();
            imported.sourcePath = sourcePath;
            imported.meshName = meshName;
        }
        return true;
    }

    return false;
}

bool SetEntityAnimationData(uint32_t entityId, const std::vector<ImportedJoint>& joints,
    const std::vector<ImportedAnimationClip>& clips) {
    if (!s_ActiveScene || joints.empty()) {
        return false;
    }

    Ref<Scene> scene = s_ActiveScene;
    auto view = scene->GetAllEntitiesViewWith<TagComponent>();
    for (auto handle : view) {
        if (static_cast<uint32_t>(handle) != entityId) {
            continue;
        }

        Entity entity{handle, scene.get()};
        auto& animator = entity.HasComponent<AnimatorComponent>()
            ? entity.GetComponent<AnimatorComponent>()
            : entity.AddComponent<AnimatorComponent>();
        animator.joints = joints;
        animator.clips = clips;
        animator.boneMatrices.assign(joints.size(), glm::mat4(1.0f));
        animator.currentClip = 0;
        animator.time = 0.0f;
        return true;
    }

    return false;
}

bool SetEntityAnimationClips(uint32_t entityId, const std::vector<ImportedAnimationClip>& clips) {
    if (!s_ActiveScene || clips.empty()) {
        return false;
    }

    auto view = s_ActiveScene->GetAllEntitiesViewWith<TagComponent>();
    for (auto handle : view) {
        if (static_cast<uint32_t>(handle) != entityId) {
            continue;
        }

        Entity root = FindHierarchyRoot(*s_ActiveScene, Entity{handle, s_ActiveScene.get()});
        bool applied = false;
        std::function<void(Entity)> applyToHierarchy = [&](Entity entity) {
            if (!entity) {
                return;
            }

            if (entity.HasComponent<AnimatorComponent>()) {
                auto& animator = entity.GetComponent<AnimatorComponent>();
                std::unordered_map<std::string, int32_t> targetJoints;
                for (size_t jointIndex = 0; jointIndex < animator.joints.size(); ++jointIndex) {
                    targetJoints[animator.joints[jointIndex].name] = static_cast<int32_t>(jointIndex);
                }

                // Append rather than replace: an entity may accumulate many clips (e.g. Mixamo
                // walk/run/idle) that feed into its own AnimatorController without clobbering
                // clips already loaded, and without touching any other entity's animator.
                for (ImportedAnimationClip clip : clips) {
                    for (auto& channel : clip.channels) {
                        const auto jointIt = targetJoints.find(channel.jointName);
                        if (jointIt != targetJoints.end()) {
                            channel.jointIndex = jointIt->second;
                        } else {
                            channel.jointIndex = -1;
                        }
                    }

                    std::string baseName = clip.name.empty() ? "Animation" : clip.name;
                    std::string uniqueName = baseName;
                    for (uint32_t suffix = 2; std::any_of(animator.clips.begin(), animator.clips.end(),
                             [&](const ImportedAnimationClip& existing) { return existing.name == uniqueName; });
                         ++suffix) {
                        uniqueName = baseName + " (" + std::to_string(suffix) + ")";
                    }
                    clip.name = uniqueName;
                    animator.clips.push_back(std::move(clip));
                }

                if (animator.controller.states.empty()) {
                    animator.currentClip = static_cast<uint32_t>(animator.clips.size() - 1);
                    animator.time = 0.0f;
                    animator.playing = true;
                }
                applied = true;
            }

            if (!entity.HasComponent<HierarchyComponent>()) {
                return;
            }
            const auto children = entity.GetComponent<HierarchyComponent>().children;
            for (const UUID& childUuid : children) {
                applyToHierarchy(FindEntityByUUID(*s_ActiveScene, childUuid));
            }
        };

        applyToHierarchy(root);
        return applied;
    }

    return false;
}

bool SetEntityAnimationPreviewClip(uint32_t entityId, const std::string& clipName) {
    if (!s_ActiveScene || clipName.empty()) {
        return false;
    }

    auto view = s_ActiveScene->GetAllEntitiesViewWith<TagComponent>();
    for (auto handle : view) {
        if (static_cast<uint32_t>(handle) != entityId) {
            continue;
        }

        bool selected = false;
        Entity root = FindHierarchyRoot(*s_ActiveScene, Entity{handle, s_ActiveScene.get()});
        ForEachAnimatorInHierarchy(*s_ActiveScene, root, [&](AnimatorComponent& animator) {
            const auto clipIt = std::find_if(animator.clips.begin(), animator.clips.end(), [&](const ImportedAnimationClip& clip) {
                return clip.name == clipName;
            });
            if (clipIt == animator.clips.end()) {
                return;
            }
            animator.currentClip = static_cast<uint32_t>(clipIt - animator.clips.begin());
            animator.time = 0.0f;
            animator.stateTime = 0.0f;
            animator.previousState = -1;
            selected = true;
        });
        return selected;
    }
    return false;
}

bool SetEntityAnimationPlayback(uint32_t entityId, bool playing, float speed) {
    if (!s_ActiveScene) {
        return false;
    }

    auto view = s_ActiveScene->GetAllEntitiesViewWith<TagComponent>();
    for (auto handle : view) {
        if (static_cast<uint32_t>(handle) != entityId) {
            continue;
        }

        bool updated = false;
        Entity root = FindHierarchyRoot(*s_ActiveScene, Entity{handle, s_ActiveScene.get()});
        ForEachAnimatorInHierarchy(*s_ActiveScene, root, [&](AnimatorComponent& animator) {
            animator.playing = playing;
            animator.speed = speed;
            updated = true;
        });
        return updated;
    }
    return false;
}

bool SetEntityAnimatorController(uint32_t entityId, const AnimatorController& controller) {
    if (!s_ActiveScene) {
        return false;
    }

    auto view = s_ActiveScene->GetAllEntitiesViewWith<TagComponent>();
    for (auto handle : view) {
        if (static_cast<uint32_t>(handle) != entityId) {
            continue;
        }

        bool updated = false;
        Entity root = FindHierarchyRoot(*s_ActiveScene, Entity{handle, s_ActiveScene.get()});
        ForEachAnimatorInHierarchy(*s_ActiveScene, root, [&](AnimatorComponent& animator) {
            if (&animator.controller != &controller) {
                animator.controller = controller;
            }
            if (animator.currentState >= static_cast<int32_t>(animator.controller.states.size())) {
                animator.currentState = -1;
                animator.stateTime = 0.0f;
            }
            updated = true;
        });
        return updated;
    }
    return false;
}

bool AddEntityAnimationClipSlot(uint32_t entityId) {
    if (!s_ActiveScene) {
        return false;
    }

    auto view = s_ActiveScene->GetAllEntitiesViewWith<TagComponent>();
    for (auto handle : view) {
        if (static_cast<uint32_t>(handle) != entityId) {
            continue;
        }

        bool added = false;
        Entity root = FindHierarchyRoot(*s_ActiveScene, Entity{handle, s_ActiveScene.get()});
        ForEachAnimatorInHierarchy(*s_ActiveScene, root, [&](AnimatorComponent& animator) {
            ImportedAnimationClip slot{};
            slot.name = "Empty Clip " + std::to_string(animator.clips.size() + 1);
            animator.clips.push_back(std::move(slot));
            added = true;
        });
        return added;
    }
    return false;
}

bool SetEntityAnimationClipSlot(uint32_t entityId, size_t slotIndex, const ImportedAnimationClip& sourceClip) {
    if (!s_ActiveScene) {
        return false;
    }

    auto view = s_ActiveScene->GetAllEntitiesViewWith<TagComponent>();
    for (auto handle : view) {
        if (static_cast<uint32_t>(handle) != entityId) {
            continue;
        }

        bool replaced = false;
        Entity root = FindHierarchyRoot(*s_ActiveScene, Entity{handle, s_ActiveScene.get()});
        ForEachAnimatorInHierarchy(*s_ActiveScene, root, [&](AnimatorComponent& animator) {
            if (slotIndex >= animator.clips.size()) {
                return;
            }
            ImportedAnimationClip clip = sourceClip;
            std::unordered_map<std::string, int32_t> targetJoints;
            for (size_t jointIndex = 0; jointIndex < animator.joints.size(); ++jointIndex) {
                targetJoints[animator.joints[jointIndex].name] = static_cast<int32_t>(jointIndex);
            }
            for (ImportedAnimationChannel& channel : clip.channels) {
                const auto jointIt = targetJoints.find(channel.jointName);
                channel.jointIndex = jointIt == targetJoints.end() ? -1 : jointIt->second;
            }
            const std::string previousName = animator.clips[slotIndex].name;
            animator.clips[slotIndex] = std::move(clip);
            for (AnimationState& state : animator.controller.states) {
                if (state.clipName == previousName) {
                    state.clipName = animator.clips[slotIndex].name;
                }
            }
            replaced = true;
        });
        return replaced;
    }
    return false;
}

bool RenameEntityAnimationClip(uint32_t entityId, const std::string& oldName, const std::string& newName) {
    if (!s_ActiveScene || oldName.empty() || newName.empty()) {
        return false;
    }

    auto view = s_ActiveScene->GetAllEntitiesViewWith<TagComponent>();
    for (auto handle : view) {
        if (static_cast<uint32_t>(handle) != entityId) {
            continue;
        }

        bool renamed = false;
        Entity root = FindHierarchyRoot(*s_ActiveScene, Entity{handle, s_ActiveScene.get()});
        ForEachAnimatorInHierarchy(*s_ActiveScene, root, [&](AnimatorComponent& animator) {
            for (ImportedAnimationClip& clip : animator.clips) {
                if (clip.name == oldName) {
                    clip.name = newName;
                    renamed = true;
                }
            }
            for (AnimationState& state : animator.controller.states) {
                if (state.clipName == oldName) {
                    state.clipName = newName;
                }
            }
        });
        return renamed;
    }
    return false;
}

bool RemoveEntityAnimationClip(uint32_t entityId, const std::string& clipName) {
    if (!s_ActiveScene || clipName.empty()) {
        return false;
    }

    auto view = s_ActiveScene->GetAllEntitiesViewWith<TagComponent>();
    for (auto handle : view) {
        if (static_cast<uint32_t>(handle) != entityId) {
            continue;
        }

        bool removed = false;
        Entity root = FindHierarchyRoot(*s_ActiveScene, Entity{handle, s_ActiveScene.get()});
        ForEachAnimatorInHierarchy(*s_ActiveScene, root, [&](AnimatorComponent& animator) {
            for (size_t clipIndex = 0; clipIndex < animator.clips.size();) {
                if (animator.clips[clipIndex].name != clipName) {
                    ++clipIndex;
                    continue;
                }
                animator.clips.erase(animator.clips.begin() + static_cast<std::ptrdiff_t>(clipIndex));
                if (animator.currentClip > clipIndex) {
                    --animator.currentClip;
                }
                removed = true;
            }
            if (animator.clips.empty()) {
                animator.currentClip = 0;
                animator.time = 0.0f;
            } else {
                animator.currentClip = std::min(animator.currentClip, static_cast<uint32_t>(animator.clips.size() - 1));
            }
            for (AnimationState& state : animator.controller.states) {
                if (state.clipName == clipName) {
                    state.clipName.clear();
                }
            }
        });
        return removed;
    }
    return false;
}

uint32_t CreateMaterial(const std::string& name) {
    MaterialRecord material{};
    material.id = s_NextMaterialId++;
    material.name = MakeUniqueMaterialName(name);
    s_Materials.push_back(material);
    return material.id;
}

uint32_t GetDefaultMaterialId() {
    return EnsureDefaultMaterialId();
}

void SetDefaultMaterialId(uint32_t materialId) {
    s_DefaultMaterialId = materialId;
}

std::vector<MaterialView> GetMaterials() {
    std::vector<MaterialView> materials;
    materials.reserve(s_Materials.size());
    for (const auto& material : s_Materials) {
        MaterialView view{};
        view.id = material.id;
        view.name = material.name;
        view.assetPath = material.assetPath;
        view.textures = material.textures;
        view.surfaceFactors = material.surfaceFactors;
        view.colors = material.colors;
        view.renderSettings = material.renderSettings;
        materials.push_back(std::move(view));
    }
    return materials;
}

bool SaveMaterialAsset(uint32_t materialId) {
    for (const auto& material : s_Materials) {
        if (material.id == materialId && !material.assetPath.empty()) {
            MaterialView view{};
            view.id = material.id;
            view.name = material.name;
            view.assetPath = material.assetPath;
            view.textures = material.textures;
            view.surfaceFactors = material.surfaceFactors;
            view.colors = material.colors;
            view.renderSettings = material.renderSettings;
            return MaterialAssetSerializer::Serialize(view, material.assetPath);
        }
    }

    return false;
}

void ClearMaterials() {
    s_Materials.clear();
    s_NextMaterialId = 1;
    s_DefaultMaterialId = 0;
}

uint32_t RestoreMaterial(uint32_t id, const std::string& name, const MaterialTextures& textures,
    const MaterialSurfaceFactors& surfaceFactors, const MaterialColors& colors) {
    MaterialRecord material{};
    material.id = id;
    material.name = name;
    material.textures = textures;
    material.surfaceFactors = surfaceFactors;
    material.colors = colors;
    s_Materials.push_back(material);
    s_NextMaterialId = std::max(s_NextMaterialId, id + 1);
    return id;
}

bool SetMaterialName(uint32_t materialId, const std::string& name) {
    if (name.empty()) {
        return false;
    }

    for (auto& material : s_Materials) {
        if (material.id != materialId) {
            continue;
        }

        material.name = MakeUniqueMaterialName(name, materialId);
        SaveMaterialAsset(materialId);
        return true;
    }

    return false;
}

bool SetMaterialAssetPath(uint32_t materialId, const std::string& path) {
    for (auto& material : s_Materials) {
        if (material.id == materialId) {
            material.assetPath = path;
            return true;
        }
    }

    return false;
}

bool SetMaterialColors(uint32_t materialId, const MaterialColors& colors) {
    for (auto& material : s_Materials) {
        if (material.id != materialId) {
            continue;
        }

        material.colors.baseColor = glm::clamp(colors.baseColor, glm::vec3(0.0f), glm::vec3(1.0f));
        material.colors.emissiveColor = glm::max(colors.emissiveColor, glm::vec3(0.0f));
        material.colors.emissiveEnabled = colors.emissiveEnabled;
        material.colors.hdrBloomEnabled = colors.hdrBloomEnabled;
        material.colors.emissiveBloomEnabled = colors.emissiveBloomEnabled;
        SaveMaterialAsset(materialId);
        return true;
    }

    return false;
}

bool SetMaterialTexturePath(uint32_t materialId, TextureSlot slot, const std::string& path) {
    for (auto& material : s_Materials) {
        if (material.id != materialId) {
            continue;
        }

        switch (slot) {
        case TextureSlot::Albedo:
            material.textures.albedoPath = path;
            SaveMaterialAsset(materialId);
            return true;
        case TextureSlot::Normal:
            material.textures.normalPath = path;
            SaveMaterialAsset(materialId);
            return true;
        case TextureSlot::Height:
            material.textures.heightPath = path;
            SaveMaterialAsset(materialId);
            return true;
        case TextureSlot::Roughness:
            material.textures.roughnessPath = path;
            SaveMaterialAsset(materialId);
            return true;
        case TextureSlot::Metallic:
            material.textures.metallicPath = path;
            SaveMaterialAsset(materialId);
            return true;
        case TextureSlot::AmbientOcclusion:
            material.textures.ambientOcclusionPath = path;
            SaveMaterialAsset(materialId);
            return true;
        case TextureSlot::Emissive:
            material.textures.emissivePath = path;
            SaveMaterialAsset(materialId);
            return true;
        default:
            return false;
        }
    }

    return false;
}

bool SetMaterialSurfaceFactors(uint32_t materialId, float roughnessFactor, float metallicFactor,
    float normalScale, float occlusionStrength) {
    for (auto& material : s_Materials) {
        if (material.id != materialId) {
            continue;
        }

        material.surfaceFactors.roughnessFactor = std::clamp(roughnessFactor, 0.0f, 1.0f);
        material.surfaceFactors.metallicFactor = std::clamp(metallicFactor, 0.0f, 1.0f);
        material.surfaceFactors.normalScale = std::max(0.0f, normalScale);
        material.surfaceFactors.occlusionStrength = std::clamp(occlusionStrength, 0.0f, 1.0f);
        SaveMaterialAsset(materialId);
        return true;
    }

    return false;
}

bool SetMaterialRenderSettings(uint32_t materialId, const MaterialRenderSettings& settings) {
    for (auto& material : s_Materials) {
        if (material.id != materialId) {
            continue;
        }

        material.renderSettings.alphaMode = settings.alphaMode;
        material.renderSettings.alphaCutoff = std::clamp(settings.alphaCutoff, 0.0f, 1.0f);
        material.renderSettings.doubleSided = settings.doubleSided;
        material.renderSettings.unlit = settings.unlit;
        SaveMaterialAsset(materialId);
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

MaterialColors ResolveMaterialColors(uint32_t materialId, const MaterialColors& fallback) {
    for (const auto& material : s_Materials) {
        if (material.id == materialId) {
            return material.colors;
        }
    }

    return fallback;
}

MaterialRenderSettings ResolveMaterialRenderSettings(uint32_t materialId, const MaterialRenderSettings& fallback) {
    for (const auto& material : s_Materials) {
        if (material.id == materialId) {
            return material.renderSettings;
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
    EnvironmentSettings settings{};
    if (!s_ActiveScene) {
        return settings;
    }

    Entity entity = FindEnvironmentEntity(*s_ActiveScene);
    if (!entity) {
        return settings;
    }

    const auto& environment = entity.GetComponent<EnvironmentComponent>();
    settings.enabled = environment.enabled;
    settings.hdrPath = environment.hdrPath;
    settings.intensity = environment.intensity;
    settings.diffuseStrength = environment.diffuseStrength;
    settings.specularStrength = environment.specularStrength;
    settings.ambientStrength = environment.ambientStrength;
    settings.aaTechnique = static_cast<AATechnique>(environment.aaTechnique);
    settings.msaaSampleCount = environment.msaaSampleCount;
    return settings;
}
void SetEnvironmentSettings(const EnvironmentSettings& settings) {
    Ref<Scene> scene = EnsureScene();
    Entity entity = FindEnvironmentEntity(*scene);
    if (!entity) {
        entity = scene->CreateEntity("Environment");
        entity.AddComponent<EnvironmentComponent>();
    }

    auto& environment = entity.GetComponent<EnvironmentComponent>();
    environment.enabled = settings.enabled;
    environment.hdrPath = settings.hdrPath;
    environment.intensity = std::max(0.0f, settings.intensity);
    environment.diffuseStrength = std::max(0.0f, settings.diffuseStrength);
    environment.specularStrength = std::max(0.0f, settings.specularStrength);
    environment.ambientStrength = std::max(0.0f, settings.ambientStrength);
    environment.aaTechnique = static_cast<uint32_t>(settings.aaTechnique);
    environment.msaaSampleCount = settings.msaaSampleCount;
}

SpecularSettings GetSpecularSettings() {
    SpecularSettings settings{};
    settings.strength = s_SpecularStrength;
    settings.shininessMin = s_SpecularShininessMin;
    settings.shininessMax = s_SpecularShininessMax;
    return settings;
}

void SetSpecularSettings(const SpecularSettings& settings) {
    s_SpecularStrength = std::max(0.0f, settings.strength);
    s_SpecularShininessMin = std::max(1.0f, settings.shininessMin);
    s_SpecularShininessMax = std::max(s_SpecularShininessMin, settings.shininessMax);
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
}

} // namespace World

} // namespace Piece
