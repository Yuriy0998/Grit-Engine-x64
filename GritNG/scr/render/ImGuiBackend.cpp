#include "ImGuiBackend.h"
#include <imgui.h>      // ← если ещё не было раньше ImGuizmo
#include <ImGuizmo.h>

#include "ImGuiBackend.h"
#include "../platform/Window.h"
#include "../core/Input.h"
#include "../core/Log.h"


#include <bx/math.h>
#include <bgfx/bgfx.h>
#include <cstring>

namespace {
    ImGuiKey toImGuiKey(grit::Key k) {
        using K = grit::Key;
        switch (k) {
        case K::Tab: return ImGuiKey_Tab;
        case K::Left: return ImGuiKey_LeftArrow;
        case K::Right: return ImGuiKey_RightArrow;
        case K::Up: return ImGuiKey_UpArrow;
        case K::Down: return ImGuiKey_DownArrow;
        case K::PageUp: return ImGuiKey_PageUp;
        case K::PageDown: return ImGuiKey_PageDown;
        case K::Home: return ImGuiKey_Home;
        case K::End: return ImGuiKey_End;
        case K::Insert: return ImGuiKey_Insert;
        case K::Delete: return ImGuiKey_Delete;
        case K::Backspace: return ImGuiKey_Backspace;
        case K::Space: return ImGuiKey_Space;
        case K::Enter: return ImGuiKey_Enter;
        case K::KPEnter: return ImGuiKey_KeypadEnter;
        case K::Escape: return ImGuiKey_Escape;
        case K::Apostrophe: return ImGuiKey_Apostrophe;
        case K::Comma: return ImGuiKey_Comma;
        case K::Minus: return ImGuiKey_Minus;
        case K::Period: return ImGuiKey_Period;
        case K::Slash: return ImGuiKey_Slash;
        case K::Semicolon: return ImGuiKey_Semicolon;
        case K::Equals: return ImGuiKey_Equal;
        case K::LeftBracket: return ImGuiKey_LeftBracket;
        case K::Backslash: return ImGuiKey_Backslash;
        case K::RightBracket: return ImGuiKey_RightBracket;
        case K::GraveAccent: return ImGuiKey_GraveAccent;
        case K::CapsLock: return ImGuiKey_CapsLock;

        case K::LeftShift: return ImGuiKey_LeftShift;
        case K::RightShift: return ImGuiKey_RightShift;
        case K::LeftCtrl: return ImGuiKey_LeftCtrl;
        case K::RightCtrl: return ImGuiKey_RightCtrl;
        case K::LeftAlt: return ImGuiKey_LeftAlt;
        case K::RightAlt: return ImGuiKey_RightAlt;
        case K::LeftSuper: return ImGuiKey_LeftSuper;
        case K::RightSuper: return ImGuiKey_RightSuper;

        case K::Num0: return ImGuiKey_0; case K::Num1: return ImGuiKey_1;
        case K::Num2: return ImGuiKey_2; case K::Num3: return ImGuiKey_3;
        case K::Num4: return ImGuiKey_4; case K::Num5: return ImGuiKey_5;
        case K::Num6: return ImGuiKey_6; case K::Num7: return ImGuiKey_7;
        case K::Num8: return ImGuiKey_8; case K::Num9: return ImGuiKey_9;

        case K::A: return ImGuiKey_A; case K::B: return ImGuiKey_B;
        case K::C: return ImGuiKey_C; case K::D: return ImGuiKey_D;
        case K::E: return ImGuiKey_E; case K::F: return ImGuiKey_F;
        case K::G: return ImGuiKey_G; case K::H: return ImGuiKey_H;
        case K::I: return ImGuiKey_I; case K::J: return ImGuiKey_J;
        case K::K: return ImGuiKey_K; case K::L: return ImGuiKey_L;
        case K::M: return ImGuiKey_M; case K::N: return ImGuiKey_N;
        case K::O: return ImGuiKey_O; case K::P: return ImGuiKey_P;
        case K::Q: return ImGuiKey_Q; case K::R: return ImGuiKey_R;
        case K::S: return ImGuiKey_S; case K::T: return ImGuiKey_T;
        case K::U: return ImGuiKey_U; case K::V: return ImGuiKey_V;
        case K::W: return ImGuiKey_W; case K::X: return ImGuiKey_X;
        case K::Y: return ImGuiKey_Y; case K::Z: return ImGuiKey_Z;

        case K::F1: return ImGuiKey_F1;  case K::F2: return ImGuiKey_F2;
        case K::F3: return ImGuiKey_F3;  case K::F4: return ImGuiKey_F4;
        case K::F5: return ImGuiKey_F5;  case K::F6: return ImGuiKey_F6;
        case K::F7: return ImGuiKey_F7;  case K::F8: return ImGuiKey_F8;
        case K::F9: return ImGuiKey_F9;  case K::F10: return ImGuiKey_F10;
        case K::F11: return ImGuiKey_F11; case K::F12: return ImGuiKey_F12;

        case K::KP0: return ImGuiKey_Keypad0; case K::KP1: return ImGuiKey_Keypad1;
        case K::KP2: return ImGuiKey_Keypad2; case K::KP3: return ImGuiKey_Keypad3;
        case K::KP4: return ImGuiKey_Keypad4; case K::KP5: return ImGuiKey_Keypad5;
        case K::KP6: return ImGuiKey_Keypad6; case K::KP7: return ImGuiKey_Keypad7;
        case K::KP8: return ImGuiKey_Keypad8; case K::KP9: return ImGuiKey_Keypad9;
        case K::KPAdd: return ImGuiKey_KeypadAdd;
        case K::KPSubtract: return ImGuiKey_KeypadSubtract;
        case K::KPMultiply: return ImGuiKey_KeypadMultiply;
        case K::KPDivide: return ImGuiKey_KeypadDivide;
        case K::KPDecimal: return ImGuiKey_KeypadDecimal;

        default: return ImGuiKey_None;
        }
    }
}
namespace grit::render {

    bool ImGuiBackend::init(bgfx::ViewId viewId, ShaderRegistry& shaders)
    {
        m_viewId = viewId;
        m_shaders = &shaders;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());  // ← ДОБАВЬ
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.BackendRendererName = "bgfx-grit";
        io.IniFilename = "imgui.ini";

        // === Загрузка шрифта с кириллицей ===
        {
            ImFontConfig cfg;
            cfg.OversampleH = 2;
            cfg.OversampleV = 2;
            cfg.PixelSnapH = true;

            // Диапазоны глифов: латиница + кириллица
            static const ImWchar ranges[] = {
                0x0020, 0x00FF, // Basic Latin + Latin Supplement
                0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
                0x2000, 0x206F, // General Punctuation (кавычки, тире)
                0,
            };

            // Путь к TTF со шрифтом, содержащим кириллицу.
            // Вариант A: положить в assets/fonts/ (рекомендуется)
            const char* fontPath = "assets/fonts/HuaweiSans.ttf";
            ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath, 16.0f, &cfg, ranges);

            // Fallback: системный шрифт Windows (Segoe UI содержит кириллицу)
            if (!font) {
                font = io.Fonts->AddFontFromFileTTF(
                    "C:\\Windows\\Fonts\\segoeui.ttf", 16.0f, &cfg, ranges);
            }

            // Если и это не удалось — дефолтный (без кириллицы)
            if (!font) {
                GRIT_WARN("ImGuiBackend: cyrillic font not found, using default");
                io.Fonts->AddFontDefault();
            }
        }

        m_layout.begin()
            .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();

        m_progEntry = m_shaders->loadProgram("vs_imgui", "fs_imgui");
        if (!m_progEntry || !bgfx::isValid(m_progEntry->program)) {
            GRIT_ERROR("ImGuiBackend: failed to load imgui shaders");
            return false;
        }

        m_uTex = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

        // Font atlas
        unsigned char* pixels = nullptr;
        int w = 0, h = 0;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);

        m_font = bgfx::createTexture2D(
            (uint16_t)w, (uint16_t)h, false, 1,
            bgfx::TextureFormat::RGBA8, 0,
            bgfx::copy(pixels, w * h * 4));

        io.Fonts->SetTexID((ImTextureID)(uintptr_t)m_font.idx);

        m_initialized = true;
        return true;
    }

    void ImGuiBackend::shutdown()
    {
        if (!m_initialized) return;
        if (bgfx::isValid(m_font)) bgfx::destroy(m_font);
        if (bgfx::isValid(m_uTex)) bgfx::destroy(m_uTex);
        // программу не уничтожаем — она в ShaderRegistry
        ImGui::DestroyContext();
        m_initialized = false;
    }

    bool ImGuiBackend::wantCaptureMouse() const {
        return m_initialized && ImGui::GetIO().WantCaptureMouse;
    }
    bool ImGuiBackend::wantCaptureKeyboard() const {
        return m_initialized && ImGui::GetIO().WantCaptureKeyboard;
    }

    void ImGuiBackend::beginFrame(grit::Window& window,
        grit::Input& input,
        float deltaSeconds)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)window.width(), (float)window.height());
        io.DeltaTime = deltaSeconds > 0.f ? deltaSeconds : (1.f / 60.f);

        io.MousePos = ImVec2((float)input.mouseX(), (float)input.mouseY());
        io.MouseDown[0] = input.isMouseDown(MouseButton::Left);
        io.MouseDown[1] = input.isMouseDown(MouseButton::Right);
        io.MouseDown[2] = input.isMouseDown(MouseButton::Middle);
        io.MouseWheel = input.wheelDelta();

        // === Клавиатура ===
        for (const auto& ev : input.keyEvents()) {
            ImGuiKey ik = toImGuiKey(ev.key);
            if (ik != ImGuiKey_None)
                io.AddKeyEvent(ik, ev.down);
        }

        // Модификаторы (по текущему состоянию)
        io.AddKeyEvent(ImGuiMod_Ctrl,
            input.isDown(grit::Key::LeftCtrl) || input.isDown(grit::Key::RightCtrl));
        io.AddKeyEvent(ImGuiMod_Shift,
            input.isDown(grit::Key::LeftShift) || input.isDown(grit::Key::RightShift));
        io.AddKeyEvent(ImGuiMod_Alt,
            input.isDown(grit::Key::LeftAlt) || input.isDown(grit::Key::RightAlt));
        io.AddKeyEvent(ImGuiMod_Super,
            input.isDown(grit::Key::LeftSuper) || input.isDown(grit::Key::RightSuper));

        // === Текстовый ввод ===
        for (unsigned c : input.charQueue())
            io.AddInputCharacter(c);

        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
    }

    void ImGuiBackend::endFrame()
    {
        ImGui::Render();
        ImDrawData* draw = ImGui::GetDrawData();
        if (!draw || draw->CmdListsCount == 0) return;
        if (!m_progEntry || !bgfx::isValid(m_progEntry->program)) return;

        const ImGuiIO& io = ImGui::GetIO();
        const float fb_w = io.DisplaySize.x;
        const float fb_h = io.DisplaySize.y;
        if (fb_w <= 0.f || fb_h <= 0.f) return;

        const bgfx::ProgramHandle program = m_progEntry->program;

        bgfx::setViewMode(m_viewId, bgfx::ViewMode::Sequential);
        bgfx::setViewRect(m_viewId, 0, 0, (uint16_t)fb_w, (uint16_t)fb_h);

        float ortho[16];
        const bgfx::Caps* caps = bgfx::getCaps();
        bx::mtxOrtho(ortho, 0.f, fb_w, fb_h, 0.f, -1.f, 1.f, 0.f, caps->homogeneousDepth);
        bgfx::setViewTransform(m_viewId, nullptr, ortho);

        for (int n = 0; n < draw->CmdListsCount; ++n)
        {
            const ImDrawList* list = draw->CmdLists[n];
            const uint32_t numVtx = (uint32_t)list->VtxBuffer.size();
            const uint32_t numIdx = (uint32_t)list->IdxBuffer.size();
            if (numVtx == 0 || numIdx == 0) continue;

            if (bgfx::getAvailTransientVertexBuffer(numVtx, m_layout) < numVtx) break;
            if (bgfx::getAvailTransientIndexBuffer(numIdx) < numIdx) break;

            bgfx::TransientVertexBuffer tvb;
            bgfx::TransientIndexBuffer  tib;
            bgfx::allocTransientVertexBuffer(&tvb, numVtx, m_layout);
            bgfx::allocTransientIndexBuffer(&tib, numIdx);

            std::memcpy(tvb.data, list->VtxBuffer.Data, numVtx * sizeof(ImDrawVert));
            std::memcpy(tib.data, list->IdxBuffer.Data, numIdx * sizeof(ImDrawIdx));

            uint32_t idxOffset = 0;
            for (const ImDrawCmd& cmd : list->CmdBuffer)
            {
                if (cmd.UserCallback) {
                    cmd.UserCallback(list, &cmd);
                }
                else if (cmd.ElemCount > 0) {
                    const float cx1 = bx::max(cmd.ClipRect.x, 0.f);
                    const float cy1 = bx::max(cmd.ClipRect.y, 0.f);
                    const float cx2 = bx::min(cmd.ClipRect.z, fb_w);
                    const float cy2 = bx::min(cmd.ClipRect.w, fb_h);
                    if (cx2 > cx1 && cy2 > cy1) {
                        bgfx::setScissor((uint16_t)cx1, (uint16_t)cy1,
                            (uint16_t)(cx2 - cx1), (uint16_t)(cy2 - cy1));

                        bgfx::TextureHandle th{ (uint16_t)(uintptr_t)cmd.GetTexID() };
                        if (bgfx::isValid(th)) {
                            bgfx::setTexture(0, m_uTex, th);
                        }

                        bgfx::setVertexBuffer(0, &tvb, 0, numVtx);
                        bgfx::setIndexBuffer(&tib, idxOffset, cmd.ElemCount);

                        bgfx::setState(
                            BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A
                            | BGFX_STATE_MSAA
                            | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA,
                                BGFX_STATE_BLEND_INV_SRC_ALPHA)
                        );

                        bgfx::submit(m_viewId, program);
                    }
                }
                idxOffset += cmd.ElemCount;
            }
        }
    }

} // namespace grit::render