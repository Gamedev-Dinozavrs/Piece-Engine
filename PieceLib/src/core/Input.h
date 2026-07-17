#pragma once

#include <core/KeyCodes.h>
#include <core/MouseButtonCodes.h>

namespace Piece {

class Input {
public:
    static void SetWindow(void* window);

    static bool IsKeyPressed(int keycode);
    static bool IsKeyPressed(KeyCode keycode);
    static bool IsMouseButtonPressed(int button);
    static bool IsMouseButtonPressed(MouseButton button);
    static void GetMousePosition(double& x, double& y);
    static void SetMouseCaptured(bool captured);

private:
    static void* s_Window;
};

} // namespace Piece