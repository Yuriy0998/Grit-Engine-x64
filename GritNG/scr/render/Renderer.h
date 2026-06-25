#pragma once
#include "../core/Types.h"
#include "Camera.h"
#include "ShaderRegistry.h"
#include "Environment.h"
#include "TimeOfDay.h"
#include "Skydome.h"
#include "DebugDraw.h"

#include <bgfx/bgfx.h>
#include <string>
#include <entt/entt.hpp>

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

        // ===== Accessors =====
        Camera& camera() { return m_camera; }
        const Camera& camera()       const { return m_camera; }

        Environment& env() { return m_env; }
        const Environment& env()          const { return m_env; }

        TimeOfDay& timeOfDay() { return m_timeOfDay; }
        const TimeOfDay& timeOfDay()    const { return m_timeOfDay; }

        ShaderRegistry& shaders() { return m_shaders; }

        bgfx::ProgramHandle defaultProgram() const {
            if (m_defaultEntry) return m_defaultEntry->program;
            bgfx::ProgramHandle invalid = BGFX_INVALID_HANDLE;
            return invalid;
        }

        bgfx::UniformHandle albedoSampler()     const { return m_uTexColor; }

        const float* viewMatrix() const { return m_viewMtx; }
        const float* projMatrix() const { return m_projMtx; }

        bgfx::TextureHandle viewportColorTexture() const { return m_vpColor; }
        u32                 viewportWidth()        const { return m_vpWidth; }
        u32                 viewportHeight()       const { return m_vpHeight; }

        void setSelectedEntity(entt::entity e) { m_selectedEntity = e; }
        DebugDraw& debug() { return m_debug; }

        // ===== Public flags =====
        bool m_showGrid = true;

    private:
        // ===== View IDs (порядок submission'а) =====
        static constexpr bgfx::ViewId kViewBackbuffer = 0;
        static constexpr bgfx::ViewId kViewSky = 1;
        static constexpr bgfx::ViewId kViewScene = 2;

        // ===== Helpers =====
        void createViewportTargets(u32 w, u32 h);
        void destroyViewportTargets();
        void drawGrid();

        // ===== Device =====
        rhi::Device* m_device = nullptr;

        // ===== Shaders =====
        ShaderRegistry          m_shaders;
        ShaderRegistry::Entry* m_defaultEntry = nullptr;
        ShaderRegistry::Entry* m_gridEntry = nullptr;
        ShaderRegistry::Entry* m_skyEntry = nullptr;

        // ===== Sky / TimeOfDay =====
        Skydome    m_skydome;
        TimeOfDay  m_timeOfDay;

        // ===== Camera & environment =====
        Camera      m_camera;
        Environment m_env;

        float m_viewMtx[16] = {};
        float m_projMtx[16] = {};

        static constexpr u32 kMaxPointLights = 8;
        static constexpr u32 kMaxSpotLights = 8;


        DebugDraw                m_debug;
        const ShaderRegistry::Entry* m_debugEntry = nullptr;

        // Текущая выделенная entity — для рисования зоны действия лампы.
        // Renderer об этом узнает через сеттер из EditorUI.
        entt::entity             m_selectedEntity = entt::null;

        // ===== Viewport FB =====
        bgfx::FrameBufferHandle m_vpFB = BGFX_INVALID_HANDLE;
        bgfx::TextureHandle     m_vpColor = BGFX_INVALID_HANDLE;
        bgfx::TextureHandle     m_vpDepth = BGFX_INVALID_HANDLE;
        u32                     m_vpWidth = 0;
        u32                     m_vpHeight = 0;

        // ===== Common =====
        bgfx::TextureHandle m_whitePixel = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle m_uTexColor = BGFX_INVALID_HANDLE;

        // ===== Lights =====
        static constexpr u32 kMaxDirLights = 4;
        bgfx::UniformHandle m_uDirLightDir = BGFX_INVALID_HANDLE; // vec4[kMaxDirLights]
        bgfx::UniformHandle m_uDirLightColor = BGFX_INVALID_HANDLE; // vec4[kMaxDirLights]
        bgfx::UniformHandle m_uDirLightCount = BGFX_INVALID_HANDLE; // vec4: x=count, y=ambient


        // Point lights
        bgfx::UniformHandle m_uPointLightPos = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle m_uPointLightColor = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle m_uPointLightCount = BGFX_INVALID_HANDLE;

        // Spot lights
        bgfx::UniformHandle m_uSpotLightPos = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle m_uSpotLightDir = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle m_uSpotLightColor = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle m_uSpotLightParams = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle m_uSpotLightCount = BGFX_INVALID_HANDLE;

        // ===== Material =====
        bgfx::UniformHandle m_uBaseColor = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle m_uTilingOffset = BGFX_INVALID_HANDLE;

        // ===== Grid =====
        bgfx::UniformHandle m_uGridInvViewProj = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle m_uGridParams = BGFX_INVALID_HANDLE;

        // ===== Environment uniforms =====
        bgfx::UniformHandle m_uFogParams = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle m_uFogColor = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle m_uAmbientColor = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle m_uCameraPos = BGFX_INVALID_HANDLE;
    };

} // namespace grit::render