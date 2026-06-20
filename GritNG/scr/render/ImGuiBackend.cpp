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
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // пока НЕ включаем — оно требует доп. backend platform
        io.BackendRendererName = "bgfx-grit";
        io.IniFilename = "imgui.ini"; // ВКЛЮЧАЕМ сохранение layout
       // io.IniFilename = nullptr;

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