
#pragma once

#include <imgui.h>      // ДО любого использования ImVec2 и до ImGuizmo
#include <ImGuizmo.h>   // строго после imgui.h
#include <entt/entt.hpp>
#include <cstdint>

namespace grit {
    class Engine;
}

namespace grit::editor {

    class EditorUI {
    public:
        void init(Engine* engine) { m_engine = engine; }

        // Вызывается между ImGui::NewFrame() и ImGui::Render() — рисует все панели.
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
        void drawViewport();
        void drawToolbar();
        void drawAssets();
        void drawGizmo();

        GizmoOp m_gizmoOp = GizmoOp::Translate;
        bool    m_playing = false;

        Engine* m_engine = nullptr;
        entt::entity  m_selected = entt::null;
        bool          m_enabled = true;
        uint32_t m_lastVpW = 0;
        uint32_t m_lastVpH = 0;
        std::string m_assetsRoot = "assets";
        std::string m_assetsCurrent = "assets";



        bool m_consoleAutoScroll = true;
        bool m_filterInfo = true;
        bool m_filterWarn = true;
        bool m_filterErr = true;
        char m_consoleFilter[64] = "";

        // Состояние гизмо
        bool m_gizmoUseLocal = false;   // World / Local space

        // Состояния панелей
        bool m_showDemo = false;
    };

} // namespace grit::editor