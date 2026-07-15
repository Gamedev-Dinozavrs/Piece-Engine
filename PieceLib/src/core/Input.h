#pragma once

namespace Piece {

class Input {
public:
    static void SetWindow(void* window);

    static bool IsKeyPressed(int keycode);
    static bool IsMouseButtonPressed(int button);
    static void GetMousePosition(double& x, double& y);
    static void SetMouseCaptured(bool captured);

private:
    static void* s_Window;
};

} // namespace Piece