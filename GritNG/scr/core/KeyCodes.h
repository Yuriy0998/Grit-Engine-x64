#pragma once
#include "Types.h"

namespace grit {

    // Виртуальные коды клавиш (платформенно-независимые).
    // Маппинг для Win32 делается в Window.cpp.
    enum class Key : u16 {
        Unknown = 0,

        // Буквы
        A, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

        // Цифры (верхний ряд)
        Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

        // Функциональные
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

        // Управляющие
        Escape, Tab, CapsLock,
        Space, Backspace, Enter,
        Insert, Delete, Home, End, PageUp, PageDown,
        Left, Right, Up, Down,

        // Модификаторы
        LeftShift, RightShift,
        LeftCtrl, RightCtrl,
        LeftAlt, RightAlt,
        LeftSuper, RightSuper, // Win-клавиши

        // Numpad
        KP0, KP1, KP2, KP3, KP4, KP5, KP6, KP7, KP8, KP9,
        KPAdd, KPSubtract, KPMultiply, KPDivide, KPDecimal, KPEnter,

        // Знаки препинания
        Minus, Equals,
        LeftBracket, RightBracket,
        Backslash, Semicolon, Apostrophe,
        Comma, Period, Slash, GraveAccent,

        Count
    };

    enum class MouseButton : u8 {
        Left = 0,
        Right = 1,
        Middle = 2,
        X1 = 3,
        X2 = 4,
        Count
    };

} // namespace grit