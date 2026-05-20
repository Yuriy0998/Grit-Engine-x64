#pragma once
#include <string>
#include <bgfx/bgfx.h>

namespace grit {

    namespace ecs { class World; }
    namespace asset { class AssetManager; }

    namespace scene {

        class SceneSerializer {
        public:
            // Сохраняет всё содержимое world'а в JSON.
            // Возвращает true при успехе.
            static bool saveScene(const ecs::World& world, const std::string& path);

            // Загружает сцену в world (НЕ очищает существующий — добавляет к нему).
            // Чтобы заменить — очисти world заранее.
            // assets/defaultProgram нужны для восстановления мешей и материалов.
            static bool loadScene(ecs::World& world,
                const std::string& path,
                asset::AssetManager& assets,
                bgfx::ProgramHandle   defaultProgram);
        };

    } // namespace scene
} // namespace grit