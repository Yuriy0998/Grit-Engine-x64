#include "Renderer.h"
#include "../ecs/World.h"
#include "StaticMesh.h"
#include "Material.h"
#include "Texture.h"
#include <bgfx/bgfx.h>

namespace grit::render {

    void Renderer::uploadEnvironment()
    {
        const float fogParams[4] = {
            m_env.fogStart, m_env.fogEnd, m_env.fogDensity, (float)(int)m_env.fogMode };
        const float fogColor[4] = {
            m_env.fogColor.x, m_env.fogColor.y, m_env.fogColor.z,
            m_env.fogEnabled ? 1.f : 0.f };
        const float ambientCol[4] = {
            m_env.ambientColor.x * m_env.ambientIntensity,
            m_env.ambientColor.y * m_env.ambientIntensity,
            m_env.ambientColor.z * m_env.ambientIntensity, 1.f };
        const float camPos[4] = {
            m_camera.position.x, m_camera.position.y, m_camera.position.z, 0.f };

        bgfx::setUniform(m_uFogParams, fogParams);
        bgfx::setUniform(m_uFogColor, fogColor);
        bgfx::setUniform(m_uAmbientColor, ambientCol);
        bgfx::setUniform(m_uCameraPos, camPos);
    }

    void Renderer::uploadShadowUniforms()
    {
        if (m_shadowsEnabled && bgfx::isValid(m_shadowMap)) {
            bgfx::setUniform(m_uShadowMtx, m_shadowMtx);
            const f32 texel = 1.f / (f32)m_shadowMapSize;
            const f32 p[4] = { m_shadowBias, texel, m_softShadows ? 1.f : 0.f, 1.f };
            bgfx::setUniform(m_uShadowParams, p);
        }
        else {
            const f32 p[4] = { 0, 0, 0, 0 };
            bgfx::setUniform(m_uShadowParams, p);
        }
    }

    void Renderer::renderMeshes(ecs::World& world)
    {
        auto view = world.registry().view<grit::ecs::Transform, grit::ecs::MeshRenderer>();
        for (auto ent : view) {
            auto& tr = view.get<grit::ecs::Transform>(ent);
            auto& mr = view.get<grit::ecs::MeshRenderer>(ent);
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
                    if (indexCount == UINT32_MAX) bgfx::setIndexBuffer(ib);
                    else bgfx::setIndexBuffer(ib, indexStart, indexCount);

                    bgfx::TextureHandle tex = m_whitePixel;
                    if (mat && mat->albedo && mat->albedo->valid())
                        tex = mat->albedo->handle();
                    bgfx::setTexture(0, m_uTexColor, tex);

                    bgfx::setTexture(1, m_uShadowMap,
                        (m_shadowsEnabled && bgfx::isValid(m_shadowMap))
                        ? m_shadowMap : m_whitePixel);

                    f32 baseColor[4] = { 1,1,1,1 };
                    f32 tilingOffset[4] = { 1,1,0,0 };
                    f32 pbrParams[4] = { 0.f, 0.8f, 0.f, 0.f };
                    f32 emissive[4] = { 0,0,0,0 };
                    if (mat) {
                        baseColor[0] = mat->baseColor.x; baseColor[1] = mat->baseColor.y;
                        baseColor[2] = mat->baseColor.z; baseColor[3] = mat->baseColor.w;
                        tilingOffset[0] = mat->tilingOffset.x; tilingOffset[1] = mat->tilingOffset.y;
                        tilingOffset[2] = mat->tilingOffset.z; tilingOffset[3] = mat->tilingOffset.w;
                        pbrParams[0] = mat->pbrParams.x; pbrParams[1] = mat->pbrParams.y;
                        pbrParams[2] = mat->pbrParams.z; pbrParams[3] = mat->pbrParams.w;
                        emissive[0] = mat->emissive.x; emissive[1] = mat->emissive.y;
                        emissive[2] = mat->emissive.z; emissive[3] = mat->emissive.w;
                    }
                    bgfx::setUniform(m_uBaseColor, baseColor);
                    bgfx::setUniform(m_uTilingOffset, tilingOffset);
                    bgfx::setUniform(m_uPbrParams, pbrParams);
                    bgfx::setUniform(m_uEmissive, emissive);

                    bgfx::setState(BGFX_STATE_DEFAULT);
                    bgfx::submit(kViewScene, prog);
                };

            auto pickMaterial = [&](u32 idx) -> std::shared_ptr<Material> {
                if (mr.materials.empty()) return nullptr;
                const size_t n = mr.materials.size();
                return mr.materials[(idx < n) ? idx : (n - 1)];
                };

            const auto& subs = mr.mesh->submeshes();
            if (subs.empty()) {
                submitRange(pickMaterial(0), 0, UINT32_MAX);
            }
            else {
                for (const auto& s : subs)
                    submitRange(pickMaterial(s.materialIndex), s.indexStart, s.indexCount);
            }
        }
    }

} // namespace grit::render