#include "Window.h"
#include "../core/Log.h"
#include "../core/Input.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#pragma comment(lib, "user32.lib")
namespace grit {

    static const wchar_t* kClassName = L"GritNGWindow";

    // --- Маппинг Win32 VK → Key ---
    static Key vkToKey(WPARAM vk, LPARAM lParam) {
        // Различаем Left/Right Shift/Ctrl/Alt через scancode + extended flag
        const u32 scancode = (lParam >> 16) & 0xFF;
        const bool extended = (lParam & (1 << 24)) != 0;

        switch (vk) {
        case 'A': return Key::A; case 'B': return Key::B; case 'C': return Key::C;
        case 'D': return Key::D; case 'E': return Key::E; case 'F': return Key::F;
        case 'G': return Key::G; case 'H': return Key::H; case 'I': return Key::I;
        case 'J': return Key::J; case 'K': return Key::K; case 'L': return Key::L;
        case 'M': return Key::M; case 'N': return Key::N; case 'O': return Key::O;
        case 'P': return Key::P; case 'Q': return Key::Q; case 'R': return Key::R;
        case 'S': return Key::S; case 'T': return Key::T; case 'U': return Key::U;
        case 'V': return Key::V; case 'W': return Key::W; case 'X': return Key::X;
        case 'Y': return Key::Y; case 'Z': return Key::Z;

        case '0': return Key::Num0; case '1': return Key::Num1; case '2': return Key::Num2;
        case '3': return Key::Num3; case '4': return Key::Num4; case '5': return Key::Num5;
        case '6': return Key::Num6; case '7': return Key::Num7; case '8': return Key::Num8;
        case '9': return Key::Num9;

        case VK_F1:  return Key::F1;  case VK_F2:  return Key::F2;  case VK_F3:  return Key::F3;
        case VK_F4:  return Key::F4;  case VK_F5:  return Key::F5;  case VK_F6:  return Key::F6;
        case VK_F7:  return Key::F7;  case VK_F8:  return Key::F8;  case VK_F9:  return Key::F9;
        case VK_F10: return Key::F10; case VK_F11: return Key::F11; case VK_F12: return Key::F12;

        case VK_ESCAPE:    return Key::Escape;
        case VK_TAB:       return Key::Tab;
        case VK_CAPITAL:   return Key::CapsLock;
        case VK_SPACE:     return Key::Space;
        case VK_BACK:      return Key::Backspace;
        case VK_RETURN:    return extended ? Key::KPEnter : Key::Enter;
        case VK_INSERT:    return Key::Insert;
        case VK_DELETE:    return Key::Delete;
        case VK_HOME:      return Key::Home;
        case VK_END:       return Key::End;
        case VK_PRIOR:     return Key::PageUp;
        case VK_NEXT:      return Key::PageDown;
        case VK_LEFT:      return Key::Left;
        case VK_RIGHT:     return Key::Right;
        case VK_UP:        return Key::Up;
        case VK_DOWN:      return Key::Down;

        case VK_SHIFT: {
            // Распознаём L/R Shift через scancode
            UINT lShift = MapVirtualKeyW(VK_LSHIFT, MAPVK_VK_TO_VSC);
            return (scancode == lShift) ? Key::LeftShift : Key::RightShift;
        }
        case VK_LSHIFT:    return Key::LeftShift;
        case VK_RSHIFT:    return Key::RightShift;

        case VK_CONTROL:   return extended ? Key::RightCtrl : Key::LeftCtrl;
        case VK_LCONTROL:  return Key::LeftCtrl;
        case VK_RCONTROL:  return Key::RightCtrl;

        case VK_MENU:      return extended ? Key::RightAlt : Key::LeftAlt;
        case VK_LMENU:     return Key::LeftAlt;
        case VK_RMENU:     return Key::RightAlt;

        case VK_LWIN:      return Key::LeftSuper;
        case VK_RWIN:      return Key::RightSuper;

        case VK_NUMPAD0:   return Key::KP0; case VK_NUMPAD1: return Key::KP1;
        case VK_NUMPAD2:   return Key::KP2; case VK_NUMPAD3: return Key::KP3;
        case VK_NUMPAD4:   return Key::KP4; case VK_NUMPAD5: return Key::KP5;
        case VK_NUMPAD6:   return Key::KP6; case VK_NUMPAD7: return Key::KP7;
        case VK_NUMPAD8:   return Key::KP8; case VK_NUMPAD9: return Key::KP9;
        case VK_ADD:       return Key::KPAdd;
        case VK_SUBTRACT:  return Key::KPSubtract;
        case VK_MULTIPLY:  return Key::KPMultiply;
        case VK_DIVIDE:    return Key::KPDivide;
        case VK_DECIMAL:   return Key::KPDecimal;

        case VK_OEM_MINUS:  return Key::Minus;
        case VK_OEM_PLUS:   return Key::Equals;
        case VK_OEM_4:      return Key::LeftBracket;
        case VK_OEM_6:      return Key::RightBracket;
        case VK_OEM_5:      return Key::Backslash;
        case VK_OEM_1:      return Key::Semicolon;
        case VK_OEM_7:      return Key::Apostrophe;
        case VK_OEM_COMMA:  return Key::Comma;
        case VK_OEM_PERIOD: return Key::Period;
        case VK_OEM_2:      return Key::Slash;
        case VK_OEM_3:      return Key::GraveAccent;
        }
        return Key::Unknown;
    }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        Window* self = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        switch (msg) {
        case WM_CLOSE:
        case WM_DESTROY:
            if (self) self->_onClose();
            return 0;

        case WM_SIZE: {
            if (self) {
                u32 w = LOWORD(lp);
                u32 h = HIWORD(lp);
                if (w > 0 && h > 0) self->_onResize(w, h);
            }
            return 0;
        }

        case WM_SETFOCUS:
            if (self) self->_onFocus(true);
            return 0;
        case WM_KILLFOCUS:
            if (self) self->_onFocus(false);
            return 0;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            if (self && self->_input()) {
                Key k = vkToKey(wp, lp);
                // Не фильтруем auto-repeat: ImGui это нужно для повтора клавиш.
                // (для игровой логики wasPressed по-прежнему сработает один раз,
                //  т.к. m_keysPrev обновляется раз в кадр)
                self->_input()->_onKey(k, true);
            }
            if (wp == VK_F10 || wp == VK_MENU) return 0;
            break;
        }
        case WM_CHAR: {
            if (self && self->_input()) {
                // wp — UTF-16 code unit. Печатаемые символы >= 32, плюс не-управляющие.
                if (wp >= 32 && wp != 127)              // отсекаем управляющие
                    self->_input()->_onChar((unsigned)wp);
            }
            return 0;
        }
        case WM_KEYUP:
        case WM_SYSKEYUP: {
            if (self && self->_input()) {
                Key k = vkToKey(wp, lp);
                self->_input()->_onKey(k, false);
            }
            break;
        }

        case WM_LBUTTONDOWN: if (self && self->_input()) { self->_input()->_onMouse(MouseButton::Left, true);  SetCapture(hwnd); } return 0;
        case WM_LBUTTONUP:   if (self && self->_input()) { self->_input()->_onMouse(MouseButton::Left, false); ReleaseCapture(); } return 0;
        case WM_RBUTTONDOWN: if (self && self->_input()) { self->_input()->_onMouse(MouseButton::Right, true);  SetCapture(hwnd); } return 0;
        case WM_RBUTTONUP:   if (self && self->_input()) { self->_input()->_onMouse(MouseButton::Right, false); ReleaseCapture(); } return 0;
        case WM_MBUTTONDOWN: if (self && self->_input()) { self->_input()->_onMouse(MouseButton::Middle, true);  SetCapture(hwnd); } return 0;
        case WM_MBUTTONUP:   if (self && self->_input()) { self->_input()->_onMouse(MouseButton::Middle, false); ReleaseCapture(); } return 0;
        case WM_XBUTTONDOWN: {
            if (self && self->_input()) {
                MouseButton b = (HIWORD(wp) == XBUTTON1) ? MouseButton::X1 : MouseButton::X2;
                self->_input()->_onMouse(b, true);
                SetCapture(hwnd);
            }
            return TRUE;
        }

        case WM_XBUTTONUP: {
            if (self && self->_input()) {
                MouseButton b = (HIWORD(wp) == XBUTTON1) ? MouseButton::X1 : MouseButton::X2;
                self->_input()->_onMouse(b, false);
                ReleaseCapture();
            }
            return TRUE;
        }

        case WM_MOUSEMOVE: {
            if (self && self->_input()) {
                i32 x = GET_X_LPARAM(lp);
                i32 y = GET_Y_LPARAM(lp);
                self->_input()->_onMouseMove(x, y);
            }
            return 0;
        }
        case WM_INPUT: {
            if (self && self->_input()) {
                UINT size = 0;
                GetRawInputData((HRAWINPUT)lp, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
                if (size > 0 && size <= 64) { // raw mouse — маленькая структура
                    BYTE buf[64];
                    if (GetRawInputData((HRAWINPUT)lp, RID_INPUT, buf, &size, sizeof(RAWINPUTHEADER)) == size) {
                        RAWINPUT* ri = (RAWINPUT*)buf;
                        if (ri->header.dwType == RIM_TYPEMOUSE) {
                            const RAWMOUSE& m = ri->data.mouse;
                            if ((m.usFlags & MOUSE_MOVE_ABSOLUTE) == 0) {
                                // Относительные дельты — то, что нам нужно
                                if (m.lLastX != 0 || m.lLastY != 0) {
                                    self->_input()->_onRawMouseDelta(m.lLastX, m.lLastY);
                                }
                            }
                        }
                    }
                }
            }
            return 0;
        }

        case WM_MOUSEWHEEL: {
            if (self && self->_input()) {
                i32 delta = GET_WHEEL_DELTA_WPARAM(wp);
                f32 norm = (f32)delta / (f32)WHEEL_DELTA;
                self->_input()->_onMouseWheel(norm);
            }
            return 0;
        }
        }

        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    bool Window::create(const WindowDesc& desc) {
        HINSTANCE hInst = GetModuleHandleW(nullptr);

        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = kClassName;

        static bool s_registered = false;
        if (!s_registered) {
            if (!RegisterClassExW(&wc)) {
                GRIT_ERROR("RegisterClassEx failed");
                return false;
            }
            s_registered = true;
        }

        // Конвертация UTF-8 → UTF-16 для заголовка
        std::wstring wtitle;
        {
            int needed = MultiByteToWideChar(CP_UTF8, 0, desc.title.c_str(), -1, nullptr, 0);
            if (needed > 0) {
                wtitle.resize(needed - 1);
                MultiByteToWideChar(CP_UTF8, 0, desc.title.c_str(), -1, wtitle.data(), needed);
            }
        }

        DWORD style = WS_OVERLAPPEDWINDOW;

        RECT rc{ 0, 0, (LONG)desc.width, (LONG)desc.height };
        AdjustWindowRect(&rc, style, FALSE);

        HWND hwnd = CreateWindowExW(
            0, kClassName, wtitle.c_str(), style,
            CW_USEDEFAULT, CW_USEDEFAULT,
            rc.right - rc.left, rc.bottom - rc.top,
            nullptr, nullptr, hInst, nullptr);

        if (!hwnd) {
            GRIT_ERROR("CreateWindowEx failed");
            return false;
        }

        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)this);
        ShowWindow(hwnd, SW_SHOW);
        RAWINPUTDEVICE rid{};
        rid.usUsagePage = 0x01; // Generic Desktop
        rid.usUsage = 0x02; // Mouse
        rid.dwFlags = 0;    // не RIDEV_NOLEGACY — нам нужны и обычные WM_MOUSE* для UI
        rid.hwndTarget = hwnd;
        if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
            GRIT_WARN("RegisterRawInputDevices failed");
        }
        UpdateWindow(hwnd);

        m_hwnd = hwnd;
        m_width = desc.width;
        m_height = desc.height;
        m_running = true;
        return true;
    }

    void Window::destroy() {
        if (m_hwnd) {
            DestroyWindow((HWND)m_hwnd);
            m_hwnd = nullptr;
        }
    }

    bool Window::pumpMessages() {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        // Захват курсора: если запрошен, держим в центре окна каждый кадр
        if (m_mouseCaptured && m_hasFocus) {
            warpCursorToCenter();
        }

        return m_running;
    }

    bool Window::resized() {
        bool r = m_resized;
        m_resized = false;
        return r;
    }

    void Window::_onFocus(bool focused) {
        m_hasFocus = focused;
        if (!focused) {
            // Снимаем захват и сбрасываем нажатия — иначе клавиша «залипнет»,
            // если пользователь Alt-Tab'нул с зажатой W.
            if (m_mouseCaptured) {
                ShowCursor(TRUE);
                ClipCursor(nullptr);
                m_mouseCaptured = false;
                if (m_input) m_input->_setCaptured(false);
            }
            if (m_input) m_input->_onFocusLost();
        }
    }

    void Window::setMouseCapture(bool capture) {
        if (capture == m_mouseCaptured) return;
        if (!m_hwnd) return;

        if (capture) {
            ShowCursor(FALSE);
            // Зажимаем курсор внутри клиентской области окна
            RECT rc;
            GetClientRect((HWND)m_hwnd, &rc);
            POINT tl{ rc.left, rc.top }, br{ rc.right, rc.bottom };
            ClientToScreen((HWND)m_hwnd, &tl);
            ClientToScreen((HWND)m_hwnd, &br);
            RECT screen{ tl.x, tl.y, br.x, br.y };
            ClipCursor(&screen);
            warpCursorToCenter();
        }
        else {
            ShowCursor(TRUE);
            ClipCursor(nullptr);
        }
        m_mouseCaptured = capture;
        if (m_input) m_input->_setCaptured(capture);
    }

    void Window::warpCursorToCenter() {
        if (!m_hwnd) return;
        RECT rc;
        GetClientRect((HWND)m_hwnd, &rc);
        POINT center{ (rc.right - rc.left) / 2, (rc.bottom - rc.top) / 2 };
        ClientToScreen((HWND)m_hwnd, &center);
        SetCursorPos(center.x, center.y);
        // Сбрасываем lastMouse, чтобы WM_MOUSEMOVE от warp'а не дал WM_MOUSEMOVE-дельту.
        // Raw input при этом продолжает давать корректные дельты сам по себе.
        if (m_input) m_input->_resetMouseDelta();
    }

} // namespace grit