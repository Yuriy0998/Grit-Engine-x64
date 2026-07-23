#include "Renderer.h"
#include "Renderer_Internal.h"
#include "../rhi/Device.h"
#include "../core/Log.h"
#include "../ecs/World.h"
#include <bgfx/bgfx.h>
#include <bx/math.h>

namespace grit::render {

    bool Renderer::init(rhi::Device* dev, FileWatcher* watcher) {
        m_device = dev;
        for (int i = 0; i < kBloomMipCount; ++i) {
            m_bloomTex[i] = BGFX_INVALID_HANDLE;
            m_bloomFB[i] = BGFX_INVALID_HANDLE;
        }
        createViewportTargets(dev->width(), dev->height());
        m_camera.aspect = (m_vpHeight > 0)
            ? (f32)m_vpWidth / (f32)m_vpHeight : 16.f / 9.f;

        m_shaders.init(watcher);
        const uint32_t blackPixel = 0xFF000000u;
        m_blackPixel = bgfx::createTexture2D(
            1, 1, false, 1, bgfx::TextureFormat::RGBA8,
            BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE,
            bgfx::copy(&blackPixel, 4));

        // --- Default lit shader ---
        m_defaultEntry = m_shaders.loadProgram("vs_textured", "fs_textured");
        if (!m_defaultEntry || !bgfx::isValid(m_defaultEntry->program)) {
            GRIT_ERROR("Renderer: cannot load vs_textured/fs_textured");
            return false;
        }

        // --- Sky shader + skydome ---
        m_skyEntry = m_shaders.loadProgram("vs_sky", "fs_sky");
        if (m_skyEntry && bgfx::isValid(m_skyEntry->program)) {
            m_skydome.init(m_skyEntry->program);
        }
        else {
            GRIT_WARN("Renderer: sky shader failed to load");
        }

        // --- Grid shader ---
        m_gridEntry = m_shaders.loadProgram("vs_grid", "fs_grid");
        if (!m_gridEntry || !bgfx::isValid(m_gridEntry->program)) {
            GRIT_WARN("Renderer: grid shader failed to load");
        }
        m_uGridInvViewProj = bgfx::createUniform("gridInvVP", bgfx::UniformType::Mat4);
        m_uGridParams = bgfx::createUniform("gridParams", bgfx::UniformType::Vec4);
        m_uGridViewProj = bgfx::createUniform("gridVP", bgfx::UniformType::Mat4);   // ← НОВОЕ

        // --- Debug draw shader ---
        m_debugEntry = m_shaders.loadProgram("vs_debug", "fs_debug");
        if (m_debugEntry && bgfx::isValid(m_debugEntry->program)) {
            m_debug.init(m_debugEntry->program);
        }
        else {
            GRIT_WARN("Renderer: debug shader failed to load");
        }


        // --- Common ---
        m_uTexColor = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);

        // --- Lights ---
        m_uDirLightDir = bgfx::createUniform("u_dirLightDir", bgfx::UniformType::Vec4, kMaxDirLights);
        m_uDirLightColor = bgfx::createUniform("u_dirLightColor", bgfx::UniformType::Vec4, kMaxDirLights);
        m_uDirLightCount = bgfx::createUniform("u_dirLightCount", bgfx::UniformType::Vec4);

        m_uPointLightPos = bgfx::createUniform("u_pointLightPos", bgfx::UniformType::Vec4, kMaxPointLights);
        m_uPointLightColor = bgfx::createUniform("u_pointLightColor", bgfx::UniformType::Vec4, kMaxPointLights);
        m_uPointLightCount = bgfx::createUniform("u_pointLightCount", bgfx::UniformType::Vec4);

        m_uSpotLightPos = bgfx::createUniform("u_spotLightPos", bgfx::UniformType::Vec4, kMaxSpotLights);
        m_uSpotLightDir = bgfx::createUniform("u_spotLightDir", bgfx::UniformType::Vec4, kMaxSpotLights);
        m_uSpotLightColor = bgfx::createUniform("u_spotLightColor", bgfx::UniformType::Vec4, kMaxSpotLights);
        m_uSpotLightParams = bgfx::createUniform("u_spotLightParams", bgfx::UniformType::Vec4, kMaxSpotLights);
        m_uSpotLightCount = bgfx::createUniform("u_spotLightCount", bgfx::UniformType::Vec4);

        // --- Bloom ---
        m_bloomBrightEntry = m_shaders.loadProgram("vs_fullscreen", "fs_bloom_bright");
        m_bloomDownsampleEntry = m_shaders.loadProgram("vs_fullscreen", "fs_bloom_downsample");
        m_bloomUpsampleEntry = m_shaders.loadProgram("vs_fullscreen", "fs_bloom_upsample");

        if (!m_bloomBrightEntry || !bgfx::isValid(m_bloomBrightEntry->program) ||
            !m_bloomDownsampleEntry || !bgfx::isValid(m_bloomDownsampleEntry->program) ||
            !m_bloomUpsampleEntry || !bgfx::isValid(m_bloomUpsampleEntry->program)) {
            GRIT_WARN("Renderer: bloom shaders failed to load");
        }

        m_uBloomSource = bgfx::createUniform("s_source", bgfx::UniformType::Sampler);
        m_uBloomTexel = bgfx::createUniform("u_bloomTexel", bgfx::UniformType::Vec4);
        m_uBloomParams = bgfx::createUniform("u_bloomParams", bgfx::UniformType::Vec4);
        m_uBloomSampler = bgfx::createUniform("s_bloom", bgfx::UniformType::Sampler);

        // --- Material ---
        m_uBaseColor = bgfx::createUniform("u_baseColor", bgfx::UniformType::Vec4);
        m_uTilingOffset = bgfx::createUniform("u_tilingOffset", bgfx::UniformType::Vec4);
        m_uPbrParams = bgfx::createUniform("u_pbrParams", bgfx::UniformType::Vec4); // NEW
        m_uEmissive = bgfx::createUniform("u_emissive", bgfx::UniformType::Vec4); // NEW

        // 1x1 white pixel
        const u32 whitePixel = 0xFFFFFFFFu;
        m_whitePixel = bgfx::createTexture2D(
            1, 1, false, 1, bgfx::TextureFormat::RGBA8,
            BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE,
            bgfx::copy(&whitePixel, 4));

        // --- Environment uniforms ---
        m_uFogParams = bgfx::createUniform("u_fogParams", bgfx::UniformType::Vec4);
        m_uFogColor = bgfx::createUniform("u_fogColor", bgfx::UniformType::Vec4);
        m_uAmbientColor = bgfx::createUniform("u_ambientColor", bgfx::UniformType::Vec4);
        m_uCameraPos = bgfx::createUniform("u_cameraPos", bgfx::UniformType::Vec4);

        // --- Shadow uniforms ---
        m_uShadowMap = bgfx::createUniform("s_shadowMap", bgfx::UniformType::Sampler);
        m_uShadowMtx = bgfx::createUniform("u_shadowMtx", bgfx::UniformType::Mat4);
        m_uShadowParams = bgfx::createUniform("u_shadowParams", bgfx::UniformType::Vec4);

        // --- Shadow shader (depth-only) ---
        m_shadowEntry = m_shaders.loadProgram("vs_shadow", "fs_shadow");
        if (!m_shadowEntry || !bgfx::isValid(m_shadowEntry->program)) {
            GRIT_WARN("Renderer: shadow shader failed to load");
        }

        createShadowResources();

        m_camera.position = { 0.f, 1.5f, -4.f };
        m_camera.target = { 0.f, 0.f,  0.f };

        // --- Tone mapping ---
        m_tonemapEntry = m_shaders.loadProgram("vs_fullscreen", "fs_tonemap");
        if (!m_tonemapEntry || !bgfx::isValid(m_tonemapEntry->program)) {
            GRIT_WARN("Renderer: tonemap shader failed to load");
        }
        m_uTonemapParams = bgfx::createUniform("u_tonemapParams", bgfx::UniformType::Vec4);
        m_uHdrTexSampler = bgfx::createUniform("s_hdr", bgfx::UniformType::Sampler);

        
        GRIT_INFO("Renderer initialized");
        return true;
    }

    void Renderer::shutdown() {
        if (bgfx::isValid(m_whitePixel)) { bgfx::destroy(m_whitePixel);       m_whitePixel = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uTexColor)) { bgfx::destroy(m_uTexColor);        m_uTexColor = BGFX_INVALID_HANDLE; }

        if (bgfx::isValid(m_uTonemapParams)) { bgfx::destroy(m_uTonemapParams); m_uTonemapParams = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uHdrTexSampler)) { bgfx::destroy(m_uHdrTexSampler); m_uHdrTexSampler = BGFX_INVALID_HANDLE; }

        if (bgfx::isValid(m_uDirLightDir)) { bgfx::destroy(m_uDirLightDir);     m_uDirLightDir = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uDirLightColor)) { bgfx::destroy(m_uDirLightColor);   m_uDirLightColor = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uDirLightCount)) { bgfx::destroy(m_uDirLightCount);   m_uDirLightCount = BGFX_INVALID_HANDLE; }

        if (bgfx::isValid(m_uPointLightPos)) { bgfx::destroy(m_uPointLightPos);   m_uPointLightPos = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uPointLightColor)) { bgfx::destroy(m_uPointLightColor); m_uPointLightColor = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uPointLightCount)) { bgfx::destroy(m_uPointLightCount); m_uPointLightCount = BGFX_INVALID_HANDLE; }

        if (bgfx::isValid(m_uSpotLightPos)) { bgfx::destroy(m_uSpotLightPos);    m_uSpotLightPos = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uSpotLightDir)) { bgfx::destroy(m_uSpotLightDir);    m_uSpotLightDir = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uSpotLightColor)) { bgfx::destroy(m_uSpotLightColor);  m_uSpotLightColor = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uSpotLightParams)) { bgfx::destroy(m_uSpotLightParams); m_uSpotLightParams = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uSpotLightCount)) { bgfx::destroy(m_uSpotLightCount);  m_uSpotLightCount = BGFX_INVALID_HANDLE; }

        if (bgfx::isValid(m_uBaseColor)) { bgfx::destroy(m_uBaseColor);       m_uBaseColor = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uTilingOffset)) { bgfx::destroy(m_uTilingOffset);    m_uTilingOffset = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uPbrParams)) { bgfx::destroy(m_uPbrParams); m_uPbrParams = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uEmissive)) { bgfx::destroy(m_uEmissive);  m_uEmissive = BGFX_INVALID_HANDLE; }

        if (bgfx::isValid(m_uGridInvViewProj)) { bgfx::destroy(m_uGridInvViewProj); m_uGridInvViewProj = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uGridParams)) { bgfx::destroy(m_uGridParams);      m_uGridParams = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uGridViewProj)) { bgfx::destroy(m_uGridViewProj); m_uGridViewProj = BGFX_INVALID_HANDLE; }

        if (bgfx::isValid(m_uFogParams)) { bgfx::destroy(m_uFogParams);       m_uFogParams = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uFogColor)) { bgfx::destroy(m_uFogColor);        m_uFogColor = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uAmbientColor)) { bgfx::destroy(m_uAmbientColor);    m_uAmbientColor = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uCameraPos)) { bgfx::destroy(m_uCameraPos);       m_uCameraPos = BGFX_INVALID_HANDLE; }

        if (bgfx::isValid(m_uShadowMap)) { bgfx::destroy(m_uShadowMap);    m_uShadowMap = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uShadowMtx)) { bgfx::destroy(m_uShadowMtx);    m_uShadowMtx = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uShadowParams)) { bgfx::destroy(m_uShadowParams); m_uShadowParams = BGFX_INVALID_HANDLE; }

        if (bgfx::isValid(m_uBloomSource)) { bgfx::destroy(m_uBloomSource);  m_uBloomSource = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uBloomTexel)) { bgfx::destroy(m_uBloomTexel);   m_uBloomTexel = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uBloomParams)) { bgfx::destroy(m_uBloomParams);  m_uBloomParams = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uBloomSampler)) { bgfx::destroy(m_uBloomSampler); m_uBloomSampler = BGFX_INVALID_HANDLE; }

        if (bgfx::isValid(m_blackPixel)) { bgfx::destroy(m_blackPixel); m_blackPixel = BGFX_INVALID_HANDLE; }


        destroyShadowResources();

        // Skydome владеет своими uniforms; программу не удаляем — её удалит ShaderManager
        m_skydome.shutdown();
        // m_skyEntry принадлежит ShaderRegistry — он сам удалит программу
        m_debug.shutdown();

        destroyViewportTargets();

        GRIT_INFO("Renderer shutdown");
    }

    void Renderer::render(ecs::World& world, f32 dt)
    {
        if (m_vpWidth == 0 || m_vpHeight == 0 || !bgfx::isValid(m_vpFB)) return;

        // 0) Shadow pass
        renderShadowPass(world);

        // 1) Time of Day → Environment
        m_timeOfDay.update(dt);
        m_timeOfDay.apply(m_env);

        // 2) Backbuffer
        bgfx::setViewRect(kViewBackbuffer, 0, 0,
            (uint16_t)m_device->width(), (uint16_t)m_device->height());
        bgfx::setViewClear(kViewBackbuffer,
            BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x202225ff, 1.0f, 0);
        bgfx::touch(kViewBackbuffer);

        // 3) Камера + матрицы
        m_camera.aspect = (f32)m_vpWidth / (f32)m_vpHeight;
        m_camera.buildView(m_viewMtx);
        m_camera.buildProj(m_projMtx, bgfx::getCaps()->homogeneousDepth);

        // 4) Sky
        bgfx::setViewFrameBuffer(kViewSky, m_vpFB);
        bgfx::setViewRect(kViewSky, 0, 0, (uint16_t)m_vpWidth, (uint16_t)m_vpHeight);
        bgfx::setViewClear(kViewSky,
            BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x202225ff, 1.0f, 0);
        bgfx::setViewTransform(kViewSky, m_viewMtx, m_projMtx);
        bgfx::touch(kViewSky);
        if (m_skyEntry && bgfx::isValid(m_skyEntry->program))
            m_skydome.render(kViewSky, m_env, m_camera.position);

        // 5) Scene view setup
        bgfx::setViewFrameBuffer(kViewScene, m_vpFB);
        bgfx::setViewRect(kViewScene, 0, 0, (uint16_t)m_vpWidth, (uint16_t)m_vpHeight);
        bgfx::setViewClear(kViewScene, BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);
        bgfx::setViewTransform(kViewScene, m_viewMtx, m_projMtx);
        bgfx::touch(kViewScene);

        // 6) Lights → uniforms
        collectLights(world);

        // 7) Environment + 7.5) Shadow uniforms
        uploadEnvironment();
        uploadShadowUniforms();

        // 8) Meshes
        renderMeshes(world);

        // Grid (после мешей — depth уже заполнен)
        if (m_showGrid) drawGrid();

        // 9) Debug overlay
        renderDebug(world);

        // 9.5) Bloom
        bool bloomReady = false;
        renderBloom(bloomReady);

        // 10) Tone mapping
        renderTonemap(bloomReady);
    }

} // namespace grit::render






