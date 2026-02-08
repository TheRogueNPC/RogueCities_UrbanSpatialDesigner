#pragma once

#include <string>
#include <imgui.h>

namespace RCG
{

    /**
     * User preferences and settings persistence
     * Handles saving/loading window layout, tool settings, and application state
     */
    class UserPreferences
    {
    public:
        // Window state
        struct WindowPrefs
        {
            float city_params_width = 300.0f;
            float viewport_height = 600.0f;
            bool city_params_visible = true;
            bool viewport_visible = true;
            bool tools_visible = true;
            bool export_visible = true;
            bool parameters_visible = true;
            bool view_options_visible = true;
            bool district_visible = true;
            bool roads_visible = true;
            bool rivers_visible = true;
            bool river_index_visible = true;
            bool road_index_visible = true;
            bool lot_index_visible = true;
            bool building_index_visible = false;
            bool event_log_visible = true;
            bool presets_visible = true;
            bool progress_visible = false;
            bool color_settings_visible = true;
            bool tools_locked = false;
            bool parameters_locked = false;
            bool view_options_locked = false;
            bool district_locked = false;
            bool roads_locked = false;
            bool rivers_locked = false;
            bool river_index_locked = false;
            bool road_index_locked = false;
            bool lot_index_locked = false;
            bool building_index_locked = false;
            bool export_locked = false;
            bool event_log_locked = false;
            bool presets_locked = false;
            bool progress_locked = false;
            bool color_settings_locked = false;
        };

        // Tool settings
        struct ToolPrefs
        {
            int selected_growth_rule = 0;     // 0=RADIAL, 1=GRID, 2=DELTA
            int selected_tool = 0;            // 0=View, 1=Paint Rules, 2=Paint Density, 3=Place Axioms, 4=Draw Rivers, 5=Mark Zones, 6=Road Edit, 7=Lot Edit
            int selected_axiom_type = 0;      // 0=Radial, 1=Delta, 2=Block, 3=Grid Corrective
            int selected_delta_terminal = 0;  // 0=Top, 1=BottomLeft, 2=BottomRight
            int selected_radial_mode = 0;     // 0=Roundabout, 1=Center, 2=Both
            int selected_block_mode = 0;      // 0=Strict, 1=Free, 2=Corner
            int selected_influencer_type = 0; // 0=None, 1=Market, 2=Keep, 3=Temple, 4=Harbor, 5=Park, 6=Gate, 7=Well
            float axiom_size = 6.0f;
            float axiom_opacity = 0.85f;
            int selected_river_type = 0; // 0=Meandering, 1=Channelized, 2=Braided, 3=Anabranching
            float river_width = 12.0f;
            float river_opacity = 0.85f;
            // Lot Edit tool settings
            int lot_placement_mode = 0;         // 0=Lot, 1=Building
            int selected_lot_type = 1;          // CityModel::LotType enum value (1=Residential)
            int selected_building_type = 1;     // CityModel::BuildingType enum value (1=Residential)
            bool lock_user_placed_types = true; // If true, user-placed lots/buildings keep their types
            float brush_size = 30.0f;
            float brush_opacity = 0.8f;
            bool grid_snap = false;
            float grid_size = 32.0f;
        };

        // Application settings
        struct AppPrefs
        {
            struct KeyBinding
            {
                int key = ImGuiKey_None;
                bool ctrl = false;
                bool alt = false;
                bool shift = false;
            };

            struct Keymap
            {
                KeyBinding toggle_grid{ImGuiKey_1, true, false, false};
                KeyBinding toggle_axioms{ImGuiKey_2, true, false, false};
                KeyBinding toggle_roads{ImGuiKey_3, true, false, false};
                KeyBinding toggle_river_splines{ImGuiKey_4, true, false, false};
                KeyBinding toggle_road_intersections{ImGuiKey_5, true, false, false};
                KeyBinding reset_view{ImGuiKey_0, true, false, false};
            };

            struct IndexTabs
            {
                bool show_list = true;
                bool show_details = true;
                bool show_settings = true;
            };

            bool auto_regenerate = true;
            int regeneration_delay_ms = 100;
            bool show_grid = true;
            bool show_axioms = true;
            bool show_roads = true;
            bool show_axiom_influence = true;
            bool show_river_splines = true;
            bool show_river_markers = true;
            ImVec4 river_spline_color = ImVec4(0.25f, 0.65f, 0.95f, 0.9f);
            bool show_road_intersections = false;
            bool show_density_overlay = false;
            bool show_rules_overlay = true;
            bool show_district_borders = true;
            bool show_block_polygons = true;
            bool show_block_faces_debug = true;
            bool show_block_closable_faces = false;  // Show closable vs non-closable faces
            bool show_district_grid_overlay = false; // Show district grid cells
            bool show_district_ids = false;          // Show district IDs on grid
            bool enable_debug_logging = false;
            bool debug_use_segment_roads_for_blocks = false;
            bool enable_viewports = false;
            int district_grid_resolution = 128;
            float district_secondary_threshold = 0.2f;
            float district_weight_scale = 1.0f;
            bool district_rd_mode = false;
            float district_rd_mix = 0.0f;
            float district_desire_weight_axiom = 0.6f;
            float district_desire_weight_frontage = 0.4f;
            int grid_bounds = 1024;
            bool enable_culling = true;
            float cull_near = 0.1f;
            float cull_far = 1000.0f;
            float ui_scale = 1.0f;
            bool use_light_theme = false;
            float viewport_brightness = 1.0f;
            int generation_mode = 1;       // 0=Live, 1=Manual (defaults to Manual, resets each session)
            int river_generation_mode = 1; // 0=Live, 1=Manual (defaults to Manual, resets each session)
            bool show_selected_road_overlay = true;
            ImVec4 selected_road_color = ImVec4(1.0f, 0.8f, 0.0f, 1.0f); // Yellow highlight
            // Road highlighting options
            bool show_origin_axiom_highlight = true;
            bool show_end_axiom_highlight = true;
            bool show_influence_highlight = true;
            bool show_intersection_highlight = true;
            ImVec4 origin_axiom_color = ImVec4(0.0f, 1.0f, 0.0f, 0.8f);           // Green for origin
            ImVec4 end_axiom_color = ImVec4(1.0f, 0.5f, 0.0f, 0.8f);              // Orange for end
            ImVec4 influence_highlight_color = ImVec4(0.5f, 0.5f, 1.0f, 0.6f);    // Blue for influences
            ImVec4 intersection_highlight_color = ImVec4(1.0f, 0.0f, 1.0f, 0.8f); // Magenta for intersections
            // Road snapping options
            bool road_snap_to_grid = false;
            bool road_snap_to_axioms = true;
            bool road_dynamic_snap = true; // Snap to intersection points
            float road_snap_distance = 20.0f;
            // Road density as parallel spacing
            float min_parallel_spacing = 10.0f;
            float max_parallel_spacing = 50.0f;
            // Progress tracking
            bool show_progress = true;
            bool show_generation_stats = true;
            std::string current_operation = "";
            float generation_progress = 0.0f; // 0.0 to 1.0
            int roads_generated = 0;
            float generation_time_ms = 0.0f;

            Keymap keymap{};

            IndexTabs district_tabs{};
            IndexTabs road_tabs{};
            IndexTabs lot_tabs{};
            IndexTabs building_tabs{};
        };

        // Display/Rendering
        struct DisplayPrefs
        {
            ImVec4 background_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
            ImVec4 grid_color = ImVec4(0.3f, 0.3f, 0.3f, 0.3f);
            bool vsync = true;
            bool high_dpi = true;
            // Road visualization colors
            ImVec4 highway_road_color = ImVec4(0.95f, 0.35f, 0.2f, 1.0f);   // Red-orange
            ImVec4 arterial_road_color = ImVec4(0.95f, 0.55f, 0.25f, 1.0f); // Orange
            ImVec4 avenue_road_color = ImVec4(0.95f, 0.7f, 0.3f, 1.0f);     // Yellow-orange
            ImVec4 boulevard_road_color = ImVec4(0.9f, 0.8f, 0.35f, 1.0f);  // Warm yellow
            ImVec4 street_road_color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);      // Gray
            ImVec4 lane_road_color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);        // Darker gray
            ImVec4 alleyway_road_color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);    // Dark gray
            ImVec4 culdesac_road_color = ImVec4(0.65f, 0.65f, 0.5f, 1.0f);  // Olive gray
            ImVec4 drive_road_color = ImVec4(0.55f, 0.55f, 0.6f, 1.0f);     // Cool gray
            ImVec4 driveway_road_color = ImVec4(0.5f, 0.5f, 0.55f, 1.0f);   // Dark cool gray
            ImVec4 user_major_road_color = ImVec4(0.2f, 1.0f, 0.2f, 1.0f);  // Bright green for user major
            ImVec4 user_minor_road_color = ImVec4(0.1f, 0.6f, 0.1f, 1.0f);  // Dark green for user minor
            // Axiom visualization colors
            ImVec4 radial_axiom_color = ImVec4(0.3f, 0.5f, 1.0f, 0.85f); // Blue for radial
            ImVec4 delta_axiom_color = ImVec4(0.2f, 1.0f, 0.3f, 0.85f);  // Green for delta
            ImVec4 block_axiom_color = ImVec4(1.0f, 0.5f, 0.2f, 0.85f);  // Orange for block
            ImVec4 grid_axiom_color = ImVec4(1.0f, 0.2f, 0.5f, 0.85f);   // Pink for grid corrective
        };

        // Constructor - loads preferences from disk
        UserPreferences();
        ~UserPreferences();

        // Load/Save
        void load_from_file(const std::string &filepath = "user_prefs.json");
        void save_to_file(const std::string &filepath = "user_prefs.json");
        void load_from_ini(const std::string &filepath = "user_prefs.ini");
        void save_to_ini(const std::string &filepath = "user_prefs.ini");

        // Accessors
        WindowPrefs &windows() { return window_prefs; }
        ToolPrefs &tools() { return tool_prefs; }
        AppPrefs &app() { return app_prefs; }
        DisplayPrefs &display() { return display_prefs; }

        // Get full path for saved files
        static std::string get_config_dir();
        static std::string get_preferences_path();
        static std::string get_layout_path();
        static std::string get_ini_path();

    private:
        WindowPrefs window_prefs;
        ToolPrefs tool_prefs;
        AppPrefs app_prefs;
        DisplayPrefs display_prefs;

        bool load_json(const std::string &json_string);
        std::string save_json();
    };

} // namespace RCG
