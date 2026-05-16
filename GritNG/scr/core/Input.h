#pragma once
#include "Types.h"
#include "KeyCodes.h"
#include <bitset>

namespace grit {

    class Input {
    public:
        // --- Клавиатура ---
        bool isDown(Key k) const { return m_keysCur[(size_t)k]; }
        bool wasPressed(Key k) const { return  m_keysCur[(size_t)k] && !m_keysPrev[(size_t)k]; }
        bool wasReleased(Key k) const { return !m_keysCur[(size_t)k] && m_keysPrev[(size_t)k]; }

        // --- Мышь ---
        bool isMouseDown(MouseButton b) const { return m_mouseCur[(size_t)b]; }
        bool wasMousePressed(MouseButton b) const { return  m_mouseCur[(size_t)b] && !m_mousePrev[(size_t)b]; }
        bool wasMouseReleased(MouseButton b) const { return !m_mouseCur[(size_t)b] && m_mousePrev[(size_t)b]; }

        // Позиция курсора в клиентских координатах окна (пикселях)
        i32 mouseX() const { return m_mouseX; }
        i32 mouseY() const { return m_mouseY; }

        // Смещение за кадр
        i32 mouseDeltaX() const { return m_mouseDX; }
        i32 mouseDeltaY() const { return m_mouseDY; }

        // Колесо (накопленные «щелчки» за кадр, +вверх / -вниз)
        f32 wheelDelta() const { return m_wheelDelta; }

        // --- Управление режимом ввода ---
        // Захват курсора (для FPS-камеры): мышь возвращается в центр окна каждый кадр,
        // курсор скрывается.
        void setMouseCaptured(bool capture) { m_mouseCaptureRequested = capture; }
        bool isMouseCaptured() const { return m_mouseCaptured; }

        // --- Вызываются Engine'ом ---
        void beginFrame();   // snapshot: prev = cur, обнуляем дельты
        void endFrame();     // ничего, на будущее

        // --- Колбэки от Window (вызываются из WndProc) ---
        void _onKey(Key k, bool down);
        void _onMouse(MouseButton b, bool down);
        void _onMouseMove(i32 x, i32 y);
        void _onMouseWheel(f32 delta);
        void _onFocusLost();   // сбросить все нажатия

        // --- Внутреннее, для Window: запрошен ли захват, и подтверждение ---
        bool _captureRequested() const { return m_mouseCaptureRequested; }
        void _setCaptured(bool v) { m_mouseCaptured = v; }

    private:
        std::bitset<(size_t)Key::Count>          m_keysCur;
        std::bitset<(size_t)Key::Count>          m_keysPrev;
        std::bitset<(size_t)MouseButton::Count>  m_mouseCur;
        std::bitset<(size_t)MouseButton::Count>  m_mousePrev;

        i32  m_mouseX = 0, m_mouseY = 0;
        i32  m_mouseDX = 0, m_mouseDY = 0;
        i32  m_lastMouseX = 0, m_lastMouseY = 0;
        bool m_hasLastMouse = false;

        f32  m_wheelDelta = 0.f;

        bool m_mouseCaptureRequested = false;
        bool m_mouseCaptured = false;
    };

} // namespace grit