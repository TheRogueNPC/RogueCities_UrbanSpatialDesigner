#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include "UserPreferences.hpp"
#include "CityParams.hpp"
#include "ViewportWindow.hpp"
#include "RoadTypes.hpp"

namespace RCG
{

    /**
     * Left panel: Drawing tools and brush controls
     */
    class ToolsWindow
    {
    public:
        ToolsWindow(UserPreferences &prefs);
        ~ToolsWindow();

        /**
         * Render tools panel and handle tool selection
         */
        void render();

        /**
         * Get currently selected tool
         * 0=View, 1=Paint Rules, 2=Paint Density, 3=Place Axioms, 4=Draw Rivers, 5=Mark Zones, 6=Road Edit, 7=Lot Edit
         */
        int get_selected_tool() const { return user_prefs.tools().selected_tool; }

        /**
         * Get brush settings
         */
        float get_brush_size() const { return user_prefs.tools().brush_size; }
        float get_brush_opacity() const { return user_prefs.tools().brush_opacity; }
        int get_selected_rule() const { return user_prefs.tools().selected_growth_rule; }

        /**
         * Checkbox states
         */
        bool get_grid_snap() const { return user_prefs.tools().grid_snap; }

    private:
        UserPreferences &user_prefs;

        void render_tool_buttons();
        void render_brush_controls();
        void render_rule_selector();
    };

    /**
     * Right panel: City parameters and generation controls
     */
    class ParametersWindow
    {
    public:
        ParametersWindow(UserPreferences &prefs);
        ~ParametersWindow();

        /**
         * Render parameters panel
         */
        void render(CityParams &params);

        /**
         * Check if "Export" button was clicked
         */
        bool export_clicked() const { return export_button_clicked; }
        bool generate_city_clicked() const { return generate_city_button_clicked; }
        bool regenerate_lots_clicked() const { return regenerate_lots_button_clicked; }

    private:
        UserPreferences &user_prefs;
        bool export_button_clicked;
        bool generate_city_button_clicked;
        bool regenerate_lots_button_clicked;

        void render_basic_params(CityParams &params);
        void render_advanced_params(CityParams &params);
        void render_action_buttons();
    };

    /**
     * Bottom panel: Status bar and export controls
     */
    class ExportWindow
    {
    public:
        ExportWindow(UserPreferences &prefs);
        ~ExportWindow();

        /**
         * Render export panel
         */
        void render();

        /**
         * Get export format (0=JSON, 1=OBJ, 2=SVG)
         */
        int get_export_format() const { return export_format; }

        /**
         * Check if export was triggered
         */
        bool export_triggered() const { return export_button_clicked; }

        /**
         * Display status message
         */
        void set_status(const std::string &message) { status_message = message; }

    private:
        UserPreferences &user_prefs;
        int export_format;
        bool export_button_clicked;
        std::string status_message;

        void render_export_options();
        void render_status_bar();
    };

    /**
     * View options window (viewport display toggles)
     */
    class ViewOptionsWindow
    {
    public:
        ViewOptionsWindow(UserPreferences &prefs);
        ~ViewOptionsWindow();

        void render();

    private:
        UserPreferences &user_prefs;
    };

    /**
     * Roads window (generation controls)
     */
    class RoadsWindow
    {
    public:
        RoadsWindow(UserPreferences &prefs);
        ~RoadsWindow();

        void render(CityParams &params, bool has_city);

        bool generate_clicked() const { return generate_button_clicked; }
        bool clear_clicked() const { return clear_button_clicked; }
        bool clear_all_clicked() const { return clear_all_button_clicked; }

    private:
        UserPreferences &user_prefs;
        bool generate_button_clicked;
        bool clear_button_clicked;
        bool clear_all_button_clicked;
    };

    /**
     * Rivers window (generation controls)
     */
    class RiversWindow
    {
    public:
        RiversWindow(UserPreferences &prefs);
        ~RiversWindow();

        void render(CityParams &params, bool has_rivers);

        bool generate_clicked() const { return generate_button_clicked; }
        bool clear_clicked() const { return clear_button_clicked; }

    private:
        UserPreferences &user_prefs;
        bool generate_button_clicked;
        bool clear_button_clicked;
    };

    /**
     * District index window: unified axiom + district controls
     */
    class DistrictIndexWindow
    {
    public:
        DistrictIndexWindow(UserPreferences &prefs);
        ~DistrictIndexWindow();

        /**
         * Render district index panel
         */
        void render(ViewportWindow &viewport, CityParams &city_params, CityModel::City &city);

    private:
        UserPreferences &user_prefs;
        int last_selected_id;
        int hovered_axiom_id;
        std::unordered_set<int> selected_axiom_ids;
        int active_axiom_id;
        int selected_district_id;
        int hovered_district_id;
        std::unordered_set<int> selected_district_ids;
        int active_district_id;
        int axiom_filter_index;
        int district_filter_index;
        bool show_axioms;
        bool show_districts;
        char name_buffer[128];
    };

    /**
     * River index window: list river splines and edit control points
     */
    class RiverIndexWindow
    {
    public:
        RiverIndexWindow(UserPreferences &prefs);
        ~RiverIndexWindow();

        void render(ViewportWindow &viewport);

    private:
        UserPreferences &user_prefs;
        int last_selected_id;
        char name_buffer[128];
    };

    /**
     * Event log window: shows app-level log messages
     */
    class EventLogWindow
    {
    public:
        EventLogWindow(UserPreferences &prefs);
        ~EventLogWindow();

        /**
         * Render event log panel
         */
        void render();

        /**
         * Add a new log entry
         */
        void add_entry(const std::string &message);

        /**
         * Clear log entries
         */
        void clear();

    private:
        UserPreferences &user_prefs;
        std::vector<std::string> entries;
        bool auto_scroll;
        int max_entries;
    };

    /**
     * Presets window: manage and apply city generation presets
     */
    class PresetsWindow
    {
    public:
        PresetsWindow(UserPreferences &prefs);
        ~PresetsWindow();

        /**
         * Render presets panel
         */
        void render(CityParams &params);

        /**
         * Check if preset was applied
         */
        bool preset_applied() const { return apply_clicked; }

        /**
         * Get selected preset name
         */
        std::string get_selected_preset() const { return selected_preset_name; }

    private:
        UserPreferences &user_prefs;
        int selected_preset_index;
        std::string selected_preset_name;
        bool apply_clicked;
        bool save_clicked;
        char new_preset_name[128];
        char new_preset_description[256];
    };

    /**
     * Progress indicator window: shows generation progress and stats
     */
    class ProgressWindow
    {
    public:
        ProgressWindow(UserPreferences &prefs);
        ~ProgressWindow();

        /**
         * Render progress panel
         */
        void render();

        /**
         * Update progress (0.0 to 1.0)
         */
        void set_progress(float progress);

        /**
         * Set current operation description
         */
        void set_operation(const std::string &operation);

        /**
         * Update generation statistics
         */
        void set_stats(int roads_generated, float time_ms);

        /**
         * Show/hide progress indicator
         */
        void set_visible(bool visible);

    private:
        UserPreferences &user_prefs;
        bool is_visible;
    };

    /**
     * Color settings window: customize visualization colors
     */
    class ColorSettingsWindow
    {
    public:
        ColorSettingsWindow(UserPreferences &prefs);
        ~ColorSettingsWindow();

        /**
         * Render color settings panel
         */
        void render();

        /**
         * Reset to default colors
         */
        void reset_to_defaults();

    private:
        UserPreferences &user_prefs;
    };

    /**
     * Data structure for tracking road metadata
     */
    struct RoadIntersection
    {
        int road_id;                   // ID of intersecting road
        CityModel::RoadType road_type; // Type of intersecting road
        float x;                       // X coordinate of intersection
        float y;                       // Y coordinate of intersection
    };

    /**
     * Road type classification
     */
    enum class RoadCategory
    {
        Generated, // Auto-generated road (can be cleared)
        User       // User-owned/locked road (preserved across regen/clear)
    };

    struct RoadIndexEntry
    {
        int id;                                      // Unique road ID
        int axiom_origin_id;                         // ID of axiom where road originates
        int axiom_end_id;                            // ID of axiom where road ends (-1 if none)
        std::vector<int> influenced_by_axioms;       // List of axiom IDs that influence this road
        std::vector<RoadIntersection> intersections; // Roads this one intersects with
        CityModel::RoadType road_type;               // Full road type
        RoadCategory category;                       // Generated vs User-created
        bool locked = false;                         // True if protected from regen/clear
        bool from_generated = false;                 // True if converted from generated road
        bool intersections_computed = false;         // Intersection list is valid
        std::vector<ImVec2> nodes;                   // Editable polyline nodes (world coords)
        // User road control points (for M_Major/M_Minor)
        float start_x, start_y; // Start control point
        float end_x, end_y;     // End control point
    };

    /**
     * Road index window: tracks generated roads per axiom
     */
    class RoadIndexWindow
    {
    public:
        RoadIndexWindow(UserPreferences &prefs);
        ~RoadIndexWindow();

        /**
         * Render road index panel
         */
        void render(ViewportWindow &viewport, CityParams &params, std::vector<RoadIndexEntry> &roads, uint32_t max_total_roads, uint32_t current_road_count);

        /**
         * Get selected road ID
         */
        int get_selected_road_id() const { return selected_road_id; }

        /**
         * Check for user road creation requests
         */
        bool add_major_clicked() const { return add_major_button_clicked; }
        bool add_minor_clicked() const { return add_minor_button_clicked; }
        bool remove_road_clicked() const { return remove_road_button_clicked; }
        int get_road_to_remove() const { return road_to_remove_id; }
        bool lock_road_clicked() const { return lock_road_button_clicked; }
        int get_road_to_lock() const { return road_to_lock_id; }
        bool unlock_road_clicked() const { return unlock_road_button_clicked; }
        int get_road_to_unlock() const { return road_to_unlock_id; }

    private:
        UserPreferences &user_prefs;
        int selected_road_id;
        int hovered_road_id;
        std::unordered_set<int> selected_road_ids;
        int active_road_id;
        bool add_major_button_clicked;
        bool add_minor_button_clicked;
        bool remove_road_button_clicked;
        int road_to_remove_id;
        bool lock_road_button_clicked;
        int road_to_lock_id;
        bool unlock_road_button_clicked;
        int road_to_unlock_id;
    };

    /**
     * Lot index window: tracks lots per district
     */
    class LotIndexWindow
    {
    public:
        LotIndexWindow(UserPreferences &prefs);
        ~LotIndexWindow();

        void render(ViewportWindow &viewport, const CityModel::City &city, uint32_t max_lots);

        int get_selected_lot_id() const { return selected_lot_id; }

    private:
        UserPreferences &user_prefs;
        int selected_lot_id;
        int hovered_lot_id;
        std::unordered_set<int> selected_lot_ids;
        int active_lot_id;
        int district_filter_index;
        int lot_filter_index;
        bool color_by_district;
    };

    /**
     * Building index window: placeholder for future building tokens
     */
    class BuildingIndexWindow
    {
    public:
        BuildingIndexWindow(UserPreferences &prefs);
        ~BuildingIndexWindow();

        void render(const CityModel::City &city, uint32_t max_building_sites);

    private:
        UserPreferences &user_prefs;
        int selected_building_id;
    };

} // namespace RCG
