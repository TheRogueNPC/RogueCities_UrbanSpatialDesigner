#pragma once

#include <imgui.h>
#include <memory>
#include "UserPreferences.hpp"

namespace RCG
{

    /**
     * Manages ImGui window layout, docking, and UI structure
     * Organizes windows into a professional docking layout
     */
    class UIManager
    {
    public:
        UIManager(UserPreferences &prefs);
        ~UIManager();

        /**
         * Initialize docking layout for the frame
         * Call at start of ImGui::NewFrame()
         */
        void begin_docking();

        /**
         * End docking - call before ImGui::Render()
         */
        void end_docking();

        /**
         * Setup ImGui style and colors
         */
        void setup_style();
        void apply_theme(bool light_theme);

        /**
         * Get docking space IDs
         */
        ImGuiID get_central_dockid() const { return central_dock_id; }
        ImGuiID get_left_dockid() const { return left_dock_id; }
        ImGuiID get_right_dockid() const { return right_dock_id; }
        ImGuiID get_bottom_dockid() const { return bottom_dock_id; }

        /**
         * Layout:
         *
         * ┌──────────────────────────────────────┐
         * │        MenuBar (File, Edit, View)    │
         * ├──────────┬──────────────┬────────────┤
         * │          │              │            │
         * │  Tools   │   Viewport   │  Parameters│
         * │  (Left)  │   (Central)  │  (Right)   │
         * │          │              │            │
         * ├──────────┴──────────────┴────────────┤
         * │        Export/Status Bar (Bottom)    │
         * └──────────────────────────────────────┘
         */

        struct DockingLayout
        {
            ImVec2 viewport_size;
            ImVec2 left_panel_size;
            ImVec2 right_panel_size;
            bool is_configured;
        };

        DockingLayout &layout() { return docking_layout; }

    private:
        ImGuiID central_dock_id;
        ImGuiID left_dock_id;
        ImGuiID right_dock_id;
        ImGuiID bottom_dock_id;
        ImGuiID root_dock_id;

        DockingLayout docking_layout;
        UserPreferences &user_prefs;
        bool layout_initialized;
        bool current_light_theme;

        void setup_docking_space();
        void apply_user_layout();
    };

} // namespace RCG
