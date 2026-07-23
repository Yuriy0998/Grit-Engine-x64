#include "PrefabSerializer.h"
#include "../ecs/World.h"
#include "../ecs/SceneSystem.h"
#include "../ecs/Light.h"
#include "../asset/AssetManager.h"
#include "../render/Material.h"
#include "../render/StaticMesh.h"
#include "../render/Texture.h"
#include "../core/Log.h"
#include "../render/Primitives.h"


#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

namespace grit::scene {

    using json = nlohmann::json;

    // ============================================================
    // Хелперы (те же, что в SceneSerializer)
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
    // Сериализация одного entity в JSON
    // ============================================================
    static json entityToJson(entt::registry& reg, entt::entity e)
    {
        json je;

        // Name
        if (auto* n = reg.try_get<ecs::Name>(e)) {
            je["name"] = n->value;
        }

        // Transform
        if (auto* t = reg.try_get<ecs::Transform>(e)) {
            json jt;
            jt["position"] = vec3_to_json(t->position);
            jt["rotationEuler"] = vec3_to_json(t->rotationEuler);
            jt["scale"] = vec3_to_json(t->scale);
            je["transform"] = jt;
        }

        // MeshRenderer
        if (auto* mr = reg.try_get<ecs::MeshRenderer>(e); mr && (mr->mesh || !mr->meshPath.empty())) {
            
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
        if (auto* L = reg.try_get<ecs::LightDirectional>(e)) {
            json jl;
            jl["direction"] = vec3_to_json(L->direction);
            jl["color"] = vec3_to_json(L->color);
            jl["intensity"] = L->intensity;
            jl["ambient"] = L->ambient;
            je["lightDirectional"] = jl;
        }

        // LightPoint
        if (auto* L = reg.try_get<ecs::LightPoint>(e)) {
            json jl;
            jl["color"] = vec3_to_json(L->color);
            jl["intensity"] = L->intensity;
            jl["range"] = L->range;
            je["lightPoint"] = jl;
        }

        // LightSpot
        if (auto* L = reg.try_get<ecs::LightSpot>(e)) {
            json jl;
            jl["direction"] = vec3_to_json(L->direction);
            jl["color"] = vec3_to_json(L->color);
            jl["intensity"] = L->intensity;
            jl["range"] = L->range;
            jl["innerCone"] = L->innerCone;
            jl["outerCone"] = L->outerCone;
            je["lightSpot"] = jl;
        }

        return je;
    }

    // ============================================================
    // Десериализация одного entity из JSON
    // ============================================================
    static void applyJsonToEntity(const json& je,
        ecs::World& world,
        entt::entity         e,
        asset::AssetManager& assets,
        bgfx::ProgramHandle  defaultProgram)
    {
        auto& reg = world.registry();

        if (je.contains("name")) {
            reg.emplace_or_replace<ecs::Name>(
                e, ecs::Name{ je["name"].get<std::string>() });
        }

        if (je.contains("transform")) {
            const auto& jt = je["transform"];
            ecs::Transform t;
            t.position = vec3_from_json(jt.at("position"));
            t.rotationEuler = vec3_from_json(jt.at("rotationEuler"));
            t.scale = vec3_from_json(jt.at("scale"));
            reg.emplace_or_replace<ecs::Transform>(e, t);
        }
        else {
            // Если Transform в префабе нет — всё равно даём дефолтный
            if (!reg.any_of<ecs::Transform>(e))
                reg.emplace<ecs::Transform>(e);
        }

        if (je.contains("meshRenderer")) {
            const auto& jmr = je["meshRenderer"];
            ecs::MeshRenderer mr;
            mr.visible = jmr.value("visible", true);
            mr.meshPath = jmr.value("meshPath", std::string{});

            if (!mr.meshPath.empty()) {
                // Проверяем на примитив
                const std::string primPrefix = "@primitive:";
                if (mr.meshPath.rfind(primPrefix, 0) == 0) {
                    const std::string kind = mr.meshPath.substr(primPrefix.size());
                    if (kind == "Cube")   mr.mesh = render::Primitives::createCube();
                    else if (kind == "Sphere") mr.mesh = render::Primitives::createSphere();
                    else if (kind == "Plane")  mr.mesh = render::Primitives::createPlane();
                }
                else {
                    auto loaded = assets.loadModel(mr.meshPath, defaultProgram);
                    mr.mesh = loaded.mesh;
                }
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

            reg.emplace_or_replace<ecs::MeshRenderer>(e, std::move(mr));
        }

        if (je.contains("lightDirectional")) {
            const auto& jl = je["lightDirectional"];
            ecs::LightDirectional L;
            L.direction = vec3_from_json(jl.at("direction"));
            L.color = vec3_from_json(jl.at("color"));
            L.intensity = jl.value("intensity", 1.0f);
            L.ambient = jl.value("ambient", 0.0f);
            reg.emplace_or_replace<ecs::LightDirectional>(e, L);
        }

        if (je.contains("lightPoint")) {
            const auto& jl = je["lightPoint"];
            ecs::LightPoint L;
            L.color = vec3_from_json(jl.at("color"));
            L.intensity = jl.value("intensity", 1.0f);
            L.range = jl.value("range", 10.0f);
            reg.emplace_or_replace<ecs::LightPoint>(e, L);
        }

        if (je.contains("lightSpot")) {
            const auto& jl = je["lightSpot"];
            ecs::LightSpot L;
            L.direction = vec3_from_json(jl.at("direction"));
            L.color = vec3_from_json(jl.at("color"));
            L.intensity = jl.value("intensity", 1.0f);
            L.range = jl.value("range", 10.0f);
            L.innerCone = jl.value("innerCone", 0.5f);
            L.outerCone = jl.value("outerCone", 0.7f);
            reg.emplace_or_replace<ecs::LightSpot>(e, L);
        }
    }

    // ============================================================
    // SAVE
    // ============================================================
    bool PrefabSerializer::savePrefab(const ecs::World& world,
        entt::entity      e,
        const std::string& path)
    {
        auto& reg = const_cast<entt::registry&>(world.registry());
        if (!reg.valid(e)) {
            GRIT_ERROR("PrefabSerializer::savePrefab: invalid entity");
            return false;
        }

        json root;
        root["version"] = 1;
        root["kind"] = "prefab";

        std::string name = "Prefab";
        if (auto* n = reg.try_get<ecs::Name>(e); n && !n->value.empty())
            name = n->value;
        root["name"] = name;

        root["entity"] = entityToJson(reg, e);

        // Гарантируем что папка есть
        std::error_code ec;
        std::filesystem::create_directories(
            std::filesystem::path(path).parent_path(), ec);

        std::ofstream f(path);
        if (!f) {
            GRIT_ERROR("PrefabSerializer::savePrefab: cannot open %s", path.c_str());
            return false;
        }
        f << root.dump(2);
        GRIT_INFO("Prefab saved: %s", path.c_str());
        return true;
    }

    // ============================================================
    // LOAD
    // ============================================================
    entt::entity PrefabSerializer::loadPrefab(ecs::World& world,
        const std::string& path,
        asset::AssetManager& assets,
        bgfx::ProgramHandle  defaultProgram)
    {
        std::ifstream f(path);
        if (!f) {
            GRIT_ERROR("PrefabSerializer::loadPrefab: cannot open %s", path.c_str());
            return entt::null;
        }

        json root;
        try {
            f >> root;
        }
        catch (const std::exception& ex) {
            GRIT_ERROR("PrefabSerializer::loadPrefab: parse error: %s", ex.what());
            return entt::null;
        }

        if (!root.contains("entity")) {
            GRIT_ERROR("PrefabSerializer::loadPrefab: missing 'entity' in %s",
                path.c_str());
            return entt::null;
        }

        entt::entity e = world.createEntity();
        applyJsonToEntity(root["entity"], world, e, assets, defaultProgram);

        // Если в префабе не было имени — даём имя от файла
        auto& reg = world.registry();
        if (!reg.any_of<ecs::Name>(e)) {
            std::string stem = std::filesystem::path(path).stem().string();
            reg.emplace<ecs::Name>(e, ecs::Name{ stem });
        }

        GRIT_INFO("Prefab loaded: %s", path.c_str());
        return e;
    }

} // namespace grit::scene