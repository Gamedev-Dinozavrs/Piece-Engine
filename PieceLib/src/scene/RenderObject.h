#pragma once

#include <scene/Camera.h>
#include <scene/Mesh.h>
#include <core/Core.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <string>

namespace Piece {

enum class PrimitiveType {
    Unknown = 0,
    Quad,
    Cube,
    Sphere
};

struct MaterialTextures {
    std::string albedoPath;
    std::string normalPath;
    std::string heightPath;
    std::string roughnessPath;
    std::string metallicPath;
    std::string ambientOcclusionPath;
    std::string emissivePath;
};

struct MaterialColors {
    glm::vec3 baseColor{1.0f, 1.0f, 1.0f};
    glm::vec3 emissiveColor{0.0f, 0.0f, 0.0f};
    bool emissiveEnabled{false};
    bool hdrBloomEnabled{false};
    bool emissiveBloomEnabled{true};
    float bloomThreshold{0.8f};
    float bloomIntensity{0.35f};
    float bloomRadius{2.0f};
};

class RenderObject {
public:
    RenderObject(Ref<Mesh> mesh, PrimitiveType primitiveType = PrimitiveType::Unknown, uint32_t objectId = 0,
        const glm::vec3& position = glm::vec3(0.0f), const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f))
        : mesh_(std::move(mesh)), primitiveType_(primitiveType), objectId_(objectId), position_(position), rotation_(rotation), scale_(scale) {}

    void setPosition(const glm::vec3& position) { position_ = position; }
    void setRotation(const glm::vec3& rotation) { rotation_ = rotation; }
    void setScale(const glm::vec3& scale) { scale_ = scale; }

    glm::mat4 modelMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position_);
        model = glm::rotate(model, glm::radians(rotation_.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation_.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation_.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale_);
        return model;
    }

    Ref<Mesh> mesh() const { return mesh_; }
    PrimitiveType primitiveType() const { return primitiveType_; }
    uint32_t objectId() const { return objectId_; }
    MaterialTextures& materialTextures() { return materialTextures_; }
    const MaterialTextures& materialTextures() const { return materialTextures_; }

private:
    Ref<Mesh> mesh_;
    PrimitiveType primitiveType_{PrimitiveType::Unknown};
    uint32_t objectId_{0};
    MaterialTextures materialTextures_{};
    glm::vec3 position_;
    glm::vec3 rotation_;
    glm::vec3 scale_;
};

} // namespace Piece
