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

        Camera& camera() { return m_camera; }

        bgfx::ProgramHandle defaultProgram() const {
            if (m_defaultEntry) return m_defaultEntry->program;
            bgfx::ProgramHandle invalid = BGFX_INVALID_HANDLE;
            return invalid;
        }

        ShaderRegistry& shaders() { return m_shaders; }

        bgfx::UniformHandle albedoSampler()  const { return m_uTexColor; }


    private:
        rhi::Device* m_device = nullptr;
        Camera              m_camera;
        
        bgfx::UniformHandle m_uTexColor = BGFX_INVALID_HANDLE;
        bgfx::TextureHandle m_whitePixel = BGFX_INVALID_HANDLE; // fallback
        ShaderRegistry           m_shaders;
        ShaderRegistry::Entry* m_defaultEntry = nullptr;

        bgfx::UniformHandle m_uLightDir = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle m_uLightColor = BGFX_INVALID_HANDLE;

        bgfx::UniformHandle m_uBaseColor = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle m_uTilingOffset = BGFX_INVALID_HANDLE;
    };

} // namespace grit::render