#pragma once
#include <entt/entt.hpp>
#include "../core/Math.h"
#include "../asset/AssetHandle.h"
#include <string>
#include <memory>

namespace grit::render { class StaticMesh; class Material; }

namespace grit::ecs {

    struct Transform {
        vec3 position{ 0.f, 0.f, 0.f };
        vec3 rotationEuler{ 0.f, 0.f, 0.f };
        vec3 scale{ 1.f, 1.f, 1.f };
    };

    struct Name {
        std::string value;
    };

    struct MeshRenderer {
        asset::AssetHandle<render::StaticMesh>   mesh;
        std::shared_ptr<render::Material>        material;  // может быть nullptr → default
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