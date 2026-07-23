#include "EditorUI.h"
#include "EditorUI_Internal.h"
#include <cstring>
#include <imgui.h>

#include "../Engine.h"
#include "../ecs/World.h"
#include "../ecs/Light.h"
#include "../render/Renderer.h"
#include "../render/Primitives.h"

namespace grit::editor {

    using namespace grit::editor::detail;

    entt::entity EditorUI::createEmpty(const char* name)
    {
        auto& reg = m_engine->world().registry();
        auto e = reg.create();
        reg.emplace<grit::ecs::Name>(e, grit::ecs::Name{ name });
        reg.emplace<grit::ecs::Transform>(e);
        m_selected = e;
        return e;
    }

    entt::entity EditorUI::createPrimitive(const char* kind)
    {
        auto& reg = m_engine->world().registry();
        auto e = reg.create();
        reg.emplace<grit::ecs::Name>(e, grit::ecs::Name{ kind });
        reg.emplace<grit::ecs::Transform>(e);

        grit::ecs::MeshRenderer mr;
        if (std::strcmp(kind, "Cube") == 0) mr.mesh = grit::render::Primitives::createCube();
        else if (std::strcmp(kind, "Sphere") == 0) mr.mesh = grit::render::Primitives::createSphere();
        else if (std::strcmp(kind, "Plane") == 0) mr.mesh = grit::render::Primitives::createPlane();
        mr.visible = true;

        fillDefaultMaterials(mr, m_engine->renderer().defaultProgram());  // ← хелпер

        reg.emplace<grit::ecs::MeshRenderer>(e, std::move(mr));
        m_selected = e;
        return e;
    }

    entt::entity EditorUI::createLight()
    {
        auto& reg = m_engine->world().registry();
        auto e = reg.create();
        reg.emplace<grit::ecs::Name>(e, grit::ecs::Name{ "Directional Light" });
        reg.emplace<grit::ecs::Transform>(e);

        grit::ecs::LightDirectional L;
        L.direction = { -0.3f, -1.0f, -0.2f };
        L.color = { 1.0f,  1.0f,  1.0f };
        L.intensity = 1.0f;
        L.ambient = 0.15f;
        reg.emplace<grit::ecs::LightDirectional>(e, L);
        m_selected = e;
        return e;
    }

    void EditorUI::createPointLight()
    {
        auto& reg = m_engine->world().registry();
        auto e = reg.create();
        reg.emplace<grit::ecs::Name>(e, "Point Light");
        auto& tr = reg.emplace<grit::ecs::Transform>(e);
        tr.position = { 0.0f, 2.0f, 0.0f };
        reg.emplace<grit::ecs::LightPoint>(e);
        m_selected = e;
    }

    void EditorUI::createSpotLight()
    {
        auto& reg = m_engine->world().registry();
        auto e = reg.create();
        reg.emplace<grit::ecs::Name>(e, "Spot Light");
        auto& tr = reg.emplace<grit::ecs::Transform>(e);
        tr.position = { 0.0f, 3.0f, 0.0f };
        reg.emplace<grit::ecs::LightSpot>(e);
        m_selected = e;
    }

    void EditorUI::duplicateEntity(entt::entity src)
    {
        auto& reg = m_engine->world().registry();
        auto dst = reg.create();

        if (auto* n = reg.try_get<grit::ecs::Name>(src))
            reg.emplace<grit::ecs::Name>(dst, grit::ecs::Name{ n->value + " (Copy)" });
        if (auto* t = reg.try_get<grit::ecs::Transform>(src)) {
            auto copy = *t;
            copy.position.x += 0.5f;
            reg.emplace<grit::ecs::Transform>(dst, copy);
        }
        if (auto* mr = reg.try_get<grit::ecs::MeshRenderer>(src))
            reg.emplace<grit::ecs::MeshRenderer>(dst, *mr);
        if (auto* L = reg.try_get<grit::ecs::LightDirectional>(src))
            reg.emplace<grit::ecs::LightDirectional>(dst, *L);

        m_selected = dst;
    }

    void EditorUI::drawCreateEntityMenu()
    {
        if (ImGui::MenuItem("Create Empty")) createEmpty();
        ImGui::Separator();
        if (ImGui::BeginMenu("3D Object")) {
            if (ImGui::MenuItem("Cube"))   createPrimitive("Cube");
            if (ImGui::MenuItem("Sphere")) createPrimitive("Sphere");
            if (ImGui::MenuItem("Plane"))  createPrimitive("Plane");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Light")) {
            if (ImGui::MenuItem("Directional Light")) createLight();
            if (ImGui::MenuItem("Point Light"))       createPointLight();
            if (ImGui::MenuItem("Spot Light"))        createSpotLight();
            ImGui::EndMenu();
        }
    }

} // namespace grit::editor