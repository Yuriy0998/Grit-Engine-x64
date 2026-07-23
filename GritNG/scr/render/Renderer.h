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

        bool& bloomEnabled() { return m_bloomEnabled; }
        f32& bloomThreshold() { return m_bloomThreshold; }
        f32& bloomKnee() { return m_bloomKnee; }
        f32& bloomIntensity() { return m_bloomIntensity; }
        f32& bloomFilterRadius() { return m_bloomFilterRadius; }

        bgfx::TextureHandle viewportColorTexture() const {
            return bgfx::isValid(m_vpColorLDR) ? m_vpColorLDR : m_vpColor;
        }

        f32& exposure() { return m_exposure; }
        int& tonemapOperator() { return m_tonemapOperator; }
        f32& gamma() { return m_gamma; }

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

       
        u32                 viewportWidth()        const { return m_vpWidth; }
        u32                 viewportHeight()       const { return m_vpHeight; }

        void setSelectedEntity(entt::entity e) { m_selectedEntity = e; }
        DebugDraw& debug() { return m_debug; }

        // ===== Public flags =====
        bool m_showGrid = true;

        // Shadow API (для UI)
        bool& shadowsEnabled() { return m_shadowsEnabled; }
        f32& shadowDistance() { return m_shadowDistance; }
        f32& shadowBias() { return m_shadowBias; }
        bool& softShadows() { return m_softShadows; }
        u32   shadowMapSize() const { return m_shadowMapSize; }
        void  setShadowMapSize(u32 s);

        bgfx::TextureHandle shadowMapTexture() const { return m_shadowMap; }

    private:
        // ===== View IDs (порядок submission'а) =====
        static constexpr bgfx::ViewId kViewShadow = 0;
        static constexpr bgfx::ViewId kViewBackbuffer = 1;
        static constexpr bgfx::ViewId kViewSky = 2;
        static constexpr bgfx::ViewId kViewScene = 3;
        static constexpr bgfx::ViewId kViewBloomBright = 5;
        static constexpr bgfx::ViewId kViewBloomDownsample0 = 6;   // 6..10
        static constexpr bgfx::ViewId kViewBloomUpsample0 = 11;  // 11..14
        static constexpr bgfx::ViewId kViewTonemap = 16;



        // ===== Helpers: targets =====
        void createViewportTargets(u32 w, u32 h);
        void createBloomTargets(u32 w, u32 h);
        void destroyBloomTargets();
        void destroyViewportTargets();

        // ===== Helpers: shadow =====
        void createShadowResources();
        void destroyShadowResources();
        void renderShadowPass(ecs::World& world);

        // ===== Render passes (реализованы в отдельных .cpp) =====
        void collectLights(ecs::World& world);   // Renderer_Lights.cpp
        void uploadEnvironment();                // Renderer_Mesh.cpp
        void uploadShadowUniforms();             // Renderer_Mesh.cpp
        void renderMeshes(ecs::World& world);    // Renderer_Mesh.cpp
        void renderDebug(ecs::World& world);     // Renderer_Debug.cpp
        void renderBloom(bool& outBloomReady);   // Renderer_Bloom.cpp
        void renderTonemap(bool bloomReady);     // Renderer_Bloom.cpp
        void drawGrid();                         // Renderer_Grid.cpp


        // ===== Device =====
        rhi::Device* m_device = nullptr;

        // ===== Shaders =====
        ShaderRegistry          m_shaders;
        ShaderRegistry::Entry* m_defaultEntry = nullptr;
        ShaderRegistry::Entry* m_gridEntry = nullptr;
        ShaderRegistry::Entry* m_skyEntry = nullptr;



        // ===== Bloom =====
        static constexpr int kBloomMipCount = 5;

        
        bgfx::TextureHandle     m_bloomTex[kBloomMipCount];
        bgfx::FrameBufferHandle m_bloomFB[kBloomMipCount];
        bool m_bloomTargetsCreated = false;  // <-- NEW
        uint16_t                 m_bloomWidth[kBloomMipCount] = {};
        uint16_t                 m_bloomHeight[kBloomMipCount] = {};

        const ShaderRegistry::Entry* m_bloomBrightEntry = nullptr;
        const ShaderRegistry::Entry* m_bloomDownsampleEntry = nullptr;
        const ShaderRegistry::Entry* m_bloomUpsampleEntry = nullptr;

       

        // UI params
        bool m_bloomEnabled = true;
        f32  m_bloomThreshold = 1.0f;
        f32  m_bloomKnee = 0.5f;
        f32  m_bloomIntensity = 0.6f;
        f32  m_bloomFilterRadius = 1.0f;

        // View IDs для bloom
        // kViewBloomBright + downsample/upsample passes

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


        // Параметры
        f32  m_exposure = 1.0f;
        int  m_tonemapOperator = 1;   // 0=Reinhard, 1=ACES, 2=None
        f32  m_gamma = 2.2f;

        // --- Shadow map ---
        bool                       m_shadowsEnabled = true;
        bool                       m_softShadows = true;
        u32                        m_shadowMapSize = 2048;
        f32                        m_shadowDistance = 50.0f;
        f32                        m_shadowBias = 0.005f;
        f32 m_shadowMtx[16]{}; // light view-proj для main pass (Шаг 5)

        bgfx::TextureHandle        m_shadowMap = BGFX_INVALID_HANDLE;
        bgfx::FrameBufferHandle    m_shadowFB = BGFX_INVALID_HANDLE;
        const ShaderRegistry::Entry* m_shadowEntry = nullptr;



        DebugDraw                m_debug;
        const ShaderRegistry::Entry* m_debugEntry = nullptr;

        // Текущая выделенная entity — для рисования зоны действия лампы.
        // Renderer об этом узнает через сеттер из EditorUI.
        entt::entity             m_selectedEntity = entt::null;

        // ===== Viewport FB =====
        bgfx::FrameBufferHandle m_vpFB = BGFX_INVALID_HANDLE;
        bgfx::TextureHandle     m_vpColor = BGFX_INVALID_HANDLE;
        bgfx::TextureHandle     m_vpDepth = BGFX_INVALID_HANDLE;
        // HDR -> LDR
        bgfx::FrameBufferHandle m_vpFBLDR = BGFX_INVALID_HANDLE;
        bgfx::TextureHandle     m_vpColorLDR = BGFX_INVALID_HANDLE;
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
        bgfx::UniformHandle m_uPbrParams = BGFX_INVALID_HANDLE;  // NEW
        bgfx::UniformHandle m_uEmissive = BGFX_INVALID_HANDLE;  // NEW

        // ===== Grid =====
        bgfx::UniformHandle m_uGridInvViewProj = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle m_uGridParams = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle m_uGridViewProj = BGFX_INVALID_HANDLE;   // ← НОВОЕ

        // ===== Environment uniforms =====
        bgfx::UniformHandle m_uFogParams = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle m_uFogColor = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle m_uAmbientColor = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle m_uCameraPos = BGFX_INVALID_HANDLE;


        // ===== Bloom =====
        bgfx::UniformHandle m_uBloomSource = BGFX_INVALID_HANDLE; // sampler 0
        bgfx::UniformHandle m_uBloomTexel = BGFX_INVALID_HANDLE; // vec4
        bgfx::UniformHandle m_uBloomParams = BGFX_INVALID_HANDLE; // vec4
        bgfx::UniformHandle m_uBloomSampler = BGFX_INVALID_HANDLE; // s_bloom sampler 1 для tonemap

        // Tone mapping
        const ShaderRegistry::Entry* m_tonemapEntry = nullptr;   // или твой тип для loadProgram
        bgfx::UniformHandle m_uTonemapParams = BGFX_INVALID_HANDLE; // x=exposure, y=operator, z=gamma
        bgfx::UniformHandle m_uHdrTexSampler = BGFX_INVALID_HANDLE;

        bgfx::TextureHandle m_blackPixel = BGFX_INVALID_HANDLE;

        // Uniforms для main shader (sampling)
        bgfx::UniformHandle        m_uShadowMap = BGFX_INVALID_HANDLE; // sampler
        bgfx::UniformHandle        m_uShadowMtx = BGFX_INVALID_HANDLE; // mat4 light VP
        bgfx::UniformHandle        m_uShadowParams = BGFX_INVALID_HANDLE; // vec4(bias, texelSize, soft, enabled)


        // ===== Light cache (переиспользуется между кадрами) =====
        struct LightCache {
            f32 dirDir[kMaxDirLights * 4]{};
            f32 dirColor[kMaxDirLights * 4]{};
            u32 dirCount = 0;
            f32 ambient = 0.f;

            f32 pointPos[kMaxPointLights * 4]{};
            f32 pointColor[kMaxPointLights * 4]{};
            u32 pointCount = 0;

            f32 spotPos[kMaxSpotLights * 4]{};
            f32 spotDir[kMaxSpotLights * 4]{};
            f32 spotColor[kMaxSpotLights * 4]{};
            f32 spotParams[kMaxSpotLights * 4]{};
            u32 spotCount = 0;
        };
        LightCache m_lights;

    };

} // namespace grit::render