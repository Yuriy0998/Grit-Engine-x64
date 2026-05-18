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

namespace grit::render {

    bool Renderer::init(rhi::Device* dev, FileWatcher* watcher) {
        m_device = dev;

        m_shaders.init(watcher);
        m_defaultEntry = m_shaders.loadProgram("vs_textured", "fs_textured");
        if (!m_defaultEntry || !bgfx::isValid(m_defaultEntry->program)) {
            GRIT_ERROR("Renderer: cannot load vs_textured/fs_textured");
            return false;
        }

        m_uTexColor = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
        m_uLightDir = bgfx::createUniform("u_lightDir", bgfx::UniformType::Vec4);
        m_uLightColor = bgfx::createUniform("u_lightColor", bgfx::UniformType::Vec4);
        m_uBaseColor = bgfx::createUniform("u_baseColor", bgfx::UniformType::Vec4);
        m_uTilingOffset = bgfx::createUniform("u_tilingOffset", bgfx::UniformType::Vec4);

        // 1x1 белая текстура для меша без материала/текстуры
        const u32 whitePixel = 0xFFFFFFFFu;
        m_whitePixel = bgfx::createTexture2D(
            1, 1, false, 1, bgfx::TextureFormat::RGBA8,
            BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE,
            bgfx::copy(&whitePixel, 4));

        m_camera.position = { 0.f, 1.5f, -4.f };
        m_camera.target = { 0.f, 0.f, 0.f };
        m_camera.aspect = (f32)dev->width() / (f32)dev->height();

        GRIT_INFO("Renderer initialized");
        return true;
    }

    void Renderer::shutdown() {
        if (bgfx::isValid(m_whitePixel)) {
            bgfx::destroy(m_whitePixel);
            m_whitePixel = BGFX_INVALID_HANDLE;
        }
        if (bgfx::isValid(m_uTexColor)) {
            bgfx::destroy(m_uTexColor);
            m_uTexColor = BGFX_INVALID_HANDLE;
        }

        if (bgfx::isValid(m_uLightDir)) { bgfx::destroy(m_uLightDir);   m_uLightDir = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uLightColor)) { bgfx::destroy(m_uLightColor); m_uLightColor = BGFX_INVALID_HANDLE; }

        if (bgfx::isValid(m_uBaseColor)) { bgfx::destroy(m_uBaseColor);    m_uBaseColor = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uTilingOffset)) { bgfx::destroy(m_uTilingOffset); m_uTilingOffset = BGFX_INVALID_HANDLE; }
        
        GRIT_INFO("Renderer shutdown");
    }

    void Renderer::render(ecs::World& world, f32 /*dt*/) {
        if (m_device->height() > 0) {
            m_camera.aspect = (f32)m_device->width() / (f32)m_device->height();
        }

        f32 view[16], proj[16];
        m_camera.buildView(view);
        m_camera.buildProj(proj, bgfx::getCaps()->homogeneousDepth);
        bgfx::setViewTransform(0, view, proj);

        // --- Сбор directional света ---
        // Берём первый встретившийся (поддержку нескольких добавим позже).
        f32 lightDir[4] = { -0.4f, -0.8f, 0.3f, 0.15f }; // дефолт если нет света
        f32 lightColor[4] = { 1.0f,  1.0f, 1.0f, 1.0f };

        {
            auto lview = world.registry().view<grit::ecs::LightDirectional>();
            for (auto ent : lview) {
                auto& L = lview.get<grit::ecs::LightDirectional>(ent);

                f32 lx = L.direction.x, ly = L.direction.y, lz = L.direction.z;
                f32 len = std::sqrt(lx * lx + ly * ly + lz * lz);
                if (len < 1e-6f) len = 1.0f;

                lightDir[0] = lx / len;
                lightDir[1] = ly / len;
                lightDir[2] = lz / len;
                lightDir[3] = L.ambient;

                lightColor[0] = L.color.x;
                lightColor[1] = L.color.y;
                lightColor[2] = L.color.z;
                lightColor[3] = L.intensity;
                break; // используем первый
            }
        }

        bgfx::setUniform(m_uLightDir, lightDir);
        bgfx::setUniform(m_uLightColor, lightColor);

        // --- Рендер мешей ---
        auto view2 = world.registry().view<grit::ecs::Transform, grit::ecs::MeshRenderer>();
        for (auto ent : view2) {
            auto& tr = view2.get<grit::ecs::Transform>(ent);
            auto& mr = view2.get<grit::ecs::MeshRenderer>(ent);
            if (!mr.visible || !mr.mesh) continue;

            bgfx::setTransform(tr.worldMatrix);

            bgfx::setVertexBuffer(0, mr.mesh->vb().handle());
            bgfx::setIndexBuffer(mr.mesh->ib().handle());

            bgfx::TextureHandle tex = m_whitePixel;
            if (mr.material && mr.material->albedo && mr.material->albedo->valid()) {
                tex = mr.material->albedo->handle();
            }
            bgfx::setTexture(0, m_uTexColor, tex);

            bgfx::ProgramHandle prog = defaultProgram();
            if (!bgfx::isValid(prog)) continue;

            // --- Параметры материала ---
            f32 baseColor[4] = { 1, 1, 1, 1 };
            f32 tilingOffset[4] = { 1, 1, 0, 0 };
            if (mr.material) {
                baseColor[0] = mr.material->baseColor.x;
                baseColor[1] = mr.material->baseColor.y;
                baseColor[2] = mr.material->baseColor.z;
                baseColor[3] = mr.material->baseColor.w;

                tilingOffset[0] = mr.material->tilingOffset.x;
                tilingOffset[1] = mr.material->tilingOffset.y;
                tilingOffset[2] = mr.material->tilingOffset.z;
                tilingOffset[3] = mr.material->tilingOffset.w;
            }
            bgfx::setUniform(m_uBaseColor, baseColor);
            bgfx::setUniform(m_uTilingOffset, tilingOffset);

            bgfx::setState(BGFX_STATE_DEFAULT);
            bgfx::submit(0, prog);
        }
    }

} // namespace grit::render