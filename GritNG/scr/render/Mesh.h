#pragma once
#include "../rhi/Resources.h"

namespace grit::render {

    class Mesh {
    public:
        bool createCube();
        void destroy();

        const rhi::VertexBuffer& vb() const { return m_vb; }
        const rhi::IndexBuffer& ib() const { return m_ib; }

    private:
        rhi::VertexLayout m_layout;
        rhi::VertexBuffer m_vb;
        rhi::IndexBuffer  m_ib;
    };

} // namespace grit::render