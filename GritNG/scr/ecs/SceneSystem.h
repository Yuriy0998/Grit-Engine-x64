#pragma once

#include "../core/Types.h"
#include <entt/entt.hpp>

namespace grit::ecs {

    class World;

    // ”правл€ет иерархией Transform'ов и пересчитывает world-матрицы.
    //
    // »спользование:
    //   SceneSystem scene;
    //   scene.setParent(world, child, parent);   // создать св€зь
    //   scene.setParent(world, child, entt::null); // отв€зать (стать корнем)
    //   scene.removeEntity(world, e);            // безопасное удаление с детьми
    //   scene.update(world);                     // каждый кадр Ч обновить worldMatrix
    class SceneSystem {
    public:
        // ”становить родител€. parent == entt::null Ч сделать корнем.
        // Ѕезопасно от циклов: если parent €вл€етс€ потомком child Ч операци€ отклон€етс€.
        bool setParent(World& world, entt::entity child, entt::entity parent);

        // ”далить entity и рекурсивно всех потомков.
        void removeEntity(World& world, entt::entity e);

        // ѕересчитать worldMatrix у всех Transform'ов (DFS от корней).
        void update(World& world);

        // ¬спомогательное: создать дочернее entity (с Transform и Parent уже выставленными)
        entt::entity createChild(World& world, entt::entity parent);

    private:
        void recomputeRecursive(entt::registry& reg,
            entt::entity    e,
            const f32       parentMatrix[16]);

        // ”далить из Children старого родител€
        void detachFromParent(entt::registry& reg, entt::entity child);
    };

} // namespace grit::ecs