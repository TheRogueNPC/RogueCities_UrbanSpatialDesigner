#include "UIManager.hpp"
#include "ImGuiCompat.hpp"
#include <imgui_internal.h>

namespace RCG
{

    UIManager::UIManager(UserPreferences &prefs)
        : user_prefs(prefs), layout_initialized(false),
          central_dock_id(0), left_dock_id(0), right_dock_id(0), bottom_dock_id(0), root_dock_id(0),
          current_light_theme(prefs.app().use_light_theme)
    {
        setup_style();
        docking_layout.is_configured = false;
    }

    UIManager::~UIManager()
    {
    }

    void UIManager::begin_docking()
    {
#if RCG_IMGUI_HAS_DOCKING
        ImGuiIO &io = ImGui::GetIO();
        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");

        if (current_light_theme != user_prefs.app().use_light_theme)
        {
            apply_theme(user_prefs.app().use_light_theme);
            current_light_theme = user_prefs.app().use_light_theme;
        }

        // Enable docking
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        if (!layout_initialized)
        {
            if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr)
            {
                ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

                ImGuiID dock_main = dockspace_id;
                ImGuiID dock_left = 0;
                ImGuiID dock_right = 0;
                ImGuiID dock_right_bottom = 0;
                ImGuiID dock_view = 0;
                ImGuiID dock_bottom = 0;

                ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.25f, &dock_right, &dock_main);
                ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Left, 0.22f, &dock_left, &dock_main);
                ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, 0.22f, &dock_bottom, &dock_main);
                ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.18f, &dock_view, &dock_main);
                ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Down, 0.35f, &dock_right_bottom, &dock_right);

                ImGui::DockBuilderDockWindow("Tools", dock_left);
                ImGui::DockBuilderDockWindow("Parameters", dock_right);
                ImGui::DockBuilderDockWindow("District Index", dock_right_bottom);
                ImGui::DockBuilderDockWindow("Roads", dock_right_bottom);
                ImGui::DockBuilderDockWindow("Rivers", dock_right_bottom);
                ImGui::DockBuilderDockWindow("River Index", dock_right_bottom);
                ImGui::DockBuilderDockWindow("Lot Index", dock_right_bottom);
                ImGui::DockBuilderDockWindow("Building Index", dock_right_bottom);
                ImGui::DockBuilderDockWindow("Export & Status", dock_bottom);
                ImGui::DockBuilderDockWindow("Event Log", dock_bottom);
                ImGui::DockBuilderDockWindow("View Options", dock_view);
                ImGui::DockBuilderDockWindow("City Viewport", dock_main);

                ImGui::DockBuilderFinish(dockspace_id);
            }
            apply_user_layout();
            layout_initialized = true;
        }

        root_dock_id = ImGui::DockSpaceOverViewport(dockspace_id,
                                                    viewport,
                                                    ImGuiDockNodeFlags_PassthruCentralNode);
#else
        if (!layout_initialized)
        {
            layout_initialized = true;
        }
#endif
    }

    void UIManager::end_docking()
    {
        // ImGui handles docking teardown automatically
    }

    void UIManager::setup_style()
    {
        ImGuiStyle &style = ImGui::GetStyle();
        style.WindowRounding = 5.0f;
        style.FrameRounding = 4.0f;
        style.GrabRounding = 4.0f;
        style.PopupRounding = 4.0f;

        // Adjust padding and spacing for compact UI
        style.WindowPadding = ImVec2(6.0f, 6.0f);
        style.FramePadding = ImVec2(4.0f, 3.0f);
        style.CellPadding = ImVec2(4.0f, 2.0f);

        // Apply user's UI scale
        style.ScaleAllSizes(user_prefs.app().ui_scale);

        apply_theme(user_prefs.app().use_light_theme);
    }

    void UIManager::apply_theme(bool light_theme)
    {
        if (light_theme)
        {
            ImGui::StyleColorsLight();
        }
        else
        {
            ImGui::StyleColorsDark();
        }

        ImGuiStyle &style = ImGui::GetStyle();
        ImVec4 *colors = style.Colors;

        if (light_theme)
        {
            ImVec4 base_bg = ImVec4(0.984f, 0.953f, 0.819f, 1.0f);   // #FBF3D1
            ImVec4 panel = ImVec4(0.871f, 0.871f, 0.820f, 1.0f);     // #DEDED1
            ImVec4 panel_alt = ImVec4(0.773f, 0.780f, 0.737f, 1.0f); // #C5C7BC
            ImVec4 accent = ImVec4(0.714f, 0.682f, 0.624f, 1.0f);    // #B6AE9F
            ImVec4 text = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);

            colors[ImGuiCol_WindowBg] = base_bg;
            colors[ImGuiCol_TitleBg] = panel_alt;
            colors[ImGuiCol_TitleBgActive] = panel_alt;
            colors[ImGuiCol_TitleBgCollapsed] = panel_alt;
            colors[ImGuiCol_MenuBarBg] = panel_alt;
            colors[ImGuiCol_FrameBg] = panel;
            colors[ImGuiCol_FrameBgHovered] = panel_alt;
            colors[ImGuiCol_FrameBgActive] = accent;
            colors[ImGuiCol_Button] = panel_alt;
            colors[ImGuiCol_ButtonHovered] = accent;
            colors[ImGuiCol_ButtonActive] = accent;
            colors[ImGuiCol_Header] = panel_alt;
            colors[ImGuiCol_HeaderHovered] = accent;
            colors[ImGuiCol_HeaderActive] = accent;
            colors[ImGuiCol_Separator] = accent;
            colors[ImGuiCol_Tab] = panel_alt;
            colors[ImGuiCol_TabActive] = accent;
            colors[ImGuiCol_TabHovered] = accent;
            colors[ImGuiCol_SliderGrab] = ImVec4(0.45f, 0.38f, 0.30f, 1.0f);
            colors[ImGuiCol_SliderGrabActive] = ImVec4(0.30f, 0.24f, 0.18f, 1.0f);
            colors[ImGuiCol_CheckMark] = accent;
            colors[ImGuiCol_Text] = text;
        }
        else
        {
            ImVec4 base_bg = ImVec4(0.133f, 0.157f, 0.192f, 1.0f); // #222831
            ImVec4 panel = ImVec4(0.224f, 0.243f, 0.274f, 1.0f);   // #393E46
            ImVec4 accent = ImVec4(0.580f, 0.537f, 0.475f, 1.0f);  // #948979
            ImVec4 text = ImVec4(0.875f, 0.816f, 0.722f, 1.0f);    // #DFD0B8

            colors[ImGuiCol_WindowBg] = base_bg;
            colors[ImGuiCol_TitleBg] = panel;
            colors[ImGuiCol_TitleBgActive] = panel;
            colors[ImGuiCol_TitleBgCollapsed] = panel;
            colors[ImGuiCol_MenuBarBg] = panel;
            colors[ImGuiCol_FrameBg] = panel;
            colors[ImGuiCol_FrameBgHovered] = accent;
            colors[ImGuiCol_FrameBgActive] = accent;
            colors[ImGuiCol_Button] = panel;
            colors[ImGuiCol_ButtonHovered] = accent;
            colors[ImGuiCol_ButtonActive] = accent;
            colors[ImGuiCol_Header] = panel;
            colors[ImGuiCol_HeaderHovered] = accent;
            colors[ImGuiCol_HeaderActive] = accent;
            colors[ImGuiCol_Separator] = accent;
            colors[ImGuiCol_Tab] = panel;
            colors[ImGuiCol_TabActive] = accent;
            colors[ImGuiCol_TabHovered] = accent;
            colors[ImGuiCol_SliderGrab] = ImVec4(0.85f, 0.75f, 0.55f, 1.0f);
            colors[ImGuiCol_SliderGrabActive] = ImVec4(0.95f, 0.85f, 0.60f, 1.0f);
            colors[ImGuiCol_CheckMark] = accent;
            colors[ImGuiCol_Text] = text;
        }
    }

    void UIManager::setup_docking_space()
    {
#if RCG_IMGUI_HAS_DOCKING
        root_dock_id = ImGui::DockSpaceOverViewport(ImGui::GetID("MainDockSpace"),
                                                    ImGui::GetMainViewport(),
                                                    ImGuiDockNodeFlags_PassthruCentralNode);
#endif
    }

    void UIManager::apply_user_layout()
    {
        // This will be called after docking space is created
        // Layout preferences are applied automatically by ImGui's docking system
    }

} // namespace RCG
