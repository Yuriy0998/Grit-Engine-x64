#include "EditorUI.h"
#include "EditorUI_Internal.h"
#include <imgui.h>
#include <cstdio>

#include "../Engine.h"
#include "../ecs/World.h"
#include "../core/Log.h"
#include "../scene/PrefabSerializer.h"

namespace grit::editor {

    void EditorUI::drawHierarchy()
    {
        if (!ImGui::Begin("Hierarchy", &m_showHierarchy)) { ImGui::End(); return; }

        auto& reg = m_engine->world().registry();

        if (ImGui::BeginPopupContextWindow("HierarchyEmptyPopup",
            ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
        {
            drawCreateEntityMenu();   // ← дедуплицировано
            ImGui::EndPopup();
        }

        entt::entity toDelete = entt::null;

        auto view = reg.view<grit::ecs::Name>();
        for (auto e : view) {
            auto& name = view.get<grit::ecs::Name>(e);
            ImGui::PushID((int)entt::to_integral(e));

            const bool selected = (e == m_selected);

            if (m_renaming == e) {
                // Первый кадр rename — инициализация буфера + запрос фокуса
                const bool firstFrame = (m_renameJustStarted == e);
                if (firstFrame) {
                    std::snprintf(m_renameBuf, sizeof(m_renameBuf), "%s", name.value.c_str());
                    ImGui::SetKeyboardFocusHere();
                }

                ImGuiInputTextFlags fl =
                    ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll;

                const bool enter = ImGui::InputText("##rename", m_renameBuf, sizeof(m_renameBuf), fl);

                const bool active = ImGui::IsItemActive();
                const bool escape = ImGui::IsKeyPressed(ImGuiKey_Escape);

                bool commit = false;
                bool cancel = false;

                if (enter) {
                    commit = true;
                }
                else if (escape) {
                    cancel = true;
                }
                // Потеря фокуса — коммитим. НО не на первом кадре
                // (на первом кадре фокус ещё не установлен → active == false → мгновенный commit).
                else if (!firstFrame && !active) {
                    commit = true;
                }

                if (commit) {
                    if (m_renameBuf[0]) name.value = m_renameBuf;
                    m_renaming = entt::null;
                    m_renameJustStarted = entt::null;
                }
                else if (cancel) {
                    m_renaming = entt::null;
                    m_renameJustStarted = entt::null;
                }

                // Сбрасываем флаг первого кадра ПОСЛЕ всех проверок
                if (firstFrame)
                    m_renameJustStarted = entt::null;
            }
            else {
                if (ImGui::Selectable(name.value.c_str(), selected,
                    ImGuiSelectableFlags_AllowDoubleClick)) {
                    m_selected = e;
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        m_renaming = e;
                        m_renameJustStarted = e;
                    }
                }
            }

            if (ImGui::BeginPopupContextItem("EntityPopup")) {
                m_selected = e;

                if (ImGui::MenuItem("Rename", "F2")) {
                    m_renaming = e;
                    m_renameJustStarted = e;
                }
                if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
                    duplicateEntity(e);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Save as Prefab...")) {
                    m_prefabSaveTarget = e;
                    std::string base = "Prefab";
                    if (auto* n = reg.try_get<grit::ecs::Name>(e); n && !n->value.empty())
                        base = n->value;
                    std::snprintf(m_prefabSaveName, sizeof(m_prefabSaveName), "%s", base.c_str());
                    m_prefabSaveRequested = true;
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Delete", "Del")) {
                    toDelete = e;
                }
                ImGui::EndPopup();
            }
            ImGui::PopID();
        }

        // --- Save Prefab modal ---
        if (ImGui::BeginPopupModal("SavePrefabPopup", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Save as prefab to assets/prefabs/");
            ImGui::SetNextItemWidth(300);
            ImGui::InputText("Name", m_prefabSaveName, sizeof(m_prefabSaveName));
            ImGui::TextDisabled("File: assets/prefabs/%s.grprefab", m_prefabSaveName);
            ImGui::Separator();

            const bool nameValid = m_prefabSaveName[0] != 0;
            if (!nameValid) ImGui::BeginDisabled();
            if (ImGui::Button("Save", ImVec2(120, 0))) {
                std::string path = std::string("assets/prefabs/") + m_prefabSaveName + ".grprefab";
                if (grit::scene::PrefabSerializer::savePrefab(
                    m_engine->world(), m_prefabSaveTarget, path))
                    GRIT_INFO("Prefab saved: %s", path.c_str());
                m_prefabSaveTarget = entt::null;
                ImGui::CloseCurrentPopup();
            }
            if (!nameValid) ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                m_prefabSaveTarget = entt::null;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (m_prefabSaveRequested) {
            ImGui::OpenPopup("SavePrefabPopup");
            m_prefabSaveRequested = false;
        }

        if (toDelete != entt::null) {
            if (m_selected == toDelete) m_selected = entt::null;
            if (m_renaming == toDelete) m_renaming = entt::null;
            reg.destroy(toDelete);
        }

        ImGui::End();
    }

} // namespace grit::editor