#include <PiecePCH.h>

#include "EditorCamera.h"

#include <core/Input.h>
#include <core/KeyCodes.h>
#include <core/MouseButtonCodes.h>

namespace Piece {

EditorCamera::EditorCamera(float fovY, float aspectRatio, float nearClip, float farClip)
    : m_FovY(fovY), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip) {
    setPerspective(fovY, aspectRatio, nearClip, farClip);
    updateView();
}

void EditorCamera::onUpdate(float deltaTime) {
    updateInputState();

    const float movementSpeed = m_MoveSpeed * (Input::IsKeyPressed(KeyCode::LeftShift) ? m_FastMultiplier : 1.0f);
    const float step = movementSpeed * deltaTime;

    if (Input::IsKeyPressed(KeyCode::W)) {
        m_FocalPoint += getForwardDirection() * step;
    }
    if (Input::IsKeyPressed(KeyCode::S)) {
        m_FocalPoint -= getForwardDirection() * step;
    }
    if (Input::IsKeyPressed(KeyCode::D)) {
        m_FocalPoint += getRightDirection() * step;
    }
    if (Input::IsKeyPressed(KeyCode::A)) {
        m_FocalPoint -= getRightDirection() * step;
    }
    if (Input::IsKeyPressed(KeyCode::E)) {
        m_FocalPoint += getUpDirection() * step;
    }
    if (Input::IsKeyPressed(KeyCode::Q)) {
        m_FocalPoint -= getUpDirection() * step;
    }

    double mouseX = 0.0;
    double mouseY = 0.0;
    Input::GetMousePosition(mouseX, mouseY);

    if (m_FirstMouse) {
        m_LastMouseX = mouseX;
        m_LastMouseY = mouseY;
        m_FirstMouse = false;
    }

    const double deltaX = mouseX - m_LastMouseX;
    const double deltaY = mouseY - m_LastMouseY;
    m_LastMouseX = mouseX;
    m_LastMouseY = mouseY;

    if (m_LeftMouseHeld) {
        m_Yaw += static_cast<float>(deltaX) * m_MouseSensitivity;
        m_Pitch -= static_cast<float>(deltaY) * m_MouseSensitivity;
        m_Pitch = std::clamp(m_Pitch, -89.0f, 89.0f);
    }

    if (m_RightMouseHeld) {
        const glm::vec3 right = getRightDirection();
        const glm::vec3 up = getUpDirection();
        const glm::vec3 panOffset = static_cast<float>(-deltaX) * m_PanSpeed * right + static_cast<float>(deltaY) * m_PanSpeed * up;
        m_FocalPoint += panOffset * m_Distance;
    }

    updateView();
}

void EditorCamera::onMouseScroll(float offsetY) {
    m_Distance -= offsetY * m_ZoomSpeed;
    m_Distance = std::clamp(m_Distance, 1.0f, 50.0f);
    updateView();
}

void EditorCamera::setPerspective(float fovY, float aspectRatio, float nearClip, float farClip) {
    m_FovY = fovY;
    m_AspectRatio = aspectRatio;
    m_NearClip = nearClip;
    m_FarClip = farClip;

    m_Projection = glm::perspective(glm::radians(m_FovY), m_AspectRatio, m_NearClip, m_FarClip);
    m_Projection[1][1] *= -1.0f;
}

void EditorCamera::setViewportSize(float width, float height) {
    if (width <= 0.0f || height <= 0.0f) {
        return;
    }

    setPerspective(m_FovY, width / height, m_NearClip, m_FarClip);
}

void EditorCamera::setPosition(const glm::vec3& position) {
    m_Position = position;
    m_FocalPoint = position + getForwardDirection() * m_Distance;
    updateView();
}

void EditorCamera::updateView() {
    const glm::quat orientation = getOrientation();
    m_Position = m_FocalPoint - getForwardDirection() * m_Distance;
    const glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation);
    m_View = glm::inverse(transform);
}

void EditorCamera::updateInputState() {
    const bool leftMouseHeld = Input::IsMouseButtonPressed(MouseButton::Left);
    const bool rightMouseHeld = Input::IsMouseButtonPressed(MouseButton::Right);

    if (leftMouseHeld != m_LeftMouseHeld || rightMouseHeld != m_RightMouseHeld) {
        m_LeftMouseHeld = leftMouseHeld;
        m_RightMouseHeld = rightMouseHeld;
        m_FirstMouse = true;
        Input::SetMouseCaptured(m_LeftMouseHeld || m_RightMouseHeld);
    }
}

glm::quat EditorCamera::getOrientation() const {
    return glm::quat(glm::vec3(glm::radians(-m_Pitch), glm::radians(-m_Yaw), 0.0f));
}

glm::vec3 EditorCamera::getForwardDirection() const {
    return glm::rotate(getOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
}

glm::vec3 EditorCamera::getRightDirection() const {
    return glm::rotate(getOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 EditorCamera::getUpDirection() const {
    return glm::rotate(getOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
}

} // namespace Piece