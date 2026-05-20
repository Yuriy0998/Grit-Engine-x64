#pragma once
#include "../rhi/Resources.h"
#include "../core/Math.h"
#include <vector>

namespace grit::render {

    // Вершина базового статического меша
    struct StaticVertex {
        f32 px, py, pz;     // position
        f32 nx, ny, nz;     // normal
        f32 u, v;           // texcoord0
    };

    // Диапазон индексов в общем index buffer'е меша.
    // Каждый сабмеш рисуется отдельным submit'ом со своим материалом.
    struct Submesh {
        u32 indexStart = 0;       // начальный индекс
        u32 indexCount = 0;       // сколько индексов рисовать
        u32 materialIndex = 0;       // индекс в массиве материалов MeshRenderer
    };

    class StaticMesh {
    public:
        StaticMesh() = default;
        ~StaticMesh();

        StaticMesh(const StaticMesh&) = delete;
        StaticMesh& operator=(const StaticMesh&) = delete;

        bool createFromArrays(const StaticVertex* verts, u32 vcount,
            const u32* indices, u32 icount);

        // Установить сабмеши (вызывается после createFromArrays, например из loadGrMesh).
        // Если не вызывать — меш рисуется как один сабмеш с materialIndex=0.
        void setSubmeshes(std::vector<Submesh> subs) { m_submeshes = std::move(subs); }

        void destroy();

        const rhi::VertexBuffer& vb() const { return m_vb; }
        const rhi::IndexBuffer& ib() const { return m_ib; }

        u32  indexCount()              const { return m_indexCount; }
        const std::vector<Submesh>& submeshes() const { return m_submeshes; }

        static const rhi::VertexLayout& staticLayout();

    private:
        rhi::VertexBuffer    m_vb;
        rhi::IndexBuffer     m_ib;
        u32                  m_indexCount = 0;
        std::vector<Submesh> m_submeshes;
    };

} // namespace grit::render