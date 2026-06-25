#pragma once
#include "../core/Types.h"
#include "../core/Math.h"
#include <bgfx/bgfx.h>
#include <vector>

namespace grit::render {

    // Простой 3D line renderer (transient buffers).
    // Накопить линии за кадр → flush() в указанный view.
    class DebugDraw {
    public:
        bool init(bgfx::ProgramHandle program);
        void shutdown();

        // Очистить накопленные линии (вызывать в начале кадра).
        void begin();

        // Добавить линию (мировые координаты).
        void line(const vec3& a, const vec3& b, u32 abgr);

        // Удобные хелперы.
        void wireSphere(const vec3& center, f32 radius, u32 abgr, int segments = 24);
        void wireCircle(const vec3& center, const vec3& normal, f32 radius, u32 abgr, int segments = 24);
        void wireCone(const vec3& apex, const vec3& dir, f32 length, f32 angleRad, u32 abgr, int segments = 16);
        void wireBox(const vec3& center, const vec3& halfExtents, u32 abgr);
        void arrow(const vec3& from, const vec3& to, u32 abgr);
        void axes(const vec3& origin, f32 size = 1.0f);
        void lightIcon(const vec3& pos, f32 size, u32 abgr); // 6-лучевая звёздочка

        // Отправить накопленные линии в bgfx view.
        void flush(bgfx::ViewId view);

        bool empty() const { return m_verts.empty(); }

    private:
        struct Vertex {
            f32 x, y, z;
            u32 abgr;
        };

        bgfx::ProgramHandle  m_program{ BGFX_INVALID_HANDLE };
        bgfx::VertexLayout   m_layout;
        std::vector<Vertex>  m_verts;
    };

} // namespace grit::render