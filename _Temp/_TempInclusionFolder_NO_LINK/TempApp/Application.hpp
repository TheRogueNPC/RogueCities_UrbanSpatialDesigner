#pragma once

#include <memory>
#include <cstdint>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "UserPreferences.hpp"
#include "UIManager.hpp"
#include "DockManager.hpp"
#include "ViewportWindow.hpp"
#include "Windows.hpp"
#include "CityParams.hpp"
#include "CityModel.h"
#include "CityParams.h"

namespace RCG
{

    /**
     * Main application controller
     * Coordinates UI, viewport, and generation
     */
    class Application
    {
    public:
        Application();
        ~Application();

        /**
         * Initialize the application
         * Returns true on success
         */
        bool initialize();

        /**
         * Main event loop
         * Returns false when application should close
         */
        bool update();

        /**
         * Render current frame
         */
        void render();

        /**
         * Check if window should close
         */
        bool should_close() const;

    private:
        struct EditorState
        {
            CityParams params;
            std::vector<ViewportWindow::AxiomData> axioms;
            std::vector<ViewportWindow::RiverData> rivers;
            int selected_axiom_id = -1;
            int selected_river_id = -1;
        };

        UserPreferences user_preferences;
        CityParams city_params;
        CityModel::City generated_city;
        bool has_generated_city;

        std::unique_ptr<UIManager> ui_manager;
        DockManager dock_manager;
        std::unique_ptr<ToolsWindow> tools_window;
        std::unique_ptr<ParametersWindow> parameters_window;
        std::unique_ptr<ExportWindow> export_window;
        std::unique_ptr<ViewOptionsWindow> view_options_window;
        std::unique_ptr<RoadsWindow> roads_window;
        std::unique_ptr<RiversWindow> rivers_window;
        std::unique_ptr<DistrictIndexWindow> district_index_window;
        std::unique_ptr<RiverIndexWindow> river_index_window;
        std::unique_ptr<EventLogWindow> event_log_window;
        std::unique_ptr<RoadIndexWindow> road_index_window;
        std::unique_ptr<LotIndexWindow> lot_index_window;
        std::unique_ptr<BuildingIndexWindow> building_index_window;
        std::unique_ptr<PresetsWindow> presets_window;
        std::unique_ptr<ProgressWindow> progress_window;
        std::unique_ptr<ColorSettingsWindow> color_settings_window;
        std::unique_ptr<ViewportWindow> viewport_window;

        // Road index tracking
        std::vector<RoadIndexEntry> road_index;

        // User-created roads (M_Major/M_Minor) - persist across Clear Roads
        std::vector<RoadIndexEntry> user_roads;
        int next_user_road_id = 10000; // Start high to avoid collision with generated IDs

        // GLFW/OpenGL
        GLFWwindow *window;
        size_t last_axiom_count;
        size_t last_axiom_revision;
        size_t last_generated_axiom_revision;
        uint64_t last_city_params_hash;
        size_t last_user_lot_revision;
        size_t last_user_road_revision;
        bool show_exit_prompt;
        bool exit_confirmed;
        std::vector<EditorState> undo_stack;
        std::vector<EditorState> redo_stack;
        EditorState last_state;
        bool history_initialized = false;
        bool applying_history = false;
        static constexpr size_t kHistoryLimit = 32;

        void autosave_state();
        bool save_snapshot(const std::string &reason);
        void cleanup_temp_state();
        std::string get_temp_dir() const;
        std::string get_snapshot_dir() const;

        /**
         * Handle user input from all windows
         */
        void handle_input();

        /**
         * Update city generation if parameters changed
         */
        void update_city_generation();
        void record_generation_state();

        /**
         * Export city in requested format
         */
        void handle_export();

        void export_json();
        void export_obj();
        void export_svg();

        /**
         * User road management
         */
        void add_user_road(int axiom_id, bool is_major);
        void remove_user_road(int road_id);
        void merge_user_roads_to_index();
        void sync_user_roads_to_viewport();
        void register_docks();

        /**
         * Cleanup resources
         */
        void shutdown();

        bool show_preferences;

        EditorState capture_state() const;
        void apply_state(const EditorState &state);
        void push_undo_state(const EditorState &state);
        void update_history();
        bool can_undo() const;
        bool can_redo() const;
        void undo();
        void redo();
        static bool states_equal(const EditorState &a, const EditorState &b);
    };

} // namespace RCG
