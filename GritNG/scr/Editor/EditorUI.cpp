
#include "EditorUI.h"
#include <imgui.h>
#include <ImGuizmo.h>

#include "../Engine.h"
#include "../ecs/World.h"
#include "../ecs/SceneSystem.h"
#include "../ecs/Light.h"
#include "../render/Material.h"
#include "../render/StaticMesh.h"
#include "../render/Texture.h"
#include "../render/Renderer.h"
#include "../scene/SceneSerializer.h"
#include "../core/Log.h"
#include "../core/LogSink.h"

#include <functional>
#include <cstdio>
#include <string>
#include <bx/math.h>





namespace grit::editor {

    void EditorUI::draw()
    {

        if (!m_enabled) return;


        ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureKeyboard) {
            if (ImGui::IsKeyPressed(ImGuiKey_Q)) { /* select-only, выкл гизмо позже */ }
            if (ImGui::IsKeyPressed(ImGuiKey_W)) m_gizmoOp = GizmoOp::Translate;
            if (ImGui::IsKeyPressed(ImGuiKey_E)) m_gizmoOp = GizmoOp::Rotate;
            if (ImGui::IsKeyPressed(ImGuiKey_R)) m_gizmoOp = GizmoOp::Scale;
        }

        if (!m_enabled || !m_engine) return;

        // --- Хост-окно для DockSpace на весь экран ---
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::SetNextWindowViewport(vp->ID);

        ImGuiWindowFlags hostFlags =
            ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoBringToFrontOnFocus
            | ImGuiWindowFlags_NoNavFocus
            | ImGuiWindowFlags_NoDocking
            | ImGuiWindowFlags_MenuBar
            | ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("##DockHost", nullptr, hostFlags);
        ImGui::PopStyleVar(3);

        // DockSpace
        const ImGuiID dockId = ImGui::GetID("GritDockSpace");
        ImGui::DockSpace(dockId, ImVec2(0, 0),
            ImGuiDockNodeFlags_PassthruCentralNode);

        // Menu bar внутри host-окна
        if (ImGui::BeginMenuBar()) {
            drawMenuBar();
            ImGui::EndMenuBar();
        }

        ImGui::End(); // ##DockHost

        // Сами панели — теперь они dockable
        drawHierarchy();
        drawInspector();
        drawConsole();    // НОВОЕ — пока заглушка
        drawViewport();
        drawToolbar();
        drawAssets();

        if (m_showDemo) ImGui::ShowDemoWindow(&m_showDemo);

        // Status bar — фиксированная полоса внизу
        {
            const ImGuiViewport* vp = ImGui::GetMainViewport();
            const float h = ImGui::GetFrameHeight();
            ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x, vp->WorkPos.y + vp->WorkSize.y - h));
            ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x, h));

            ImGuiWindowFlags fl =
                ImGuiWindowFlags_NoDecoration
                | ImGuiWindowFlags_NoMove
                | ImGuiWindowFlags_NoSavedSettings
                | ImGuiWindowFlags_NoDocking
                | ImGuiWindowFlags_NoBringToFrontOnFocus;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 2));
            ImGui::Begin("##StatusBar", nullptr, fl);

            const auto& t = m_engine->time();
            ImGui::Text("FPS %.0f  |  %.2f ms  |  Frame %llu",
                t.fps(), t.msPerFrame(),
                (unsigned long long)t.frameCount());

            ImGui::SameLine(ImGui::GetWindowWidth() - 220);

            if (m_selected != entt::null) {
                auto& reg = m_engine->world().registry();
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
        GRIT_INFO("Test info %d", 42);
        GRIT_WARN("Test warn %s", "hello");
        GRIT_ERROR("Test error %.2f", 3.14f);
    }

    void EditorUI::drawConsole()
    {
        if (!ImGui::Begin("Console")) { ImGui::End(); return; }

        // Тулбар
        if (ImGui::Button("Clear")) grit::LogSink::get().clear();
        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &m_consoleAutoScroll);
        ImGui::SameLine();
        ImGui::Checkbox("Info", &m_filterInfo); ImGui::SameLine();
        ImGui::Checkbox("Warn", &m_filterWarn); ImGui::SameLine();
        ImGui::Checkbox("Error", &m_filterErr); ImGui::SameLine();
        ImGui::SetNextItemWidth(180);
        ImGui::InputTextWithHint("##filter", "filter...",
            m_consoleFilter, sizeof(m_consoleFilter));

        ImGui::Separator();

        if (ImGui::BeginChild("##log", ImVec2(0, 0), false,
            ImGuiWindowFlags_HorizontalScrollbar))
        {
            const auto entries = grit::LogSink::get().snapshot();
            const std::string filter = m_consoleFilter;

            for (const auto& e : entries) {
                bool show = false;
                ImVec4 col(0.88f, 0.88f, 0.88f, 1.f);
                switch (e.level) {
                case grit::LogLevel::Info:  show = m_filterInfo; col = ImVec4(0.85f, 0.85f, 0.85f, 1); break;
                case grit::LogLevel::Warn:  show = m_filterWarn; col = ImVec4(1.00f, 0.78f, 0.20f, 1); break;
                case grit::LogLevel::Error: show = m_filterErr;  col = ImVec4(1.00f, 0.35f, 0.30f, 1); break;
                }
                if (!show) continue;
                if (!filter.empty() && e.text.find(filter) == std::string::npos) continue;

                ImGui::PushStyleColor(ImGuiCol_Text, col);
                ImGui::TextUnformatted(e.text.c_str());
                ImGui::PopStyleColor();
            }

            if (m_consoleAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();

        ImGui::End();
    }

    void EditorUI::drawGizmo()
    {
        if (m_selected == entt::null) return;

        auto& reg = m_engine->world().registry();
        auto* tr = reg.try_get<grit::ecs::Transform>(m_selected);
        if (!tr) return;

        // 1) Настройка экранного прямоугольника
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
        ImGuizmo::SetRect(m_vpScreenPos.x, m_vpScreenPos.y,
            m_vpScreenSize.x, m_vpScreenSize.y);

        // 2) Матрицы камеры
        const float* view = m_engine->renderer().viewMatrix();
        const float* proj = m_engine->renderer().projMatrix();

        // 3) Собираем матрицу трансформа (T*R*S) из компонентов
        float matrix[16];
        float t[3] = { tr->position.x,      tr->position.y,      tr->position.z };
        float r[3] = { tr->rotationEuler.x, tr->rotationEuler.y, tr->rotationEuler.z }; // градусы
        float s[3] = { tr->scale.x,         tr->scale.y,         tr->scale.z };
        ImGuizmo::RecomposeMatrixFromComponents(t, r, s, matrix);

        // 4) Операция и пространство
        ImGuizmo::OPERATION op;
        switch (m_gizmoOp) {
        case GizmoOp::Translate: op = ImGuizmo::TRANSLATE; break;
        case GizmoOp::Rotate:    op = ImGuizmo::ROTATE;    break;
        case GizmoOp::Scale:     op = ImGuizmo::SCALE;     break;
        default:                 op = ImGuizmo::TRANSLATE; break;
        }
        ImGuizmo::MODE mode = m_gizmoUseLocal ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

        // 5) Манипуляция
        if (ImGuizmo::Manipulate(view, proj, op, mode, matrix))
        {
            float nt[3], nr[3], ns[3];
            ImGuizmo::DecomposeMatrixToComponents(matrix, nt, nr, ns);

            if (op == ImGuizmo::TRANSLATE) {
                tr->position = { nt[0], nt[1], nt[2] };
            }
            else if (op == ImGuizmo::ROTATE) {
                tr->rotationEuler = { nr[0], nr[1], nr[2] };
            }
            else if (op == ImGuizmo::SCALE) {
                tr->scale = { ns[0], ns[1], ns[2] };
            }
        }
    }

    void EditorUI::drawAssets()
    {
        if (!ImGui::Begin("Assets")) { ImGui::End(); return; }

        namespace fs = std::filesystem;

        ImGui::Text("Path: %s", m_assetsCurrent.c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("Up")) {
            fs::path p(m_assetsCurrent);
            if (p.has_parent_path() && p != fs::path(m_assetsRoot)) {
                m_assetsCurrent = p.parent_path().string();
            }
        }
        ImGui::Separator();

        std::error_code ec;
        if (!fs::exists(m_assetsCurrent, ec)) {
            ImGui::TextDisabled("Folder not found: %s", m_assetsCurrent.c_str());
            ImGui::End();
            return;
        }

        for (const auto& entry : fs::directory_iterator(m_assetsCurrent, ec))
        {
            const auto& path = entry.path();
            const std::string name = path.filename().string();

            if (entry.is_directory()) {
                if (ImGui::Selectable(("[D] " + name).c_str(), false,
                    ImGuiSelectableFlags_AllowDoubleClick))
                {
                    if (ImGui::IsMouseDoubleClicked(0))
                        m_assetsCurrent = path.string();
                }
            }
            else {
                ImGui::Selectable(("    " + name).c_str(), false);
                // позже — drag&drop в Viewport/Hierarchy
            }
        }

        ImGui::End();
    }

    void EditorUI::drawToolbar()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 6));
        if (!ImGui::Begin("Toolbar", nullptr,
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse))
        {
            ImGui::End();
            ImGui::PopStyleVar();
            return;
        }

        auto toolButton = [&](const char* label, bool active) {
            if (active) {
                ImVec4 a = ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive);
                ImGui::PushStyleColor(ImGuiCol_Button, a);
            }
            bool clicked = ImGui::Button(label, ImVec2(36, 26));
            if (active) ImGui::PopStyleColor();
            return clicked;
            };

        if (toolButton("T", m_gizmoOp == GizmoOp::Translate)) m_gizmoOp = GizmoOp::Translate;
        ImGui::SameLine();
        if (toolButton("R", m_gizmoOp == GizmoOp::Rotate))    m_gizmoOp = GizmoOp::Rotate;
        ImGui::SameLine();
        if (toolButton("S", m_gizmoOp == GizmoOp::Scale))     m_gizmoOp = GizmoOp::Scale;




        ImGui::SameLine();
        ImGui::Dummy(ImVec2(10, 0)); ImGui::SameLine();
        const char* spaceLabel = m_gizmoUseLocal ? "Local" : "World";
        if (ImGui::Button(spaceLabel, ImVec2(64, 26)))
            m_gizmoUseLocal = !m_gizmoUseLocal;

        ImGui::SameLine();
        ImGui::Dummy(ImVec2(20, 0));
        ImGui::SameLine();

        if (ImGui::Button(m_playing ? "Stop" : "Play", ImVec2(56, 26))) {
            m_playing = !m_playing;
            // TODO: m_engine->setPaused(!m_playing);
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void EditorUI::drawViewport()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        if (!ImGui::Begin("Viewport"))
        {
            ImGui::End();
            ImGui::PopStyleVar();
            return;
        }

        const ImVec2 avail = ImGui::GetContentRegionAvail();
        const uint32_t w = (uint32_t)bx::max(1.0f, avail.x);
        const uint32_t h = (uint32_t)bx::max(1.0f, avail.y);

        if (w != m_lastVpW || h != m_lastVpH) {
            m_engine->renderer().resizeViewport(w, h);
            m_lastVpW = w;
            m_lastVpH = h;
        }

        // Запоминаем экранную геометрию для гизмо
        m_vpScreenPos = ImGui::GetCursorScreenPos();
        m_vpScreenSize = avail;
        m_vpHovered = false;

        auto tex = m_engine->renderer().viewportColorTexture();
        if (bgfx::isValid(tex)) {
            const bool obl = bgfx::getCaps()->originBottomLeft;
            ImVec2 uv0 = obl ? ImVec2(0, 1) : ImVec2(0, 0);
            ImVec2 uv1 = obl ? ImVec2(1, 0) : ImVec2(1, 1);
            ImGui::Image((ImTextureID)(uintptr_t)tex.idx, avail, uv0, uv1);
            m_vpHovered = ImGui::IsItemHovered();
        }

        // Рисуем гизмо здесь же, поверх image
        drawGizmo();

        ImGui::End();
        ImGui::PopStyleVar();
    }


    // ----------------------------------------------------------
    void EditorUI::drawMenuBar()
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save Scene"))
                grit::scene::SceneSerializer::saveScene(m_engine->world(), "scene.json");
            if (ImGui::MenuItem("Reload Scene")) {
                auto& reg = m_engine->world().registry();
                reg.clear();
                m_selected = entt::null;
                grit::scene::SceneSerializer::loadScene(
                    m_engine->world(), "scene.json",
                    m_engine->assets(),
                    m_engine->renderer().defaultProgram());
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit")) m_engine->requestQuit();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("ImGui Demo", nullptr, &m_showDemo);
            ImGui::EndMenu();
        }

        // Метрики справа
        const auto& t = m_engine->time();
        char buf[128];
        std::snprintf(buf, sizeof(buf), "FPS %.0f | %.2f ms",
            t.fps(), t.msPerFrame());
        const float w = ImGui::CalcTextSize(buf).x + 16.f;
        ImGui::SameLine(ImGui::GetWindowWidth() - w);
        ImGui::TextUnformatted(buf);
    }

    

    // ----------------------------------------------------------
    void EditorUI::drawHierarchy()
    {
        ImGui::SetNextWindowSize(ImVec2(280, 400), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Hierarchy")) { ImGui::End(); return; }

        auto& reg = m_engine->world().registry();

        // Рекурсивный отрисовщик
        std::function<void(entt::entity)> drawNode = [&](entt::entity e)
            {
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
                    | ImGuiTreeNodeFlags_SpanAvailWidth;
                if (e == m_selected) flags |= ImGuiTreeNodeFlags_Selected;

                const char* label = "<no name>";
                if (auto* n = reg.try_get<ecs::Name>(e); n && !n->value.empty())
                    label = n->value.c_str();

                auto* c = reg.try_get<ecs::Children>(e);
                const bool hasChildren = (c && !c->list.empty());
                if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf;

                bool open = ImGui::TreeNodeEx((void*)(uintptr_t)(uint32_t)e,
                    flags, "%s", label);
                if (ImGui::IsItemClicked()) m_selected = e;

                if (open) {
                    if (c) for (auto child : c->list) drawNode(child);
                    ImGui::TreePop();
                }
            };

        // Перебираем root-узлы
        auto& storage = reg.storage<entt::entity>();
        for (auto ent : storage) {
            if (auto* p = reg.try_get<ecs::Parent>(ent); p && p->entity != entt::null)
                continue;
            drawNode(ent);
        }

        ImGui::End();
    }

    // ----------------------------------------------------------
    static void inspectTransform(ecs::Transform& t)
    {
        ImGui::DragFloat3("Position", &t.position.x, 0.05f);
        ImGui::DragFloat3("Rotation", &t.rotationEuler.x, 0.5f);
        ImGui::DragFloat3("Scale", &t.scale.x, 0.01f, 0.001f, 1000.f);
    }

    static void inspectMaterial(render::Material& m, int idx)
    {
        ImGui::PushID(idx);
        ImGui::Text("Material %d", idx);
        ImGui::ColorEdit4("Base Color", &m.baseColor.x);
        ImGui::DragFloat4("Tiling/Off", &m.tilingOffset.x, 0.01f);
        ImGui::DragFloat("Metallic", &m.pbrParams.x, 0.01f, 0.f, 1.f);
        ImGui::DragFloat("Roughness", &m.pbrParams.y, 0.01f, 0.f, 1.f);
        ImGui::ColorEdit3("Emissive", &m.emissive.x);
        if (m.albedo) {
            ImGui::Text("Albedo: %s", m.albedo->path().c_str());
        }
        else {
            ImGui::TextDisabled("Albedo: <none>");
        }
        ImGui::PopID();
    }

    static void inspectLight(ecs::LightDirectional& L)
    {
        ImGui::DragFloat3("Direction", &L.direction.x, 0.01f, -1.f, 1.f);
        ImGui::ColorEdit3("Color", &L.color.x);
        ImGui::DragFloat("Intensity", &L.intensity, 0.01f, 0.f, 10.f);
        ImGui::DragFloat("Ambient", &L.ambient, 0.01f, 0.f, 1.f);
    }

    void EditorUI::drawInspector()
    {
        ImGui::SetNextWindowSize(ImVec2(320, 480), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Inspector")) { ImGui::End(); return; }

        if (m_selected == entt::null) {
            ImGui::TextDisabled("No entity selected");
            ImGui::End();
            return;
        }

        auto& reg = m_engine->world().registry();
        if (!reg.valid(m_selected)) {
            m_selected = entt::null;
            ImGui::End();
            return;
        }

        // Name
        if (auto* n = reg.try_get<ecs::Name>(m_selected)) {
            char buf[128];
            std::snprintf(buf, sizeof(buf), "%s", n->value.c_str());
            if (ImGui::InputText("Name", buf, sizeof(buf))) {
                n->value = buf;
            }
        }
        ImGui::Separator();

        // Transform
        if (auto* t = reg.try_get<ecs::Transform>(m_selected)) {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                inspectTransform(*t);
            }
        }

        // MeshRenderer
        if (auto* mr = reg.try_get<ecs::MeshRenderer>(m_selected)) {
            if (ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("Visible", &mr->visible);
                ImGui::Text("Mesh: %s",
                    mr->meshPath.empty() ? "<built-in>" : mr->meshPath.c_str());
                ImGui::Text("Materials: %d", (int)mr->materials.size());

                for (size_t i = 0; i < mr->materials.size(); ++i) {
                    if (mr->materials[i]) {
                        ImGui::Separator();
                        inspectMaterial(*mr->materials[i], (int)i);
                    }
                }
            }
        }

        // LightDirectional
        if (auto* L = reg.try_get<ecs::LightDirectional>(m_selected)) {
            if (ImGui::CollapsingHeader("Light (Directional)",
                ImGuiTreeNodeFlags_DefaultOpen))
            {
                inspectLight(*L);
            }
        }

        ImGui::End();
    }

} // namespace grit::editor