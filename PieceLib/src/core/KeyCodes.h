#pragma once

#include <cstdint>

namespace Piece
{
    enum class KeyState
    {
        Pressed,
        Released,
        Held
    };

    enum class KeyCode : uint16_t
    {
        // Printable keys
        Space              = 32,
        Apostrophe         = 39,  // '
        Comma              = 44,  // ,
        Minus              = 45,  // -
        Period             = 46,  // .
        Slash              = 47,  // /
        Num0               = 48,
        Num1               = 49,
        Num2               = 50,
        Num3               = 51,
        Num4               = 52,
        Num5               = 53,
        Num6               = 54,
        Num7               = 55,
        Num8               = 56,
        Num9               = 57,
        Semicolon          = 59,  // ;
        Equal              = 61,  // =

        A = 65, B, C, D, E, F, G, H, I, J,
        K, L, M, N, O, P, Q, R, S, T,
        U, V, W, X, Y, Z,

        LeftBracket        = 91,  // [
        Backslash          = 92,  /* backslash */
        RightBracket       = 93,  // ]
        GraveAccent        = 96,  // `
        World1             = 161,
        World2             = 162,

        // Function keys
        Escape             = 256,
        Enter              = 257,
        Tab                = 258,
        Backspace          = 259,
        Insert             = 260,
        Delete             = 261,
        Right              = 262,
        Left               = 263,
        Down               = 264,
        Up                 = 265,
        PageUp             = 266,
        PageDown           = 267,
        Home               = 268,
        End                = 269,

        CapsLock           = 280,
        ScrollLock         = 281,
        NumLock            = 282,
        PrintScreen        = 283,
        Pause              = 284,

        F1 = 290, F2, F3, F4, F5, F6, F7, F8,
        F9, F10, F11, F12, F13, F14, F15, F16,
        F17, F18, F19, F20, F21, F22, F23, F24, F25,

        // Keypad
        KP0                = 320,
        KP1                = 321,
        KP2                = 322,
        KP3                = 323,
        KP4                = 324,
        KP5                = 325,
        KP6                = 326,
        KP7                = 327,
        KP8                = 328,
        KP9                = 329,
        KPDecimal          = 330,
        KPDivide           = 331,
        KPMultiply         = 332,
        KPSubtract         = 333,
        KPAdd              = 334,
        KPEnter            = 335,
        KPEqual            = 336,

        // Modifiers
        LeftShift          = 340,
        LeftControl        = 341,
        LeftAlt            = 342,
        LeftSuper          = 343,
        RightShift         = 344,
        RightControl       = 345,
        RightAlt           = 346,
        RightSuper         = 347,
        Menu               = 348
    };

    inline KeyCode ToKeyCode(int key)
    {
        return static_cast<KeyCode>(key);
    }

    inline int ToInt(KeyCode key)
    {
        return static_cast<int>(key);
    }

    inline const char* ToString(KeyCode key)
    {
        switch (key)
        {
            case KeyCode::Space: return "Space";
            case KeyCode::Apostrophe: return "Apostrophe";
            case KeyCode::Comma: return "Comma";
            case KeyCode::Minus: return "Minus";
            case KeyCode::Period: return "Period";
            case KeyCode::Slash: return "Slash";

            case KeyCode::Num0: return "0";
            case KeyCode::Num1: return "1";
            case KeyCode::Num2: return "2";
            case KeyCode::Num3: return "3";
            case KeyCode::Num4: return "4";
            case KeyCode::Num5: return "5";
            case KeyCode::Num6: return "6";
            case KeyCode::Num7: return "7";
            case KeyCode::Num8: return "8";
            case KeyCode::Num9: return "9";

            case KeyCode::A: return "A";
            case KeyCode::B: return "B";
            case KeyCode::C: return "C";
            case KeyCode::D: return "D";
            case KeyCode::E: return "E";
            case KeyCode::F: return "F";
            case KeyCode::G: return "G";
            case KeyCode::H: return "H";
            case KeyCode::I: return "I";
            case KeyCode::J: return "J";
            case KeyCode::K: return "K";
            case KeyCode::L: return "L";
            case KeyCode::M: return "M";
            case KeyCode::N: return "N";
            case KeyCode::O: return "O";
            case KeyCode::P: return "P";
            case KeyCode::Q: return "Q";
            case KeyCode::R: return "R";
            case KeyCode::S: return "S";
            case KeyCode::T: return "T";
            case KeyCode::U: return "U";
            case KeyCode::V: return "V";
            case KeyCode::W: return "W";
            case KeyCode::X: return "X";
            case KeyCode::Y: return "Y";
            case KeyCode::Z: return "Z";

            case KeyCode::Escape: return "Escape";
            case KeyCode::Enter: return "Enter";
            case KeyCode::Tab: return "Tab";
            case KeyCode::Backspace: return "Backspace";

            case KeyCode::Left: return "Left";
            case KeyCode::Right: return "Right";
            case KeyCode::Up: return "Up";
            case KeyCode::Down: return "Down";

            case KeyCode::LeftShift: return "LeftShift";
            case KeyCode::RightShift: return "RightShift";
            case KeyCode::LeftControl: return "LeftControl";
            case KeyCode::RightControl: return "RightControl";
            case KeyCode::LeftAlt: return "LeftAlt";
            case KeyCode::RightAlt: return "RightAlt";

            case KeyCode::F1: return "F1";
            case KeyCode::F2: return "F2";
            case KeyCode::F3: return "F3";
            case KeyCode::F4: return "F4";
            case KeyCode::F5: return "F5";
            case KeyCode::F6: return "F6";
            case KeyCode::F7: return "F7";
            case KeyCode::F8: return "F8";
            case KeyCode::F9: return "F9";
            case KeyCode::F10: return "F10";
            case KeyCode::F11: return "F11";
            case KeyCode::F12: return "F12";

            default: return "Unknown";
        }
    }
}