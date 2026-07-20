#pragma once

#include <core/UUID.h>
#include <scene/Camera.h>
#include <scene/RenderObject.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <cstdint>
#include <vector>
#include <string>

namespace Piece {

enum class NormalSource : uint8_t {
    Derivative = 0,
    Vertex
};

struct TagComponent {
    std::string tag{"Entity"};
    UUID id{};

    TagComponent() = default;
    TagComponent(const TagComponent&) = default;
    TagComponent(const std::string& tagValue) : tag(tagValue) {}
    TagComponent(const std::string& tagValue, UUID uuidValue) : tag(tagValue), id(uuidValue) {}
};

struct HierarchyComponent {
    UUID parent{0};
    std::vector<UUID> children;
};

enum class ImportGroupMode : uint8_t {
    PreserveGroups = 0,
    GroupByMaterial,
    MergeAll
};

struct ImportedModelComponent {
    std::string sourcePath;
    ImportGroupMode mode{ImportGroupMode::PreserveGroups};
    bool grouped{true};
};

struct TransformComponent {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 rotation{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};

    TransformComponent() = default;
    TransformComponent(const TransformComponent&) = default;
    TransformComponent(const glm::vec3& translation) : position(translation) {}
    TransformComponent(const glm::vec3& translation, const glm::vec3& scaling) : position(translation), scale(scaling) {}
    TransformComponent(const glm::vec3& translation, const glm::vec3& scaling, const glm::vec3& eulerRotation)
        : position(translation), rotation(eulerRotation), scale(scaling) {}

    glm::mat4 GetTransform() const {
        const glm::mat4 rotationMatrix = glm::toMat4(glm::quat(glm::radians(rotation)));
        return glm::translate(glm::mat4(1.0f), position) * rotationMatrix * glm::scale(glm::mat4(1.0f), scale);
    }
};

struct MeshRendererComponent {
    PrimitiveType primitiveType{PrimitiveType::Unknown};
    NormalSource normalSource{NormalSource::Derivative};
    Ref<Mesh> mesh{};
    uint32_t materialId{0};
    MaterialTextures materialTextures{};

    MeshRendererComponent() = default;
    MeshRendererComponent(const MeshRendererComponent&) = default;
    MeshRendererComponent(PrimitiveType primitive, NormalSource source = NormalSource::Derivative)
        : primitiveType(primitive), normalSource(source) {}
};

struct MaterialComponent {
    uint32_t materialId{0};

    MaterialComponent() = default;
    MaterialComponent(const MaterialComponent&) = default;
    explicit MaterialComponent(uint32_t id)
        : materialId(id) {}
};

struct DirectionalLightComponent {
    glm::vec3 direction{-0.4f, -1.0f, -0.2f};
    glm::vec3 color{1.0f, 1.0f, 1.0f};
    float intensity{1.2f};

    DirectionalLightComponent() = default;
    DirectionalLightComponent(const DirectionalLightComponent&) = default;
};

struct PointLightComponent {
    glm::vec3 color{1.0f, 1.0f, 1.0f};
    float intensity{1.0f};
    float radius{1.0f};

    PointLightComponent() = default;
    PointLightComponent(const PointLightComponent&) = default;
};

struct SpotLightComponent {
    glm::vec3 direction{0.0f, -1.0f, 0.0f};
    glm::vec3 color{1.0f, 1.0f, 1.0f};
    float intensity{1.0f};
    float innerCutoffDegrees{15.0f};
    float outerCutoffDegrees{20.0f};

    SpotLightComponent() = default;
    SpotLightComponent(const SpotLightComponent&) = default;
};

struct CameraComponent {
    Camera camera{};
    bool primary{true};
    bool fixedAspectRatio{false};

    CameraComponent() = default;
    CameraComponent(const CameraComponent&) = default;
};

} // namespace Piece