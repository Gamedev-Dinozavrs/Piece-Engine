#include <PiecePCH.h>
#include "Input.h"

#include <GLFW/glfw3.h>

namespace Piece {

void* Input::s_Window = nullptr;

void Input::SetWindow(void* window) {
    s_Window = window;
}

bool Input::IsKeyPressed(int keycode) {
    if (!s_Window) {
        return false;
    }

    return glfwGetKey(static_cast<GLFWwindow*>(s_Window), keycode) == GLFW_PRESS;
}

bool Input::IsMouseButtonPressed(int button) {
    if (!s_Window) {
        return false;
    }

    return glfwGetMouseButton(static_cast<GLFWwindow*>(s_Window), button) == GLFW_PRESS;
}

void Input::GetMousePosition(double& x, double& y) {
    x = 0.0;
    y = 0.0;

    if (!s_Window) {
        return;
    }

    glfwGetCursorPos(static_cast<GLFWwindow*>(s_Window), &x, &y);
}

void Input::SetMouseCaptured(bool captured) {
    if (!s_Window) {
        return;
    }

    glfwSetInputMode(static_cast<GLFWwindow*>(s_Window), GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

} // namespace Piece