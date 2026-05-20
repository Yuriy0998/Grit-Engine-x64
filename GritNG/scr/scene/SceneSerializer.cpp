#include "SceneSerializer.h"
#include "../ecs/World.h"
#include "../ecs/SceneSystem.h"
#include "../ecs/Light.h"
#include "../asset/AssetManager.h"
#include "../render/Material.h"
#include "../render/StaticMesh.h"
#include "../render/Texture.h"
#include "../core/Log.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <unordered_map>

namespace grit::scene {

    using json = nlohmann::json;

    // ============================================================
    // Хелперы
    // ============================================================
    static json vec3_to_json(const vec3& v) { return json::array({ v.x, v.y, v.z }); }
    static json vec4_to_json(const vec4& v) { return json::array({ v.x, v.y, v.z, v.w }); }

    static vec3 vec3_from_json(const json& j) {
        return { j.at(0).get<f32>(), j.at(1).get<f32>(), j.at(2).get<f32>() };
    }
    static vec4 vec4_from_json(const json& j) {
        return { j.at(0).get<f32>(), j.at(1).get<f32>(),
                 j.at(2).get<f32>(), j.at(3).get<f32>() };
    }

    static json material_to_json(const render::Material& m) {
        json j;
        j["baseColor"] = vec4_to_json(m.baseColor);
        j["tilingOffset"] = vec4_to_json(m.tilingOffset);
        j["pbrParams"] = vec4_to_json(m.pbrParams);
        j["emissive"] = vec4_to_json(m.emissive);
        j["albedo"] = m.albedo ? m.albedo->path() : std::string{};
        return j;
    }

    static std::shared_ptr<render::Material> material_from_json(
        const json& j, asset::AssetManager& assets, bgfx::ProgramHandle prog)
    {
        auto m = std::make_shared<render::Material>();
        m->program = prog;
        m->baseColor = vec4_from_json(j.at("baseColor"));
        m->tilingOffset = vec4_from_json(j.at("tilingOffset"));

        if (j.contains("pbrParams")) m->pbrParams = vec4_from_json(j["pbrParams"]);
        if (j.contains("emissive"))  m->emissive = vec4_from_json(j["emissive"]);

        const std::string tex = j.value("albedo", std::string{});
        if (!tex.empty()) {
            m->albedo = assets.loadTexture(tex);
        }
        return m;
    }

    // ============================================================
    // SAVE
    // ============================================================
    bool SceneSerializer::saveScene(const ecs::World& world, const std::string& path)
    {
        auto& reg = const_cast<entt::registry&>(world.registry()); // storage<> требует non-const

        json root;
        root["version"] = 1;
        root["entities"] = json::array();

        auto idOf = [](entt::entity e) -> std::string {
            return std::to_string((u32)entt::to_integral(e));
            };

        // --- Перебираем все entity ---
        auto& estorage = reg.storage<entt::entity>();
        for (auto ent : estorage) {
            json je;
            je["id"] = idOf(ent);

            // Name
            if (auto* n = reg.try_get<ecs::Name>(ent)) {
                je["name"] = n->value;
            }

            // Transform
            if (auto* t = reg.try_get<ecs::Transform>(ent)) {
                json jt;
                jt["position"] = vec3_to_json(t->position);
                jt["rotationEuler"] = vec3_to_json(t->rotationEuler);
                jt["scale"] = vec3_to_json(t->scale);
                je["transform"] = jt;
            }

            // Parent
            if (auto* p = reg.try_get<ecs::Parent>(ent); p && p->entity != entt::null) {
                je["parent"] = idOf(p->entity);
            }

            // MeshRenderer
            if (auto* mr = reg.try_get<ecs::MeshRenderer>(ent); mr && mr->mesh) {
                json jmr;
                jmr["meshPath"] = mr->meshPath;
                jmr["visible"] = mr->visible;
                jmr["materials"] = json::array();
                for (auto& m : mr->materials) {
                    jmr["materials"].push_back(m ? material_to_json(*m) : json{});
                }
                je["meshRenderer"] = jmr;
            }

            // LightDirectional
            if (auto* L = reg.try_get<ecs::LightDirectional>(ent)) {
                json jl;
                jl["direction"] = vec3_to_json(L->direction);
                jl["color"] = vec3_to_json(L->color);
                jl["intensity"] = L->intensity;
                jl["ambient"] = L->ambient;
                je["lightDirectional"] = jl;
            }

            root["entities"].push_back(std::move(je));
        }

        std::ofstream f(path);
        if (!f) {
            GRIT_ERROR("SceneSerializer::saveScene: cannot open %s", path.c_str());
            return false;
        }
        f << root.dump(2);
        GRIT_INFO("Scene saved: %s (%zu entities)", path.c_str(),
            root["entities"].size());
        return true;
    }

    // ============================================================
    // LOAD
    // ============================================================
    bool SceneSerializer::loadScene(ecs::World& world,
        const std::string& path,
        asset::AssetManager& assets,
        bgfx::ProgramHandle   defaultProgram)
    {
        std::ifstream f(path);
        if (!f) {
            GRIT_ERROR("SceneSerializer::loadScene: cannot open %s", path.c_str());
            return false;
        }

        json root;
        try {
            f >> root;
        }
        catch (const std::exception& e) {
            GRIT_ERROR("SceneSerializer::loadScene: parse error: %s", e.what());
            return false;
        }

        if (!root.contains("entities")) {
            GRIT_ERROR("SceneSerializer::loadScene: missing 'entities' in %s", path.c_str());
            return false;
        }

        auto& reg = world.registry();

        // oldId(string) -> новый entt::entity
        std::unordered_map<std::string, entt::entity> idMap;

        const auto& ents = root.at("entities");

        // --- Первый проход: создаём entity и компоненты (кроме Parent) ---
        for (const auto& je : ents) {
            entt::entity ent = world.createEntity();
            idMap[je.at("id").get<std::string>()] = ent;

            if (je.contains("name")) {
                reg.emplace<ecs::Name>(ent, ecs::Name{ je["name"].get<std::string>() });
            }

            if (je.contains("transform")) {
                const auto& jt = je["transform"];
                ecs::Transform t;
                t.position = vec3_from_json(jt.at("position"));
                t.rotationEuler = vec3_from_json(jt.at("rotationEuler"));
                t.scale = vec3_from_json(jt.at("scale"));
                reg.emplace<ecs::Transform>(ent, t);
            }

            if (je.contains("meshRenderer")) {
                const auto& jmr = je["meshRenderer"];
                ecs::MeshRenderer mr;
                mr.visible = jmr.value("visible", true);
                mr.meshPath = jmr.value("meshPath", std::string{});

                if (!mr.meshPath.empty()) {
                    auto loaded = assets.loadModel(mr.meshPath, defaultProgram);
                    mr.mesh = loaded.mesh;
                }

                if (jmr.contains("materials")) {
                    for (const auto& jm : jmr["materials"]) {
                        if (jm.is_null() || jm.empty()) {
                            mr.materials.push_back(nullptr);
                        }
                        else {
                            mr.materials.push_back(
                                material_from_json(jm, assets, defaultProgram));
                        }
                    }
                }

                reg.emplace<ecs::MeshRenderer>(ent, std::move(mr));
            }

            if (je.contains("lightDirectional")) {
                const auto& jl = je["lightDirectional"];
                ecs::LightDirectional L;
                L.direction = vec3_from_json(jl.at("direction"));
                L.color = vec3_from_json(jl.at("color"));
                L.intensity = jl.value("intensity", 1.0f);
                L.ambient = jl.value("ambient", 0.0f);
                reg.emplace<ecs::LightDirectional>(ent, L);
            }
        }

        // --- Второй проход: иерархия ---
        ecs::SceneSystem scene;
        for (const auto& je : ents) {
            if (!je.contains("parent")) continue;
            auto childIt = idMap.find(je.at("id").get<std::string>());
            auto parentIt = idMap.find(je.at("parent").get<std::string>());
            if (childIt == idMap.end() || parentIt == idMap.end()) continue;

            scene.setParent(world, childIt->second, parentIt->second);
        }

        GRIT_INFO("Scene loaded: %s (%zu entities)", path.c_str(), ents.size());
        return true;
    }

} // namespace grit::scene