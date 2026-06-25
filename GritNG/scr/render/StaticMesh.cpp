#include "StaticMesh.h"

namespace grit::render {

    static rhi::VertexLayout g_layout;
    static bool              g_layoutBuilt = false;

    const rhi::VertexLayout& StaticMesh::staticLayout() {
        if (!g_layoutBuilt) {
            g_layout
                .add(rhi::VertexAttrib::Semantic::Position, 3, rhi::VertexAttrib::Type::Float)
                .add(rhi::VertexAttrib::Semantic::Normal, 3, rhi::VertexAttrib::Type::Float)
                .add(rhi::VertexAttrib::Semantic::TexCoord0, 2, rhi::VertexAttrib::Type::Float);
            g_layout.build();
            g_layoutBuilt = true;
        }
        return g_layout;
    }

    StaticMesh::~StaticMesh() {
        destroy();
    }

    bool StaticMesh::createFromArrays(const StaticVertex* verts, u32 vcount,
        const u32* indices, u32 icount) {
        if (!m_vb.create(verts, vcount * sizeof(StaticVertex), staticLayout()))
            return false;
        // glTF индексы — обычно 32-битные
        if (!m_ib.create(indices, icount * sizeof(u32), true))
            return false;
        m_indexCount = icount;
        return true;
    }

    void StaticMesh::destroy() {
        m_vb.destroy();
        m_ib.destroy();
    }

} // namespace grit::render