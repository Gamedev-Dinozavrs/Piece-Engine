#pragma once

#include <core/Core.h>
#include <scene/RenderObject.h>

#include <array>
#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace Piece {

class Scene;

enum class TextureSlot {
    Albedo = 0,
    Normal,
    Height,
    Roughness,
    AmbientOcclusion
};

struct QuadMaterialView {
    uint32_t id{0};
    std::string albedoPath;
    std::string normalPath;
    std::string heightPath;
    std::string roughnessPath;
    std::string ambientOcclusionPath;
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

struct SpawnTransform {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 rotation{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
};

struct MaterialView {
    uint32_t id{0};
    std::string name;
    MaterialTextures textures{};
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

std::vector<RenderEntityView> GetRenderEntities();
bool SetEntityTransform(uint32_t entityId, const SpawnTransform& transform);
bool SetEntityMaterial(uint32_t entityId, uint32_t materialId);

uint32_t CreateMaterial(const std::string& name = "Material");
std::vector<MaterialView> GetMaterials();
bool SetMaterialTexturePath(uint32_t materialId, TextureSlot slot, const std::string& path);
MaterialTextures ResolveMaterialTextures(uint32_t materialId, const MaterialTextures& fallback = {});

LightingSettings GetLightingSettings();
void SetLightingSettings(const LightingSettings& settings);

} // namespace World

} // namespace Piece
