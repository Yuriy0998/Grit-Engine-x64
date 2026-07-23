#include "Renderer.h"
#include "../ecs/World.h"
#include "../ecs/Light.h"
#include "../core/Math.h"
#include <bgfx/bgfx.h>
#include <cmath>

namespace grit::render {

    void Renderer::renderDebug(ecs::World& world)
    {
        if (!m_debugEntry || !bgfx::isValid(m_debugEntry->program)) return;

        m_debug.begin();
        const u32 kColSelected = 0xff00ffffu;
        auto& reg = world.registry();

        // --- Directional lights ---
        for (auto ent : reg.view<grit::ecs::LightDirectional>()) {
            auto& L = reg.get<grit::ecs::LightDirectional>(ent);
            vec3 pos{ 0,0,0 };
            if (auto* tr = reg.try_get<grit::ecs::Transform>(ent)) pos = tr->position;

            u32 col = 0xff000000u
                | ((u32)(L.color.z * 255.f) << 16)
                | ((u32)(L.color.y * 255.f) << 8)
                | ((u32)(L.color.x * 255.f));
            if (ent == m_selectedEntity) col = kColSelected;

            m_debug.lightIcon(pos, 0.25f, col);

            vec3 d = L.direction;
            f32 dl = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
            if (dl > 1e-6f) {
                d.x /= dl; d.y /= dl; d.z /= dl;
                vec3 to{ pos.x + d.x * 1.5f, pos.y + d.y * 1.5f, pos.z + d.z * 1.5f };
                m_debug.arrow(pos, to, col);
            }
        }

        // --- Point lights ---
        for (auto ent : reg.view<grit::ecs::Transform, grit::ecs::LightPoint>()) {
            auto& tr = reg.get<grit::ecs::Transform>(ent);
            auto& L = reg.get<grit::ecs::LightPoint>(ent);
            u32 col = 0xff000000u
                | ((u32)(L.color.z * 255.f) << 16)
                | ((u32)(L.color.y * 255.f) << 8)
                | ((u32)(L.color.x * 255.f));
            bool sel = (ent == m_selectedEntity);
            m_debug.lightIcon(tr.position, 0.25f, sel ? kColSelected : col);
            if (sel) m_debug.wireSphere(tr.position, L.range, col, 32);
        }

        // --- Spot lights ---
        for (auto ent : reg.view<grit::ecs::Transform, grit::ecs::LightSpot>()) {
            auto& tr = reg.get<grit::ecs::Transform>(ent);
            auto& L = reg.get<grit::ecs::LightSpot>(ent);
            u32 col = 0xff000000u
                | ((u32)(L.color.z * 255.f) << 16)
                | ((u32)(L.color.y * 255.f) << 8)
                | ((u32)(L.color.x * 255.f));
            bool sel = (ent == m_selectedEntity);
            m_debug.lightIcon(tr.position, 0.25f, sel ? kColSelected : col);
            if (sel) {
                m_debug.wireCone(tr.position, L.direction, L.range, L.outerCone, col, 24);
                m_debug.wireCone(tr.position, L.direction, L.range, L.innerCone,
                    (col & 0x00ffffffu) | 0x80000000u, 24);
            }
        }

        m_debug.flush(kViewScene);
    }

} // namespace grit::render