#pragma once
#include "../core/Types.h"
#include "../core/KeyCodes.h"
#include <string>

namespace grit {

    class Input; // fwd

    struct WindowDesc {
        std::string title = "GritNG";
        u32         width = 1280;
        u32         height = 720;
    };

    class Window {
    public:
        bool create(const WindowDesc& desc);
        void destroy();

        // Возвращает false, если окно закрыто
        bool pumpMessages();

        void* nativeHandle()  const { return m_hwnd; }
        void* nativeDisplay() const { return nullptr; }

        u32  width()  const { return m_width; }
        u32  height() const { return m_height; }
        bool resized();      // считывает и сбрасывает флаг

        // Привязка Input — Window будет слать события туда
        void setInput(Input* input) { m_input = input; }

        // Захват курсора (вызывается из Engine, который синкается с Input)
        void setMouseCapture(bool capture);
        bool isMouseCaptured() const { return m_mouseCaptured; }

        // Центр клиентской области (для центрирования курсора в FPS-режиме)
        void warpCursorToCenter();

        // Внутренние методы (вызываются из WndProc) — публичные, чтобы обойтись без friend
        void _onClose() { m_running = false; }
        void _onResize(u32 w, u32 h) { m_width = w; m_height = h; m_resized = true; }
        void _onFocus(bool focused);
        Input* _input() const { return m_input; }

    private:
        void* m_hwnd = nullptr;
        u32   m_width = 0;
        u32   m_height = 0;
        bool  m_resized = false;
        bool  m_running = true;
        bool  m_hasFocus = true;
        bool  m_mouseCaptured = false;

        Input* m_input = nullptr;
    };

} // namespace grit