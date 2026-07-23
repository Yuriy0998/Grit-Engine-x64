#include "EditorUI.h"
#include "EditorUI_Internal.h"
#include <imgui.h>
#include <ImGuizmo.h>
#include <cinttypes>

#include "../Engine.h"
#include "../ecs/World.h"
#include "../render/Renderer.h"
#include "../scene/SceneSerializer.h"   // ← ДОБАВИТЬ

namespace grit::editor {

    using namespace grit::editor::detail;

    void EditorUI::draw()
    {
        if (!m_enabled || !m_engine) return;

        ImGuizmo::BeginFrame();

        auto& reg = m_engine->world().registry();
        ImGuiIO& io = ImGui::GetIO();

        // --- Хоткеи ---
        if (!io.WantCaptureKeyboard) {
            if (ImGui::IsKeyPressed(ImGuiKey_W)) m_gizmoOp = GizmoOp::Translate;
            if (ImGui::IsKeyPressed(ImGuiKey_E)) m_gizmoOp = GizmoOp::Rotate;
            if (ImGui::IsKeyPressed(ImGuiKey_R)) m_gizmoOp = GizmoOp::Scale;

            if (m_selected != entt::null) {
                if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
                    reg.destroy(m_selected);
                    m_selected = entt::null;
                }
                else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D)) {
                    duplicateEntity(m_selected);
                }
                else if (ImGui::IsKeyPressed(ImGuiKey_F2)) {
                    m_renaming = m_selected;
                }
            }
        }

        // --- Хост-окно ---
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowViewport(vp->ID);

        ImGuiWindowFlags hostFlags =
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus
            | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBackground
            | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize
            | ImGuiWindowFlags_NoSavedSettings;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x, 0.0f));
        ImGui::Begin("##DockHost", nullptr, hostFlags);
        ImGui::PopStyleVar(3);

        if (ImGui::BeginMenuBar()) { drawMenuBar(); ImGui::EndMenuBar(); }
        drawTabsBar();
        drawToolbar();
        m_headerHeight = ImGui::GetWindowSize().y;
        ImGui::End();

        if (m_showEnvironment) drawEnvironment();

        drawCenterArea();
        drawLeftSideBar();

        if (m_showHierarchy) drawHierarchy();
        if (m_showInspector) drawInspector();
        if (m_showConsole)   drawConsole();
        if (m_showAssets)    drawAssets();
        if (m_showDemo)      ImGui::ShowDemoWindow(&m_showDemo);

        // --- Status bar ---
        {
            const float h = ImGui::GetFrameHeight();
            ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x, vp->WorkPos.y + vp->WorkSize.y - h));
            ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x, h));

            ImGuiWindowFlags fl = ImGuiWindowFlags_NoDecoration
                | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
                | ImGuiWindowFlags_NoBringToFrontOnFocus;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 2));
            ImGui::Begin("##StatusBar", nullptr, fl);

            const auto& t = m_engine->time();
            ImGui::Text("FPS %.0f  |  %.2f ms  |  Frame %" PRIu64,
                t.fps(), t.msPerFrame(), (uint64_t)t.frameCount());

            ImGui::SameLine(ImGui::GetWindowWidth() - 220);
            if (m_selected != entt::null) {
                const char* name = "<entity>";
                if (auto* n = reg.try_get<ecs::Name>(m_selected); n && !n->value.empty())
                    name = n->value.c_str();
                ImGui::Text("Selected: %s", name);
            }
            else {
                ImGui::TextDisabled("Selected: -");
            }
            ImGui::End();
            ImGui::PopStyleVar();
        }

        if (m_showTimeOfDay) drawTimeOfDay();
        if (m_cameraCaptured) ImGui::SetMouseCursor(ImGuiMouseCursor_None);

        m_engine->renderer().setSelectedEntity(m_selected);
    }

    // ===== MenuBar =====
    void EditorUI::drawMenuBar()
    {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save Scene"))
                grit::scene::SceneSerializer::saveScene(m_engine->world(), "scene.json");
            if (ImGui::MenuItem("Reload Scene")) {
                auto& reg = m_engine->world().registry();
                reg.clear();
                m_selected = entt::null;
                grit::scene::SceneSerializer::loadScene(
                    m_engine->world(), "scene.json",
                    m_engine->assets(), m_engine->renderer().defaultProgram());
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit")) m_engine->requestQuit();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            const bool hasSel = (m_selected != entt::null);
            if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, hasSel))
                duplicateEntity(m_selected);
            if (ImGui::MenuItem("Delete", "Del", false, hasSel)) {
                m_engine->world().registry().destroy(m_selected);
                m_selected = entt::null;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("GameObject")) {
            drawCreateEntityMenu();      // ← дедуплицировано
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Hierarchy", nullptr, &m_showHierarchy);
            ImGui::MenuItem("Inspector", nullptr, &m_showInspector);
            ImGui::MenuItem("Console", nullptr, &m_showConsole);
            ImGui::MenuItem("Assets", nullptr, &m_showAssets);
            ImGui::Separator();
            ImGui::MenuItem("ImGui Demo", nullptr, &m_showDemo);
            ImGui::MenuItem("Time of Day", nullptr, &m_showTimeOfDay);
            ImGui::EndMenu();
        }

        const auto& t = m_engine->time();
        char buf[128];
        std::snprintf(buf, sizeof(buf), "FPS %.0f | %.2f ms", t.fps(), t.msPerFrame());
        const float w = ImGui::CalcTextSize(buf).x + 16.f;
        ImGui::SameLine(ImGui::GetWindowWidth() - w);
        ImGui::TextUnformatted(buf);
    }

} // namespace grit::editor