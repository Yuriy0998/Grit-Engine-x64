#pragma once

#include "../core/Types.h"
#include "../core/Math.h"
#include <entt/entt.hpp>
#include <memory>
#include <string>
#include <vector>

namespace grit::ecs {

    // Локальная TRS + кэш world-матрицы
    struct Transform {
        vec3 position{ 0.f, 0.f, 0.f };
        vec3 rotationEuler{ 0.f, 0.f, 0.f };  // в радианах, порядок XYZ
        vec3 scale{ 1.f, 1.f, 1.f };

        // --- Заполняется SceneSystem каждый кадр, читать только! ---
        f32  worldMatrix[16] = {
            1,0,0,0,
            0,1,0,0,
            0,0,1,0,
            0,0,0,1
        };
    };

    // Связь с родителем. Корневые entity этого компонента не имеют.
    struct Parent {
        entt::entity entity = entt::null;
    };

    // Список детей. Поддерживается SceneSystem'ом автоматически
    // (при изменении Parent — пересобирается).
    // Можно читать для итерации, но не модифицировать вручную.
    struct Children {
        std::vector<entt::entity> list;
    };

    struct Name {
        std::string value;
    };

    namespace render { class StaticMesh; class Material; }
}

namespace grit::render {
    class StaticMesh;
    class Material;
}

namespace grit::ecs {

    struct MeshRenderer {
        std::shared_ptr<grit::render::StaticMesh> mesh;
        std::shared_ptr<grit::render::Material>   material;
        bool visible = true;
    };

} // namespace grit::ecs