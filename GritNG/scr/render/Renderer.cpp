#include "Renderer.h"
#include "../rhi/Device.h"
#include "../rhi/Resources.h"
#include "../ecs/World.h"
#include "../core/Log.h"
#include "../core/Math.h"
#include "../ecs/Light.h"

#include "StaticMesh.h"
#include "Material.h"
#include "Texture.h"

#include <bgfx/bgfx.h>
#include <bx/math.h>
#include <cmath>
#include <cstring>

namespace grit::render {

    bool Renderer::init(rhi::Device* dev, FileWatcher* watcher) {
        m_device = dev;

        createViewportTargets(dev->width(), dev->height());

        m_camera.aspect = (m_vpHeight > 0)
            ? (f32)m_vpWidth / (f32)m_vpHeight
            : 16.f / 9.f;

        m_shaders.init(watcher);

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

        // --- Material ---
        m_uBaseColor = bgfx::createUniform("u_baseColor", bgfx::UniformType::Vec4);
        m_uTilingOffset = bgfx::createUniform("u_tilingOffset", bgfx::UniformType::Vec4);

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

        m_camera.position = { 0.f, 1.5f, -4.f };
        m_camera.target = { 0.f, 0.f,  0.f };

        GRIT_INFO("Renderer initialized");
        return true;
    }

    void Renderer::drawGrid()
    {
        if (!m_gridEntry || !bgfx::isValid(m_gridEntry->program)) return;

        static bgfx::VertexLayout layout;
        static bool layoutInit = false;
        if (!layoutInit) {
            layout.begin()
                .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
                .end();
            layoutInit = true;
        }

        struct PosVtx { float x, y, z; };
        const uint32_t kNumVerts = 6;

        if (kNumVerts != bgfx::getAvailTransientVertexBuffer(kNumVerts, layout))
            return;

        bgfx::TransientVertexBuffer tvb;
        bgfx::allocTransientVertexBuffer(&tvb, kNumVerts, layout);
        PosVtx* v = (PosVtx*)tvb.data;
        v[0] = { -1.f, -1.f, 0.f }; v[1] = { 1.f, -1.f, 0.f }; v[2] = { 1.f,  1.f, 0.f };
        v[3] = { -1.f, -1.f, 0.f }; v[4] = { 1.f,  1.f, 0.f }; v[5] = { -1.f,  1.f, 0.f };

        float vp[16];
        bx::mtxMul(vp, m_viewMtx, m_projMtx);
        float invVp[16];
        bx::mtxInverse(invVp, vp);
        bgfx::setUniform(m_uGridInvViewProj, invVp);

        float gridParams[4] = { m_camera.nearPlane, m_camera.farPlane, 0.f, 0.f };
        bgfx::setUniform(m_uGridParams, gridParams);

        bgfx::setVertexBuffer(0, &tvb);

        bgfx::setState(
            BGFX_STATE_WRITE_RGB
            | BGFX_STATE_WRITE_A
            | BGFX_STATE_DEPTH_TEST_ALWAYS
            | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA)
        );

        bgfx::submit(kViewScene, m_gridEntry->program);
    }

    void Renderer::shutdown() {
        if (bgfx::isValid(m_whitePixel)) { bgfx::destroy(m_whitePixel);       m_whitePixel = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uTexColor)) { bgfx::destroy(m_uTexColor);        m_uTexColor = BGFX_INVALID_HANDLE; }

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

        if (bgfx::isValid(m_uGridInvViewProj)) { bgfx::destroy(m_uGridInvViewProj); m_uGridInvViewProj = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uGridParams)) { bgfx::destroy(m_uGridParams);      m_uGridParams = BGFX_INVALID_HANDLE; }

        if (bgfx::isValid(m_uFogParams)) { bgfx::destroy(m_uFogParams);       m_uFogParams = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uFogColor)) { bgfx::destroy(m_uFogColor);        m_uFogColor = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uAmbientColor)) { bgfx::destroy(m_uAmbientColor);    m_uAmbientColor = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uCameraPos)) { bgfx::destroy(m_uCameraPos);       m_uCameraPos = BGFX_INVALID_HANDLE; }

        // Skydome владеет своими uniforms; программу не удаляем — её удалит ShaderManager
        m_skydome.shutdown();
        // m_skyEntry принадлежит ShaderRegistry — он сам удалит программу
        m_debug.shutdown();

        destroyViewportTargets();

        GRIT_INFO("Renderer shutdown");
    }

    void Renderer::createViewportTargets(u32 w, u32 h)
    {
        if (w == 0 || h == 0) return;
        if (w == m_vpWidth && h == m_vpHeight && bgfx::isValid(m_vpFB)) return;

        destroyViewportTargets();

        const uint64_t colorFlags =
            BGFX_TEXTURE_RT
            | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP
            | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT;
        const uint64_t depthFlags = BGFX_TEXTURE_RT_WRITE_ONLY;

        m_vpColor = bgfx::createTexture2D(
            (uint16_t)w, (uint16_t)h, false, 1,
            bgfx::TextureFormat::BGRA8, colorFlags);

        m_vpDepth = bgfx::createTexture2D(
            (uint16_t)w, (uint16_t)h, false, 1,
            bgfx::TextureFormat::D24S8, depthFlags);

        bgfx::TextureHandle atts[2] = { m_vpColor, m_vpDepth };
        m_vpFB = bgfx::createFrameBuffer(2, atts, false);

        m_vpWidth = w;
        m_vpHeight = h;
    }

    void Renderer::destroyViewportTargets()
    {
        if (bgfx::isValid(m_vpFB)) { bgfx::destroy(m_vpFB);    m_vpFB = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_vpColor)) { bgfx::destroy(m_vpColor); m_vpColor = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_vpDepth)) { bgfx::destroy(m_vpDepth); m_vpDepth = BGFX_INVALID_HANDLE; }
        m_vpWidth = m_vpHeight = 0;
    }

    void Renderer::resizeViewport(u32 w, u32 h)
    {
        createViewportTargets(w, h);
    }

    void Renderer::render(ecs::World& world, f32 dt)
    {
        if (m_vpWidth == 0 || m_vpHeight == 0 || !bgfx::isValid(m_vpFB)) return;

        // ============================================================
        // 1) Time of Day → Environment
        // ============================================================
        m_timeOfDay.update(dt);
        m_timeOfDay.apply(m_env);

        // ============================================================
        // 2) Backbuffer (view 0): просто touch, чтобы bgfx был доволен
        // ============================================================
        bgfx::setViewRect(kViewBackbuffer, 0, 0,
            (uint16_t)m_device->width(),
            (uint16_t)m_device->height());
        bgfx::setViewClear(kViewBackbuffer,
            BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
            0x1a1a1aff, 1.0f, 0);
        bgfx::touch(kViewBackbuffer);

        // ============================================================
        // 3) Камера + матрицы
        // ============================================================
        m_camera.aspect = (f32)m_vpWidth / (f32)m_vpHeight;
        m_camera.buildView(m_viewMtx);
        m_camera.buildProj(m_projMtx, bgfx::getCaps()->homogeneousDepth);

        // ============================================================
        // 4) View Sky — рисуем небо в тот же FB, без записи Z
        // ============================================================
        bgfx::setViewFrameBuffer(kViewSky, m_vpFB);
        bgfx::setViewRect(kViewSky, 0, 0,
            (uint16_t)m_vpWidth, (uint16_t)m_vpHeight);
        // Очищаем и цвет (на случай если skydome не покроет всё) и depth
        bgfx::setViewClear(kViewSky,
            BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
            0x202225ff, 1.0f, 0);
        bgfx::setViewTransform(kViewSky, m_viewMtx, m_projMtx);
        bgfx::touch(kViewSky);

        if (m_skyEntry && bgfx::isValid(m_skyEntry->program)) {
            m_skydome.render(kViewSky, m_env, m_camera.position);
        }

        // ============================================================
        // 5) View Scene — основная геометрия
        // ============================================================
        bgfx::setViewFrameBuffer(kViewScene, m_vpFB);
        bgfx::setViewRect(kViewScene, 0, 0,
            (uint16_t)m_vpWidth, (uint16_t)m_vpHeight);
        // Цвет НЕ очищаем — небо уже нарисовано. Только depth.
        bgfx::setViewClear(kViewScene,
            BGFX_CLEAR_DEPTH,
            0x000000ff, 1.0f, 0);
        bgfx::setViewTransform(kViewScene, m_viewMtx, m_projMtx);
        bgfx::touch(kViewScene);

        // --- Grid ---
        if (m_showGrid) drawGrid();

        // ============================================================
        // 6) Сбор directional lights
        // ============================================================
        f32 dirLightDir[kMaxDirLights * 4] = {};
        f32 dirLightColor[kMaxDirLights * 4] = {};
        u32 dirLightCount = 0;
        f32 ambient = 0.0f;

        {
            auto lview = world.registry().view<grit::ecs::LightDirectional>();
            for (auto ent : lview) {
                if (dirLightCount >= kMaxDirLights) break;

                auto& L = lview.get<grit::ecs::LightDirectional>(ent);

                f32 lx = L.direction.x, ly = L.direction.y, lz = L.direction.z;
                f32 len = std::sqrt(lx * lx + ly * ly + lz * lz);
                if (len < 1e-6f) len = 1.0f;

                const u32 i = dirLightCount * 4;
                dirLightDir[i + 0] = lx / len;
                dirLightDir[i + 1] = ly / len;
                dirLightDir[i + 2] = lz / len;
                dirLightDir[i + 3] = 0.0f;

                dirLightColor[i + 0] = L.color.x;
                dirLightColor[i + 1] = L.color.y;
                dirLightColor[i + 2] = L.color.z;
                dirLightColor[i + 3] = L.intensity;

                if (dirLightCount == 0) ambient = L.ambient;
                ++dirLightCount;
            }
        }

        // ============================================================
        // 6.5) Point lights
        // ============================================================
        f32 pointPos[kMaxPointLights * 4] = {};
        f32 pointColor[kMaxPointLights * 4] = {};
        u32 pointCount = 0;
        {
            auto lview = world.registry().view<grit::ecs::Transform, grit::ecs::LightPoint>();
            for (auto ent : lview) {
                if (pointCount >= kMaxPointLights) break;
                const auto& tr = lview.get<grit::ecs::Transform>(ent);
                const auto& L = lview.get<grit::ecs::LightPoint>(ent);

                const u32 i = pointCount * 4;
                pointPos[i + 0] = tr.position.x;
                pointPos[i + 1] = tr.position.y;
                pointPos[i + 2] = tr.position.z;
                pointPos[i + 3] = L.range;

                pointColor[i + 0] = L.color.x;
                pointColor[i + 1] = L.color.y;
                pointColor[i + 2] = L.color.z;
                pointColor[i + 3] = L.intensity;
                ++pointCount;
            }
        }
        {
            f32 cnt[4] = { (f32)pointCount, 0, 0, 0 };
            bgfx::setUniform(m_uPointLightPos, pointPos, kMaxPointLights);
            bgfx::setUniform(m_uPointLightColor, pointColor, kMaxPointLights);
            bgfx::setUniform(m_uPointLightCount, cnt);
        }

        // ============================================================
        // 6.6) Spot lights
        // ============================================================
        f32 spotPos[kMaxSpotLights * 4] = {};
        f32 spotDir[kMaxSpotLights * 4] = {};
        f32 spotColor[kMaxSpotLights * 4] = {};
        f32 spotParams[kMaxSpotLights * 4] = {};
        u32 spotCount = 0;
        {
            auto lview = world.registry().view<grit::ecs::Transform, grit::ecs::LightSpot>();
            for (auto ent : lview) {
                if (spotCount >= kMaxSpotLights) break;
                const auto& tr = lview.get<grit::ecs::Transform>(ent);
                const auto& L = lview.get<grit::ecs::LightSpot>(ent);

                const u32 i = spotCount * 4;
                spotPos[i + 0] = tr.position.x;
                spotPos[i + 1] = tr.position.y;
                spotPos[i + 2] = tr.position.z;
                spotPos[i + 3] = L.range;

                // Нормализуем direction (в локальной системе пока)
                f32 dx = L.direction.x, dy = L.direction.y, dz = L.direction.z;
                f32 dl = std::sqrt(dx * dx + dy * dy + dz * dz);
                if (dl < 1e-6f) dl = 1.0f;
                spotDir[i + 0] = dx / dl;
                spotDir[i + 1] = dy / dl;
                spotDir[i + 2] = dz / dl;
                spotDir[i + 3] = 0.0f;

                spotColor[i + 0] = L.color.x;
                spotColor[i + 1] = L.color.y;
                spotColor[i + 2] = L.color.z;
                spotColor[i + 3] = L.intensity;

                spotParams[i + 0] = std::cos(L.innerCone);
                spotParams[i + 1] = std::cos(L.outerCone);
                spotParams[i + 2] = 0.0f;
                spotParams[i + 3] = 0.0f;
                ++spotCount;
            }
        }
        {
            f32 cnt[4] = { (f32)spotCount, 0, 0, 0 };
            bgfx::setUniform(m_uSpotLightPos, spotPos, kMaxSpotLights);
            bgfx::setUniform(m_uSpotLightDir, spotDir, kMaxSpotLights);
            bgfx::setUniform(m_uSpotLightColor, spotColor, kMaxSpotLights);
            bgfx::setUniform(m_uSpotLightParams, spotParams, kMaxSpotLights);
            bgfx::setUniform(m_uSpotLightCount, cnt);
        }

        // Если в сцене нет directional light — используем "солнце" из TimeOfDay
        if (dirLightCount == 0) {
            const u32 i = 0;
            dirLightDir[i + 0] = m_env.sunDirection.x;
            dirLightDir[i + 1] = m_env.sunDirection.y;
            dirLightDir[i + 2] = m_env.sunDirection.z;
            dirLightDir[i + 3] = 0.0f;

            dirLightColor[i + 0] = m_env.sunColor.x;
            dirLightColor[i + 1] = m_env.sunColor.y;
            dirLightColor[i + 2] = m_env.sunColor.z;
            dirLightColor[i + 3] = m_env.sunIntensity;

            dirLightCount = 1;
            ambient = 0.15f;
        }

        f32 lightCountVec[4] = { (f32)dirLightCount, ambient, 0.0f, 0.0f };
        bgfx::setUniform(m_uDirLightDir, dirLightDir, kMaxDirLights);
        bgfx::setUniform(m_uDirLightColor, dirLightColor, kMaxDirLights);
        bgfx::setUniform(m_uDirLightCount, lightCountVec);

        // ============================================================
        // 7) Environment uniforms (раз в кадр)
        // ============================================================
        {
            const float fogParams[4] = {
                m_env.fogStart,
                m_env.fogEnd,
                m_env.fogDensity,
                (float)(int)m_env.fogMode
            };
            const float fogColor[4] = {
                m_env.fogColor.x, m_env.fogColor.y, m_env.fogColor.z,
                m_env.fogEnabled ? 1.0f : 0.0f
            };
            const float ambientCol[4] = {
                m_env.ambientColor.x * m_env.ambientIntensity,
                m_env.ambientColor.y * m_env.ambientIntensity,
                m_env.ambientColor.z * m_env.ambientIntensity,
                1.0f
            };
            const float camPos[4] = {
                m_camera.position.x,
                m_camera.position.y,
                m_camera.position.z,
                0.0f
            };

            bgfx::setUniform(m_uFogParams, fogParams);
            bgfx::setUniform(m_uFogColor, fogColor);
            bgfx::setUniform(m_uAmbientColor, ambientCol);
            bgfx::setUniform(m_uCameraPos, camPos);
        }

        // ============================================================
        // 8) Рендер мешей
        // ============================================================
        auto view2 = world.registry().view<grit::ecs::Transform, grit::ecs::MeshRenderer>();
        for (auto ent : view2) {
            auto& tr = view2.get<grit::ecs::Transform>(ent);
            auto& mr = view2.get<grit::ecs::MeshRenderer>(ent);
            if (!mr.visible || !mr.mesh) continue;

            bgfx::ProgramHandle prog = defaultProgram();
            if (!bgfx::isValid(prog)) continue;

            const auto& vb = mr.mesh->vb().handle();
            const auto& ib = mr.mesh->ib().handle();

            auto submitRange = [&](const std::shared_ptr<Material>& mat,
                u32 indexStart, u32 indexCount)
                {
                    bgfx::setTransform(tr.worldMatrix);
                    bgfx::setVertexBuffer(0, vb);

                    if (indexCount == UINT32_MAX) {
                        bgfx::setIndexBuffer(ib);
                    }
                    else {
                        bgfx::setIndexBuffer(ib, indexStart, indexCount);
                    }

                    bgfx::TextureHandle tex = m_whitePixel;
                    if (mat && mat->albedo && mat->albedo->valid()) {
                        tex = mat->albedo->handle();
                    }
                    bgfx::setTexture(0, m_uTexColor, tex);

                    f32 baseColor[4] = { 1, 1, 1, 1 };
                    f32 tilingOffset[4] = { 1, 1, 0, 0 };
                    if (mat) {
                        baseColor[0] = mat->baseColor.x;
                        baseColor[1] = mat->baseColor.y;
                        baseColor[2] = mat->baseColor.z;
                        baseColor[3] = mat->baseColor.w;

                        tilingOffset[0] = mat->tilingOffset.x;
                        tilingOffset[1] = mat->tilingOffset.y;
                        tilingOffset[2] = mat->tilingOffset.z;
                        tilingOffset[3] = mat->tilingOffset.w;
                    }
                    bgfx::setUniform(m_uBaseColor, baseColor);
                    bgfx::setUniform(m_uTilingOffset, tilingOffset);

                    bgfx::setState(BGFX_STATE_DEFAULT);
                    bgfx::submit(kViewScene, prog);
                };

            auto pickMaterial = [&](u32 materialIndex) -> std::shared_ptr<Material> {
                if (mr.materials.empty()) return nullptr;
                const size_t n = mr.materials.size();
                const size_t i = (materialIndex < n) ? (size_t)materialIndex : (n - 1);
                return mr.materials[i];
                };

            const auto& subs = mr.mesh->submeshes();
            if (subs.empty()) {
                submitRange(pickMaterial(0), 0, UINT32_MAX);
            }
            else {
                for (const auto& s : subs) {
                    submitRange(pickMaterial(s.materialIndex), s.indexStart, s.indexCount);
                }
            }
        }


        // ============================================================
        // 9) Debug overlay — иконки света + зоны действия выделенного
        // ============================================================
        if (m_debugEntry && bgfx::isValid(m_debugEntry->program)) {
            m_debug.begin();

            const u32 kColSelected = 0xff00ffffu; // ярко-жёлтый (ABGR)
            const u32 kAlphaZone = 0x40000000u; // 25% alpha (будет |= с цветом ниже)

            auto& reg = world.registry();

            // --- Directional lights (рисуем стрелку из позиции Transform или (0,0,0)) ---
            {
                auto lview = reg.view<grit::ecs::LightDirectional>();
                for (auto ent : lview) {
                    auto& L = lview.get<grit::ecs::LightDirectional>(ent);

                    vec3 pos{ 0, 0, 0 };
                    if (auto* tr = reg.try_get<grit::ecs::Transform>(ent)) {
                        pos = tr->position;
                    }

                    u32 col = 0xff000000u
                        | ((u32)(L.color.z * 255.f) << 16)
                        | ((u32)(L.color.y * 255.f) << 8)
                        | ((u32)(L.color.x * 255.f));
                    if (ent == m_selectedEntity) col = kColSelected;

                    m_debug.lightIcon(pos, 0.25f, col);

                    // Стрелка направления
                    vec3 d = L.direction;
                    f32 dl = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
                    if (dl > 1e-6f) {
                        d.x /= dl; d.y /= dl; d.z /= dl;
                        vec3 to{ pos.x + d.x * 1.5f, pos.y + d.y * 1.5f, pos.z + d.z * 1.5f };
                        m_debug.arrow(pos, to, col);
                    }
                }
            }

            // --- Point lights ---
            {
                auto lview = reg.view<grit::ecs::Transform, grit::ecs::LightPoint>();
                for (auto ent : lview) {
                    auto& tr = lview.get<grit::ecs::Transform>(ent);
                    auto& L = lview.get<grit::ecs::LightPoint>(ent);

                    u32 col = 0xff000000u
                        | ((u32)(L.color.z * 255.f) << 16)
                        | ((u32)(L.color.y * 255.f) << 8)
                        | ((u32)(L.color.x * 255.f));
                    bool selected = (ent == m_selectedEntity);
                    u32 iconCol = selected ? kColSelected : col;

                    m_debug.lightIcon(tr.position, 0.25f, iconCol);

                    // Зона действия (radius) — только если выделен
                    if (selected) {
                        m_debug.wireSphere(tr.position, L.range, col, 32);
                    }
                }
            }

            // --- Spot lights ---
            {
                auto lview = reg.view<grit::ecs::Transform, grit::ecs::LightSpot>();
                for (auto ent : lview) {
                    auto& tr = lview.get<grit::ecs::Transform>(ent);
                    auto& L = lview.get<grit::ecs::LightSpot>(ent);

                    u32 col = 0xff000000u
                        | ((u32)(L.color.z * 255.f) << 16)
                        | ((u32)(L.color.y * 255.f) << 8)
                        | ((u32)(L.color.x * 255.f));
                    bool selected = (ent == m_selectedEntity);
                    u32 iconCol = selected ? kColSelected : col;

                    m_debug.lightIcon(tr.position, 0.25f, iconCol);

                    if (selected) {
                        m_debug.wireCone(tr.position, L.direction, L.range, L.outerCone, col, 24);
                        // Тонкий внутренний конус
                        m_debug.wireCone(tr.position, L.direction, L.range, L.innerCone,
                            (col & 0x00ffffffu) | 0x80000000u, 24);
                    }
                }
            }

            m_debug.flush(kViewScene);
        }
    }

  
  

} // namespace grit::render