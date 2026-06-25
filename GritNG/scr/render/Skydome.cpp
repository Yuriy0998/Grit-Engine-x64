#include "Skydome.h"
#include "Environment.h"

#include <bx/math.h>
#include <vector>

namespace grit::render {

    struct SkyVertex {
        float pos[3];
    };

    static bgfx::VertexLayout s_layout;
    static bool               s_layoutBuilt = false;

    static const bgfx::VertexLayout& skyLayout() {
        if (!s_layoutBuilt) {
            s_layout.begin()
                .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
                .end();
            s_layoutBuilt = true;
        }
        return s_layout;
    }

    static void buildSphere(std::vector<SkyVertex>& verts,
        std::vector<u16>& indices,
        int slices, int stacks, float radius)
    {
        verts.clear();
        indices.clear();

        for (int i = 0; i <= stacks; ++i) {
            float v = (float)i / (float)stacks;
            float phi = v * bx::kPi;             // 0 .. pi (top → bottom)
            for (int j = 0; j <= slices; ++j) {
                float u = (float)j / (float)slices;
                float theta = u * 2.0f * bx::kPi;
                float x = bx::sin(phi) * bx::cos(theta);
                float y = bx::cos(phi);
                float z = bx::sin(phi) * bx::sin(theta);
                verts.push_back({ x * radius, y * radius, z * radius });
            }
        }

        int ring = slices + 1;
        for (int i = 0; i < stacks; ++i) {
            for (int j = 0; j < slices; ++j) {
                u16 a = (u16)(i * ring + j);
                u16 b = (u16)(a + ring);
                // развернём треугольники "внутрь" — нормали смотрят к камере
                indices.push_back(a);
                indices.push_back((u16)(a + 1));
                indices.push_back(b);

                indices.push_back(b);
                indices.push_back((u16)(a + 1));
                indices.push_back((u16)(b + 1));
            }
        }
    }

    bool Skydome::init(bgfx::ProgramHandle program) {
        m_program = program;

        std::vector<SkyVertex> verts;
        std::vector<u16>       inds;
        buildSphere(verts, inds, 32, 16, 1.0f);

        m_vbh = bgfx::createVertexBuffer(
            bgfx::copy(verts.data(), (u32)(verts.size() * sizeof(SkyVertex))),
            skyLayout());

        m_ibh = bgfx::createIndexBuffer(
            bgfx::copy(inds.data(), (u32)(inds.size() * sizeof(u16))));

        m_indexCount = (u32)inds.size();

        m_uSkyTop = bgfx::createUniform("u_skyTop", bgfx::UniformType::Vec4);
        m_uSkyBottom = bgfx::createUniform("u_skyBottom", bgfx::UniformType::Vec4);
        m_uSunDir = bgfx::createUniform("u_sunDirSky", bgfx::UniformType::Vec4);
        m_uSunColor = bgfx::createUniform("u_sunColorSky", bgfx::UniformType::Vec4);
        m_uSkyParams = bgfx::createUniform("u_skyParams", bgfx::UniformType::Vec4);

        return bgfx::isValid(m_vbh) && bgfx::isValid(m_ibh) && bgfx::isValid(m_program);
    }

    void Skydome::shutdown() {
        if (bgfx::isValid(m_vbh)) { bgfx::destroy(m_vbh);        m_vbh = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_ibh)) { bgfx::destroy(m_ibh);        m_ibh = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uSkyTop)) { bgfx::destroy(m_uSkyTop);    m_uSkyTop = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uSkyBottom)) { bgfx::destroy(m_uSkyBottom); m_uSkyBottom = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uSunDir)) { bgfx::destroy(m_uSunDir);    m_uSunDir = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uSunColor)) { bgfx::destroy(m_uSunColor);  m_uSunColor = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_uSkyParams)) { bgfx::destroy(m_uSkyParams); m_uSkyParams = BGFX_INVALID_HANDLE; }
        // m_program не удаляем — это чужой ресурс
    }

    void Skydome::render(bgfx::ViewId viewId,
        const Environment& env,
        const grit::vec3& cameraPos)
    {
        if (!bgfx::isValid(m_program)) return;

        // model = scale * translate(camera)
        float mtx[16];
        bx::mtxSRT(mtx,
            500.0f, 500.0f, 500.0f,        // scale
            0.0f, 0.0f, 0.0f,               // rotation
            cameraPos.x, cameraPos.y, cameraPos.z);

        bgfx::setTransform(mtx);

        const float skyTop[4] = { env.skyTopColor.x,    env.skyTopColor.y,    env.skyTopColor.z,    1.0f };
        const float skyBottom[4] = { env.skyBottomColor.x, env.skyBottomColor.y, env.skyBottomColor.z, 1.0f };
        const float sunDir[4] = { -env.sunDirection.x, -env.sunDirection.y, -env.sunDirection.z, 0.0f };
        const float sunCol[4] = { env.sunColor.x, env.sunColor.y, env.sunColor.z, env.sunIntensity };
        const float params[4] = { env.timeOfDay, 0.04f, 128.0f, 0.0f };

        bgfx::setUniform(m_uSkyTop, skyTop);
        bgfx::setUniform(m_uSkyBottom, skyBottom);
        bgfx::setUniform(m_uSunDir, sunDir);
        bgfx::setUniform(m_uSunColor, sunCol);
        bgfx::setUniform(m_uSkyParams, params);

        bgfx::setVertexBuffer(0, m_vbh);
        bgfx::setIndexBuffer(m_ibh, 0, m_indexCount);

        const uint64_t state =
            BGFX_STATE_WRITE_RGB
            | BGFX_STATE_WRITE_A;
        // depth test/write — выключены (skydome это фон)

        bgfx::setState(state);
        bgfx::submit(viewId, m_program);
    }

} // namespace grit::render