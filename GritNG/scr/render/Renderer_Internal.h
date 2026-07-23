#pragma once
#include <bgfx/bgfx.h>

namespace grit::render::detail {

    struct PosTexVertex {
        float x, y, z;
        float u, v;
        static void init(bgfx::VertexLayout& l) {
            l.begin()
                .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
                .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
                .end();
        }
    };

    inline void submitFullscreenTriangle(bgfx::ViewId view,
        bgfx::ProgramHandle prog, uint64_t state)
    {
        bgfx::VertexLayout layout;
        PosTexVertex::init(layout);
        if (bgfx::getAvailTransientVertexBuffer(3, layout) < 3) return;

        bgfx::TransientVertexBuffer tvb;
        bgfx::allocTransientVertexBuffer(&tvb, 3, layout);

        auto* v = (PosTexVertex*)tvb.data;
        v[0] = { -1.f, -1.f, 0.f, 0.f, 0.f };
        v[1] = { 3.f, -1.f, 0.f, 2.f, 0.f };
        v[2] = { -1.f,  3.f, 0.f, 0.f, 2.f };

        bgfx::setVertexBuffer(0, &tvb);
        bgfx::setState(state);
        bgfx::submit(view, prog);
    }

} // namespace grit::render::detail