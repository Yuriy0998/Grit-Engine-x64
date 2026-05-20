#pragma once
#include <entt/entt.hpp>
#include "../core/Math.h"
#include "../core/Types.h"
#include "../asset/AssetHandle.h"
#include <string>
#include <memory>

namespace grit::render { class StaticMesh; class Material; }

namespace grit::ecs {

    struct Transform {
        vec3 position{ 0.f, 0.f, 0.f };
        vec3 rotationEuler{ 0.f, 0.f, 0.f };
        vec3 scale{ 1.f, 1.f, 1.f };

        // Заполняется SceneSystem каждый кадр (parent * local). Read-only для пользователя.
        f32  worldMatrix[16] = {
            1,0,0,0,
            0,1,0,0,
            0,0,1,0,
            0,0,0,1
        };
    };

    struct Name {
        std::string value;
    };

    struct MeshRenderer {
        std::shared_ptr<grit::render::StaticMesh> mesh;
        std::vector<std::shared_ptr<grit::render::Material>> materials;

        // Путь к ассету меша (для сериализации). Опциональный.
        std::string meshPath;

        bool visible = true;

        void setMaterial(std::shared_ptr<grit::render::Material> m) {
            materials.clear();
            if (m) materials.push_back(std::move(m));
        }
        std::shared_ptr<grit::render::Material> material() const {
            return materials.empty() ? nullptr : materials.front();
        }
    };

    // --- Иерархия ---
    struct Parent {
        entt::entity entity = entt::null;
    };

    struct Children {
        std::vector<entt::entity> list;
    };

    class World {
    public:
        entt::registry& registry() { return m_reg; }
        const entt::registry& registry() const { return m_reg; }
        entt::entity createEntity() { return m_reg.create(); }

    private:
        entt::registry m_reg;
    };

} // namespace grit::ecs