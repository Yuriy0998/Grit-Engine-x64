#include "Renderer.h"
#include <bgfx/bgfx.h>
#include <bx/math.h>

namespace grit::render {

    void Renderer::drawGrid()
    {
        if (!m_gridEntry || !bgfx::isValid(m_gridEntry->program)) return;

        static bgfx::VertexLayout layout;
        static bool layoutInit = false;
        if (!layoutInit) {
            layout.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).end();
            layoutInit = true;
        }

        struct PosVtx { float x, y, z; };
        const uint32_t kNumVerts = 6;
        if (kNumVerts != bgfx::getAvailTransientVertexBuffer(kNumVerts, layout)) return;

        bgfx::TransientVertexBuffer tvb;
        bgfx::allocTransientVertexBuffer(&tvb, kNumVerts, layout);
        PosVtx* v = (PosVtx*)tvb.data;
        v[0] = { -1,-1,0 }; v[1] = { 1,-1,0 }; v[2] = { 1,1,0 };
        v[3] = { -1,-1,0 }; v[4] = { 1,1,0 };  v[5] = { -1,1,0 };

        float vp[16];  bx::mtxMul(vp, m_viewMtx, m_projMtx);
        float invVp[16]; bx::mtxInverse(invVp, vp);
        bgfx::setUniform(m_uGridInvViewProj, invVp);
        bgfx::setUniform(m_uGridViewProj, vp);

        float gridParams[4] = { m_camera.nearPlane, m_camera.farPlane, 0.f, 0.f };
        bgfx::setUniform(m_uGridParams, gridParams);

        bgfx::setVertexBuffer(0, &tvb);
        bgfx::setState(
            BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A
            | BGFX_STATE_DEPTH_TEST_LESS   // тест против мешей, Z не пишем
            | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA)
        );
        bgfx::submit(kViewScene, m_gridEntry->program);
    }

} // namespace grit::render