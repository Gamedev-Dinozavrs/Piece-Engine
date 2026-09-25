#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdint>

namespace Piece {

enum class ProjectionType : uint8_t {
    Perspective = 0,
    Orthographic = 1
};

class Camera {
public:
    Camera() = default;

    void setPerspective(float fovY, float aspect, float zNear, float zFar) {
        m_ProjectionType = ProjectionType::Perspective;
        m_FovY = fovY;
        m_Aspect = aspect;
        m_NearClip = zNear;
        m_FarClip = zFar;
        RecalculateProjection();
    }

    void setOrthographic(float size, float aspect, float zNear, float zFar) {
        m_ProjectionType = ProjectionType::Orthographic;
        m_OrthoSize = size;
        m_Aspect = aspect;
        m_NearClip = zNear;
        m_FarClip = zFar;
        RecalculateProjection();
    }

    // Re-applies the current projection type/params at a new aspect ratio (e.g. on viewport resize)
    // without resetting FOV/orthographic size back to defaults.
    void setAspectRatio(float aspect) {
        m_Aspect = aspect;
        RecalculateProjection();
    }

    void setPosition(const glm::vec3& position) { position_ = position; }
    void setRotation(const glm::vec3& rotation) { rotation_ = rotation; }

    ProjectionType getProjectionType() const { return m_ProjectionType; }
    float getFovY() const { return m_FovY; }
    float getOrthographicSize() const { return m_OrthoSize; }
    float getNearClip() const { return m_NearClip; }
    float getFarClip() const { return m_FarClip; }
    float getAspectRatio() const { return m_Aspect; }

    const glm::mat4& projection() const { return projection_; }
    const glm::mat4& view() const { return view_; }
    const glm::vec3& position() const { return position_; }

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
    void RecalculateProjection() {
        if (m_ProjectionType == ProjectionType::Perspective) {
            projection_ = glm::perspective(glm::radians(m_FovY), m_Aspect, m_NearClip, m_FarClip);
        } else {
            const float halfHeight = m_OrthoSize * 0.5f;
            const float halfWidth = halfHeight * m_Aspect;
            projection_ = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, m_NearClip, m_FarClip);
        }
        projection_[1][1] *= -1.0f;
    }

    glm::mat4 projection_{1.0f};
    glm::mat4 view_{1.0f};
    glm::vec3 position_{0.0f, 0.0f, 3.0f};
    glm::vec3 rotation_{0.0f, 0.0f, 0.0f};

    ProjectionType m_ProjectionType = ProjectionType::Perspective;
    float m_FovY = 70.0f;
    float m_OrthoSize = 10.0f;
    float m_Aspect = 1.0f;
    float m_NearClip = 0.1f;
    float m_FarClip = 100.0f;
};

} // namespace Piece

