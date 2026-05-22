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

        enum class GizmoOp { Translate, Rotate, Scale };

        bool   m_vpHovered = false;
        GizmoOp gizmoOp() const { return m_gizmoOp; }

        ImVec2 m_vpScreenPos{};
        ImVec2 m_vpScreenSize{};

    private:
        void drawHierarchy();
        void drawInspector();
        void drawMenuBar();
        void drawConsole();
        void drawAssets();
        void drawGizmo();

        void drawTabsBar();      // фиксированная полоса вкладок
        void drawToolbar();      // фиксированная полоса инструментов (БЕЗ Begin!)
        void drawCenterArea();   // dockable окно "Viewport"

        enum class CenterTab { MapEditor, Game, LuaScript };
        CenterTab m_centerTab = CenterTab::MapEditor;

        // Создание/изменение entity
        entt::entity createEmpty(const char* name = "Empty");
        entt::entity createPrimitive(const char* kind); // "Cube" / "Sphere" / "Plane"
        entt::entity createLight();
        void         duplicateEntity(entt::entity src);

        // Состояние
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



        bool m_consoleAutoScroll = true;
        bool m_filterInfo = true;
        bool m_filterWarn = true;
        bool m_filterErr = true;
        char m_consoleFilter[64] = "";

        bool m_showDemo = false;
    };

} // namespace grit::editor