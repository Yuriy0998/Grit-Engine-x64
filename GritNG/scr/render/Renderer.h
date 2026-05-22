#pragma once
#include "../core/Types.h"
#include "Camera.h"
#include "ShaderRegistry.h"

#include <bgfx/bgfx.h>
#include <string>


namespace grit::ecs { class World; }
namespace grit::rhi { class Device; }
namespace grit { class FileWatcher; }

namespace grit::render {

    class Renderer {
    public:
        bool init(rhi::Device* dev, FileWatcher* watcher);
        void shutdown();
        void render(ecs::World& world, f32 dt);
        void resizeViewport(u32 w, u32 h);
        Camera& camera() { return m_camera; }
        bool m_showGrid = true;

        bgfx::ProgramHandle defaultProgram() const {
            if (m_defaultEntry) return m_defaultEntry->program;
            bgfx::ProgramHandle invalid = BGFX_INVALID_HANDLE;
            return invalid;
        }

        const float* viewMatrix() const { return m_viewMtx; }
        const float* projMatrix() const { return m_projMtx; }
      
        bgfx::TextureHandle viewportColorTexture() const { return m_vpColor; }
        u32 viewportWidth()  const { return m_vpWidth; }
        u32 viewportHeight() const { return m_vpHeight; }

        ShaderRegistry& shaders() { return m_shaders; }

        bgfx::UniformHandle albedoSampler()  const { return m_uTexColor; }


    private:
        rhi::Device* m_device = nullptr;
        Camera              m_camera;

        void createViewportTargets(u32 w, u32 h);
        void destroyViewportTargets();

        bgfx::FrameBufferHandle m_vpFB = BGFX_INVALID_HANDLE;
        bgfx::TextureHandle     m_vpColor = BGFX_INVALID_HANDLE;
        bgfx::TextureHandle     m_vpDepth = BGFX_INVALID_HANDLE;
        u32 m_vpWidth = 0;
        u32 m_vpHeight = 0;

        static constexpr bgfx::ViewId kViewScene = 1; // было 0
        
        bgfx::UniformHandle m_uTexColor = BGFX_INVALID_HANDLE;
        bgfx::TextureHandle m_whitePixel = BGFX_INVALID_HANDLE; // fallback
        ShaderRegistry           m_shaders;
        ShaderRegistry::Entry* m_defaultEntry = nullptr;

        static constexpr u32 kMaxDirLights = 4;

        bgfx::UniformHandle m_uDirLightDir = BGFX_INVALID_HANDLE;  // vec4[kMaxDirLights]: xyz=dir, w=unused
        bgfx::UniformHandle m_uDirLightColor = BGFX_INVALID_HANDLE;  // vec4[kMaxDirLights]: rgb=color, a=intensity
        bgfx::UniformHandle m_uDirLightCount = BGFX_INVALID_HANDLE;  // vec4: x=count, y=ambient

        bgfx::UniformHandle m_uBaseColor = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle m_uTilingOffset = BGFX_INVALID_HANDLE;

        ShaderRegistry::Entry* m_gridEntry = nullptr;
        bgfx::UniformHandle m_uGridInvViewProj = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle m_uGridParams = BGFX_INVALID_HANDLE;

        void drawGrid();

        float m_viewMtx[16] = {};
        float m_projMtx[16] = {};
    };

} // namespace grit::render