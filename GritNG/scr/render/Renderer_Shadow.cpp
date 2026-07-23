#include "Renderer.h"
#include "../ecs/World.h"
#include "../ecs/Light.h"
#include "../core/Log.h"
#include "StaticMesh.h"
#include <bgfx/bgfx.h>
#include <bx/math.h>
#include <cmath>
#include <cstring>

namespace grit::render {

    void Renderer::createShadowResources()
    {
        destroyShadowResources();

        const uint64_t flags =
            BGFX_TEXTURE_RT
            | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT
            | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;

        m_shadowMap = bgfx::createTexture2D(
            (uint16_t)m_shadowMapSize, (uint16_t)m_shadowMapSize,
            false, 1, bgfx::TextureFormat::D32F, flags);

        bgfx::TextureHandle atts[] = { m_shadowMap };
        m_shadowFB = bgfx::createFrameBuffer(1, atts, false);

        GRIT_INFO("Shadow map created: {}x{}", m_shadowMapSize, m_shadowMapSize);
    }

    void Renderer::destroyShadowResources()
    {
        if (bgfx::isValid(m_shadowFB)) { bgfx::destroy(m_shadowFB);  m_shadowFB = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_shadowMap)) { bgfx::destroy(m_shadowMap); m_shadowMap = BGFX_INVALID_HANDLE; }
    }

    void Renderer::setShadowMapSize(u32 s)
    {
        if (s == m_shadowMapSize) return;
        m_shadowMapSize = s;
        createShadowResources();
    }

    void Renderer::renderShadowPass(ecs::World& world)
    {
        if (!m_shadowsEnabled) return;
        if (!m_shadowEntry || !bgfx::isValid(m_shadowEntry->program)) return;
        if (!bgfx::isValid(m_shadowFB)) return;

        auto& reg = world.registry();

        vec3 lightDir{ -0.3f, -1.0f, -0.2f };
        bool foundLight = false;
        for (auto e : reg.view<grit::ecs::LightDirectional>()) {
            auto& L = reg.get<grit::ecs::LightDirectional>(e);
            lightDir = L.direction;
            foundLight = true;
            break;
        }
        if (!foundLight) return;

        f32 dl = std::sqrt(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
        if (dl < 1e-6f) return;
        lightDir.x /= dl; lightDir.y /= dl; lightDir.z /= dl;

        const f32 halfSize = m_shadowDistance * 0.5f;
        vec3 camPos = m_camera.position;
        const f32 texelWorldSize = m_shadowDistance / (f32)m_shadowMapSize;
        camPos.x = std::floor(camPos.x / texelWorldSize) * texelWorldSize;
        camPos.y = std::floor(camPos.y / texelWorldSize) * texelWorldSize;
        camPos.z = std::floor(camPos.z / texelWorldSize) * texelWorldSize;

        const f32 lightDistance = m_shadowDistance;
        const bx::Vec3 eye = {
            camPos.x - lightDir.x * lightDistance,
            camPos.y - lightDir.y * lightDistance,
            camPos.z - lightDir.z * lightDistance };
        const bx::Vec3 at = { camPos.x, camPos.y, camPos.z };

        bx::Vec3 up = { 0.0f, 1.0f, 0.0f };
        if (std::fabs(lightDir.y) > 0.99f) up = { 0.0f, 0.0f, 1.0f };

        f32 lightView[16];
        bx::mtxLookAt(lightView, eye, at, up);

        f32 lightProj[16];
        const f32 zNear = 0.1f;
        const f32 zFar = lightDistance * 2.0f + m_shadowDistance;
        const bool homogeneous = bgfx::getCaps()->homogeneousDepth;
        bx::mtxOrtho(lightProj, -halfSize, halfSize, -halfSize, halfSize,
            zNear, zFar, 0.0f, homogeneous);

        f32 lightVP[16];
        bx::mtxMul(lightVP, lightView, lightProj);
        std::memcpy(m_shadowMtx, lightVP, sizeof(lightVP));

        bgfx::setViewName(kViewShadow, "Shadow");
        bgfx::setViewFrameBuffer(kViewShadow, m_shadowFB);
        bgfx::setViewRect(kViewShadow, 0, 0,
            (uint16_t)m_shadowMapSize, (uint16_t)m_shadowMapSize);
        bgfx::setViewClear(kViewShadow, BGFX_CLEAR_DEPTH, 0x202225ff, 1.0f, 0);
        bgfx::setViewTransform(kViewShadow, lightView, lightProj);

        const uint64_t state =
            BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CW;

        auto vw = reg.view<grit::ecs::Transform, grit::ecs::MeshRenderer>();
        for (auto e : vw) {
            auto& tr = vw.get<grit::ecs::Transform>(e);
            auto& mr = vw.get<grit::ecs::MeshRenderer>(e);
            if (!mr.visible || !mr.mesh) continue;

            f32 world[16];
            bx::mtxSRT(world,
                tr.scale.x, tr.scale.y, tr.scale.z,
                tr.rotationEuler.x, tr.rotationEuler.y, tr.rotationEuler.z,
                tr.position.x, tr.position.y, tr.position.z);

            bgfx::setTransform(world);
            bgfx::setVertexBuffer(0, mr.mesh->vb().handle());
            bgfx::setIndexBuffer(mr.mesh->ib().handle());
            bgfx::setState(state);
            bgfx::submit(kViewShadow, m_shadowEntry->program);
        }
    }

} // namespace grit::render