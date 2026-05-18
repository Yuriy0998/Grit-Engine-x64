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


int main() {
    grit::Engine engine;

    grit::EngineDesc desc;
    desc.window.title = "GritNG Sample - Stage 3.4 (Time & Stats)";
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

    // --- Меш + метаданные из .grmesh ---
    const auto* grmesh = assets.loadGrMesh("assets/meshes/cube.grmesh");
    if (!grmesh || !grmesh->mesh) { engine.shutdown(); return -1; }
    auto cubeMesh = grmesh->mesh;

    const auto defaultProgram = engine.renderer().defaultProgram();

    // --- Базовый материал из .grmesh (если в нём есть описание материалов) ---
    // Если в .grmesh нет материалов — делаем дефолтный с белым цветом и default.png
    std::shared_ptr<grit::render::Material> baseMaterial;
    if (!grmesh->materials.empty()) {
        baseMaterial = assets.createMaterial(grmesh->materials[0], defaultProgram);
    }
    else {
        baseMaterial = std::make_shared<grit::render::Material>();
        baseMaterial->program = defaultProgram;
        baseMaterial->albedo = assets.loadTexture("assets/textures/default.png");
    }

    // --- Хелпер: клонируем базовый материал, меняем только цвет ---
    // Так все варианты разделяют одну и ту же текстуру/программу,
    // отличаются только параметрами.
    auto tintedMaterial = [&](grit::vec4 color) {
        auto m = std::make_shared<grit::render::Material>(*baseMaterial);
        m->baseColor = color;
        return m;
        };

    auto matRoot = tintedMaterial({ 1.00f, 0.85f, 0.30f, 1.0f }); // золотой
    auto matSat = tintedMaterial({ 0.30f, 0.65f, 1.00f, 1.0f }); // голубой
    auto matMoon = tintedMaterial({ 0.85f, 0.85f, 0.85f, 1.0f }); // серый

    auto makeRenderable = [&](grit::vec3 pos, grit::vec3 scale,
        std::shared_ptr<grit::render::Material> material) {
            auto e = world.createEntity();
            grit::ecs::Transform t;
            t.position = pos;
            t.scale = scale;
            world.registry().emplace<grit::ecs::Transform>(e, t);

            grit::ecs::MeshRenderer mr;
            mr.mesh = cubeMesh;
            mr.material = material;
            world.registry().emplace<grit::ecs::MeshRenderer>(e, mr);
            return e;
        };

    // --- Центральный куб ---
    auto root = makeRenderable({ 0.f, 0.f, 0.f }, { 1.5f, 1.5f, 1.5f }, matRoot);
    world.registry().emplace<grit::ecs::Name>(root, grit::ecs::Name{ "Root" });

    // --- Directional light (солнце) ---
    {
        auto sun = world.createEntity();
        auto& L = world.registry().emplace<grit::ecs::LightDirectional>(sun);
        L.direction = { -0.4f, -0.8f, 0.3f };
        L.color = { 1.0f, 0.95f, 0.85f };  // тёплый солнечный
        L.intensity = 1.2f;
        L.ambient = 0.20f;

        world.registry().emplace<grit::ecs::Name>(sun, grit::ecs::Name{ "Sun" });
    }

    // --- 4 «спутника» как дети ---
    std::vector<entt::entity> satellites;
    const float satOrbit = 4.f;
    for (int i = 0; i < 4; ++i) {
        float ang = (float)i * (3.14159265f * 0.5f);
        grit::vec3 p{ std::cos(ang) * satOrbit, 0.f, std::sin(ang) * satOrbit };
        auto sat = makeRenderable(p, { 0.6f, 0.6f, 0.6f }, matSat);
        scene.setParent(world, sat, root);
        satellites.push_back(sat);

        // --- «Луна» как внук ---
        auto moon = makeRenderable({ 1.2f, 0.5f, 0.f }, { 0.3f, 0.3f, 0.3f }, matMoon);
        scene.setParent(world, moon, sat);
    }

    // --- Камера ---
    auto& cam = engine.renderer().camera();
    grit::render::CameraController camCtrl;
    camCtrl.setCamera(&cam);
    camCtrl.setInput(&input);
    camCtrl.setPositionAndTarget({ 0.f, 6.f, -14.f }, { 0.f, 0.f, 0.f });
    camCtrl.config().baseSpeed = 6.f;

    bool rotating = true;

    engine.setOnUpdate([&](float dt) {
        if (input.wasPressed(grit::Key::Escape)) {
            engine.requestQuit();
            return;
        }
        if (input.wasPressed(grit::Key::Space)) {
            rotating = !rotating;
        }

        camCtrl.update(dt);

        if (rotating) {
            // Вращаем root — спутники и луны крутятся вместе
            auto& rt = world.registry().get<grit::ecs::Transform>(root);
            rt.rotationEuler.y += dt * 0.6f;

            // Спутники вращаются вокруг своей оси — луны крутятся с ними
            for (auto s : satellites) {
                auto& st = world.registry().get<grit::ecs::Transform>(s);
                st.rotationEuler.y += dt * 2.0f;
            }
        }
        });

    GRIT_INFO("Hierarchy demo: root rotates -> satellites orbit -> moons follow");
    GRIT_INFO("RMB - look, WASD/QE - move, Space - pause rotation, Esc - quit");
    GRIT_INFO("F1 - bgfx stats, F2 - wireframe, RMB+WASD - camera, Space - pause, Esc - quit");

    while (engine.tick()) {}
    engine.shutdown();
    return 0;
}