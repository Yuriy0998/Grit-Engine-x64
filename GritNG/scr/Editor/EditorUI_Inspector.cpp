#include "EditorUI.h"
#include "EditorUI_Internal.h"
#include <imgui.h>
#include <cstdio>
#include <cstring>

#include "../Engine.h"
#include "../ecs/World.h"
#include "../ecs/Light.h"
#include "../render/Material.h"
#include "../render/StaticMesh.h"
#include "../render/Texture.h"

namespace grit::editor {

    using namespace grit::editor::detail;

    void EditorUI::drawInspector()
    {
        if (!ImGui::Begin("Inspector", &m_showInspector)) {
            ImGui::End();
            return;
        }

        if (m_selected == entt::null) {
            ImGui::TextDisabled("No selection");
            ImGui::End();
            return;
        }

        auto& reg = m_engine->world().registry();
        const entt::entity e = m_selected;

        if (!reg.valid(e)) {
            ImGui::TextDisabled("Invalid entity");
            ImGui::End();
            return;
        }

        // ===== Name =====
        if (auto* n = reg.try_get<grit::ecs::Name>(e)) {
            char buf[128];
            std::strncpy(buf, n->value.c_str(), sizeof(buf));
            buf[sizeof(buf) - 1] = 0;
            if (ImGui::InputText("Name", buf, sizeof(buf)))
                n->value = buf;
        }

        // ===== Transform =====
        if (auto* tr = reg.try_get<grit::ecs::Transform>(e)) {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat3("Position", &tr->position.x, 0.05f);
                ImGui::DragFloat3("Rotation", &tr->rotationEuler.x, 0.5f);
                ImGui::DragFloat3("Scale", &tr->scale.x, 0.05f, 0.01f, 100.0f);
            }
        }

        // ===== MeshRenderer / Materials =====
        if (auto* mr = reg.try_get<grit::ecs::MeshRenderer>(e)) {
            if (ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("Visible", &mr->visible);

                if (mr->mesh)
                    ImGui::Text("Submeshes: %zu", mr->mesh->submeshes().size());

                for (size_t i = 0; i < mr->materials.size(); ++i) {
                    auto& mat = mr->materials[i];
                    if (!mat) continue;

                    ImGui::PushID((int)i);
                    char header[64];
                    std::snprintf(header, sizeof(header), "Material %zu", i);
                    if (ImGui::CollapsingHeader(header, ImGuiTreeNodeFlags_DefaultOpen)) {
                        ImGui::ColorEdit4("Base Color", &mat->baseColor.x);
                        ImGui::SliderFloat("Metallic", &mat->pbrParams.x, 0.0f, 1.0f);
                        ImGui::SliderFloat("Roughness", &mat->pbrParams.y, 0.04f, 1.0f);
                        ImGui::DragFloat4("Tiling/Offset", &mat->tilingOffset.x, 0.01f);
                        ImGui::ColorEdit3("Emissive", &mat->emissive.x);
                        ImGui::DragFloat("Emissive Intensity", &mat->emissive.w, 0.05f, 0.0f, 50.0f);

                        if (mat->albedo)
                            ImGui::Text("Albedo: %s", mat->albedo->path().c_str());
                        else
                            ImGui::TextDisabled("Albedo: <none>");
                    }
                    ImGui::PopID();
                }
            }
        }

        // ===== Directional Light =====
        if (auto* L = reg.try_get<grit::ecs::LightDirectional>(e)) {
            if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat3("Direction", &L->direction.x, 0.01f);
                ImGui::ColorEdit3("Color", &L->color.x);
                ImGui::DragFloat("Intensity", &L->intensity, 0.05f, 0.0f, 20.0f);
                ImGui::DragFloat("Ambient", &L->ambient, 0.01f, 0.0f, 1.0f);
                if (ImGui::Button("Remove##DirLight"))
                    reg.remove<grit::ecs::LightDirectional>(e);
            }
        }

        // ===== Point Light =====
        if (auto* L = reg.try_get<grit::ecs::LightPoint>(e)) {
            if (ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::ColorEdit3("Color", &L->color.x);
                ImGui::DragFloat("Intensity", &L->intensity, 0.05f, 0.0f, 50.0f);
                ImGui::DragFloat("Range", &L->range, 0.1f, 0.01f, 1000.0f);
                if (ImGui::Button("Remove##PointLight"))
                    reg.remove<grit::ecs::LightPoint>(e);
            }
        }

        // ===== Spot Light =====
        if (auto* L = reg.try_get<grit::ecs::LightSpot>(e)) {
            if (ImGui::CollapsingHeader("Spot Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat3("Direction", &L->direction.x, 0.01f);
                ImGui::ColorEdit3("Color", &L->color.x);
                ImGui::DragFloat("Intensity", &L->intensity, 0.05f, 0.0f, 50.0f);
                ImGui::DragFloat("Range", &L->range, 0.1f, 0.01f, 1000.0f);

                float innerDeg = L->innerCone * kRad2Deg;
                float outerDeg = L->outerCone * kRad2Deg;
                if (ImGui::DragFloat("Inner Cone (deg)", &innerDeg, 0.5f, 1.0f, 89.0f)) {
                    L->innerCone = innerDeg * kDeg2Rad;
                    if (L->outerCone < L->innerCone) L->outerCone = L->innerCone;
                }
                if (ImGui::DragFloat("Outer Cone (deg)", &outerDeg, 0.5f, 1.0f, 89.0f)) {
                    L->outerCone = outerDeg * kDeg2Rad;
                    if (L->innerCone > L->outerCone) L->innerCone = L->outerCone;
                }
                if (ImGui::Button("Remove##SpotLight"))
                    reg.remove<grit::ecs::LightSpot>(e);
            }
        }

        // ===== Add Component =====
        ImGui::Separator();
        if (ImGui::Button("Add Component"))
            ImGui::OpenPopup("AddComponentPopup");

        if (ImGui::BeginPopup("AddComponentPopup")) {
            if (ImGui::BeginMenu("Light")) {
                if (ImGui::MenuItem("Directional Light", nullptr, false,
                    !reg.any_of<grit::ecs::LightDirectional>(e)))
                    reg.emplace<grit::ecs::LightDirectional>(e);
                if (ImGui::MenuItem("Point Light", nullptr, false,
                    !reg.any_of<grit::ecs::LightPoint>(e)))
                    reg.emplace<grit::ecs::LightPoint>(e);
                if (ImGui::MenuItem("Spot Light", nullptr, false,
                    !reg.any_of<grit::ecs::LightSpot>(e)))
                    reg.emplace<grit::ecs::LightSpot>(e);
                ImGui::EndMenu();
            }
            ImGui::EndPopup();
        }

        ImGui::End();
    }

} // namespace grit::editor