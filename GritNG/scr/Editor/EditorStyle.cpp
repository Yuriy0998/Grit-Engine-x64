#include "EditorStyle.h"
#include <imgui.h>

namespace grit::editor {

    void applyDarkTheme()
    {
        ImGuiStyle& s = ImGui::GetStyle();
        ImVec4* c = s.Colors;

        // --- Геометрия ---
        s.WindowRounding = 4.f;
        s.ChildRounding = 4.f;
        s.FrameRounding = 3.f;
        s.PopupRounding = 4.f;
        s.ScrollbarRounding = 6.f;
        s.GrabRounding = 3.f;
        s.TabRounding = 4.f;

        s.WindowPadding = ImVec2(8, 8);
        s.FramePadding = ImVec2(6, 4);
        s.ItemSpacing = ImVec2(8, 4);
        s.ItemInnerSpacing = ImVec2(6, 4);
        s.IndentSpacing = 18.f;
        s.ScrollbarSize = 12.f;
        s.GrabMinSize = 10.f;

        s.WindowBorderSize = 0.f;
        s.ChildBorderSize = 0.f;
        s.PopupBorderSize = 1.f;
        s.FrameBorderSize = 0.f;
        s.TabBorderSize = 0.f;

        // --- Палитра ---
        const ImVec4 bg = ImVec4(0.110f, 0.110f, 0.120f, 1.00f);
        const ImVec4 bgDark = ImVec4(0.082f, 0.082f, 0.090f, 1.00f);
        const ImVec4 bgLight = ImVec4(0.160f, 0.160f, 0.175f, 1.00f);
        const ImVec4 panel = ImVec4(0.150f, 0.150f, 0.165f, 1.00f);
        const ImVec4 panelHov = ImVec4(0.210f, 0.210f, 0.230f, 1.00f);
        const ImVec4 panelAct = ImVec4(0.250f, 0.250f, 0.270f, 1.00f);
        const ImVec4 border = ImVec4(0.000f, 0.000f, 0.000f, 0.35f);
        const ImVec4 text = ImVec4(0.880f, 0.880f, 0.880f, 1.00f);
        const ImVec4 textDim = ImVec4(0.560f, 0.560f, 0.580f, 1.00f);
        const ImVec4 accent = ImVec4(0.260f, 0.590f, 0.980f, 1.00f); // голубой Unity-style
        const ImVec4 accentDim = ImVec4(0.260f, 0.590f, 0.980f, 0.40f);

        c[ImGuiCol_Text] = text;
        c[ImGuiCol_TextDisabled] = textDim;

        c[ImGuiCol_WindowBg] = bg;
        c[ImGuiCol_ChildBg] = bg;
        c[ImGuiCol_PopupBg] = bgLight;

        c[ImGuiCol_Border] = border;
        c[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

        c[ImGuiCol_FrameBg] = bgDark;
        c[ImGuiCol_FrameBgHovered] = panelHov;
        c[ImGuiCol_FrameBgActive] = panelAct;

        c[ImGuiCol_TitleBg] = bgDark;
        c[ImGuiCol_TitleBgActive] = panel;
        c[ImGuiCol_TitleBgCollapsed] = bgDark;

        c[ImGuiCol_MenuBarBg] = bgDark;

        c[ImGuiCol_ScrollbarBg] = bgDark;
        c[ImGuiCol_ScrollbarGrab] = panel;
        c[ImGuiCol_ScrollbarGrabHovered] = panelHov;
        c[ImGuiCol_ScrollbarGrabActive] = panelAct;

        c[ImGuiCol_CheckMark] = accent;
        c[ImGuiCol_SliderGrab] = accent;
        c[ImGuiCol_SliderGrabActive] = accent;

        c[ImGuiCol_Button] = panel;
        c[ImGuiCol_ButtonHovered] = panelHov;
        c[ImGuiCol_ButtonActive] = panelAct;

        c[ImGuiCol_Header] = accentDim;
        c[ImGuiCol_HeaderHovered] = ImVec4(accent.x, accent.y, accent.z, 0.55f);
        c[ImGuiCol_HeaderActive] = accent;

        c[ImGuiCol_Separator] = border;
        c[ImGuiCol_SeparatorHovered] = accent;
        c[ImGuiCol_SeparatorActive] = accent;

        c[ImGuiCol_ResizeGrip] = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_ResizeGripHovered] = accentDim;
        c[ImGuiCol_ResizeGripActive] = accent;

        c[ImGuiCol_Tab] = bgDark;
        c[ImGuiCol_TabHovered] = panelHov;
        c[ImGuiCol_TabActive] = panel;
        c[ImGuiCol_TabUnfocused] = bgDark;
        c[ImGuiCol_TabUnfocusedActive] = panel;

        c[ImGuiCol_DockingPreview] = accent;
        c[ImGuiCol_DockingEmptyBg] = bgDark;

        c[ImGuiCol_PlotLines] = textDim;
        c[ImGuiCol_PlotLinesHovered] = accent;
        c[ImGuiCol_PlotHistogram] = accent;
        c[ImGuiCol_PlotHistogramHovered] = ImVec4(1, 0.65f, 0.2f, 1);

        c[ImGuiCol_TableHeaderBg] = panel;
        c[ImGuiCol_TableBorderStrong] = border;
        c[ImGuiCol_TableBorderLight] = ImVec4(0, 0, 0, 0.20f);
        c[ImGuiCol_TableRowBg] = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_TableRowBgAlt] = ImVec4(1, 1, 1, 0.03f);

        c[ImGuiCol_TextSelectedBg] = accentDim;
        c[ImGuiCol_DragDropTarget] = accent;
        c[ImGuiCol_NavHighlight] = accent;
        c[ImGuiCol_NavWindowingHighlight] = accent;
        c[ImGuiCol_NavWindowingDimBg] = ImVec4(0, 0, 0, 0.40f);
        c[ImGuiCol_ModalWindowDimBg] = ImVec4(0, 0, 0, 0.55f);
    }

} // namespace grit::editor