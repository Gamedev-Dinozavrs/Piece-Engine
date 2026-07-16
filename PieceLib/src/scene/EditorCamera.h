#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Piece {

class EditorCamera {
public:
    EditorCamera(float fovY, float aspectRatio, float nearClip, float farClip);

    void onUpdate(float deltaTime);
    void onMouseScroll(float offsetY);
    void setPerspective(float fovY, float aspectRatio, float nearClip, float farClip);
    void setViewportSize(float width, float height);
    void setPosition(const glm::vec3& position);

    const glm::mat4& projection() const { return m_Projection; }
    const glm::mat4& view() const { return m_View; }
    const glm::vec3& position() const { return m_Position; }
    const glm::vec3& focalPoint() const { return m_FocalPoint; }

private:
    void updateView();
    void updateInputState();
    glm::quat getOrientation() const;
    glm::vec3 getForwardDirection() const;
    glm::vec3 getRightDirection() const;
    glm::vec3 getUpDirection() const;

    glm::mat4 m_Projection{1.0f};
    glm::mat4 m_View{1.0f};
    glm::vec3 m_Position{0.0f, 0.0f, 3.0f};

    float m_FovY = 70.0f;
    float m_AspectRatio = 1.0f;
    float m_NearClip = 0.1f;
    float m_FarClip = 100.0f;

    float m_Yaw = 0.0f;
    float m_Pitch = 0.0f;

    float m_MoveSpeed = 5.0f;
    float m_MouseSensitivity = 0.08f;
    float m_FastMultiplier = 3.0f;
    float m_PanSpeed = 0.0025f;
    float m_ZoomSpeed = 2.0f;

    bool m_LeftMouseHeld = false;
    bool m_RightMouseHeld = false;
    bool m_FirstMouse = true;
    double m_LastMouseX = 0.0;
    double m_LastMouseY = 0.0;

    glm::vec3 m_FocalPoint{0.0f, 0.0f, 0.0f};
    float m_Distance = 4.0f;
};

} // namespace Piece