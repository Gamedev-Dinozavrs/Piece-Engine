#pragma once

#include <scene/Camera.h>
#include <scene/Mesh.h>
#include <glm/glm.hpp>
#include <memory>

namespace Piece {

class RenderObject {
public:
    RenderObject(std::shared_ptr<Mesh> mesh, const glm::vec3& position = glm::vec3(0.0f), const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f))
        : mesh_(std::move(mesh)), position_(position), rotation_(rotation), scale_(scale) {}

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

    std::shared_ptr<Mesh> mesh() const { return mesh_; }

private:
    std::shared_ptr<Mesh> mesh_;
    glm::vec3 position_;
    glm::vec3 rotation_;
    glm::vec3 scale_;
};

} // namespace Piece
