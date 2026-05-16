#pragma once
#include "../rhi/Resources.h"
#include "../core/Math.h"

namespace grit::render {

    // ¬ершина базового статического меша
    struct StaticVertex {
        f32 px, py, pz;     // position
        f32 nx, ny, nz;     // normal
        f32 u, v;           // texcoord0
    };

    class StaticMesh {
    public:
        StaticMesh() = default;
        ~StaticMesh();

        StaticMesh(const StaticMesh&) = delete;
        StaticMesh& operator=(const StaticMesh&) = delete;

        bool createFromArrays(const StaticVertex* verts, u32 vcount,
            const u32* indices, u32 icount);
        void destroy();

        const rhi::VertexBuffer& vb() const { return m_vb; }
        const rhi::IndexBuffer& ib() const { return m_ib; }

        static const rhi::VertexLayout& staticLayout();

    private:
        rhi::VertexBuffer m_vb;
        rhi::IndexBuffer  m_ib;
    };

} // namespace grit::render