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

        // --- Grid shader ---
        m_gridEntry = m_shaders.loadProgram("vs_grid", "fs_grid");
        if (!m_gridEntry || !bgfx::isValid(m_gridEntry->program)) {
            GRIT_WARN("Renderer: grid shader failed to load");
        }
        m_uGridInvViewProj = bgfx::createUniform("gridInvVP", bgfx::UniformType::Mat4);
        m_uGridParams = bgfx::createUniform("gridParams", bgfx::UniformType::Vec4);

        // --- Common ---
        m_uTexColor = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);

        // --- Lights ---
        m_uDirLightDir = bgfx::createUniform("u_dirLightDir", bgfx::UniformType::Vec4, kMaxDirLights);
        m_uDirLightColor = bgfx::createUniform("u_dirLightColor", bgfx::UniformType::Vec4, kMaxDirLights);
        m_uDirLightCount = bgfx::createUniform("u_dirLightCount", bgfx::UniformType::Vec4);

        // --- Material ---
        m_uBaseColor = bgfx::createUniform("u_baseColor", bgfx::UniformType::Vec4);
        m_uTilingOffset = bgfx::createUniform("u_tilingOffset", bgfx::UniformType::Vec4);

        // 1x1 white pixel
        const u32 whitePixel = 0xFFFFFFFFu;
        m_whitePixel = bgfx::createTexture2D(
            1, 1, false, 1, bgfx::TextureFormat::RGBA8,
            BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE,
            bgfx::copy(&whitePixel, 4));

        m_camera.position = { 0.f, 1.5f, -4.f };
        m_camera.target = { 0.f, 0.f,  0.f };

        GRIT_INFO("Renderer initialized");
        return true;
    }

    void Renderer::drawGrid()
    {
        if (!m_gridEntry || !bgfx::isValid(m_gridEntry->program)) return;

        // Layout
        static bgfx::VertexLayout layout;
        static bool layoutInit = false;
        if (!layoutInit) {
            layout.begin()
                .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
                .end();
            layoutInit = true;
        }

        // Fullscreen quad как 2 треугольника
        struct PosVtx { float x, y, z; };
        const uint32_t kNumVerts = 6;

        if (kNumVerts != bgfx::getAvailTransientVertexBuffer(kNumVerts, layout))
            return;

        bgfx::TransientVertexBuffer tvb;
        bgfx::allocTransientVertexBuffer(&tvb, kNumVerts, layout);
        PosVtx* v = (PosVtx*)tvb.data;
        v[0] = { -1.f, -1.f, 0.f }; v[1] = { 1.f, -1.f, 0.f }; v[2] = { 1.f,  1.f, 0.f };
        v[3] = { -1.f, -1.f, 0.f }; v[4] = { 1.f,  1.f, 0.f }; v[5] = { -1.f,  1.f, 0.f };

        // Inverse(viewProj)
        float vp[16];
        bx::mtxMul(vp, m_viewMtx, m_projMtx);
        float invVp[16];
        bx::mtxInverse(invVp, vp);
        bgfx::setUniform(m_uGridInvViewProj, invVp);

        float gridParams[4] = { m_camera.nearPlane, m_camera.farPlane, 0.f, 0.f };
        bgfx::setUniform(m_uGridParams, gridParams);

        bgfx::setVertexBuffer(0, &tvb);

        // Прозрачный, читает Z, не пишет Z, рисуем ДО мешей
        bgfx::setState(
            BGFX_STATE_WRITE_RGB
            | BGFX_STATE_WRITE_A
            | BGFX_STATE_DEPTH_TEST_ALWAYS
            | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA)
        );

        bgfx::submit(kViewScene, m_gridEntry->program);
    }

    void Renderer::shutdown() {
        if (bgfx::isValid(m_whitePixel)) { bgfx::destroy(m_whitePixel);     m_whitePixel = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uTexColor)) { bgfx::destroy(m_uTexColor);      m_uTexColor = BGFX_INVALID_HANDLE; }

        if (bgfx::isValid(m_uDirLightDir)) { bgfx::destroy(m_uDirLightDir);   m_uDirLightDir = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uDirLightColor)) { bgfx::destroy(m_uDirLightColor); m_uDirLightColor = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uDirLightCount)) { bgfx::destroy(m_uDirLightCount); m_uDirLightCount = BGFX_INVALID_HANDLE; }

        if (bgfx::isValid(m_uBaseColor)) { bgfx::destroy(m_uBaseColor);     m_uBaseColor = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uTilingOffset)) { bgfx::destroy(m_uTilingOffset);  m_uTilingOffset = BGFX_INVALID_HANDLE; }

        if (bgfx::isValid(m_uGridInvViewProj)) { bgfx::destroy(m_uGridInvViewProj); m_uGridInvViewProj = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uGridParams)) { bgfx::destroy(m_uGridParams);      m_uGridParams = BGFX_INVALID_HANDLE; }

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

    void Renderer::render(ecs::World& world, f32 /*dt*/)
    {
        if (m_vpWidth == 0 || m_vpHeight == 0 || !bgfx::isValid(m_vpFB)) return;

        // --- Backbuffer ---
        bgfx::setViewRect(0, 0, 0,
            (uint16_t)m_device->width(),
            (uint16_t)m_device->height());
        bgfx::setViewClear(0,
            BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
            0x1a1a1aff, 1.0f, 0);
        bgfx::touch(0);

        // --- Камера ---
        m_camera.aspect = (f32)m_vpWidth / (f32)m_vpHeight;
        m_camera.buildView(m_viewMtx);
        m_camera.buildProj(m_projMtx, bgfx::getCaps()->homogeneousDepth);

        // --- View 1 (scene) ---
        bgfx::setViewFrameBuffer(kViewScene, m_vpFB);
        bgfx::setViewRect(kViewScene, 0, 0,
            (uint16_t)m_vpWidth, (uint16_t)m_vpHeight);
        bgfx::setViewClear(kViewScene,
            BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
            0x202225ff, 1.0f, 0);
        bgfx::setViewTransform(kViewScene, m_viewMtx, m_projMtx);
        bgfx::touch(kViewScene);

        // --- Grid (после настройки view!) ---
        if (m_showGrid) drawGrid();

        // --- Сбор directional lights ---
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

        if (dirLightCount == 0) ambient = 0.15f;

        f32 lightCountVec[4] = { (f32)dirLightCount, ambient, 0.0f, 0.0f };
        bgfx::setUniform(m_uDirLightDir, dirLightDir, kMaxDirLights);
        bgfx::setUniform(m_uDirLightColor, dirLightColor, kMaxDirLights);
        bgfx::setUniform(m_uDirLightCount, lightCountVec);

        // --- Рендер мешей ---
        auto view2 = world.registry().view<grit::ecs::Transform, grit::ecs::MeshRenderer>();
        for (auto ent : view2) {
            auto& tr = view2.get<grit::ecs::Transform>(ent);
            auto& mr = view2.get<grit::ecs::MeshRenderer>(ent);
            if (!mr.visible || !mr.mesh) continue;

            bgfx::ProgramHandle prog = defaultProgram();
            if (!bgfx::isValid(prog)) continue;

            const auto& vb = mr.mesh->vb().handle();
            const auto& ib = mr.mesh->ib().handle();

            // Хелпер сабмита одного диапазона
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

                    // Текстура
                    bgfx::TextureHandle tex = m_whitePixel;
                    if (mat && mat->albedo && mat->albedo->valid()) {
                        tex = mat->albedo->handle();
                    }
                    bgfx::setTexture(0, m_uTexColor, tex);

                    // Параметры материала
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
                    bgfx::submit(kViewScene, prog);  // ← ВАЖНО: kViewScene, не 0
                };

            // Хелпер выбора материала с защитой границ
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
    }

} // namespace grit::render