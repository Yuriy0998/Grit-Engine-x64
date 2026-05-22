#include "Primitives.h"
#include "StaticMesh.h"
#include <vector>
#include <cmath>
#include <cstdint>

namespace grit::render::Primitives {

    static std::shared_ptr<StaticMesh> buildMesh(
        const std::vector<StaticVertex>& verts,
        const std::vector<uint32_t>& indices,
        bool singleSubmesh = true)
    {
        auto mesh = std::make_shared<StaticMesh>();
        if (!mesh->createFromArrays(
            verts.data(), (uint32_t)verts.size(),
            indices.data(), (uint32_t)indices.size()))
        {
            return nullptr;
        }

        if (singleSubmesh) {
            Submesh s;
            s.indexStart = 0;
            s.indexCount = (uint32_t)indices.size();
            s.materialIndex = 0;
            mesh->setSubmeshes({ s });
        }
        return mesh;
    }

    std::shared_ptr<StaticMesh> createCube(float size)
    {
        const float s = size * 0.5f;

        // 24 вершины (4 на грань) Ч корректные нормали и UV
        std::vector<StaticVertex> v = {
            // +X
            { s,-s,-s,  1,0,0, 0,1 }, { s, s,-s,  1,0,0, 0,0 },
            { s, s, s,  1,0,0, 1,0 }, { s,-s, s,  1,0,0, 1,1 },
            // -X
            {-s,-s, s, -1,0,0, 0,1 }, {-s, s, s, -1,0,0, 0,0 },
            {-s, s,-s, -1,0,0, 1,0 }, {-s,-s,-s, -1,0,0, 1,1 },
            // +Y
            {-s, s,-s,  0,1,0, 0,1 }, {-s, s, s,  0,1,0, 0,0 },
            { s, s, s,  0,1,0, 1,0 }, { s, s,-s,  0,1,0, 1,1 },
            // -Y
            {-s,-s, s,  0,-1,0, 0,1 }, {-s,-s,-s, 0,-1,0, 0,0 },
            { s,-s,-s,  0,-1,0, 1,0 }, { s,-s, s,  0,-1,0, 1,1 },
            // +Z
            { s,-s, s,  0,0,1, 0,1 }, { s, s, s,  0,0,1, 0,0 },
            {-s, s, s,  0,0,1, 1,0 }, {-s,-s, s,  0,0,1, 1,1 },
            // -Z
            {-s,-s,-s, 0,0,-1, 0,1 }, {-s, s,-s, 0,0,-1, 0,0 },
            { s, s,-s,  0,0,-1, 1,0 }, { s,-s,-s, 0,0,-1, 1,1 },
        };

        std::vector<uint32_t> idx;
        idx.reserve(36);
        for (uint32_t f = 0; f < 6; ++f) {
            uint32_t b = f * 4;
            idx.push_back(b);     idx.push_back(b + 1); idx.push_back(b + 2);
            idx.push_back(b);     idx.push_back(b + 2); idx.push_back(b + 3);
        }
        return buildMesh(v, idx);
    }

    std::shared_ptr<StaticMesh> createPlane(float size)
    {
        const float s = size * 0.5f;
        std::vector<StaticVertex> v = {
            {-s, 0, -s,  0,1,0, 0,0 },
            { s, 0, -s,  0,1,0, 1,0 },
            { s, 0,  s,  0,1,0, 1,1 },
            {-s, 0,  s,  0,1,0, 0,1 },
        };
        std::vector<uint32_t> idx = { 0,1,2, 0,2,3 };
        return buildMesh(v, idx);
    }

    std::shared_ptr<StaticMesh> createSphere(float radius, int segments)
    {
        std::vector<StaticVertex> v;
        std::vector<uint32_t> idx;

        const int   rings = segments;
        const int   sectors = segments * 2;
        const float pi = 3.14159265358979f;

        v.reserve((rings + 1) * (sectors + 1));
        for (int r = 0; r <= rings; ++r) {
            float phi = pi * (float)r / (float)rings;
            float sp = std::sin(phi), cp = std::cos(phi);
            for (int s = 0; s <= sectors; ++s) {
                float theta = 2.0f * pi * (float)s / (float)sectors;
                float st = std::sin(theta), ct = std::cos(theta);

                float nx = sp * ct, ny = cp, nz = sp * st;
                v.push_back({
                    nx * radius, ny * radius, nz * radius,
                    nx, ny, nz,
                    (float)s / (float)sectors,
                    (float)r / (float)rings
                    });
            }
        }

        const uint32_t stride = (uint32_t)sectors + 1;
        for (uint32_t r = 0; r < (uint32_t)rings; ++r) {
            for (uint32_t s = 0; s < (uint32_t)sectors; ++s) {
                uint32_t a = r * stride + s;
                uint32_t b = a + stride;
                idx.push_back(a);     idx.push_back(b);     idx.push_back(a + 1);
                idx.push_back(a + 1); idx.push_back(b);     idx.push_back(b + 1);
            }
        }
        return buildMesh(v, idx);
    }

} // namespace grit::render::Primitives