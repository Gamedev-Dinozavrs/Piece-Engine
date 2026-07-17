#pragma once

#include <cstdint>

namespace Piece
{
    enum class MouseButton : uint8_t
    {
        Button1 = 0,
        Button2 = 1,
        Button3 = 2,
        Button4 = 3,
        Button5 = 4,
        Button6 = 5,
        Button7 = 6,
        Button8 = 7,

        Last   = Button8,

        Left   = Button1,
        Right  = Button2,
        Middle = Button3
    };

    inline MouseButton ToMouseButton(int button)
    {
        return static_cast<MouseButton>(button);
    }

    inline int ToInt(MouseButton button)
    {
        return static_cast<int>(button);
    }

    inline const char* ToString(MouseButton button)
    {
        switch (button)
        {
            case MouseButton::Left:   return "Left";
            case MouseButton::Right:  return "Right";
            case MouseButton::Middle: return "Middle";

            case MouseButton::Button4: return "Button4";
            case MouseButton::Button5: return "Button5";
            case MouseButton::Button6: return "Button6";
            case MouseButton::Button7: return "Button7";
            case MouseButton::Button8: return "Button8";

            default: return "Unknown";
        }
    }

} // namespace Piece