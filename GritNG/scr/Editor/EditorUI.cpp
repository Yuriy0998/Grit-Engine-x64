#include "EditorUI.h"
#include <imgui.h>
#include <ImGuizmo.h>
#include <imgui_internal.h>

#include "../Engine.h"
#include "../ecs/World.h"
#include "../ecs/SceneSystem.h"
#include "../ecs/Light.h"
#include "../render/Material.h"
#include "../render/StaticMesh.h"
#include "../render/Texture.h"
#include "../render/Renderer.h"
#include "../render/Primitives.h"
#include "../scene/SceneSerializer.h"
#include "../core/Log.h"
#include "../core/LogSink.h"
#include "../render/Environment.h"
#include "../render/TimeOfDay.h"

#include <functional>
#include <cstdio>
#include <cstring>
#include <string>
#include <filesystem>
#include <bx/math.h>
#include "Picking.h"
#include <cfloat>

namespace grit::editor {

    // =====================================================================
    // Entity creation
    // =====================================================================

    using grit::render::Environment;

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

        if (auto* n = reg.try_get<grit::ecs::Name>(src)) {
            reg.emplace<grit::ecs::Name>(dst, grit::ecs::Name{ n->value + " (Copy)" });
        }
        if (auto* t = reg.try_get<grit::ecs::Transform>(src)) {
            auto copy = *t;
            copy.position.x += 0.5f;
            reg.emplace<grit::ecs::Transform>(dst, copy);
        }
        if (auto* mr = reg.try_get<grit::ecs::MeshRenderer>(src)) {
            reg.emplace<grit::ecs::MeshRenderer>(dst, *mr);
        }
        if (auto* L = reg.try_get<grit::ecs::LightDirectional>(src)) {
            reg.emplace<grit::ecs::LightDirectional>(dst, *L);
        }
        m_selected = dst;
    }

    // =====================================================================
    // Main draw
    // =====================================================================

    void EditorUI::draw()
    {
        if (!m_enabled || !m_engine) return;

        ImGuizmo::BeginFrame();

        // --- Хоткеи ---
        ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureKeyboard) {
            if (ImGui::IsKeyPressed(ImGuiKey_W)) m_gizmoOp = GizmoOp::Translate;
            if (ImGui::IsKeyPressed(ImGuiKey_E)) m_gizmoOp = GizmoOp::Rotate;
            if (ImGui::IsKeyPressed(ImGuiKey_R)) m_gizmoOp = GizmoOp::Scale;

            if (m_selected != entt::null) {
                auto& reg = m_engine->world().registry();
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

        // ----------------------------------------------------------------
        // Хост-окно (только верхняя полоса: menu + tabs + toolbar)
        // Автовысота, чтобы не перекрывать центральную область
        // ----------------------------------------------------------------
        const ImGuiViewport* vp = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowViewport(vp->ID);

        ImGuiWindowFlags hostFlags =
            ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoBringToFrontOnFocus
            | ImGuiWindowFlags_NoNavFocus
            | ImGuiWindowFlags_MenuBar
            | ImGuiWindowFlags_NoBackground
            | ImGuiWindowFlags_NoScrollbar
            | ImGuiWindowFlags_AlwaysAutoResize
            | ImGuiWindowFlags_NoSavedSettings;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        // Принудительно растянуть по ширине (AlwaysAutoResize применяется только к высоте, если ширина задана)
        ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x, 0.0f));

        ImGui::Begin("##DockHost", nullptr, hostFlags);
        ImGui::PopStyleVar(3);

        // 1) MenuBar
        if (ImGui::BeginMenuBar()) {
            drawMenuBar();
            ImGui::EndMenuBar();
        }

        // 2) Tabs bar
        drawTabsBar();

        // 3) Toolbar
        drawToolbar();

        // Запомнить фактическую высоту шапки для drawCenterArea()
        m_headerHeight = ImGui::GetWindowSize().y;

        ImGui::End(); // ##DockHost

        if (m_showEnvironment) drawEnvironment();

        // --- Центральная область и сайдбар ---
        drawCenterArea();
        drawLeftSideBar();

        // --- Плавающие панели ---
        if (m_showHierarchy) drawHierarchy();
        if (m_showInspector) drawInspector();
        if (m_showConsole)   drawConsole();
        if (m_showAssets)    drawAssets();

        if (m_showDemo) ImGui::ShowDemoWindow(&m_showDemo);

        // --- Status bar ---
        {
            const ImGuiViewport* sv = ImGui::GetMainViewport();
            const float h = ImGui::GetFrameHeight();
            ImGui::SetNextWindowPos(ImVec2(sv->WorkPos.x, sv->WorkPos.y + sv->WorkSize.y - h));
            ImGui::SetNextWindowSize(ImVec2(sv->WorkSize.x, h));

            ImGuiWindowFlags fl =
                ImGuiWindowFlags_NoDecoration
                | ImGuiWindowFlags_NoMove
                | ImGuiWindowFlags_NoSavedSettings
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

        if (m_showTimeOfDay)  drawTimeOfDay();

        if (m_cameraCaptured) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_None);
        }

        m_engine->renderer().setSelectedEntity(m_selected);
    }

    // =====================================================================
    // Console
    // =====================================================================

    void EditorUI::drawConsole()
    {
        if (!ImGui::Begin("Console", &m_showConsole)) { ImGui::End(); return; }

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

    // =====================================================================
    // Gizmo
    // =====================================================================

    void EditorUI::drawGizmo()
    {
        if (m_selected == entt::null) return;
        if (m_centerTab != CenterTab::MapEditor) return;

        auto& reg = m_engine->world().registry();
        auto* tr = reg.try_get<grit::ecs::Transform>(m_selected);
        if (!tr) return;

        // Матрицы view/proj (GL-стиль для ImGuizmo)
        auto& cam = m_engine->renderer().camera();
        float view[16];
        float proj[16];
        cam.buildView(view);
        cam.buildProj(proj, true);   // homogeneousDepth = true

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
        ImGuizmo::SetRect(m_vpScreenPos.x, m_vpScreenPos.y,
            m_vpScreenSize.x, m_vpScreenSize.y);

        // Собираем матрицу из компонентов
        float matrix[16];
        float t[3] = { tr->position.x,      tr->position.y,      tr->position.z };
        float r[3] = { tr->rotationEuler.x, tr->rotationEuler.y, tr->rotationEuler.z };
        float s[3] = { tr->scale.x,         tr->scale.y,         tr->scale.z };
        ImGuizmo::RecomposeMatrixFromComponents(t, r, s, matrix);

        ImGuizmo::OPERATION op;
        switch (m_gizmoOp) {
        case GizmoOp::Translate: op = ImGuizmo::TRANSLATE; break;
        case GizmoOp::Rotate:    op = ImGuizmo::ROTATE;    break;
        case GizmoOp::Scale:     op = ImGuizmo::SCALE;     break;
        default:                 op = ImGuizmo::TRANSLATE; break;
        }
        ImGuizmo::MODE mode = m_gizmoUseLocal ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

        // Snap (опционально, держим Ctrl для активации)
        const bool useSnap = ImGui::GetIO().KeyCtrl;
        float snapT[3] = { 0.5f, 0.5f, 0.5f };
        float snapR[3] = { 15.0f, 15.0f, 15.0f };
        float snapS[3] = { 0.1f, 0.1f, 0.1f };
        const float* snap = nullptr;
        if (useSnap) {
            if (op == ImGuizmo::TRANSLATE) snap = snapT;
            else if (op == ImGuizmo::ROTATE)    snap = snapR;
            else                                snap = snapS;
        }

        if (ImGuizmo::Manipulate(view, proj, op, mode, matrix, nullptr, snap))
        {
            float nt[3], nr[3], ns[3];
            ImGuizmo::DecomposeMatrixToComponents(matrix, nt, nr, ns);
            if (op == ImGuizmo::TRANSLATE) tr->position = { nt[0], nt[1], nt[2] };
            else if (op == ImGuizmo::ROTATE)    tr->rotationEuler = { nr[0], nr[1], nr[2] };
            else if (op == ImGuizmo::SCALE)     tr->scale = { ns[0], ns[1], ns[2] };
        }
    }


    void EditorUI::pickEntity(float mouseInVpX, float mouseInVpY)
    {
        auto& reg = m_engine->world().registry();
        auto& cam = m_engine->renderer().camera();

        float view[16], proj[16];
        cam.buildView(view);
        cam.buildProj(proj, true);

        Ray ray = screenToRay(mouseInVpX, mouseInVpY,
            m_vpScreenSize.x, m_vpScreenSize.y,
            view, proj);

        entt::entity best = entt::null;
        float bestT = FLT_MAX;

        // --- Меши: ray vs AABB ---
        auto vw = reg.view<grit::ecs::Transform, grit::ecs::MeshRenderer>();
        for (auto e : vw) {
            const auto& tr = vw.get<grit::ecs::Transform>(e);
            const auto& mr = vw.get<grit::ecs::MeshRenderer>(e);
            if (!mr.visible || !mr.mesh) continue;

            const auto& lmin = mr.mesh->aabbMin();
            const auto& lmax = mr.mesh->aabbMax();

            float mtx[16];
            bx::mtxSRT(mtx,
                tr.scale.x, tr.scale.y, tr.scale.z,
                tr.rotationEuler.x, tr.rotationEuler.y, tr.rotationEuler.z,
                tr.position.x, tr.position.y, tr.position.z);

            vec3 wmn, wmx;
            transformAABB(mtx, lmin, lmax, wmn, wmx);

            float t;
            if (rayIntersectAABB(ray, wmn, wmx, t)) {
                if (t < bestT) {
                    bestT = t;
                    best = e;
                }
            }
        }

        // --- Lights: ray vs sphere (по иконкам) ---
        const float kLightPickRadius = 0.35f;
        const float kLightPickRadiusSq = kLightPickRadius * kLightPickRadius;

        auto tryPickLight = [&](entt::entity ent, const vec3& pos) {
            vec3 op{ pos.x - ray.origin.x, pos.y - ray.origin.y, pos.z - ray.origin.z };
            float t = op.x * ray.dir.x + op.y * ray.dir.y + op.z * ray.dir.z;
            if (t < 0.f) return;
            vec3 closest{
                ray.origin.x + ray.dir.x * t,
                ray.origin.y + ray.dir.y * t,
                ray.origin.z + ray.dir.z * t
            };
            vec3 d{ closest.x - pos.x, closest.y - pos.y, closest.z - pos.z };
            float dsq = d.x * d.x + d.y * d.y + d.z * d.z;
            if (dsq <= kLightPickRadiusSq && t < bestT) {
                bestT = t;
                best = ent;
            }
            };

        // Directional
        for (auto ent : reg.view<grit::ecs::LightDirectional>()) {
            vec3 p{ 0, 0, 0 };
            if (auto* tr = reg.try_get<grit::ecs::Transform>(ent)) {
                p = tr->position;
            }
            tryPickLight(ent, p);
        }

        // Point
        for (auto ent : reg.view<grit::ecs::Transform, grit::ecs::LightPoint>()) {
            auto& tr = reg.get<grit::ecs::Transform>(ent);
            tryPickLight(ent, tr.position);
        }

        // Spot
        for (auto ent : reg.view<grit::ecs::Transform, grit::ecs::LightSpot>()) {
            auto& tr = reg.get<grit::ecs::Transform>(ent);
            tryPickLight(ent, tr.position);
        }

        m_selected = best;
    }


    // =====================================================================
    // Left side bar
    // =====================================================================

    void EditorUI::drawLeftSideBar()
    {
        const float barW = 44.0f;

        const ImGuiViewport* vp = ImGui::GetMainViewport();

        const float topOffset = 24.0f + 44.0f + 38.0f;
        const float bottomOffset = ImGui::GetFrameHeight();

        ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x,
            vp->WorkPos.y + topOffset));
        ImGui::SetNextWindowSize(ImVec2(barW,
            vp->WorkSize.y - topOffset - bottomOffset));

        // ВАЖНО: НЕТ NoBringToFrontOnFocus — иначе кнопки не ловят клики
        ImGuiWindowFlags fl =
            ImGuiWindowFlags_NoDecoration
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoSavedSettings;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 6));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(28, 28, 32, 255));
        ImGui::Begin("##LeftBar", nullptr, fl);

        auto sideBtn = [&](const char* label, const char* tip, bool& toggle) {
            ImVec4 col = toggle ? ImVec4(0.30f, 0.55f, 0.85f, 1.f)
                : ImVec4(0.20f, 0.20f, 0.24f, 1.f);
            ImGui::PushStyleColor(ImGuiCol_Button, col);
            if (ImGui::Button(label, ImVec2(32, 32))) toggle = !toggle;
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tip);
            };

        sideBtn("H", "Hierarchy", m_showHierarchy);
        sideBtn("I", "Inspector", m_showInspector);
        sideBtn("C", "Console", m_showConsole);
        sideBtn("A", "Assets", m_showAssets);

        ImGui::Dummy(ImVec2(0, 8));

        sideBtn("E", "Environment", m_showEnvironment);
        sideBtn("T", "Time of Day", m_showTimeOfDay);

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }
    // =====================================================================
    // Assets
    // =====================================================================

    void EditorUI::drawAssets()
    {
        if (!ImGui::Begin("Assets", &m_showAssets)) { ImGui::End(); return; }

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
            const std::string full = path.string();

            if (entry.is_directory()) {
                if (ImGui::Selectable(("[D] " + name).c_str(), false,
                    ImGuiSelectableFlags_AllowDoubleClick))
                {
                    if (ImGui::IsMouseDoubleClicked(0))
                        m_assetsCurrent = full;
                }
            }
            else {
                ImGui::Selectable(("    " + name).c_str(), false);

                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                {
                    ImGui::SetDragDropPayload("GRIT_ASSET_PATH",
                        full.c_str(), full.size() + 1);
                    ImGui::Text("Drag: %s", name.c_str());
                    ImGui::EndDragDropSource();
                }
            }
        }

        ImGui::End();
    }

    // =====================================================================
    // Tabs bar
    // =====================================================================

    void EditorUI::drawTabsBar()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16, 8));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));

        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImVec2 p0 = ImGui::GetCursorScreenPos();
            const float  w = ImGui::GetContentRegionAvail().x;
            const float  h = ImGui::GetFrameHeight() + 8.0f;
            dl->AddRectFilled(p0, ImVec2(p0.x + w, p0.y + h),
                IM_COL32(28, 28, 32, 255));
        }

        ImGuiTabBarFlags flags =
            ImGuiTabBarFlags_Reorderable
            | ImGuiTabBarFlags_AutoSelectNewTabs
            | ImGuiTabBarFlags_NoCloseWithMiddleMouseButton
            | ImGuiTabBarFlags_FittingPolicyScroll;

        if (ImGui::BeginTabBar("##MainTabs", flags))
        {
            struct TabDef {
                const char* label;
                CenterTab   id;
            };
            const TabDef defs[] = {
                { "Map Editor",        CenterTab::MapEditor },
                { "Navigation Editor", CenterTab::Navigation },
                { "Game",              CenterTab::Game },
                { "Lua Script",        CenterTab::LuaScript },
            };

            for (const auto& d : defs) {
                if (ImGui::BeginTabItem(d.label, nullptr, ImGuiTabItemFlags_NoTooltip)) {
                    m_centerTab = d.id;
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }

        ImGui::PopStyleVar(2);
    }

    // =====================================================================
    // Toolbar
    // =====================================================================

    void EditorUI::drawToolbar()
    {
        const float btnSize = 28.0f;
        const float h = btnSize + 10.0f;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 5));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 0));

        ImGui::GetWindowDrawList()->AddLine(
            ImGui::GetCursorScreenPos(),
            ImVec2(ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x,
                ImGui::GetCursorScreenPos().y),
            IM_COL32(20, 20, 22, 255), 1.0f);
        ImGui::BeginChild("##Toolbar", ImVec2(0, h), false,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        enum class IconKind { Translate, Rotate, Scale };

        auto iconButton = [&](const char* id, IconKind kind, bool active) -> bool
            {
                ImGui::PushID(id);

                const ImVec2 cursor = ImGui::GetCursorScreenPos();
                ImDrawList* dl = ImGui::GetWindowDrawList();

                bool clicked = ImGui::InvisibleButton("##b", ImVec2(btnSize, btnSize));
                bool hovered = ImGui::IsItemHovered();

                ImU32 bg;
                if (active)       bg = ImGui::GetColorU32(ImGuiCol_HeaderActive);
                else if (hovered) bg = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
                else              bg = ImGui::GetColorU32(ImGuiCol_Button);

                const ImVec2 p0 = cursor;
                const ImVec2 p1 = ImVec2(cursor.x + btnSize, cursor.y + btnSize);
                dl->AddRectFilled(p0, p1, bg, 4.0f);

                const ImU32 fg = ImGui::GetColorU32(ImGuiCol_Text);
                const ImVec2 c = ImVec2((p0.x + p1.x) * 0.5f, (p0.y + p1.y) * 0.5f);
                const float r = btnSize * 0.32f;

                switch (kind)
                {
                case IconKind::Translate:
                {
                    const float a = r;
                    const float ah = r * 0.45f;
                    dl->AddLine(ImVec2(c.x, c.y - a), ImVec2(c.x, c.y + a), fg, 1.6f);
                    dl->AddLine(ImVec2(c.x - a, c.y), ImVec2(c.x + a, c.y), fg, 1.6f);
                    dl->AddLine(ImVec2(c.x, c.y - a), ImVec2(c.x - ah * 0.5f, c.y - a + ah * 0.7f), fg, 1.6f);
                    dl->AddLine(ImVec2(c.x, c.y - a), ImVec2(c.x + ah * 0.5f, c.y - a + ah * 0.7f), fg, 1.6f);
                    dl->AddLine(ImVec2(c.x, c.y + a), ImVec2(c.x - ah * 0.5f, c.y + a - ah * 0.7f), fg, 1.6f);
                    dl->AddLine(ImVec2(c.x, c.y + a), ImVec2(c.x + ah * 0.5f, c.y + a - ah * 0.7f), fg, 1.6f);
                    dl->AddLine(ImVec2(c.x - a, c.y), ImVec2(c.x - a + ah * 0.7f, c.y - ah * 0.5f), fg, 1.6f);
                    dl->AddLine(ImVec2(c.x - a, c.y), ImVec2(c.x - a + ah * 0.7f, c.y + ah * 0.5f), fg, 1.6f);
                    dl->AddLine(ImVec2(c.x + a, c.y), ImVec2(c.x + a - ah * 0.7f, c.y - ah * 0.5f), fg, 1.6f);
                    dl->AddLine(ImVec2(c.x + a, c.y), ImVec2(c.x + a - ah * 0.7f, c.y + ah * 0.5f), fg, 1.6f);
                    break;
                }
                case IconKind::Rotate:
                {
                    const int seg = 24;
                    const float a0 = -1.4f;
                    const float a1 = 3.6f;
                    ImVec2 prev{};
                    for (int i = 0; i <= seg; ++i) {
                        float t = (float)i / (float)seg;
                        float ang = a0 + (a1 - a0) * t;
                        ImVec2 pt = ImVec2(c.x + std::cos(ang) * r,
                            c.y + std::sin(ang) * r);
                        if (i > 0) dl->AddLine(prev, pt, fg, 1.6f);
                        prev = pt;
                    }
                    float ang = a1;
                    ImVec2 tip = ImVec2(c.x + std::cos(ang) * r,
                        c.y + std::sin(ang) * r);
                    float tangent = ang + 1.5708f;
                    ImVec2 tdir = ImVec2(std::cos(tangent), std::sin(tangent));
                    ImVec2 ndir = ImVec2(std::cos(ang), std::sin(ang));
                    float ah = r * 0.45f;
                    ImVec2 base = ImVec2(tip.x - tdir.x * ah, tip.y - tdir.y * ah);
                    ImVec2 w1 = ImVec2(base.x + ndir.x * ah * 0.5f, base.y + ndir.y * ah * 0.5f);
                    ImVec2 w2 = ImVec2(base.x - ndir.x * ah * 0.5f, base.y - ndir.y * ah * 0.5f);
                    dl->AddTriangleFilled(tip, w1, w2, fg);
                    break;
                }
                case IconKind::Scale:
                {
                    const float a = r;
                    dl->AddLine(ImVec2(c.x - a * 0.7f, c.y + a * 0.7f),
                        ImVec2(c.x + a * 0.7f, c.y - a * 0.7f), fg, 1.6f);
                    dl->AddLine(ImVec2(c.x + a * 0.7f, c.y - a * 0.7f),
                        ImVec2(c.x + a * 0.7f, c.y - a * 0.2f), fg, 1.6f);
                    dl->AddLine(ImVec2(c.x + a * 0.7f, c.y - a * 0.7f),
                        ImVec2(c.x + a * 0.2f, c.y - a * 0.7f), fg, 1.6f);
                    dl->AddLine(ImVec2(c.x - a * 0.7f, c.y + a * 0.7f),
                        ImVec2(c.x - a * 0.7f, c.y + a * 0.2f), fg, 1.6f);
                    dl->AddLine(ImVec2(c.x - a * 0.7f, c.y + a * 0.7f),
                        ImVec2(c.x - a * 0.2f, c.y + a * 0.7f), fg, 1.6f);
                    float sq = 2.0f;
                    dl->AddRectFilled(ImVec2(c.x + a * 0.7f - sq, c.y - a * 0.7f - sq),
                        ImVec2(c.x + a * 0.7f + sq, c.y - a * 0.7f + sq), fg);
                    dl->AddRectFilled(ImVec2(c.x - a * 0.7f - sq, c.y + a * 0.7f - sq),
                        ImVec2(c.x - a * 0.7f + sq, c.y + a * 0.7f + sq), fg);
                    break;
                }
                }

                if (active) dl->AddRect(p0, p1, fg, 4.0f, 0, 1.5f);

                ImGui::PopID();
                return clicked;
            };

        if (iconButton("tr", IconKind::Translate, m_gizmoOp == GizmoOp::Translate))
            m_gizmoOp = GizmoOp::Translate;
        ImGui::SameLine();
        if (iconButton("ro", IconKind::Rotate, m_gizmoOp == GizmoOp::Rotate))
            m_gizmoOp = GizmoOp::Rotate;
        ImGui::SameLine();
        if (iconButton("sc", IconKind::Scale, m_gizmoOp == GizmoOp::Scale))
            m_gizmoOp = GizmoOp::Scale;

        ImGui::SameLine();
        ImGui::Dummy(ImVec2(12, 0));
        ImGui::SameLine();

        ImGui::AlignTextToFramePadding();
        bool showGrid = m_engine->renderer().m_showGrid;
        if (ImGui::Checkbox("Grid", &showGrid)) {
            m_engine->renderer().m_showGrid = showGrid;
        }

        ImGui::SameLine();
        ImGui::Dummy(ImVec2(12, 0));
        ImGui::SameLine();

        const char* spaceLabel = m_gizmoUseLocal ? "Local" : "World";
        if (ImGui::Button(spaceLabel, ImVec2(64, btnSize)))
            m_gizmoUseLocal = !m_gizmoUseLocal;

        ImGui::SameLine();
        ImGui::Dummy(ImVec2(20, 0));
        ImGui::SameLine();

        if (ImGui::Button(m_playing ? "Stop" : "Play", ImVec2(56, btnSize))) {
            m_playing = !m_playing;
        }

        ImGui::EndChild();
        ImGui::PopStyleVar(2);
    }

    // =====================================================================
    // Center area (Viewport)
    // =====================================================================

    void EditorUI::drawCenterArea()
    {
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        const float topOffset = 24.0f + 44.0f + 38.0f;
        const float bottomOffset = ImGui::GetFrameHeight();
        const float leftOffset = 44.0f;

        ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + leftOffset,
            vp->WorkPos.y + topOffset));
        ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x - leftOffset,
            vp->WorkSize.y - topOffset - bottomOffset));

        ImGuiWindowFlags fl =
            ImGuiWindowFlags_NoDecoration
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("##Viewport", nullptr, fl);

      



        switch (m_centerTab)
        {
        case CenterTab::MapEditor:
        {
            const ImVec2 avail = ImGui::GetContentRegionAvail();
            const uint32_t w = (uint32_t)bx::max(1.0f, avail.x);
            const uint32_t h = (uint32_t)bx::max(1.0f, avail.y);

            if (w != m_lastVpW || h != m_lastVpH) {
                m_engine->renderer().resizeViewport(w, h);
                m_lastVpW = w;
                m_lastVpH = h;
            }

            m_vpScreenPos = ImGui::GetCursorScreenPos();
            m_vpScreenSize = avail;
            m_vpHovered = false;

            auto tex = m_engine->renderer().viewportColorTexture();
            if (bgfx::isValid(tex))
            {
                const bool obl = bgfx::getCaps()->originBottomLeft;
                ImVec2 uv0 = obl ? ImVec2(0, 1) : ImVec2(0, 0);
                ImVec2 uv1 = obl ? ImVec2(1, 0) : ImVec2(1, 1);

                // Картинка с RT
                ImGui::Image((ImTextureID)(uintptr_t)tex.idx, avail, uv0, uv1);

               

                m_vpHovered = ImGui::IsItemHovered();

                // Picking по ЛКМ
                if (m_vpHovered
                    && !m_cameraCaptured
                    && !ImGuizmo::IsOver()
                    && !ImGuizmo::IsUsing()
                    && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    ImVec2 mouse = ImGui::GetMousePos();
                    float mx = mouse.x - m_vpScreenPos.x;
                    float my = mouse.y - m_vpScreenPos.y;
                    pickEntity(mx, my);
                }

                // Drag&Drop приём ассетов
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("GRIT_ASSET_PATH"))
                    {
                        std::string path((const char*)p->Data);
                        handleAssetDrop(path);
                    }
                    ImGui::EndDragDropTarget();
                }

                // Захват камеры: ПКМ
                const bool rmb = ImGui::IsMouseDown(ImGuiMouseButton_Right);

                if (!m_cameraCaptured
                    && m_vpHovered
                    && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                {
                    m_cameraCaptured = true;
                }
                if (m_cameraCaptured && !rmb) {
                    m_cameraCaptured = false;
                }
            }

            // Гизмо рисуем ПОСЛЕ всего, поверх Image
            drawGizmo();
            break;
        }

        case CenterTab::Navigation:
            ImGui::TextDisabled("Navigation Editor - coming soon");
            break;

        case CenterTab::Game:
            ImGui::TextDisabled("Game view - coming soon");
            break;

        case CenterTab::LuaScript:
            ImGui::TextDisabled("Lua Script editor - coming soon");
            break;
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }
    // =====================================================================
    // Asset drop
    // =====================================================================

    void EditorUI::handleAssetDrop(const std::string& path)
    {
        namespace fs = std::filesystem;
        fs::path p(path);
        const std::string ext = p.extension().string();
        const std::string name = p.stem().string();

        if (ext == ".grmesh")
        {
            auto& assets = m_engine->assets();
            auto  prog = m_engine->renderer().defaultProgram();

            auto model = assets.loadModel(path, prog);
            if (!model.mesh) {
                GRIT_WARN("Drop: failed to load mesh '%s'", path.c_str());
                return;
            }

            auto& reg = m_engine->world().registry();
            auto e = reg.create();
            reg.emplace<grit::ecs::Name>(e, grit::ecs::Name{ name });
            reg.emplace<grit::ecs::Transform>(e);

            grit::ecs::MeshRenderer mr;
            mr.mesh = model.mesh;
            mr.meshPath = path;
            mr.visible = true;
            for (auto& m : model.materials) mr.materials.push_back(m);
            reg.emplace<grit::ecs::MeshRenderer>(e, std::move(mr));

            m_selected = e;
            GRIT_INFO("Drop: spawned entity from '%s'", path.c_str());
            return;
        }

        GRIT_WARN("Drop: unsupported asset type '%s'", ext.c_str());
    }

    // =====================================================================
    // MenuBar
    // =====================================================================

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

        if (ImGui::BeginMenu("Edit"))
        {
            const bool hasSel = (m_selected != entt::null);
            if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, hasSel))
                duplicateEntity(m_selected);
            if (ImGui::MenuItem("Delete", "Del", false, hasSel)) {
                m_engine->world().registry().destroy(m_selected);
                m_selected = entt::null;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("GameObject"))
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


            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Hierarchy", nullptr, &m_showHierarchy);
            ImGui::MenuItem("Inspector", nullptr, &m_showInspector);
            ImGui::MenuItem("Console", nullptr, &m_showConsole);
            ImGui::MenuItem("Assets", nullptr, &m_showAssets);
            ImGui::Separator();
            ImGui::MenuItem("ImGui Demo", nullptr, &m_showDemo);
            ImGui::MenuItem("Time of Day", nullptr, &m_showTimeOfDay);
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




    // =====================================================================
    // Hierarchy
    // =====================================================================

    void EditorUI::drawHierarchy()
    {
        if (!ImGui::Begin("Hierarchy", &m_showHierarchy)) { ImGui::End(); return; }

        auto& reg = m_engine->world().registry();

        if (ImGui::BeginPopupContextWindow("HierarchyEmptyPopup",
            ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
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
            ImGui::EndPopup();
        }

        entt::entity toDelete = entt::null;

        auto view = reg.view<grit::ecs::Name>();
        for (auto e : view)
        {
            auto& name = view.get<grit::ecs::Name>(e);
            ImGui::PushID((int)entt::to_integral(e));

            const bool selected = (e == m_selected);

            if (m_renaming == e)
            {
                char buf[128];
                std::snprintf(buf, sizeof(buf), "%s", name.value.c_str());

                ImGui::SetKeyboardFocusHere();
                ImGuiInputTextFlags fl =
                    ImGuiInputTextFlags_EnterReturnsTrue
                    | ImGuiInputTextFlags_AutoSelectAll;

                if (ImGui::InputText("##rename", buf, sizeof(buf), fl)) {
                    name.value = buf;
                    m_renaming = entt::null;
                }
                if (!ImGui::IsItemActive() && !ImGui::IsItemFocused()
                    && ImGui::IsMouseClicked(0))
                {
                    name.value = buf;
                    m_renaming = entt::null;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                    m_renaming = entt::null;
                }
            }
            else
            {
                if (ImGui::Selectable(name.value.c_str(), selected,
                    ImGuiSelectableFlags_AllowDoubleClick))
                {
                    m_selected = e;
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        m_renaming = e;
                    }
                }
            }

            if (ImGui::BeginPopupContextItem("EntityPopup"))
            {
                m_selected = e;
                if (ImGui::MenuItem("Rename", "F2")) {
                    m_renaming = e;
                }
                if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
                    duplicateEntity(e);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Delete", "Del")) {
                    toDelete = e;
                }
                ImGui::EndPopup();
            }

            ImGui::PopID();
        }

        if (toDelete != entt::null) {
            if (m_selected == toDelete) m_selected = entt::null;
            if (m_renaming == toDelete) m_renaming = entt::null;
            reg.destroy(toDelete);
        }

        ImGui::End();
    }

    // =====================================================================
    // Inspector helpers
    // =====================================================================

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

    // =====================================================================
    // Inspector
    // =====================================================================

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
        entt::entity e = m_selected;

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
            if (ImGui::InputText("Name", buf, sizeof(buf))) {
                n->value = buf;
            }
        }

        // ===== Transform =====
        if (auto* tr = reg.try_get<grit::ecs::Transform>(e)) {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat3("Position", &tr->position.x, 0.05f);
                ImGui::DragFloat3("Rotation", &tr->rotationEuler.x, 0.5f);
                ImGui::DragFloat3("Scale", &tr->scale.x, 0.05f, 0.01f, 100.0f);
            }
        }

        // ===== MeshRenderer (твой код) =====
        // ... оставь как есть ...

        // ===== Directional Light =====
        if (auto* L = reg.try_get<grit::ecs::LightDirectional>(e)) {
            if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat3("Direction", &L->direction.x, 0.01f);
                ImGui::ColorEdit3("Color", &L->color.x);
                ImGui::DragFloat("Intensity", &L->intensity, 0.05f, 0.0f, 20.0f);
                ImGui::DragFloat("Ambient", &L->ambient, 0.01f, 0.0f, 1.0f);

                if (ImGui::Button("Remove##DirLight")) {
                    reg.remove<grit::ecs::LightDirectional>(e);
                }
            }
        }

        // ===== Point Light =====
        if (auto* L = reg.try_get<grit::ecs::LightPoint>(e)) {
            if (ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::ColorEdit3("Color", &L->color.x);
                ImGui::DragFloat("Intensity", &L->intensity, 0.05f, 0.0f, 50.0f);
                ImGui::DragFloat("Range", &L->range, 0.1f, 0.01f, 1000.0f);

                if (ImGui::Button("Remove##PointLight")) {
                    reg.remove<grit::ecs::LightPoint>(e);
                }
            }
        }

        // ===== Spot Light =====
        if (auto* L = reg.try_get<grit::ecs::LightSpot>(e)) {
            if (ImGui::CollapsingHeader("Spot Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat3("Direction", &L->direction.x, 0.01f);
                ImGui::ColorEdit3("Color", &L->color.x);
                ImGui::DragFloat("Intensity", &L->intensity, 0.05f, 0.0f, 50.0f);
                ImGui::DragFloat("Range", &L->range, 0.1f, 0.01f, 1000.0f);

                float innerDeg = L->innerCone * 57.2957795f;
                float outerDeg = L->outerCone * 57.2957795f;
                if (ImGui::DragFloat("Inner Cone (deg)", &innerDeg, 0.5f, 1.0f, 89.0f)) {
                    L->innerCone = innerDeg / 57.2957795f;
                    if (L->outerCone < L->innerCone) L->outerCone = L->innerCone;
                }
                if (ImGui::DragFloat("Outer Cone (deg)", &outerDeg, 0.5f, 1.0f, 89.0f)) {
                    L->outerCone = outerDeg / 57.2957795f;
                    if (L->innerCone > L->outerCone) L->innerCone = L->outerCone;
                }

                if (ImGui::Button("Remove##SpotLight")) {
                    reg.remove<grit::ecs::LightSpot>(e);
                }
            }
        }

        // ===== Add Component =====
        ImGui::Separator();
        if (ImGui::Button("Add Component")) {
            ImGui::OpenPopup("AddComponentPopup");
        }
        if (ImGui::BeginPopup("AddComponentPopup")) {
            if (ImGui::BeginMenu("Light")) {
                if (ImGui::MenuItem("Directional Light",
                    nullptr, false,
                    !reg.any_of<grit::ecs::LightDirectional>(e)))
                {
                    reg.emplace<grit::ecs::LightDirectional>(e);
                }
                if (ImGui::MenuItem("Point Light",
                    nullptr, false,
                    !reg.any_of<grit::ecs::LightPoint>(e)))
                {
                    reg.emplace<grit::ecs::LightPoint>(e);
                }
                if (ImGui::MenuItem("Spot Light",
                    nullptr, false,
                    !reg.any_of<grit::ecs::LightSpot>(e)))
                {
                    reg.emplace<grit::ecs::LightSpot>(e);
                }
                ImGui::EndMenu();
            }
            ImGui::EndPopup();
        }

        ImGui::End();
    }



    void EditorUI::drawTimeOfDay()
    {
        if (!ImGui::Begin("Time of Day", &m_showTimeOfDay)) {
            ImGui::End();
            return;
        }

        auto& tod = m_engine->renderer().timeOfDay();

        // Текущий час
        f32 hour = tod.hour();
        if (ImGui::SliderFloat("Hour", &hour, 0.0f, 24.0f, "%.2f h")) {
            tod.setHour(hour);
        }

        // Скорость
        f32 speed = tod.speed();
        if (ImGui::DragFloat("Speed (hours/sec)", &speed, 0.01f, 0.0f, 5.0f)) {
            tod.setSpeed(speed);
        }

        ImGui::Separator();

        // Пресеты скорости
        if (ImGui::Button("Pause"))          tod.setSpeed(0.0f);
        ImGui::SameLine();
        if (ImGui::Button("Real (24min)"))   tod.setSpeed(1.0f / 60.0f);
        ImGui::SameLine();
        if (ImGui::Button("Fast (24sec)"))   tod.setSpeed(1.0f);
        ImGui::SameLine();
        if (ImGui::Button("Crazy (2.4sec)")) tod.setSpeed(10.0f);

        ImGui::Separator();

        // Быстрые пресеты времени
        ImGui::Text("Jump to:");
        if (ImGui::Button("Dawn  (5:00)"))  tod.setHour(5.0f);   ImGui::SameLine();
        if (ImGui::Button("Noon (12:00)"))  tod.setHour(12.0f);  ImGui::SameLine();
        if (ImGui::Button("Dusk (19:00)"))  tod.setHour(19.0f);  ImGui::SameLine();
        if (ImGui::Button("Night (0:00)"))  tod.setHour(0.0f);

        ImGui::Separator();

        // Редактор keyframes
        if (ImGui::CollapsingHeader("Keyframes")) {
            for (int i = 0; i < tod.keyframeCount(); ++i) {
                auto& k = tod.keyframe(i);
                ImGui::PushID(i);
                if (ImGui::TreeNode((void*)(intptr_t)i, "Key #%d  @  %.2fh", i, k.hour)) {
                    ImGui::DragFloat("Hour", &k.hour, 0.1f, 0.0f, 24.0f);
                    ImGui::ColorEdit3("Sky Top", &k.skyTop.x);
                    ImGui::ColorEdit3("Sky Bottom", &k.skyBottom.x);
                    ImGui::ColorEdit3("Sun Color", &k.sunColor.x);
                    ImGui::DragFloat("Sun Intensity", &k.sunIntensity, 0.01f, 0.0f, 5.0f);
                    ImGui::ColorEdit3("Ambient", &k.ambientColor.x);
                    ImGui::DragFloat("Ambient Intensity", &k.ambientIntensity, 0.01f, 0.0f, 5.0f);
                    ImGui::ColorEdit3("Fog Color", &k.fogColor.x);
                    ImGui::DragFloat("Fog Start", &k.fogStart, 1.0f, 0.0f, 1000.0f);
                    ImGui::DragFloat("Fog End", &k.fogEnd, 1.0f, 0.0f, 5000.0f);
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
        }

        ImGui::End();
    }

    void EditorUI::drawEnvironment()
    {
        if (!ImGui::Begin("Environment", &m_showEnvironment)) {
            ImGui::End();
            return;
        }

        auto& env = m_engine->renderer().env();

        if (ImGui::CollapsingHeader("Fog", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Enabled", &env.fogEnabled);

            const char* modes[] = { "Linear", "Exp", "Exp2" };
            int mode = (int)env.fogMode;
            if (ImGui::Combo("Mode", &mode, modes, 3)) {
                env.fogMode = (grit::render::FogMode)mode;
            }

            ImGui::ColorEdit3("Color", &env.fogColor.x);
            ImGui::DragFloat("Start", &env.fogStart, 0.5f, 0.0f, 10000.0f);
            ImGui::DragFloat("End", &env.fogEnd, 0.5f, 0.0f, 10000.0f);
            ImGui::DragFloat("Density", &env.fogDensity, 0.001f, 0.0f, 1.0f, "%.4f");
        }

        if (ImGui::CollapsingHeader("Ambient", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::ColorEdit3("Color", &env.ambientColor.x);
            ImGui::DragFloat("Intensity", &env.ambientIntensity, 0.01f, 0.0f, 5.0f);
        }

        if (ImGui::CollapsingHeader("Sun")) {
            ImGui::DragFloat3("Direction", &env.sunDirection.x, 0.01f, -1.0f, 1.0f);
            ImGui::ColorEdit3("Color", &env.sunColor.x);
            ImGui::DragFloat("Intensity", &env.sunIntensity, 0.01f, 0.0f, 10.0f);
        }

        if (ImGui::CollapsingHeader("Sky (preview)")) {
            ImGui::ColorEdit3("Top", &env.skyTopColor.x);
            ImGui::ColorEdit3("Bottom", &env.skyBottomColor.x);
        }

        ImGui::End();
    }


    

} // namespace grit::editor