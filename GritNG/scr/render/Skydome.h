#pragma once
#include "../core/Types.h"
#include "../core/Math.h"     // ← здесь grit::vec3
#include <bgfx/bgfx.h>

namespace grit::render {

    struct  Environment;

    class Skydome {
    public:
        bool init(bgfx::ProgramHandle program);
        void shutdown();

        // Рисует skydome. cameraPos — позиция камеры (skydome крепится к ней).
        void render(bgfx::ViewId viewId,
            const Environment& env,
            const grit::vec3& cameraPos);   // ← grit::vec3

    private:
        bgfx::VertexBufferHandle m_vbh = BGFX_INVALID_HANDLE;
        bgfx::IndexBufferHandle  m_ibh = BGFX_INVALID_HANDLE;
        bgfx::ProgramHandle      m_program = BGFX_INVALID_HANDLE;

        bgfx::UniformHandle      m_uSkyTop = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle      m_uSkyBottom = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle      m_uSunDir = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle      m_uSunColor = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle      m_uSkyParams = BGFX_INVALID_HANDLE;

        u32                      m_indexCount = 0;
    };

} // namespace grit::render