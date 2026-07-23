#include "EditorUI.h"
#include "EditorUI_Internal.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <filesystem>
#include <cmath>

#include "../Engine.h"
#include "../core/LogSink.h"
#include "../render/Renderer.h"

namespace grit::editor {

    using namespace grit::editor::detail;

    // ---------- Console ----------
    static void drawLogLine(const grit::LogEntry& e)
    {
        ImVec4 col;
        switch (e.level) {
        case grit::LogLevel::Warn:  col = ImVec4(1.00f, 0.78f, 0.20f, 1); break;
        case grit::LogLevel::Error: col = ImVec4(1.00f, 0.35f, 0.30f, 1); break;
        default:                    col = ImVec4(0.85f, 0.85f, 0.85f, 1); break;
        }
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::TextUnformatted(e.text.c_str());
        ImGui::PopStyleColor();
    }

    void EditorUI::drawConsole()
    {
        if (!ImGui::Begin("Console", &m_showConsole)) { ImGui::End(); return; }

        auto& sink = grit::LogSink::get();
        if (ImGui::Button("Clear")) sink.clear();
        ImGui::SameLine(); ImGui::Checkbox("Auto-scroll", &m_consoleAutoScroll);
        ImGui::SameLine(); ImGui::Checkbox("Info", &m_filterInfo);
        ImGui::SameLine(); ImGui::Checkbox("Warn", &m_filterWarn);
        ImGui::SameLine(); ImGui::Checkbox("Error", &m_filterErr);
        ImGui::SameLine(); ImGui::SetNextItemWidth(180);
        ImGui::InputTextWithHint("##filter", "filter...", m_consoleFilter, sizeof(m_consoleFilter));
        ImGui::Separator();

        if (ImGui::BeginChild("##log", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
        {
            // ¬ј∆Ќќ: нужно API снапшота под lock без копировани€ Ч здесь
            // используетс€ snapshot() как fallback. ≈сли можете добавить
            // lockedView() в LogSink Ч замените.
            const auto& entries = sink.snapshot();
            const bool hasFilter = m_consoleFilter[0] != '\0';

            m_visibleLogIdx.clear();
            m_visibleLogIdx.reserve(entries.size());
            for (int i = 0; i < (int)entries.size(); ++i) {
                const auto& e = entries[i];
                bool show = (e.level == grit::LogLevel::Info && m_filterInfo)
                    || (e.level == grit::LogLevel::Warn && m_filterWarn)
                    || (e.level == grit::LogLevel::Error && m_filterErr);
                if (!show) continue;
                if (hasFilter && e.text.find(m_consoleFilter) == std::string::npos) continue;
                m_visibleLogIdx.push_back(i);
            }

            ImGuiListClipper clipper;
            clipper.Begin((int)m_visibleLogIdx.size());
            while (clipper.Step()) {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                    drawLogLine(entries[m_visibleLogIdx[row]]);
                }
            }

            if (m_consoleAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
        ImGui::End();
    }

    // ---------- Assets (с кэшем) ----------
    void EditorUI::refreshAssetsCache()
    {
        namespace fs = std::filesystem;
        m_assetsCache.clear();

        std::error_code ec;
        if (!fs::exists(m_assetsCurrent, ec)) {
            m_assetsCachedPath = m_assetsCurrent;
            m_assetsDirty = false;
            return;
        }

        for (const auto& entry : fs::directory_iterator(m_assetsCurrent, ec)) {
            AssetEntry a;
            a.isDir = entry.is_directory();
            a.name = entry.path().filename().string();
            a.fullPath = entry.path().string();
            a.label = (a.isDir ? "[D] " : "    ") + a.name;
            m_assetsCache.push_back(std::move(a));
        }

        // ѕапки сверху
        std::sort(m_assetsCache.begin(), m_assetsCache.end(),
            [](const AssetEntry& x, const AssetEntry& y) {
                if (x.isDir != y.isDir) return x.isDir > y.isDir;
                return x.name < y.name;
            });

        m_assetsCachedPath = m_assetsCurrent;
        m_assetsDirty = false;
    }

    void EditorUI::drawAssets()
    {
        if (!ImGui::Begin("Assets", &m_showAssets)) { ImGui::End(); return; }

        namespace fs = std::filesystem;

        if (m_assetsDirty || m_assetsCachedPath != m_assetsCurrent)
            refreshAssetsCache();

        ImGui::Text("Path: %s", m_assetsCurrent.c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("Up")) {
            fs::path p(m_assetsCurrent);
            if (p.has_parent_path() && p != fs::path(m_assetsRoot)) {
                m_assetsCurrent = p.parent_path().string();
                m_assetsDirty = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Refresh")) m_assetsDirty = true;
        ImGui::Separator();

        for (const auto& a : m_assetsCache) {
            if (a.isDir) {
                if (ImGui::Selectable(a.label.c_str(), false,
                    ImGuiSelectableFlags_AllowDoubleClick)
                    && ImGui::IsMouseDoubleClicked(0)) {
                    m_assetsCurrent = a.fullPath;
                    m_assetsDirty = true;
                }
            }
            else {
                ImGui::Selectable(a.label.c_str(), false);
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    ImGui::SetDragDropPayload("GRIT_ASSET_PATH",
                        a.fullPath.c_str(), a.fullPath.size() + 1);
                    ImGui::Text("Drag: %s", a.name.c_str());
                    ImGui::EndDragDropSource();
                }
            }
        }

        ImGui::End();
    }

    // ---------- Left side bar ----------
    void EditorUI::drawLeftSideBar()
    {
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        const float bottomOffset = ImGui::GetFrameHeight();

        ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x, vp->WorkPos.y + kTopOffset));
        ImGui::SetNextWindowSize(ImVec2(kLeftBarW,
            vp->WorkSize.y - kTopOffset - bottomOffset));

        ImGuiWindowFlags fl = ImGuiWindowFlags_NoDecoration
            | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

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

    // ---------- Tabs bar ----------
    void EditorUI::drawTabsBar()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16, 8));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));

        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImVec2 p0 = ImGui::GetCursorScreenPos();
            const float  w = ImGui::GetContentRegionAvail().x;
            const float  h = ImGui::GetFrameHeight() + 8.0f;
            dl->AddRectFilled(p0, ImVec2(p0.x + w, p0.y + h), IM_COL32(28, 28, 32, 255));
        }

        ImGuiTabBarFlags flags = ImGuiTabBarFlags_Reorderable
            | ImGuiTabBarFlags_AutoSelectNewTabs
            | ImGuiTabBarFlags_NoCloseWithMiddleMouseButton
            | ImGuiTabBarFlags_FittingPolicyScroll;

        if (ImGui::BeginTabBar("##MainTabs", flags)) {
            struct TabDef { const char* label; CenterTab id; };
            static const TabDef defs[] = {
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

    // ---------- Toolbar ----------
    namespace {
        enum class IconKind { Translate, Rotate, Scale };

        void drawIcon(ImDrawList* dl, IconKind kind, ImVec2 c, float r, ImU32 fg)
        {
            switch (kind) {
            case IconKind::Translate: {
                const float a = r, ah = r * 0.45f;
                dl->AddLine({ c.x, c.y - a }, { c.x, c.y + a }, fg, 1.6f);
                dl->AddLine({ c.x - a, c.y }, { c.x + a, c.y }, fg, 1.6f);
                dl->AddLine({ c.x, c.y - a }, { c.x - ah * 0.5f, c.y - a + ah * 0.7f }, fg, 1.6f);
                dl->AddLine({ c.x, c.y - a }, { c.x + ah * 0.5f, c.y - a + ah * 0.7f }, fg, 1.6f);
                dl->AddLine({ c.x, c.y + a }, { c.x - ah * 0.5f, c.y + a - ah * 0.7f }, fg, 1.6f);
                dl->AddLine({ c.x, c.y + a }, { c.x + ah * 0.5f, c.y + a - ah * 0.7f }, fg, 1.6f);
                dl->AddLine({ c.x - a, c.y }, { c.x - a + ah * 0.7f, c.y - ah * 0.5f }, fg, 1.6f);
                dl->AddLine({ c.x - a, c.y }, { c.x - a + ah * 0.7f, c.y + ah * 0.5f }, fg, 1.6f);
                dl->AddLine({ c.x + a, c.y }, { c.x + a - ah * 0.7f, c.y - ah * 0.5f }, fg, 1.6f);
                dl->AddLine({ c.x + a, c.y }, { c.x + a - ah * 0.7f, c.y + ah * 0.5f }, fg, 1.6f);
                break;
            }
            case IconKind::Rotate: {
                const int seg = 24;
                const float a0 = -1.4f, a1 = 3.6f;
                ImVec2 prev{};
                for (int i = 0; i <= seg; ++i) {
                    float t = (float)i / seg;
                    float ang = a0 + (a1 - a0) * t;
                    ImVec2 pt{ c.x + std::cos(ang) * r, c.y + std::sin(ang) * r };
                    if (i > 0) dl->AddLine(prev, pt, fg, 1.6f);
                    prev = pt;
                }
                float ang = a1;
                ImVec2 tip{ c.x + std::cos(ang) * r, c.y + std::sin(ang) * r };
                float tangent = ang + 1.5708f;
                ImVec2 tdir{ std::cos(tangent), std::sin(tangent) };
                ImVec2 ndir{ std::cos(ang), std::sin(ang) };
                float ah = r * 0.45f;
                ImVec2 base{ tip.x - tdir.x * ah, tip.y - tdir.y * ah };
                ImVec2 w1{ base.x + ndir.x * ah * 0.5f, base.y + ndir.y * ah * 0.5f };
                ImVec2 w2{ base.x - ndir.x * ah * 0.5f, base.y - ndir.y * ah * 0.5f };
                dl->AddTriangleFilled(tip, w1, w2, fg);
                break;
            }
            case IconKind::Scale: {
                const float a = r;
                dl->AddLine({ c.x - a * 0.7f, c.y + a * 0.7f }, { c.x + a * 0.7f, c.y - a * 0.7f }, fg, 1.6f);
                dl->AddLine({ c.x + a * 0.7f, c.y - a * 0.7f }, { c.x + a * 0.7f, c.y - a * 0.2f }, fg, 1.6f);
                dl->AddLine({ c.x + a * 0.7f, c.y - a * 0.7f }, { c.x + a * 0.2f, c.y - a * 0.7f }, fg, 1.6f);
                dl->AddLine({ c.x - a * 0.7f, c.y + a * 0.7f }, { c.x - a * 0.7f, c.y + a * 0.2f }, fg, 1.6f);
                dl->AddLine({ c.x - a * 0.7f, c.y + a * 0.7f }, { c.x - a * 0.2f, c.y + a * 0.7f }, fg, 1.6f);
                float sq = 2.0f;
                dl->AddRectFilled({ c.x + a * 0.7f - sq, c.y - a * 0.7f - sq },
                    { c.x + a * 0.7f + sq, c.y - a * 0.7f + sq }, fg);
                dl->AddRectFilled({ c.x - a * 0.7f - sq, c.y + a * 0.7f - sq },
                    { c.x - a * 0.7f + sq, c.y + a * 0.7f + sq }, fg);
                break;
            }
            }
        }
    } // anon namespace

    void EditorUI::drawToolbar()
    {
        const float btnSize = 28.0f;
        const float h = btnSize + 10.0f;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 5));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 0));

        const ImVec2 cur = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddLine(cur,
            ImVec2(cur.x + ImGui::GetContentRegionAvail().x, cur.y),
            IM_COL32(20, 20, 22, 255), 1.0f);

        ImGui::BeginChild("##Toolbar", ImVec2(0, h), false,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        auto iconButton = [&](const char* id, IconKind kind, bool active) -> bool {
            ImGui::PushID(id);
            const ImVec2 cursor = ImGui::GetCursorScreenPos();
            ImDrawList* dl = ImGui::GetWindowDrawList();

            bool clicked = ImGui::InvisibleButton("##b", ImVec2(btnSize, btnSize));
            bool hovered = ImGui::IsItemHovered();

            ImU32 bg = active ? ImGui::GetColorU32(ImGuiCol_HeaderActive)
                : hovered ? ImGui::GetColorU32(ImGuiCol_ButtonHovered)
                : ImGui::GetColorU32(ImGuiCol_Button);

            const ImVec2 p0 = cursor;
            const ImVec2 p1 = { cursor.x + btnSize, cursor.y + btnSize };
            dl->AddRectFilled(p0, p1, bg, 4.0f);

            const ImU32 fg = ImGui::GetColorU32(ImGuiCol_Text);
            const ImVec2 c = { (p0.x + p1.x) * 0.5f, (p0.y + p1.y) * 0.5f };
            drawIcon(dl, kind, c, btnSize * 0.32f, fg);

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

        ImGui::SameLine(); ImGui::Dummy(ImVec2(12, 0)); ImGui::SameLine();

        ImGui::AlignTextToFramePadding();
        bool showGrid = m_engine->renderer().m_showGrid;
        if (ImGui::Checkbox("Grid", &showGrid))
            m_engine->renderer().m_showGrid = showGrid;

        ImGui::SameLine(); ImGui::Dummy(ImVec2(12, 0)); ImGui::SameLine();
        const char* spaceLabel = m_gizmoUseLocal ? "Local" : "World";
        if (ImGui::Button(spaceLabel, ImVec2(64, btnSize)))
            m_gizmoUseLocal = !m_gizmoUseLocal;

        ImGui::SameLine(); ImGui::Dummy(ImVec2(20, 0)); ImGui::SameLine();
        if (ImGui::Button(m_playing ? "Stop" : "Play", ImVec2(56, btnSize)))
            m_playing = !m_playing;

        ImGui::EndChild();
        ImGui::PopStyleVar(2);
    }

} // namespace grit::editor