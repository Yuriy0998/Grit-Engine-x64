#include <Engine.h>
#include <core/Log.h>
#include <core/Math.h>
#include <core/Input.h>
#include <ecs/World.h>
#include <render/Material.h>
#include <render/StaticMesh.h>
#include <render/Texture.h>

int main() {
    grit::Engine engine;

    grit::EngineDesc desc;
    desc.window.title = "GritNG Sample - Stage 3.1 (Input)";
    desc.window.width = 1280;
    desc.window.height = 720;

    if (!engine.init(desc)) {
        GRIT_ERROR("Engine init failed");
        return -1;
    }

    auto& assets = engine.assets();
    auto& world = engine.world();
    auto& input = engine.input();

    auto cubeMesh = assets.loadStaticMesh("assets/meshes/cube.grmesh");
    if (!cubeMesh) {
        GRIT_ERROR("Failed to load cube.grmesh");
        engine.shutdown();
        return -1;
    }

    auto albedoTex = assets.loadTexture("assets/textures/default.png");
    auto mat = std::make_shared<grit::render::Material>();
    mat->albedo = albedoTex;
    mat->program = engine.renderer().defaultProgram();

    for (int x = -1; x <= 1; ++x) {
        for (int z = -1; z <= 1; ++z) {
            auto e = world.createEntity();
            grit::ecs::Transform t;
            t.position = grit::vec3{ (float)x * 2.f, 0.f, (float)z * 2.f };
            world.registry().emplace<grit::ecs::Transform>(e, t);

            grit::ecs::MeshRenderer mr;
            mr.mesh = cubeMesh;
            mr.material = mat;
            world.registry().emplace<grit::ecs::MeshRenderer>(e, mr);
        }
    }

    auto& cam = engine.renderer().camera();
    cam.position = grit::vec3{ 0.f, 3.f, -6.f };
    cam.target = grit::vec3{ 0.f, 0.f,  0.f };

    bool rotating = true;

    engine.setOnUpdate([&](float dt) {
        // Выход
        if (input.wasPressed(grit::Key::Escape)) {
            GRIT_INFO("ESC pressed - quitting");
            engine.requestQuit();
            return;
        }

        // Пауза вращения
        if (input.wasPressed(grit::Key::Space)) {
            rotating = !rotating;
            GRIT_INFO("Rotation: %s", rotating ? "ON" : "OFF");
        }

        // Захват мыши
        if (input.wasMousePressed(grit::MouseButton::Right)) {
            input.setMouseCaptured(true);
            GRIT_INFO("Mouse captured");
        }
        if (input.wasMouseReleased(grit::MouseButton::Right)) {
            input.setMouseCaptured(false);
            GRIT_INFO("Mouse released");
        }

        if (input.wheelDelta() != 0.f) {
            GRIT_INFO("Wheel: %+.1f", input.wheelDelta());
        }

        // Движение камеры стрелками
        float camSpeed = 5.f * dt;
        if (input.isDown(grit::Key::LeftShift)) camSpeed *= 3.f;
        if (input.isDown(grit::Key::Up))    cam.position.z += camSpeed;
        if (input.isDown(grit::Key::Down))  cam.position.z -= camSpeed;
        if (input.isDown(grit::Key::Left))  cam.position.x -= camSpeed;
        if (input.isDown(grit::Key::Right)) cam.position.x += camSpeed;

        if (rotating) {
            auto view = world.registry().view<grit::ecs::Transform>();
            for (auto ent : view) {
                auto& t = view.get<grit::ecs::Transform>(ent);
                t.rotationEuler.y += dt * 1.2f;
            }
        }
        });

    // Главный цикл — просто крутим, пока tick() не вернёт false
    while (engine.tick()) {}

    engine.shutdown();
    return 0;
}