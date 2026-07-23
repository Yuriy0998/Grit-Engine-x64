#include "Renderer.h"
#include "../ecs/World.h"
#include "../ecs/Light.h"
#include <bgfx/bgfx.h>
#include <cmath>

namespace grit::render {

    void Renderer::collectLights(ecs::World& world)
    {
        auto& reg = world.registry();
        auto& L = m_lights;
        L.dirCount = L.pointCount = L.spotCount = 0;
        L.ambient = 0.f;

        // Directional
        for (auto ent : reg.view<grit::ecs::LightDirectional>()) {
            if (L.dirCount >= kMaxDirLights) break;
            auto& l = reg.get<grit::ecs::LightDirectional>(ent);
            f32 x = l.direction.x, y = l.direction.y, z = l.direction.z;
            f32 len = std::sqrt(x * x + y * y + z * z); if (len < 1e-6f) len = 1.f;
            u32 i = L.dirCount * 4;
            L.dirDir[i + 0] = x / len; L.dirDir[i + 1] = y / len; L.dirDir[i + 2] = z / len; L.dirDir[i + 3] = 0.f;
            L.dirColor[i + 0] = l.color.x; L.dirColor[i + 1] = l.color.y;
            L.dirColor[i + 2] = l.color.z; L.dirColor[i + 3] = l.intensity;
            if (L.dirCount == 0) L.ambient = l.ambient;
            ++L.dirCount;
        }

        // Point
        for (auto ent : reg.view<grit::ecs::Transform, grit::ecs::LightPoint>()) {
            if (L.pointCount >= kMaxPointLights) break;
            auto& tr = reg.get<grit::ecs::Transform>(ent);
            auto& l = reg.get<grit::ecs::LightPoint>(ent);
            u32 i = L.pointCount * 4;
            L.pointPos[i + 0] = tr.position.x; L.pointPos[i + 1] = tr.position.y;
            L.pointPos[i + 2] = tr.position.z; L.pointPos[i + 3] = l.range;
            L.pointColor[i + 0] = l.color.x; L.pointColor[i + 1] = l.color.y;
            L.pointColor[i + 2] = l.color.z; L.pointColor[i + 3] = l.intensity;
            ++L.pointCount;
        }

        // Spot
        for (auto ent : reg.view<grit::ecs::Transform, grit::ecs::LightSpot>()) {
            if (L.spotCount >= kMaxSpotLights) break;
            auto& tr = reg.get<grit::ecs::Transform>(ent);
            auto& l = reg.get<grit::ecs::LightSpot>(ent);
            u32 i = L.spotCount * 4;
            L.spotPos[i + 0] = tr.position.x; L.spotPos[i + 1] = tr.position.y;
            L.spotPos[i + 2] = tr.position.z; L.spotPos[i + 3] = l.range;
            f32 dx = l.direction.x, dy = l.direction.y, dz = l.direction.z;
            f32 dl = std::sqrt(dx * dx + dy * dy + dz * dz); if (dl < 1e-6f) dl = 1.f;
            L.spotDir[i + 0] = dx / dl; L.spotDir[i + 1] = dy / dl; L.spotDir[i + 2] = dz / dl; L.spotDir[i + 3] = 0.f;
            L.spotColor[i + 0] = l.color.x; L.spotColor[i + 1] = l.color.y;
            L.spotColor[i + 2] = l.color.z; L.spotColor[i + 3] = l.intensity;
            L.spotParams[i + 0] = std::cos(l.innerCone);
            L.spotParams[i + 1] = std::cos(l.outerCone);
            ++L.spotCount;
        }

        // Fallback на TimeOfDay
        if (L.dirCount == 0) {
            L.dirDir[0] = m_env.sunDirection.x; L.dirDir[1] = m_env.sunDirection.y;
            L.dirDir[2] = m_env.sunDirection.z; L.dirDir[3] = 0.f;
            L.dirColor[0] = m_env.sunColor.x; L.dirColor[1] = m_env.sunColor.y;
            L.dirColor[2] = m_env.sunColor.z; L.dirColor[3] = m_env.sunIntensity;
            L.dirCount = 1; L.ambient = 0.15f;
        }

        // Upload
        f32 dirCnt[4] = { (f32)L.dirCount, L.ambient, 0.f, 0.f };
        bgfx::setUniform(m_uDirLightDir, L.dirDir, kMaxDirLights);
        bgfx::setUniform(m_uDirLightColor, L.dirColor, kMaxDirLights);
        bgfx::setUniform(m_uDirLightCount, dirCnt);

        f32 pCnt[4] = { (f32)L.pointCount, 0, 0, 0 };
        bgfx::setUniform(m_uPointLightPos, L.pointPos, kMaxPointLights);
        bgfx::setUniform(m_uPointLightColor, L.pointColor, kMaxPointLights);
        bgfx::setUniform(m_uPointLightCount, pCnt);

        f32 sCnt[4] = { (f32)L.spotCount, 0, 0, 0 };
        bgfx::setUniform(m_uSpotLightPos, L.spotPos, kMaxSpotLights);
        bgfx::setUniform(m_uSpotLightDir, L.spotDir, kMaxSpotLights);
        bgfx::setUniform(m_uSpotLightColor, L.spotColor, kMaxSpotLights);
        bgfx::setUniform(m_uSpotLightParams, L.spotParams, kMaxSpotLights);
        bgfx::setUniform(m_uSpotLightCount, sCnt);
    }

} // namespace grit::render