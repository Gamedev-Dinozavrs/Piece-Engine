#pragma once

#include <core/Core.h>
#include <assets/AssetImporter.h>
#include <scene/AnimationGraph.h>
#include <scene/RenderObject.h>

#include <array>
#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace Piece {

class Scene;

enum class AATechnique {
    Off = 0,
    MSAA = 1
};

enum class TextureSlot {
    Albedo = 0,
    Normal,
    Height,
    Roughness,
    Metallic,
    AmbientOcclusion,
    Emissive
};

struct QuadMaterialView {
    uint32_t id{0};
    std::string albedoPath;
    std::string normalPath;
    std::string heightPath;
    std::string roughnessPath;
    std::string ambientOcclusionPath;
    std::string emissivePath;
};

struct PointLightSettings {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    float radius{1.0f};
    glm::vec3 color{1.0f, 1.0f, 1.0f};
    float intensity{1.0f};
};

struct LightingSettings {
    bool directionalEnabled{false};
    glm::vec3 directionalDirection{-0.4f, -1.0f, -0.2f};
    float directionalIntensity{1.2f};
    glm::vec3 directionalColor{1.0f, 0.98f, 0.9f};
    float specularStrength{1.0f};
    float specularShininessMin{8.0f};
    float specularShininessMax{128.0f};
    uint32_t pointLightCount{0};
    std::array<PointLightSettings, 4> pointLights{};
};

struct EnvironmentSettings {
    bool enabled{false};
    std::string hdrPath;
    float intensity{1.0f};
    float diffuseStrength{1.0f};
    float specularStrength{1.0f};
    float ambientStrength{0.08f};
    AATechnique aaTechnique{AATechnique::MSAA};
    uint32_t msaaSampleCount{4};
};

struct SpawnTransform {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 rotation{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
};

struct MaterialSurfaceFactors {
    float roughnessFactor{0.75f};
    float metallicFactor{0.0f};
    float normalScale{1.0f};
    float occlusionStrength{1.0f};
};

struct MaterialView {
    uint32_t id{0};
    std::string name;
    std::string assetPath;
    MaterialTextures textures{};
    MaterialSurfaceFactors surfaceFactors{};
    MaterialColors colors{};
    MaterialRenderSettings renderSettings{};
};

struct RenderEntityView {
    uint32_t id{0};
    std::string name;
    PrimitiveType primitiveType{PrimitiveType::Unknown};
    SpawnTransform transform{};
    uint32_t materialId{0};
};

namespace World {

Ref<Scene> GetActiveScene();
void SetActiveScene(const Ref<Scene>& scene);

uint32_t SpawnPrimitive(PrimitiveType primitiveType, const SpawnTransform& transform, const std::string& name = "");
uint32_t SpawnQuad(const SpawnTransform& transform);
uint32_t SpawnCube(const SpawnTransform& transform);
uint32_t SpawnSphere(const SpawnTransform& transform);
uint32_t SpawnMesh(const Ref<Mesh>& mesh, const SpawnTransform& transform, const std::string& name = "Imported Mesh", bool hasVertexNormals = true);
uint32_t CreateEmptyObject(const std::string& name = "Empty Object");
uint32_t CreateEnvironmentObject(const std::string& name = "Environment");
bool SetEntityParent(uint32_t childEntityId, uint32_t parentEntityId);
bool DestroyEntity(uint32_t entityId);

std::vector<RenderEntityView> GetRenderEntities();
bool SetEntityTransform(uint32_t entityId, const SpawnTransform& transform);
bool SetEntityMaterial(uint32_t entityId, uint32_t materialId);
bool SetEntityImportedModelInfo(uint32_t entityId, const std::string& sourcePath, const std::string& meshName);
bool SetEntityAnimationData(uint32_t entityId, const std::vector<ImportedJoint>& joints,
    const std::vector<ImportedAnimationClip>& clips);
bool SetEntityAnimationClips(uint32_t entityId, const std::vector<ImportedAnimationClip>& clips);
bool SetEntityAnimationPreviewClip(uint32_t entityId, const std::string& clipName);
bool SetEntityAnimationPlayback(uint32_t entityId, bool playing, float speed);
bool SetEntityAnimatorController(uint32_t entityId, const AnimatorController& controller);
bool AddEntityAnimationClipSlot(uint32_t entityId);
bool SetEntityAnimationClipSlot(uint32_t entityId, size_t slotIndex, const ImportedAnimationClip& clip);
bool RenameEntityAnimationClip(uint32_t entityId, const std::string& oldName, const std::string& newName);
bool RemoveEntityAnimationClip(uint32_t entityId, const std::string& clipName);

uint32_t CreateMaterial(const std::string& name = "Material");
uint32_t GetDefaultMaterialId();
void SetDefaultMaterialId(uint32_t materialId);
std::vector<MaterialView> GetMaterials();
// Removes every material from the registry without touching the scene. Used when loading a scene file.
void ClearMaterials();
// Re-inserts a material with an explicit id (preserving references from a loaded scene file).
uint32_t RestoreMaterial(uint32_t id, const std::string& name, const MaterialTextures& textures,
    const MaterialSurfaceFactors& surfaceFactors, const MaterialColors& colors);
bool SetMaterialName(uint32_t materialId, const std::string& name);
bool SetMaterialAssetPath(uint32_t materialId, const std::string& path);
bool SaveMaterialAsset(uint32_t materialId);
bool SetMaterialColors(uint32_t materialId, const MaterialColors& colors);
bool SetMaterialTexturePath(uint32_t materialId, TextureSlot slot, const std::string& path);
bool SetMaterialSurfaceFactors(uint32_t materialId, float roughnessFactor, float metallicFactor,
    float normalScale = 1.0f, float occlusionStrength = 1.0f);
bool SetMaterialRenderSettings(uint32_t materialId, const MaterialRenderSettings& settings);
MaterialTextures ResolveMaterialTextures(uint32_t materialId, const MaterialTextures& fallback = {});
MaterialSurfaceFactors ResolveMaterialSurfaceFactors(uint32_t materialId, const MaterialSurfaceFactors& fallback = {});
MaterialColors ResolveMaterialColors(uint32_t materialId, const MaterialColors& fallback = {});
MaterialRenderSettings ResolveMaterialRenderSettings(uint32_t materialId, const MaterialRenderSettings& fallback = {});

LightingSettings GetLightingSettings();
void SetLightingSettings(const LightingSettings& settings);
EnvironmentSettings GetEnvironmentSettings();
void SetEnvironmentSettings(const EnvironmentSettings& settings);

struct SpecularSettings {
    float strength{1.0f};
    float shininessMin{8.0f};
    float shininessMax{128.0f};
};

// Specular scalars only (no entity side effects), used when loading a scene file.
SpecularSettings GetSpecularSettings();
void SetSpecularSettings(const SpecularSettings& settings);

// Destroys all entities and resets materials and lighting to defaults.
void ClearScene();

} // namespace World

} // namespace Piece
