#include "Mesh.h"

namespace grit::render {

    struct CubeVertex {
        float x, y, z;
        u32   abgr;
    };

    bool Mesh::createCube() {
        m_layout
            .add(rhi::VertexAttrib::Semantic::Position, 3, rhi::VertexAttrib::Type::Float)
            .add(rhi::VertexAttrib::Semantic::Color0, 4, rhi::VertexAttrib::Type::Uint8, true);
        m_layout.build();

        static const CubeVertex verts[] = {
            {-1.f,  1.f,  1.f, 0xff000000 },
            { 1.f,  1.f,  1.f, 0xff0000ff },
            {-1.f, -1.f,  1.f, 0xff00ff00 },
            { 1.f, -1.f,  1.f, 0xff00ffff },
            {-1.f,  1.f, -1.f, 0xffff0000 },
            { 1.f,  1.f, -1.f, 0xffff00ff },
            {-1.f, -1.f, -1.f, 0xffffff00 },
            { 1.f, -1.f, -1.f, 0xffffffff },
        };

        static const u16 indices[] = {
            0, 1, 2,  1, 3, 2,
            4, 6, 5,  5, 6, 7,
            0, 2, 4,  4, 2, 6,
            1, 5, 3,  5, 7, 3,
            0, 4, 1,  4, 5, 1,
            2, 3, 6,  6, 3, 7,
        };

        if (!m_vb.create(verts, sizeof(verts), m_layout))      return false;
        if (!m_ib.create(indices, sizeof(indices), false))     return false;
        return true;
    }

    void Mesh::destroy() {
        m_vb.destroy();
        m_ib.destroy();
    }

} // namespace grit::render