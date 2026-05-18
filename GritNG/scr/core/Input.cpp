#include "Input.h"

namespace grit {

    void Input::beginFrame() {
        m_keysPrev = m_keysCur;
        m_mousePrev = m_mouseCur;
        m_mouseDX = 0;
        m_mouseDY = 0;
        m_wheelDelta = 0.f;
        m_hadRawThisFrame = false; // <-- новое
    }

    void Input::_onRawMouseDelta(i32 dx, i32 dy) {
        if (!m_hadRawThisFrame) {
            // Этот кадр будет считаться по raw-дельтам — обнуляем то,
            // что могло прийти от WM_MOUSEMOVE до первого WM_INPUT.
            m_mouseDX = 0;
            m_mouseDY = 0;
            m_hadRawThisFrame = true;
        }
        m_mouseDX += dx;
        m_mouseDY += dy;
    }

    void Input::_onMouseMove(i32 x, i32 y) {
        // Всегда обновляем абсолютную позицию (для UI/курсора).
        if (!m_hadRawThisFrame) {
            if (m_hasLastMouse) {
                m_mouseDX += (x - m_lastMouseX);
                m_mouseDY += (y - m_lastMouseY);
            }
        }
        // (raw-режим: дельта от WM_MOUSEMOVE игнорируется, но позицию обновляем)
        m_lastMouseX = x;
        m_lastMouseY = y;
        m_hasLastMouse = true;
        m_mouseX = x;
        m_mouseY = y;
    }

    void Input::endFrame() {
        // зарезервировано
    }

    void Input::_onKey(Key k, bool down) {
        if (k == Key::Unknown) return;
        m_keysCur.set((size_t)k, down);
    }

    void Input::_onMouse(MouseButton b, bool down) {
        m_mouseCur.set((size_t)b, down);
    }

    

    void Input::_resetMouseDelta() {
        m_hasLastMouse = false;
        // не трогаем m_mouseDX/DY — они уже могли быть накоплены реальным движением в этом кадре
    }

    void Input::_onMouseWheel(f32 delta) {
        m_wheelDelta += delta;
    }

    void Input::_onFocusLost() {
        m_keysCur.reset();
        m_mouseCur.reset();
        m_mouseDX = m_mouseDY = 0;
        m_wheelDelta = 0.f;
        m_hasLastMouse = false;
    }

} // namespace grit