#pragma once

#include <algorithm>
#include <imgui.h>
#include <GL/glew.h>
#include <string>
#include <vector>
#include <cstdint>
#include <array>
#include <unordered_set>
#include <unordered_map>
#include "UserPreferences.hpp"
#include "CityModel.h"

namespace DistrictGenerator
{
    struct DistrictField;
}

namespace RCG
{

    /**
     * Manages the main city viewport window
     * Handles rendering, pan/zoom, mouse interaction
     */
    class ViewportWindow
    {
    public:
        struct RoadOverlay
        {
            int id;
            CityModel::RoadType type;
            std::vector<ImVec2> points;
            bool from_generated;
        };

        struct RoadEditEvent
        {
            int road_id{-1};
            CityModel::RoadType type{CityModel::RoadType::Street};
            std::vector<ImVec2> points;
            bool from_user{false};
        };
        struct AxiomData
        {
            int id;
            ImVec2 position;
            int type;
            float size;
            float opacity;
            int terminal_type; // For Delta fields: 0=Top, 1=BottomLeft, 2=BottomRight
            int mode;          // For Radial/Block modes
            std::string name;
            int tag_id; // Random tag for UI labels
            // Per-axiom road count settings
            int major_road_count; // Exact major roads when fuzzy_mode=false
            int minor_road_count; // Exact minor roads when fuzzy_mode=false
            bool fuzzy_mode;      // If true, use min_roads/max_roads range
            int min_roads;        // Minimum total roads when fuzzy
            int max_roads;        // Maximum total roads when fuzzy
            // District influencer type (landmark seeding)
            int influencer_type{0}; // 0=None, 1=Market, 2=Keep, 3=Temple, 4=Harbor, 5=Park, 6=Gate, 7=Well
        };

        // User-placed lot for manual placement
        struct UserLotData
        {
            int id;
            ImVec2 position;
            CityModel::LotType lot_type{CityModel::LotType::Residential};
            bool locked_type{false}; // If true, generator won't override type
        };

        // User-placed building for manual placement
        struct UserBuildingData
        {
            int id;
            ImVec2 position;
            CityModel::BuildingType building_type{CityModel::BuildingType::Residential};
            bool locked_type{false}; // If true, generator won't override type
        };

        struct RiverData
        {
            int id;
            std::vector<ImVec2> control_points;
            std::vector<ImVec2> spline;
            int type;
            float width;
            float opacity;
            std::string name;
            bool dirty;
        };
        struct Vec3
        {
            float x;
            float y;
            float z;
        };

        ViewportWindow(UserPreferences &prefs);
        ~ViewportWindow();

        /**
         * Render the viewport window and handle input
         * Returns true if viewport was interacted with
         */
        bool render();

        /**
         * Render OpenGL scene for the viewport
         */
        void render_gl();

        /**
         * Update viewport with new city texture
         */
        void update_texture(GLuint texture_id, int width, int height);

        /**
         * Get mouse position projected onto the ground plane (Y=0)
         */
        ImVec2 get_mouse_world_pos() const;

        /**
         * Pan and zoom state
         */
        ImVec2 get_pan() const { return ImVec2(camera_target.x, camera_target.z); }
        float get_zoom() const { return camera_distance; }

        void set_pan(ImVec2 new_pan)
        {
            pan = new_pan;
            camera_target.x = new_pan.x;
            camera_target.z = new_pan.y;
        }
        void set_zoom(float new_zoom)
        {
            camera_distance = std::max(1.0f, new_zoom);
            zoom = camera_distance;
        }

        /**
         * View gizmo helpers
         */
        void center_camera();
        void set_top_ortho_view();
        void set_perspective_view();

        /**
         * Access current axioms for generator integration
         */
        const std::vector<AxiomData> &get_axioms() const;
        size_t get_axiom_revision() const { return axiom_revision; }
        const std::vector<RiverData> &get_rivers() const { return rivers; }
        size_t get_river_revision() const { return river_revision; }
        void rebuild_rivers();

        void set_axioms(const std::vector<AxiomData> &new_axioms, int selected_id = -1);
        void set_rivers(const std::vector<RiverData> &new_rivers, int selected_id = -1);

        /**
         * Access user-placed lots and buildings
         */
        const std::vector<UserLotData> &get_user_lots() const { return user_lots; }
        const std::vector<UserBuildingData> &get_user_buildings() const { return user_buildings; }
        size_t get_user_lot_revision() const { return user_lot_revision; }
        size_t get_user_road_revision() const { return user_road_revision; }
        size_t get_user_building_revision() const { return user_building_revision; }
        void set_user_lots(const std::vector<UserLotData> &lots);
        void set_user_buildings(const std::vector<UserBuildingData> &buildings);
        void clear_user_lots();
        void clear_user_buildings();
        int add_user_lot(ImVec2 position, CityModel::LotType lot_type, bool locked_type);
        int add_user_building(ImVec2 position, CityModel::BuildingType building_type, bool locked_type);
        bool remove_user_lot(int id);
        bool remove_user_building(int id);
        bool set_user_lot_type(int id, CityModel::LotType lot_type);
        bool set_user_building_type(int id, CityModel::BuildingType building_type);
        bool set_user_lot_locked(int id, bool locked);
        bool set_user_building_locked(int id, bool locked);
        void request_regeneration();
        bool consume_regeneration_request();

        /**
         * Clear any placed axioms
         */
        void clear_axioms();
        void clear_rivers();

        /**
         * Handle clear requests from the viewport UI
         */
        bool consume_clear_city_request();
        bool consume_clear_axioms_request();

        /**
         * Update generated city data for rendering
         */
        void set_city(const CityModel::City &city, uint32_t max_major_roads, uint32_t max_total_roads);
        void set_district_field(const DistrictGenerator::DistrictField &field);
        void clear_city();
        void set_user_roads(const std::vector<RoadOverlay> &roads);
        void clear_user_roads();
        bool consume_road_edit(RoadEditEvent &out);

        /**
         * Reset view to fit entire city
         */
        void reset_view();

        /**
         * Get viewport canvas information
         */
        struct ViewportInfo
        {
            ImVec2 canvas_pos;  // Screen position of top-left
            ImVec2 canvas_size; // Dimensions of viewport
            ImVec2 world_size;  // Size in world coordinates
        };

        ViewportInfo get_info() const { return viewport_info; }

        /**
         * Check if viewport is hovered by mouse
         */
        bool is_hovered() const { return hovered; }
        int get_selected_axiom_id() const { return selected_axiom_id; }
        void set_selected_axiom_id(int id) { selected_axiom_id = id; }
        int get_hovered_axiom_id() const { return hovered_axiom_id; }
        void set_hovered_axiom_id(int id) { hovered_axiom_id = id; }
        const std::unordered_set<int> &get_selected_axiom_ids() const { return selected_axiom_ids; }
        void set_selected_axiom_ids(const std::unordered_set<int> &ids) { selected_axiom_ids = ids; }
        void add_selected_axiom_id(int id) { selected_axiom_ids.insert(id); }
        void toggle_selected_axiom_id(int id)
        {
            if (selected_axiom_ids.count(id))
                selected_axiom_ids.erase(id);
            else
                selected_axiom_ids.insert(id);
        }
        void clear_selected_axiom_ids() { selected_axiom_ids.clear(); }
        bool is_axiom_selected(int id) const { return selected_axiom_ids.count(id) > 0; }
        int get_active_axiom_id() const { return active_axiom_id; }
        void set_active_axiom_id(int id) { active_axiom_id = id; }
        bool set_axiom_position(int id, ImVec2 position);
        bool set_axiom_name(int id, const std::string &name);
        bool set_axiom_size(int id, float size);
        bool set_axiom_opacity(int id, float opacity);
        bool set_axiom_terminal(int id, int terminal_type);
        bool set_axiom_mode(int id, int mode);
        bool set_axiom_major_road_count(int id, int count);
        bool set_axiom_minor_road_count(int id, int count);
        bool set_axiom_fuzzy_mode(int id, bool fuzzy);
        bool set_axiom_road_range(int id, int min_roads, int max_roads);
        int get_selected_river_id() const { return selected_river_id; }
        void set_selected_river_id(int id) { selected_river_id = id; }
        int get_selected_road_id() const { return selected_road_id; }
        void set_selected_road_id(int id) { selected_road_id = id; }
        int get_hovered_road_id() const { return hovered_road_id; }
        void set_hovered_road_id(int id) { hovered_road_id = id; }
        const std::unordered_set<int> &get_selected_road_ids() const { return selected_road_ids; }
        void set_selected_road_ids(const std::unordered_set<int> &ids) { selected_road_ids = ids; }
        void add_selected_road_id(int id) { selected_road_ids.insert(id); }
        void toggle_selected_road_id(int id)
        {
            if (selected_road_ids.count(id))
                selected_road_ids.erase(id);
            else
                selected_road_ids.insert(id);
        }
        void clear_selected_road_ids() { selected_road_ids.clear(); }
        bool is_road_selected(int id) const { return selected_road_ids.count(id) > 0; }
        int get_active_road_id() const { return active_road_id; }
        void set_active_road_id(int id) { active_road_id = id; }

        int get_selected_lot_id() const { return selected_lot_id; }
        void set_selected_lot_id(int id) { selected_lot_id = id; }
        int get_hovered_lot_id() const { return hovered_lot_id; }
        void set_hovered_lot_id(int id) { hovered_lot_id = id; }
        const std::unordered_set<int> &get_selected_lot_ids() const { return selected_lot_ids; }
        void set_selected_lot_ids(const std::unordered_set<int> &ids) { selected_lot_ids = ids; }
        void add_selected_lot_id(int id) { selected_lot_ids.insert(id); }
        void toggle_selected_lot_id(int id)
        {
            if (selected_lot_ids.count(id))
                selected_lot_ids.erase(id);
            else
                selected_lot_ids.insert(id);
        }
        void clear_selected_lot_ids() { selected_lot_ids.clear(); }
        bool is_lot_selected(int id) const { return selected_lot_ids.count(id) > 0; }
        int get_active_lot_id() const { return active_lot_id; }
        void set_active_lot_id(int id) { active_lot_id = id; }

        int get_selected_district_id() const { return selected_district_id; }
        void set_selected_district_id(int id) { selected_district_id = id; }
        int get_hovered_district_id() const { return hovered_district_id; }
        void set_hovered_district_id(int id) { hovered_district_id = id; }
        const std::unordered_set<int> &get_selected_district_ids() const { return selected_district_ids; }
        void set_selected_district_ids(const std::unordered_set<int> &ids) { selected_district_ids = ids; }
        void add_selected_district_id(int id) { selected_district_ids.insert(id); }
        void toggle_selected_district_id(int id)
        {
            if (selected_district_ids.count(id))
                selected_district_ids.erase(id);
            else
                selected_district_ids.insert(id);
        }
        void clear_selected_district_ids() { selected_district_ids.clear(); }
        bool is_district_selected(int id) const { return selected_district_ids.count(id) > 0; }
        int get_active_district_id() const { return active_district_id; }
        void set_active_district_id(int id) { active_district_id = id; }
        bool set_river_point(int id, size_t point_index, ImVec2 position);
        bool insert_river_point(int id, size_t index, ImVec2 position);
        bool remove_river_point(int id, size_t index);
        bool set_river_name(int id, const std::string &name);
        bool set_river_width(int id, float width);
        bool set_river_opacity(int id, float opacity);

        // Queue a road edit event (used by index panel for add/remove nodes/type changes)
        void submit_road_edit(int road_id, CityModel::RoadType type, const std::vector<ImVec2> &points, bool from_user);

    private:
        enum class NavigationMode
        {
            Orbit,
            Pan
        };

        enum class AxiomType
        {
            BlueCircle = 0,
            GreenTriangle = 1,
            BlockSquare = 2,
            GridCorrective = 3
        };

        ImVec2 pan;
        float zoom;
        Vec3 camera_target;
        float camera_distance;
        float camera_yaw;
        float camera_pitch;
        float fov_degrees;
        bool ortho_enabled;
        NavigationMode nav_mode;
        std::vector<AxiomData> axioms;
        int next_axiom_id;
        int selected_axiom_id;
        int hovered_axiom_id;
        std::unordered_set<int> selected_axiom_ids;
        int active_axiom_id;
        bool dragging_axiom;
        int dragging_axiom_index;
        size_t axiom_revision;
        std::vector<RiverData> rivers;
        int next_river_id;
        int selected_river_id;
        int selected_river_control_index;
        bool dragging_river;
        int dragging_river_index;
        ImVec2 dragging_river_offset;
        size_t river_revision;
        // User-placed lots and buildings
        std::vector<UserLotData> user_lots;
        std::vector<UserBuildingData> user_buildings;
        int next_user_lot_id{1};
        int next_user_building_id{1};
        size_t user_lot_revision{0};
        size_t user_building_revision{0};
        size_t user_road_revision{0};
        bool regeneration_requested{false};
        int selected_road_id;
        int hovered_road_id;
        std::unordered_set<int> selected_road_ids;
        int active_road_id;
        int selected_lot_id;
        int hovered_lot_id;
        std::unordered_set<int> selected_lot_ids;
        int active_lot_id;
        int selected_district_id;
        int hovered_district_id;
        std::unordered_set<int> selected_district_ids;
        int active_district_id;
        int last_grid_bounds;
        float last_grid_spacing;
        std::vector<std::vector<ImVec2>> city_water;
        std::array<std::vector<std::vector<ImVec2>>, CityModel::road_type_count> city_roads_by_type;
        struct DistrictOverlay
        {
            uint32_t id;
            CityModel::DistrictType type;
            std::vector<ImVec2> border;
        };
        struct LotOverlay
        {
            uint32_t id;
            uint32_t district_id;
            CityModel::LotType lot_type;
            ImVec2 centroid;
        };
        struct BuildingSiteOverlay
        {
            uint32_t id;
            uint32_t lot_id;
            uint32_t district_id;
            CityModel::BuildingType type;
            ImVec2 position;
        };
        struct BlockPolygonOverlay
        {
            std::vector<ImVec2> outer;
            std::vector<std::vector<ImVec2>> holes;
            CityModel::DistrictType type{CityModel::DistrictType::Mixed};
            bool closable{true};  // For debug visualization
        };
        
        struct DistrictGridInfo
        {
            int width{0};
            int height{0};
            ImVec2 origin{};
            ImVec2 cell_size{};
            std::vector<uint32_t> district_ids;
        };
        
        std::vector<DistrictOverlay> city_districts;
        std::vector<LotOverlay> city_lots;
        std::vector<BuildingSiteOverlay> city_building_sites;
        std::vector<BlockPolygonOverlay> city_block_polygons;
        std::vector<BlockPolygonOverlay> city_block_faces;
        DistrictGridInfo district_grid_info;  // For debug overlay
        std::vector<std::vector<ImVec2>> all_roads_indexed; // Combined roads for indexed lookup
        struct RoadPolylineRef
        {
            std::size_t type_index;
            std::size_t local_index;
            CityModel::RoadType type;
        };
        struct BezierHandles
        {
            std::vector<ImVec2> anchors;
            std::vector<ImVec2> in_handles;
            std::vector<ImVec2> out_handles;
            std::vector<bool> broken;
            bool dirty{true};
        };
        std::vector<RoadPolylineRef> all_roads_refs;
        std::vector<RoadOverlay> user_roads;
        std::unordered_map<int, BezierHandles> user_road_handles;
        std::unordered_map<int, std::vector<ImVec2>> user_road_display_cache;
        std::unordered_set<int> hidden_generated_road_ids;
        int selected_road_node_index;
        bool selected_road_is_user;
        int last_selected_road_id;
        bool dragging_road_node;
        int dragging_road_node_index;
        int dragging_road_id;
        bool dragging_road_is_user;
        bool dragging_road_handle;
        int dragging_road_handle_index;
        bool dragging_road_handle_in;
        int dragging_road_handle_road_id;
        bool road_edit_dirty;
        bool road_edit_pending;
        RoadEditEvent pending_road_edit;
        uint32_t city_major_road_count;
        uint32_t city_total_road_count;
        uint32_t city_max_major_roads;
        uint32_t city_max_total_roads;
        uint32_t city_total_lot_count;
        uint32_t city_max_lots;
        uint32_t city_total_building_sites;
        uint32_t city_max_building_sites;
        uint32_t block_road_inputs;
        uint32_t block_faces_found;
        uint32_t block_valid_blocks;
        uint32_t block_intersections;
        uint32_t block_segments;
        float city_half_extent;
        bool has_city;
        bool clear_city_requested;
        bool clear_axioms_requested;
        std::vector<ImVec2> polyline_scratch;
        bool cached_matrices_valid;
        float cached_view[16];
        float cached_proj[16];
        GLuint texture_id;
        int texture_width, texture_height;
        ViewportInfo viewport_info;
        bool hovered;
        UserPreferences &user_prefs;
        GLuint grid_vao;
        GLuint grid_vbo;
        GLuint grid_program;
        int grid_vertex_count;
        int grid_line_offset;
        int grid_line_count;
        bool grid_initialized;
        bool last_light_theme;
        GLuint fbo;
        GLuint color_texture;
        GLuint depth_buffer;
        int framebuffer_width;
        int framebuffer_height;

        ImVec2 screen_to_world(ImVec2 screen_pos) const;
        ImVec2 world_to_screen(ImVec2 world_pos) const;
        Vec3 get_camera_position() const;
        void get_camera_basis(Vec3 &forward, Vec3 &right, Vec3 &up) const;

        void handle_pan_zoom();
        void init_grid_resources();
        void destroy_grid_resources();
        void ensure_framebuffer_size(int width, int height);
        void rebuild_hidden_generated_roads();
        void ensure_user_road_handles(int road_id, const std::vector<ImVec2> &anchors);
        const std::vector<ImVec2> &get_user_road_display_points(int road_id, const std::vector<ImVec2> &anchors);
        BezierHandles *get_user_road_handles(int road_id);
    };

} // namespace RCG
