#pragma once
#include <entt/entt.hpp>
#include <bgfx/bgfx.h>
#include <string>

namespace grit {
    namespace ecs { class World; }
    namespace asset { class AssetManager; }
}

namespace grit::scene {

    class PrefabSerializer {
    public:
        // Сохраняет один entity (компоненты) в файл .grprefab (JSON).
        // Иерархия parent-child в v1 НЕ сохраняется (только сам entity).
        static bool savePrefab(const ecs::World& world,
            entt::entity      e,
            const std::string& path);

        // Загружает .grprefab, создаёт новый entity, возвращает его.
        // Позиция берётся из префаба; если нужно спавнить в другое место —
        // патчи Transform делай сам после вызова.
        // Возвращает entt::null при ошибке.
        static entt::entity loadPrefab(ecs::World& world,
            const std::string& path,
            asset::AssetManager& assets,
            bgfx::ProgramHandle  defaultProgram);
    };

} // namespace grit::scene