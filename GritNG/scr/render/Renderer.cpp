#include "Renderer.h"
#include "../rhi/Device.h"
#include "../rhi/Resources.h"
#include "../ecs/World.h"
#include "../core/Log.h"
#include "../core/Math.h"

#include "StaticMesh.h"
#include "Material.h"
#include "Texture.h"

#include <bgfx/bgfx.h>
#include <bx/math.h>

namespace grit::render {

    bool Renderer::init(rhi::Device* dev) {
        m_device = dev;

        m_defaultProgram = rhi::loadProgram("vs_textured", "fs_textured");
        if (!bgfx::isValid(m_defaultProgram)) {
            GRIT_ERROR("Renderer: cannot load vs_textured/fs_textured");
            return false;
        }

        m_uTexColor = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);

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
        if (bgfx::isValid(m_defaultProgram)) {
            bgfx::destroy(m_defaultProgram);
            m_defaultProgram = BGFX_INVALID_HANDLE;
        }
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

        // Идём по всем сущностям с Transform + MeshRenderer
        auto view2 = world.registry().view<grit::ecs::Transform, grit::ecs::MeshRenderer>();
        for (auto ent : view2) {
            auto& t = view2.get<grit::ecs::Transform>(ent);
            auto& mr = view2.get<grit::ecs::MeshRenderer>(ent);
            if (!mr.mesh) continue;

            f32 mtx[16];
            bx::mtxSRT(mtx,
                t.scale.x, t.scale.y, t.scale.z,
                t.rotationEuler.x, t.rotationEuler.y, t.rotationEuler.z,
                t.position.x, t.position.y, t.position.z);

            bgfx::setTransform(mtx);
            bgfx::setVertexBuffer(0, mr.mesh->vb().handle());
            bgfx::setIndexBuffer(mr.mesh->ib().handle());

            // Текстура
            bgfx::TextureHandle tex = m_whitePixel;
            if (mr.material && mr.material->albedo && mr.material->albedo->valid()) {
                tex = mr.material->albedo->handle();
            }
            bgfx::setTexture(0, m_uTexColor, tex);

            // Программа
            bgfx::ProgramHandle prog =
                (mr.material && bgfx::isValid(mr.material->program))
                ? mr.material->program
                : m_defaultProgram;

            bgfx::setState(BGFX_STATE_DEFAULT);
            bgfx::submit(0, prog);
        }
    }

} // namespace grit::render