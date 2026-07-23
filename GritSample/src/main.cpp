#include <Engine.h>
#include <core/Log.h>
#include <core/Math.h>
#include <core/Input.h>
#include <ecs/World.h>
#include <ecs/SceneSystem.h>
#include "ecs/Light.h"
#include <render/Material.h>
#include <render/StaticMesh.h>
#include <render/Texture.h>
#include <render/CameraController.h>
#include <scene/SceneSerializer.h>


int main() {
    grit::Engine engine;

    grit::EngineDesc desc;
    desc.window.title = "GritNG Sample - Stage 3.4 (Submeshes & Multi-material)";
    desc.window.width = 1280;
    desc.window.height = 720;

    if (!engine.init(desc)) {
        GRIT_ERROR("Engine init failed");
        return -1;
    }

    auto& assets = engine.assets();
    auto& world = engine.world();
    auto& input = engine.input();
    auto& scene = engine.scene();

    const auto defaultProgram = engine.renderer().defaultProgram();

    // --- Загружаем модель (меш + все материалы из .grmesh) ---
    auto model = assets.loadModel("assets/meshes/cube.grmesh", defaultProgram);
    if (!model.mesh) { engine.shutdown(); return -1; }
    auto cubeMesh = model.mesh;

    // Базовый материал: либо из .grmesh, либо fallback
    auto baseMaterial = !model.materials.empty()
        ? model.materials[0]
        : ([&]() {
        auto m = std::make_shared<grit::render::Material>();
        m->program = defaultProgram;
        m->albedo = assets.loadTexture("assets/textures/default.png");
        return m;
            })();

    // Тонированные клоны базового материала
    auto tintedMaterial = [&](grit::vec4 color) {
        auto m = std::make_shared<grit::render::Material>(*baseMaterial);
        m->baseColor = color;
        return m;
        };

    auto matRoot = tintedMaterial({ 1.00f, 0.85f, 0.30f, 1.0f }); // золотой
    auto matSat = tintedMaterial({ 0.30f, 0.65f, 1.00f, 1.0f }); // голубой
    auto matMoon = tintedMaterial({ 0.85f, 0.85f, 0.85f, 1.0f }); // серый
    auto matRed = tintedMaterial({ 1.00f, 0.25f, 0.25f, 1.0f }); // красный (для теста multi-material)

    auto makeRenderable = [&](grit::vec3 pos, grit::vec3 scale,
        std::shared_ptr<grit::render::Material> material) {
            auto e = world.createEntity();
            grit::ecs::Transform t;
            t.position = pos;
            t.scale = scale;
            world.registry().emplace<grit::ecs::Transform>(e, t);

            grit::ecs::MeshRenderer mr;
            mr.mesh = cubeMesh;
            mr.setMaterial(material);
            world.registry().emplace<grit::ecs::MeshRenderer>(e, mr);
            return e;
        };

    // --- Центральный куб ---
    auto root = makeRenderable({ 0.f, 0.f, 0.f }, { 1.5f, 1.5f, 1.5f }, matRoot);
    world.registry().emplace<grit::ecs::Name>(root, grit::ecs::Name{ "Root" });

    // --- DEBUG: разбиваем куб на 2 сабмеша вручную, чтобы проверить multi-material ---
    // Если у тебя в .grmesh для куба уже несколько сабмешей — этот блок не нужен,
    // материалы из .grmesh подключатся автоматически через loadModel(...).
   // --- DEBUG: разбиваем куб на 2 сабмеша вручную ---
    {
        const std::uint32_t total = cubeMesh->indexCount();
        if (total >= 6 && cubeMesh->submeshes().size() <= 1) {
            const std::uint32_t half = (total / 6) * 3;
            std::vector<grit::render::Submesh> subs;
            subs.push_back({ 0,    half,         0 });
            subs.push_back({ half, total - half, 1 });
            cubeMesh->setSubmeshes(std::move(subs));
            GRIT_INFO("DEBUG: split cube into 2 submeshes (%u + %u indices)",
                half, total - half);
        }
    }

    // Для root задаём ДВА материала — увидим обе половины куба разноцветными
    {
        auto vw = world.registry().view<grit::ecs::MeshRenderer>();
        for (auto e : vw) {
            vw.get<grit::ecs::MeshRenderer>(e).meshPath = "assets/meshes/cube.grmesh";
        }
        grit::scene::SceneSerializer::saveScene(world, "scene.json");
    }

    // --- Свет: 3 directional ---
    auto addDirLight = [&](const char* name, grit::vec3 dir, grit::vec3 color,
        float intensity, float ambient = 0.0f) {
            auto e = world.createEntity();
            auto& L = world.registry().emplace<grit::ecs::LightDirectional>(e);
            L.direction = dir;
            L.color = color;
            L.intensity = intensity;
            L.ambient = ambient;
            world.registry().emplace<grit::ecs::Name>(e, grit::ecs::Name{ name });
            return e;
        };

    addDirLight("KeyLight", { -0.4f, -0.8f,  0.3f }, { 1.0f, 0.95f, 0.85f }, 1.2f, 0.15f);
    addDirLight("FillLight", { 0.6f, -0.3f, -0.4f }, { 0.40f, 0.55f, 0.80f }, 0.6f);
    addDirLight("RimLight", { 0.0f,  0.5f, -0.7f }, { 0.90f, 0.40f, 0.60f }, 0.4f);

    // --- 4 спутника + луны ---
    std::vector<entt::entity> satellites;
    const float satOrbit = 4.f;
    for (int i = 0; i < 4; ++i) {
        float ang = (float)i * (3.14159265f * 0.5f);
        grit::vec3 p{ std::cos(ang) * satOrbit, 0.f, std::sin(ang) * satOrbit };
        auto sat = makeRenderable(p, { 0.6f, 0.6f, 0.6f }, matSat);
        scene.setParent(world, sat, root);
        satellites.push_back(sat);

        auto moon = makeRenderable({ 1.2f, 0.5f, 0.f }, { 0.3f, 0.3f, 0.3f }, matMoon);
        scene.setParent(world, moon, sat);
    }

    {
        // Заполним meshPath для всех MeshRenderer'ов
        auto vw = world.registry().view<grit::ecs::MeshRenderer>();
        for (auto e : vw) {
            vw.get<grit::ecs::MeshRenderer>(e).meshPath = "assets/meshes/cube.grmesh";
        }
        grit::scene::SceneSerializer::saveScene(world, "scene.json");
    }

    // --- Камера ---
   // --- Камера ---
    auto& cam = engine.renderer().camera();
    grit::render::CameraController camCtrl;
    camCtrl.setCamera(&cam);
    camCtrl.setInput(&input);
    camCtrl.setPositionAndTarget({ 0.f, 6.f, -14.f }, { 0.f, 0.f, 0.f });
    camCtrl.config().baseSpeed = 6.f;
    camCtrl.config().rightMouseToLook = false;   // ← ДОБАВИТЬ: режим внешнего управления (редактор)

    bool rotating = true;

    engine.setOnUpdate([&](float dt) {
        if (input.wasPressed(grit::Key::Escape)) { engine.requestQuit(); return; }
        if (input.wasPressed(grit::Key::Space))  rotating = !rotating;

        const bool camActive = engine.editorUI().cameraCaptured();
        camCtrl.setExternalLook(camActive);   // ← говорим контроллеру: «крути»
        camCtrl.update(dt);                   // ← всегда обновляем, без if

        if (rotating) {
            auto& rt = world.registry().get<grit::ecs::Transform>(root);
            rt.rotationEuler.y += dt * 0.6f;

            for (auto s : satellites) {
                auto& st = world.registry().get<grit::ecs::Transform>(s);
                st.rotationEuler.y += dt * 2.0f;
            }
        }
        });

    GRIT_INFO("Submeshes demo: center cube has 2 submeshes / 2 materials");
    GRIT_INFO("RMB - look, WASD/QE - move, Space - pause, Esc - quit");

    while (engine.tick()) {}
    engine.shutdown();
    return 0;
}