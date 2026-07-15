#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Piece {

class Camera {
public:
    Camera() = default;

    void setPerspective(float fovY, float aspect, float zNear, float zFar) {
        projection_ = glm::perspective(glm::radians(fovY), aspect, zNear, zFar);
        projection_[1][1] *= -1.0f;
    }

    void setPosition(const glm::vec3& position) { position_ = position; }
    void setRotation(const glm::vec3& rotation) { rotation_ = rotation; }

    const glm::mat4& projection() const { return projection_; }
    const glm::mat4& view() const { return view_; }

    void updateView() {
        glm::mat4 rotationMat = glm::rotate(glm::mat4(1.0f), glm::radians(rotation_.y), glm::vec3(0.0f, 1.0f, 0.0f));
        rotationMat = glm::rotate(rotationMat, glm::radians(rotation_.x), glm::vec3(1.0f, 0.0f, 0.0f));
        rotationMat = glm::rotate(rotationMat, glm::radians(rotation_.z), glm::vec3(0.0f, 0.0f, 1.0f));
        glm::vec3 forward = glm::vec3(rotationMat * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
        glm::vec3 right = glm::vec3(rotationMat * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
        glm::vec3 up = glm::cross(right, forward);
        view_ = glm::lookAt(position_, position_ + forward, up);
    }

private:
    glm::mat4 projection_{1.0f};
    glm::mat4 view_{1.0f};
    glm::vec3 position_{0.0f, 0.0f, 3.0f};
    glm::vec3 rotation_{0.0f, 0.0f, 0.0f};
};

} // namespace Piece
