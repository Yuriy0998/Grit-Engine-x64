#include "DebugDraw.h"
#include "../core/Log.h"
#include <bx/math.h>
#include <cmath>

namespace grit::render {

    static const f32 kPi = 3.14159265358979f;

    bool DebugDraw::init(bgfx::ProgramHandle program) {
        m_program = program;
        m_layout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();
        m_verts.reserve(4096);
        return bgfx::isValid(m_program);
    }

    void DebugDraw::shutdown() {
        m_verts.clear();
        m_verts.shrink_to_fit();
        m_program = BGFX_INVALID_HANDLE;
    }

    void DebugDraw::begin() {
        m_verts.clear();
    }

    void DebugDraw::line(const vec3& a, const vec3& b, u32 abgr) {
        m_verts.push_back({ a.x, a.y, a.z, abgr });
        m_verts.push_back({ b.x, b.y, b.z, abgr });
    }

    void DebugDraw::wireCircle(const vec3& center, const vec3& normal, f32 radius, u32 abgr, int segments) {
        // Построим ортонормированный базис в плоскости круга.
        vec3 n{ normal.x, normal.y, normal.z };
        f32 nl = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
        if (nl < 1e-6f) return;
        n.x /= nl; n.y /= nl; n.z /= nl;

        vec3 up = (std::fabs(n.y) < 0.99f) ? vec3{ 0,1,0 } : vec3{ 1,0,0 };
        // tangent = normalize(cross(up, n))
        vec3 t{
            up.y * n.z - up.z * n.y,
            up.z * n.x - up.x * n.z,
            up.x * n.y - up.y * n.x
        };
        f32 tl = std::sqrt(t.x * t.x + t.y * t.y + t.z * t.z);
        if (tl < 1e-6f) return;
        t.x /= tl; t.y /= tl; t.z /= tl;
        // bitangent = cross(n, t)
        vec3 b{
            n.y * t.z - n.z * t.y,
            n.z * t.x - n.x * t.z,
            n.x * t.y - n.y * t.x
        };

        vec3 prev{};
        for (int i = 0; i <= segments; ++i) {
            f32 a = (f32)i / (f32)segments * 2.f * kPi;
            f32 c = std::cos(a), s = std::sin(a);
            vec3 p{
                center.x + (t.x * c + b.x * s) * radius,
                center.y + (t.y * c + b.y * s) * radius,
                center.z + (t.z * c + b.z * s) * radius
            };
            if (i > 0) line(prev, p, abgr);
            prev = p;
        }
    }

    void DebugDraw::wireSphere(const vec3& center, f32 radius, u32 abgr, int segments) {
        wireCircle(center, { 1, 0, 0 }, radius, abgr, segments);
        wireCircle(center, { 0, 1, 0 }, radius, abgr, segments);
        wireCircle(center, { 0, 0, 1 }, radius, abgr, segments);
    }

    void DebugDraw::wireCone(const vec3& apex, const vec3& dir, f32 length, f32 angleRad, u32 abgr, int segments) {
        // dir предполагаем нормализованным
        vec3 n = dir;
        f32 nl = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
        if (nl < 1e-6f) return;
        n.x /= nl; n.y /= nl; n.z /= nl;

        f32 radius = std::tan(angleRad) * length;
        vec3 base{
            apex.x + n.x * length,
            apex.y + n.y * length,
            apex.z + n.z * length
        };

        // Базис в плоскости основания
        vec3 up = (std::fabs(n.y) < 0.99f) ? vec3{ 0,1,0 } : vec3{ 1,0,0 };
        vec3 t{
            up.y * n.z - up.z * n.y,
            up.z * n.x - up.x * n.z,
            up.x * n.y - up.y * n.x
        };
        f32 tl = std::sqrt(t.x * t.x + t.y * t.y + t.z * t.z);
        if (tl < 1e-6f) return;
        t.x /= tl; t.y /= tl; t.z /= tl;
        vec3 b{
            n.y * t.z - n.z * t.y,
            n.z * t.x - n.x * t.z,
            n.x * t.y - n.y * t.x
        };

        // Круг основания + лучи к apex
        vec3 prev{};
        for (int i = 0; i <= segments; ++i) {
            f32 a = (f32)i / (f32)segments * 2.f * kPi;
            f32 c = std::cos(a), s = std::sin(a);
            vec3 p{
                base.x + (t.x * c + b.x * s) * radius,
                base.y + (t.y * c + b.y * s) * radius,
                base.z + (t.z * c + b.z * s) * radius
            };
            if (i > 0) line(prev, p, abgr);
            // Линия к apex каждые segments/4 для красоты
            if ((i % (segments / 4)) == 0) line(apex, p, abgr);
            prev = p;
        }
    }

    void DebugDraw::wireBox(const vec3& center, const vec3& he, u32 abgr) {
        vec3 c[8] = {
            { center.x - he.x, center.y - he.y, center.z - he.z },
            { center.x + he.x, center.y - he.y, center.z - he.z },
            { center.x + he.x, center.y + he.y, center.z - he.z },
            { center.x - he.x, center.y + he.y, center.z - he.z },
            { center.x - he.x, center.y - he.y, center.z + he.z },
            { center.x + he.x, center.y - he.y, center.z + he.z },
            { center.x + he.x, center.y + he.y, center.z + he.z },
            { center.x - he.x, center.y + he.y, center.z + he.z },
        };
        int e[12][2] = {
            {0,1},{1,2},{2,3},{3,0},
            {4,5},{5,6},{6,7},{7,4},
            {0,4},{1,5},{2,6},{3,7}
        };
        for (auto& p : e) line(c[p[0]], c[p[1]], abgr);
    }

    void DebugDraw::arrow(const vec3& from, const vec3& to, u32 abgr) {
        line(from, to, abgr);
        // Простая стрелка — 4 лучика назад.
        vec3 d{ to.x - from.x, to.y - from.y, to.z - from.z };
        f32 len = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
        if (len < 1e-6f) return;
        d.x /= len; d.y /= len; d.z /= len;

        vec3 up = (std::fabs(d.y) < 0.99f) ? vec3{ 0,1,0 } : vec3{ 1,0,0 };
        vec3 t{
            up.y * d.z - up.z * d.y,
            up.z * d.x - up.x * d.z,
            up.x * d.y - up.y * d.x
        };
        f32 tl = std::sqrt(t.x * t.x + t.y * t.y + t.z * t.z);
        t.x /= tl; t.y /= tl; t.z /= tl;
        vec3 b{
            d.y * t.z - d.z * t.y,
            d.z * t.x - d.x * t.z,
            d.x * t.y - d.y * t.x
        };

        f32 head = len * 0.15f;
        vec3 backCenter{
            to.x - d.x * head,
            to.y - d.y * head,
            to.z - d.z * head
        };
        f32 r = head * 0.4f;
        vec3 p1{ backCenter.x + t.x * r, backCenter.y + t.y * r, backCenter.z + t.z * r };
        vec3 p2{ backCenter.x - t.x * r, backCenter.y - t.y * r, backCenter.z - t.z * r };
        vec3 p3{ backCenter.x + b.x * r, backCenter.y + b.y * r, backCenter.z + b.z * r };
        vec3 p4{ backCenter.x - b.x * r, backCenter.y - b.y * r, backCenter.z - b.z * r };
        line(to, p1, abgr);
        line(to, p2, abgr);
        line(to, p3, abgr);
        line(to, p4, abgr);
    }

    void DebugDraw::axes(const vec3& o, f32 size) {
        line(o, { o.x + size, o.y, o.z }, 0xff0000ffu); // X red
        line(o, { o.x, o.y + size, o.z }, 0xff00ff00u); // Y green
        line(o, { o.x, o.y, o.z + size }, 0xffff0000u); // Z blue
    }

    void DebugDraw::lightIcon(const vec3& pos, f32 size, u32 abgr) {
        // Маленькая 6-лучевая "звезда"
        const f32 s = size;
        line({ pos.x - s, pos.y, pos.z }, { pos.x + s, pos.y, pos.z }, abgr);
        line({ pos.x, pos.y - s, pos.z }, { pos.x, pos.y + s, pos.z }, abgr);
        line({ pos.x, pos.y, pos.z - s }, { pos.x, pos.y, pos.z + s }, abgr);
        // Диагонали (короче)
        const f32 d = s * 0.7f;
        line({ pos.x - d, pos.y - d, pos.z }, { pos.x + d, pos.y + d, pos.z }, abgr);
        line({ pos.x - d, pos.y + d, pos.z }, { pos.x + d, pos.y - d, pos.z }, abgr);
    }

    void DebugDraw::flush(bgfx::ViewId view) {
        if (m_verts.empty() || !bgfx::isValid(m_program)) return;

        const u32 numVerts = (u32)m_verts.size();
        if (numVerts != bgfx::getAvailTransientVertexBuffer(numVerts, m_layout)) {
            GRIT_WARN("DebugDraw: not enough transient buffer for {} verts", numVerts);
            return;
        }

        bgfx::TransientVertexBuffer tvb;
        bgfx::allocTransientVertexBuffer(&tvb, numVerts, m_layout);
        std::memcpy(tvb.data, m_verts.data(), numVerts * sizeof(Vertex));

        bgfx::setVertexBuffer(0, &tvb);
        bgfx::setState(
            BGFX_STATE_WRITE_RGB
            | BGFX_STATE_WRITE_A
            | BGFX_STATE_DEPTH_TEST_LEQUAL
            | BGFX_STATE_PT_LINES
            | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA)
        );
        bgfx::submit(view, m_program);

        m_verts.clear();
    }

} // namespace grit::render