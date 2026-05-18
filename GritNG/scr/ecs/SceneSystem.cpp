#include "SceneSystem.h"
#include "World.h"
#include "../core/Math.h"
#include "../core/Log.h"

#include <bx/math.h>
#include <algorithm>

namespace grit::ecs {

    static void composeTRS(const Transform& t, f32 out[16]) {
        bx::mtxSRT(out,
            t.scale.x, t.scale.y, t.scale.z,
            t.rotationEuler.x, t.rotationEuler.y, t.rotationEuler.z,
            t.position.x, t.position.y, t.position.z);
    }

    static void identityMatrix(f32 m[16]) {
        bx::mtxIdentity(m);
    }

    static bool isAncestor(entt::registry& reg, entt::entity node, entt::entity potentialAncestor) {
        entt::entity cur = node;
        while (cur != entt::null) {
            if (cur == potentialAncestor) return true;
            auto* p = reg.try_get<Parent>(cur);
            if (!p) break;
            cur = p->entity;
        }
        return false;
    }

    void SceneSystem::detachFromParent(entt::registry& reg, entt::entity child) {
        auto* p = reg.try_get<Parent>(child);
        if (!p || p->entity == entt::null) return;

        if (auto* siblings = reg.try_get<Children>(p->entity)) {
            auto& v = siblings->list;
            v.erase(std::remove(v.begin(), v.end(), child), v.end());
            if (v.empty()) {
                reg.remove<Children>(p->entity);
            }
        }
    }

    bool SceneSystem::setParent(World& world, entt::entity child, entt::entity parent) {
        auto& reg = world.registry();
        if (!reg.valid(child)) return false;

        if (parent != entt::null) {
            if (!reg.valid(parent)) return false;
            if (parent == child)    return false;
            if (isAncestor(reg, parent, child)) {
                GRIT_WARN("setParent: cycle detected, rejected");
                return false;
            }
        }

        detachFromParent(reg, child);

        if (parent == entt::null) {
            if (reg.all_of<Parent>(child)) {
                reg.remove<Parent>(child);
            }
        }
        else {
            reg.emplace_or_replace<Parent>(child, Parent{ parent });
            auto& ch = reg.get_or_emplace<Children>(parent);
            if (std::find(ch.list.begin(), ch.list.end(), child) == ch.list.end()) {
                ch.list.push_back(child);
            }
        }
        return true;
    }

    void SceneSystem::removeEntity(World& world, entt::entity e) {
        auto& reg = world.registry();
        if (!reg.valid(e)) return;

        if (auto* ch = reg.try_get<Children>(e)) {
            auto copy = ch->list;
            for (auto c : copy) {
                removeEntity(world, c);
            }
        }
        detachFromParent(reg, e);
        reg.destroy(e);
    }

    void SceneSystem::recomputeRecursive(entt::registry& reg,
        entt::entity    e,
        const f32       parentMatrix[16]) {
        auto* t = reg.try_get<Transform>(e);
        if (!t) {
            if (auto* ch = reg.try_get<Children>(e)) {
                for (auto c : ch->list) recomputeRecursive(reg, c, parentMatrix);
            }
            return;
        }

        f32 local[16];
        composeTRS(*t, local);

        // world = parent * local
        bx::mtxMul(t->worldMatrix, local, parentMatrix);

        if (auto* ch = reg.try_get<Children>(e)) {
            for (auto c : ch->list) {
                recomputeRecursive(reg, c, t->worldMatrix);
            }
        }
    }

    void SceneSystem::update(World& world) {
        auto& reg = world.registry();

        f32 identity[16];
        identityMatrix(identity);

        auto view = reg.view<Transform>();
        for (auto e : view) {
            if (reg.all_of<Parent>(e)) continue;
            recomputeRecursive(reg, e, identity);
        }
    }

    entt::entity SceneSystem::createChild(World& world, entt::entity parent) {
        auto e = world.createEntity();
        world.registry().emplace<Transform>(e);
        setParent(world, e, parent);
        return e;
    }

} // namespace grit::ecs