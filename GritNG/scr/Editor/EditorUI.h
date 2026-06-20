#pragma once

#include <imgui.h>
#include <ImGuizmo.h>
#include <entt/entt.hpp>
#include <cstdint>
#include <string>

namespace grit {
    class Engine;
}

namespace grit::editor {

    class EditorUI {
    public:
        void init(Engine* engine) { m_engine = engine; }

        // Вызывается между ImGui::NewFrame() и ImGui::Render()
        void draw();

        bool enabled() const { return m_enabled; }
        void setEnabled(bool v) { m_enabled = v; }

        bool viewportHovered() const { return m_vpHovered; }
        bool cameraCaptured()  const { return m_cameraCaptured; }

        enum class CenterTab { MapEditor, Navigation, Game, LuaScript };
        enum class GizmoOp { Translate, Rotate, Scale };

        GizmoOp gizmoOp() const { return m_gizmoOp; }

        bool   m_vpHovered = false;
        ImVec2 m_vpScreenPos{};
        ImVec2 m_vpScreenSize{};

    private:
        // Панели
        void drawHierarchy();
        void drawInspector();
        void drawConsole();
        void drawAssets();

        // Главные элементы UI
        void drawMenuBar();
        void drawTabsBar();
        void drawToolbar();
        void drawLeftSideBar();
        void drawCenterArea();
        void drawGizmo();
        float m_headerHeight = 0.0f;
        // Прочее
        void handleAssetDrop(const std::string& path);

        // Entity helpers
        entt::entity createEmpty(const char* name = "Empty");
        entt::entity createPrimitive(const char* kind); // "Cube"/"Sphere"/"Plane"
        entt::entity createLight();
        void         duplicateEntity(entt::entity src);

        // Состояние
        CenterTab      m_centerTab = CenterTab::MapEditor;
        GizmoOp        m_gizmoOp = GizmoOp::Translate;
        bool           m_gizmoUseLocal = false;
        bool           m_playing = false;
        bool           m_enabled = true;

        Engine* m_engine = nullptr;
        entt::entity   m_selected = entt::null;
        entt::entity   m_renaming = entt::null;

        uint32_t       m_lastVpW = 0;
        uint32_t       m_lastVpH = 0;

        std::string    m_assetsRoot = "assets";
        std::string    m_assetsCurrent = "assets";

        // Видимость панелей (по умолчанию — скрыты, как на 3-м скрине)
        bool m_showHierarchy = false;
        bool m_showInspector = false;
        bool m_showConsole = false;
        bool m_showAssets = false;

        // Console
        bool m_consoleAutoScroll = true;
        bool m_filterInfo = true;
        bool m_filterWarn = true;
        bool m_filterErr = true;
        char m_consoleFilter[64] = "";

        // Camera capture
        bool m_cameraCaptured = false;

        bool m_showDemo = false;
    };

} // namespace grit::editor