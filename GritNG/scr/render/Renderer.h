#pragma once
#include "../core/Types.h"
#include "Camera.h"
#include <bgfx/bgfx.h>

namespace grit::ecs { class World; }
namespace grit::rhi { class Device; }

namespace grit::render {

    class Renderer {
    public:
        bool init(rhi::Device* dev);
        void shutdown();
        void render(ecs::World& world, f32 dt);

        Camera& camera() { return m_camera; }

        bgfx::ProgramHandle defaultProgram() const { return m_defaultProgram; }
        bgfx::UniformHandle albedoSampler()  const { return m_uTexColor; }

    private:
        rhi::Device* m_device = nullptr;
        Camera              m_camera;
        bgfx::ProgramHandle m_defaultProgram = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle m_uTexColor = BGFX_INVALID_HANDLE;
        bgfx::TextureHandle m_whitePixel = BGFX_INVALID_HANDLE; // fallback
    };

} // namespace grit::render