#pragma once
#include "ShaderRegistry.h"
#include <bgfx/bgfx.h>

namespace grit {
    class Window;
    class Input;
}

namespace grit::render {

    class ImGuiBackend {
    public:
        bool init(bgfx::ViewId viewId, ShaderRegistry& shaders);
        void shutdown();

        // До ImGui::* вызовов
        void beginFrame(grit::Window& window,
            grit::Input& input,
            float deltaSeconds);
        // После
        void endFrame();

        bool wantCaptureMouse()    const;
        bool wantCaptureKeyboard() const;

    private:
        bgfx::ViewId            m_viewId = 255;
        ShaderRegistry* m_shaders = nullptr;
        ShaderRegistry::Entry* m_progEntry = nullptr;
        bgfx::TextureHandle     m_font = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle     m_uTex = BGFX_INVALID_HANDLE;
        bgfx::VertexLayout      m_layout;
        bool                    m_initialized = false;
    };

} // namespace grit::render