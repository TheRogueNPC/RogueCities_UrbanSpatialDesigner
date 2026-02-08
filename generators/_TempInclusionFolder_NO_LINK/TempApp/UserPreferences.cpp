#include "UserPreferences.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace RCG
{

    UserPreferences::UserPreferences()
    {
        load_from_file();
        load_from_ini();
    }

    UserPreferences::~UserPreferences()
    {
        save_to_file();
        save_to_ini();
    }

    std::string UserPreferences::get_config_dir()
    {
        std::string config_dir;
#ifdef _WIN32
        char *appdata = nullptr;
        size_t sz = 0;
        if (_dupenv_s(&appdata, &sz, "APPDATA") == 0 && appdata != nullptr)
        {
            config_dir = std::string(appdata) + "/RogueCityVisualizer";
            free(appdata);
        }
        else
        {
            config_dir = "RogueCityVisualizer";
        }
#else
        const char *home = std::getenv("HOME");
        if (home)
        {
            config_dir = std::string(home) + "/.config/RogueCityVisualizer";
        }
        else
        {
            config_dir = ".config/RogueCityVisualizer";
        }
#endif
        fs::create_directories(config_dir);
        return config_dir;
    }

    std::string UserPreferences::get_preferences_path()
    {
        return get_config_dir() + "/preferences.json";
    }

    std::string UserPreferences::get_layout_path()
    {
        return get_config_dir() + "/layout.json";
    }

    std::string UserPreferences::get_ini_path()
    {
        return get_config_dir() + "/preferences.ini";
    }

    void UserPreferences::load_from_file(const std::string &filepath)
    {
        std::string full_path = filepath == "user_prefs.json" ? get_preferences_path() : filepath;

        std::ifstream file(full_path);
        if (!file.is_open())
        {
            // Use defaults if file doesn't exist
            return;
        }

        try
        {
            json j;
            file >> j;
            load_json(j.dump());
        }
        catch (const std::exception &)
        {
            // JSON parsing failed, use defaults
            return;
        }
    }

    void UserPreferences::save_to_file(const std::string &filepath)
    {
        std::string full_path = filepath == "user_prefs.json" ? get_preferences_path() : filepath;

        try
        {
            std::string json_str = save_json();
            std::ofstream file(full_path);
            if (file.is_open())
            {
                file << json_str;
                file.close();
            }
        }
        catch (const std::exception &)
        {
            // Silently fail on save
        }
    }

    static std::string trim_line(const std::string &value)
    {
        size_t start = value.find_first_not_of(" \t\r\n");
        if (start == std::string::npos)
        {
            return "";
        }
        size_t end = value.find_last_not_of(" \t\r\n");
        return value.substr(start, end - start + 1);
    }

    void UserPreferences::load_from_ini(const std::string &filepath)
    {
        std::string full_path = filepath == "user_prefs.ini" ? get_ini_path() : filepath;

        std::ifstream file(full_path);
        if (!file.is_open())
        {
            return;
        }

        std::string line;
        while (std::getline(file, line))
        {
            line = trim_line(line);
            if (line.empty())
            {
                continue;
            }
            if (line[0] == '#' || line[0] == ';')
            {
                continue;
            }
            size_t eq = line.find('=');
            if (eq == std::string::npos)
            {
                continue;
            }
            std::string key = trim_line(line.substr(0, eq));
            std::string value = trim_line(line.substr(eq + 1));

            if (key == "ui_theme")
            {
                app_prefs.use_light_theme = (value == "light" || value == "Light");
            }
            else if (key == "viewport_brightness")
            {
                try
                {
                    app_prefs.viewport_brightness = std::stof(value);
                    if (app_prefs.viewport_brightness < 0.3f)
                        app_prefs.viewport_brightness = 0.3f;
                    if (app_prefs.viewport_brightness > 2.5f)
                        app_prefs.viewport_brightness = 2.5f;
                }
                catch (...)
                {
                }
            }
        }
    }

    void UserPreferences::save_to_ini(const std::string &filepath)
    {
        std::string full_path = filepath == "user_prefs.ini" ? get_ini_path() : filepath;

        try
        {
            std::ofstream file(full_path);
            if (!file.is_open())
            {
                return;
            }
            file << "ui_theme=" << (app_prefs.use_light_theme ? "light" : "dark") << "\n";
            file << "viewport_brightness=" << app_prefs.viewport_brightness << "\n";
        }
        catch (...)
        {
        }
    }

    bool UserPreferences::load_json(const std::string &json_string)
    {
        try
        {
            json j = json::parse(json_string);

            // Window preferences
            if (j.contains("windows"))
            {
                auto w = j["windows"];
                if (w.contains("city_params_width"))
                    window_prefs.city_params_width = w["city_params_width"];
                if (w.contains("viewport_height"))
                    window_prefs.viewport_height = w["viewport_height"];
                if (w.contains("city_params_visible"))
                    window_prefs.city_params_visible = w["city_params_visible"];
                if (w.contains("viewport_visible"))
                    window_prefs.viewport_visible = w["viewport_visible"];
                if (w.contains("tools_visible"))
                    window_prefs.tools_visible = w["tools_visible"];
                if (w.contains("export_visible"))
                    window_prefs.export_visible = w["export_visible"];
                if (w.contains("parameters_visible"))
                    window_prefs.parameters_visible = w["parameters_visible"];
                if (w.contains("view_options_visible"))
                    window_prefs.view_options_visible = w["view_options_visible"];
                if (w.contains("district_visible"))
                    window_prefs.district_visible = w["district_visible"];
                if (w.contains("roads_visible"))
                    window_prefs.roads_visible = w["roads_visible"];
                if (w.contains("rivers_visible"))
                    window_prefs.rivers_visible = w["rivers_visible"];
                if (w.contains("river_index_visible"))
                    window_prefs.river_index_visible = w["river_index_visible"];
                if (w.contains("road_index_visible"))
                    window_prefs.road_index_visible = w["road_index_visible"];
                if (w.contains("lot_index_visible"))
                    window_prefs.lot_index_visible = w["lot_index_visible"];
                if (w.contains("building_index_visible"))
                    window_prefs.building_index_visible = w["building_index_visible"];
                if (w.contains("event_log_visible"))
                    window_prefs.event_log_visible = w["event_log_visible"];
                if (w.contains("presets_visible"))
                    window_prefs.presets_visible = w["presets_visible"];
                if (w.contains("progress_visible"))
                    window_prefs.progress_visible = w["progress_visible"];
                if (w.contains("color_settings_visible"))
                    window_prefs.color_settings_visible = w["color_settings_visible"];
                if (w.contains("tools_locked"))
                    window_prefs.tools_locked = w["tools_locked"];
                if (w.contains("parameters_locked"))
                    window_prefs.parameters_locked = w["parameters_locked"];
                if (w.contains("view_options_locked"))
                    window_prefs.view_options_locked = w["view_options_locked"];
                if (w.contains("district_locked"))
                    window_prefs.district_locked = w["district_locked"];
                if (w.contains("roads_locked"))
                    window_prefs.roads_locked = w["roads_locked"];
                if (w.contains("rivers_locked"))
                    window_prefs.rivers_locked = w["rivers_locked"];
                if (w.contains("river_index_locked"))
                    window_prefs.river_index_locked = w["river_index_locked"];
                if (w.contains("road_index_locked"))
                    window_prefs.road_index_locked = w["road_index_locked"];
                if (w.contains("lot_index_locked"))
                    window_prefs.lot_index_locked = w["lot_index_locked"];
                if (w.contains("building_index_locked"))
                    window_prefs.building_index_locked = w["building_index_locked"];
                if (w.contains("export_locked"))
                    window_prefs.export_locked = w["export_locked"];
                if (w.contains("event_log_locked"))
                    window_prefs.event_log_locked = w["event_log_locked"];
                if (w.contains("presets_locked"))
                    window_prefs.presets_locked = w["presets_locked"];
                if (w.contains("progress_locked"))
                    window_prefs.progress_locked = w["progress_locked"];
                if (w.contains("color_settings_locked"))
                    window_prefs.color_settings_locked = w["color_settings_locked"];
            }
            window_prefs.viewport_visible = true;

            // Tool preferences
            if (j.contains("tools"))
            {
                auto t = j["tools"];
                if (t.contains("selected_growth_rule"))
                    tool_prefs.selected_growth_rule = t["selected_growth_rule"];
                if (t.contains("selected_tool"))
                    tool_prefs.selected_tool = t["selected_tool"];
                if (t.contains("selected_axiom_type"))
                    tool_prefs.selected_axiom_type = t["selected_axiom_type"];
                if (t.contains("selected_delta_terminal"))
                    tool_prefs.selected_delta_terminal = t["selected_delta_terminal"];
                if (t.contains("selected_radial_mode"))
                    tool_prefs.selected_radial_mode = t["selected_radial_mode"];
                if (t.contains("selected_block_mode"))
                    tool_prefs.selected_block_mode = t["selected_block_mode"];
                if (t.contains("axiom_size"))
                    tool_prefs.axiom_size = t["axiom_size"];
                if (t.contains("axiom_opacity"))
                    tool_prefs.axiom_opacity = t["axiom_opacity"];
                if (t.contains("selected_river_type"))
                    tool_prefs.selected_river_type = t["selected_river_type"];
                if (t.contains("river_width"))
                    tool_prefs.river_width = t["river_width"];
                if (t.contains("river_opacity"))
                    tool_prefs.river_opacity = t["river_opacity"];
                if (t.contains("brush_size"))
                    tool_prefs.brush_size = t["brush_size"];
                if (t.contains("brush_opacity"))
                    tool_prefs.brush_opacity = t["brush_opacity"];
                if (t.contains("grid_snap"))
                    tool_prefs.grid_snap = t["grid_snap"];
                if (t.contains("grid_size"))
                    tool_prefs.grid_size = t["grid_size"];

                tool_prefs.selected_growth_rule = std::clamp(tool_prefs.selected_growth_rule, 0, 2);
                tool_prefs.selected_tool = std::clamp(tool_prefs.selected_tool, 0, 7);
                tool_prefs.selected_axiom_type = std::clamp(tool_prefs.selected_axiom_type, 0, 3);
                tool_prefs.selected_delta_terminal = std::clamp(tool_prefs.selected_delta_terminal, 0, 2);
                tool_prefs.selected_radial_mode = std::clamp(tool_prefs.selected_radial_mode, 0, 2);
                tool_prefs.selected_block_mode = std::clamp(tool_prefs.selected_block_mode, 0, 2);
                tool_prefs.axiom_opacity = std::clamp(tool_prefs.axiom_opacity, 0.05f, 1.0f);
                tool_prefs.selected_river_type = std::clamp(tool_prefs.selected_river_type, 0, 3);
                tool_prefs.river_width = std::max(1.0f, tool_prefs.river_width);
                tool_prefs.river_opacity = std::clamp(tool_prefs.river_opacity, 0.05f, 1.0f);
            }

            // App preferences
            if (j.contains("app"))
            {
                auto a = j["app"];
                if (a.contains("auto_regenerate"))
                    app_prefs.auto_regenerate = a["auto_regenerate"];
                if (a.contains("regeneration_delay_ms"))
                    app_prefs.regeneration_delay_ms = a["regeneration_delay_ms"];
                if (a.contains("show_grid"))
                    app_prefs.show_grid = a["show_grid"];
                if (a.contains("show_axioms"))
                    app_prefs.show_axioms = a["show_axioms"];
                if (a.contains("show_roads"))
                    app_prefs.show_roads = a["show_roads"];
                if (a.contains("show_axiom_influence"))
                    app_prefs.show_axiom_influence = a["show_axiom_influence"];
                if (a.contains("show_river_splines"))
                    app_prefs.show_river_splines = a["show_river_splines"];
                if (a.contains("show_river_markers"))
                    app_prefs.show_river_markers = a["show_river_markers"];
                if (a.contains("show_road_intersections"))
                    app_prefs.show_road_intersections = a["show_road_intersections"];
                if (a.contains("river_spline_color") && a["river_spline_color"].is_array())
                {
                    auto c = a["river_spline_color"];
                    if (c.size() >= 4)
                    {
                        app_prefs.river_spline_color.x = c[0];
                        app_prefs.river_spline_color.y = c[1];
                        app_prefs.river_spline_color.z = c[2];
                        app_prefs.river_spline_color.w = c[3];
                    }
                }
                if (a.contains("show_density_overlay"))
                    app_prefs.show_density_overlay = a["show_density_overlay"];
                if (a.contains("show_rules_overlay"))
                    app_prefs.show_rules_overlay = a["show_rules_overlay"];
                if (a.contains("enable_viewports"))
                    app_prefs.enable_viewports = a["enable_viewports"];
                if (a.contains("show_district_borders"))
                    app_prefs.show_district_borders = a["show_district_borders"];
                if (a.contains("show_block_polygons"))
                    app_prefs.show_block_polygons = a["show_block_polygons"];
                if (a.contains("show_block_faces_debug"))
                    app_prefs.show_block_faces_debug = a["show_block_faces_debug"];
                if (a.contains("show_block_closable_faces"))
                    app_prefs.show_block_closable_faces = a["show_block_closable_faces"];
                if (a.contains("show_district_grid_overlay"))
                    app_prefs.show_district_grid_overlay = a["show_district_grid_overlay"];
                if (a.contains("show_district_ids"))
                    app_prefs.show_district_ids = a["show_district_ids"];
                if (a.contains("enable_debug_logging"))
                    app_prefs.enable_debug_logging = a["enable_debug_logging"];
                if (a.contains("debug_use_segment_roads_for_blocks"))
                    app_prefs.debug_use_segment_roads_for_blocks = a["debug_use_segment_roads_for_blocks"];
                if (a.contains("district_grid_resolution"))
                    app_prefs.district_grid_resolution = a["district_grid_resolution"];
                if (a.contains("district_secondary_threshold"))
                    app_prefs.district_secondary_threshold = a["district_secondary_threshold"];
                if (a.contains("district_weight_scale"))
                    app_prefs.district_weight_scale = a["district_weight_scale"];
                if (a.contains("district_rd_mode"))
                    app_prefs.district_rd_mode = a["district_rd_mode"];
                if (a.contains("district_rd_mix"))
                    app_prefs.district_rd_mix = a["district_rd_mix"];
                if (a.contains("district_desire_weight_axiom"))
                    app_prefs.district_desire_weight_axiom = a["district_desire_weight_axiom"];
                if (a.contains("district_desire_weight_frontage"))
                    app_prefs.district_desire_weight_frontage = a["district_desire_weight_frontage"];
                if (a.contains("grid_bounds"))
                    app_prefs.grid_bounds = a["grid_bounds"];
                if (a.contains("enable_culling"))
                    app_prefs.enable_culling = a["enable_culling"];
                if (a.contains("cull_near"))
                    app_prefs.cull_near = a["cull_near"];
                if (a.contains("cull_far"))
                    app_prefs.cull_far = a["cull_far"];
                if (a.contains("ui_scale"))
                    app_prefs.ui_scale = a["ui_scale"];
                if (a.contains("use_light_theme"))
                    app_prefs.use_light_theme = a["use_light_theme"];
                if (a.contains("viewport_brightness"))
                    app_prefs.viewport_brightness = a["viewport_brightness"];
                if (a.contains("generation_mode"))
                    app_prefs.generation_mode = a["generation_mode"];
                if (a.contains("river_generation_mode"))
                    app_prefs.river_generation_mode = a["river_generation_mode"];
                if (a.contains("keymap"))
                {
                    auto km = a["keymap"];
                    auto read_binding = [](UserPreferences::AppPrefs::KeyBinding &binding, const nlohmann::json &src)
                    {
                        if (src.contains("key"))
                            binding.key = src["key"];
                        if (src.contains("ctrl"))
                            binding.ctrl = src["ctrl"];
                        if (src.contains("alt"))
                            binding.alt = src["alt"];
                        if (src.contains("shift"))
                            binding.shift = src["shift"];
                    };

                    if (km.contains("toggle_grid"))
                        read_binding(app_prefs.keymap.toggle_grid, km["toggle_grid"]);
                    if (km.contains("toggle_axioms"))
                        read_binding(app_prefs.keymap.toggle_axioms, km["toggle_axioms"]);
                    if (km.contains("toggle_roads"))
                        read_binding(app_prefs.keymap.toggle_roads, km["toggle_roads"]);
                    if (km.contains("toggle_river_splines"))
                        read_binding(app_prefs.keymap.toggle_river_splines, km["toggle_river_splines"]);
                    if (km.contains("toggle_road_intersections"))
                        read_binding(app_prefs.keymap.toggle_road_intersections, km["toggle_road_intersections"]);
                    if (km.contains("reset_view"))
                        read_binding(app_prefs.keymap.reset_view, km["reset_view"]);
                }
                if (a.contains("index_tabs"))
                {
                    auto tabs = a["index_tabs"];
                    auto read_tabs = [](UserPreferences::AppPrefs::IndexTabs &target, const nlohmann::json &src)
                    {
                        if (src.contains("list"))
                            target.show_list = src["list"];
                        if (src.contains("details"))
                            target.show_details = src["details"];
                        if (src.contains("settings"))
                            target.show_settings = src["settings"];
                        if (!target.show_list && !target.show_details && !target.show_settings)
                        {
                            target.show_list = true;
                        }
                    };

                    if (tabs.contains("district"))
                        read_tabs(app_prefs.district_tabs, tabs["district"]);
                    if (tabs.contains("road"))
                        read_tabs(app_prefs.road_tabs, tabs["road"]);
                    if (tabs.contains("lot"))
                        read_tabs(app_prefs.lot_tabs, tabs["lot"]);
                    if (tabs.contains("building"))
                        read_tabs(app_prefs.building_tabs, tabs["building"]);
                }
                app_prefs.generation_mode = std::clamp(app_prefs.generation_mode, 0, 1);
                app_prefs.river_generation_mode = std::clamp(app_prefs.river_generation_mode, 0, 1);
                app_prefs.district_grid_resolution = std::clamp(app_prefs.district_grid_resolution, 32, 512);
                app_prefs.district_secondary_threshold = std::clamp(app_prefs.district_secondary_threshold, 0.0f, 2.0f);
                app_prefs.district_weight_scale = std::clamp(app_prefs.district_weight_scale, 0.2f, 4.0f);
                app_prefs.district_rd_mix = std::clamp(app_prefs.district_rd_mix, 0.0f, 1.0f);
                app_prefs.district_desire_weight_axiom = std::clamp(app_prefs.district_desire_weight_axiom, 0.0f, 2.0f);
                app_prefs.district_desire_weight_frontage = std::clamp(app_prefs.district_desire_weight_frontage, 0.0f, 2.0f);
            }

            // Display preferences
            if (j.contains("display"))
            {
                auto d = j["display"];
                if (d.contains("vsync"))
                    display_prefs.vsync = d["vsync"];
                if (d.contains("high_dpi"))
                    display_prefs.high_dpi = d["high_dpi"];
            }

            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    std::string UserPreferences::save_json()
    {
        json j;

        // Window preferences
        j["windows"]["city_params_width"] = window_prefs.city_params_width;
        j["windows"]["viewport_height"] = window_prefs.viewport_height;
        j["windows"]["city_params_visible"] = window_prefs.city_params_visible;
        j["windows"]["viewport_visible"] = window_prefs.viewport_visible;
        j["windows"]["tools_visible"] = window_prefs.tools_visible;
        j["windows"]["export_visible"] = window_prefs.export_visible;
        j["windows"]["parameters_visible"] = window_prefs.parameters_visible;
        j["windows"]["view_options_visible"] = window_prefs.view_options_visible;
        j["windows"]["district_visible"] = window_prefs.district_visible;
        j["windows"]["roads_visible"] = window_prefs.roads_visible;
        j["windows"]["rivers_visible"] = window_prefs.rivers_visible;
        j["windows"]["river_index_visible"] = window_prefs.river_index_visible;
        j["windows"]["road_index_visible"] = window_prefs.road_index_visible;
        j["windows"]["lot_index_visible"] = window_prefs.lot_index_visible;
        j["windows"]["building_index_visible"] = window_prefs.building_index_visible;
        j["windows"]["event_log_visible"] = window_prefs.event_log_visible;
        j["windows"]["presets_visible"] = window_prefs.presets_visible;
        j["windows"]["progress_visible"] = window_prefs.progress_visible;
        j["windows"]["color_settings_visible"] = window_prefs.color_settings_visible;
        j["windows"]["tools_locked"] = window_prefs.tools_locked;
        j["windows"]["parameters_locked"] = window_prefs.parameters_locked;
        j["windows"]["view_options_locked"] = window_prefs.view_options_locked;
        j["windows"]["district_locked"] = window_prefs.district_locked;
        j["windows"]["roads_locked"] = window_prefs.roads_locked;
        j["windows"]["rivers_locked"] = window_prefs.rivers_locked;
        j["windows"]["river_index_locked"] = window_prefs.river_index_locked;
        j["windows"]["road_index_locked"] = window_prefs.road_index_locked;
        j["windows"]["lot_index_locked"] = window_prefs.lot_index_locked;
        j["windows"]["building_index_locked"] = window_prefs.building_index_locked;
        j["windows"]["export_locked"] = window_prefs.export_locked;
        j["windows"]["event_log_locked"] = window_prefs.event_log_locked;
        j["windows"]["presets_locked"] = window_prefs.presets_locked;
        j["windows"]["progress_locked"] = window_prefs.progress_locked;
        j["windows"]["color_settings_locked"] = window_prefs.color_settings_locked;

        // Tool preferences
        j["tools"]["selected_growth_rule"] = tool_prefs.selected_growth_rule;
        j["tools"]["selected_tool"] = tool_prefs.selected_tool;
        j["tools"]["selected_axiom_type"] = tool_prefs.selected_axiom_type;
        j["tools"]["selected_delta_terminal"] = tool_prefs.selected_delta_terminal;
        j["tools"]["selected_radial_mode"] = tool_prefs.selected_radial_mode;
        j["tools"]["selected_block_mode"] = tool_prefs.selected_block_mode;
        j["tools"]["axiom_size"] = tool_prefs.axiom_size;
        j["tools"]["axiom_opacity"] = tool_prefs.axiom_opacity;
        j["tools"]["selected_river_type"] = tool_prefs.selected_river_type;
        j["tools"]["river_width"] = tool_prefs.river_width;
        j["tools"]["river_opacity"] = tool_prefs.river_opacity;
        j["tools"]["brush_size"] = tool_prefs.brush_size;
        j["tools"]["brush_opacity"] = tool_prefs.brush_opacity;
        j["tools"]["grid_snap"] = tool_prefs.grid_snap;
        j["tools"]["grid_size"] = tool_prefs.grid_size;

        // App preferences
        j["app"]["auto_regenerate"] = app_prefs.auto_regenerate;
        j["app"]["regeneration_delay_ms"] = app_prefs.regeneration_delay_ms;
        j["app"]["show_grid"] = app_prefs.show_grid;
        j["app"]["show_axioms"] = app_prefs.show_axioms;
        j["app"]["show_roads"] = app_prefs.show_roads;
        j["app"]["show_axiom_influence"] = app_prefs.show_axiom_influence;
        j["app"]["show_river_splines"] = app_prefs.show_river_splines;
        j["app"]["show_river_markers"] = app_prefs.show_river_markers;
        j["app"]["show_road_intersections"] = app_prefs.show_road_intersections;
        j["app"]["river_spline_color"] = {app_prefs.river_spline_color.x,
                                          app_prefs.river_spline_color.y,
                                          app_prefs.river_spline_color.z,
                                          app_prefs.river_spline_color.w};
        j["app"]["show_density_overlay"] = app_prefs.show_density_overlay;
        j["app"]["show_rules_overlay"] = app_prefs.show_rules_overlay;
        j["app"]["enable_viewports"] = app_prefs.enable_viewports;
        j["app"]["show_district_borders"] = app_prefs.show_district_borders;
        j["app"]["show_block_polygons"] = app_prefs.show_block_polygons;
        j["app"]["show_block_faces_debug"] = app_prefs.show_block_faces_debug;
        j["app"]["show_block_closable_faces"] = app_prefs.show_block_closable_faces;
        j["app"]["show_district_grid_overlay"] = app_prefs.show_district_grid_overlay;
        j["app"]["show_district_ids"] = app_prefs.show_district_ids;
        j["app"]["enable_debug_logging"] = app_prefs.enable_debug_logging;
        j["app"]["debug_use_segment_roads_for_blocks"] = app_prefs.debug_use_segment_roads_for_blocks;
        j["app"]["district_grid_resolution"] = app_prefs.district_grid_resolution;
        j["app"]["district_secondary_threshold"] = app_prefs.district_secondary_threshold;
        j["app"]["district_weight_scale"] = app_prefs.district_weight_scale;
        j["app"]["district_rd_mode"] = app_prefs.district_rd_mode;
        j["app"]["district_rd_mix"] = app_prefs.district_rd_mix;
        j["app"]["district_desire_weight_axiom"] = app_prefs.district_desire_weight_axiom;
        j["app"]["district_desire_weight_frontage"] = app_prefs.district_desire_weight_frontage;
        j["app"]["grid_bounds"] = app_prefs.grid_bounds;
        j["app"]["enable_culling"] = app_prefs.enable_culling;
        j["app"]["cull_near"] = app_prefs.cull_near;
        j["app"]["cull_far"] = app_prefs.cull_far;
        j["app"]["ui_scale"] = app_prefs.ui_scale;
        j["app"]["use_light_theme"] = app_prefs.use_light_theme;
        j["app"]["viewport_brightness"] = app_prefs.viewport_brightness;
        j["app"]["generation_mode"] = app_prefs.generation_mode;
        j["app"]["river_generation_mode"] = app_prefs.river_generation_mode;
        auto write_binding = [](const UserPreferences::AppPrefs::KeyBinding &binding)
        {
            return nlohmann::json{{"key", binding.key},
                                  {"ctrl", binding.ctrl},
                                  {"alt", binding.alt},
                                  {"shift", binding.shift}};
        };
        j["app"]["keymap"]["toggle_grid"] = write_binding(app_prefs.keymap.toggle_grid);
        j["app"]["keymap"]["toggle_axioms"] = write_binding(app_prefs.keymap.toggle_axioms);
        j["app"]["keymap"]["toggle_roads"] = write_binding(app_prefs.keymap.toggle_roads);
        j["app"]["keymap"]["toggle_river_splines"] = write_binding(app_prefs.keymap.toggle_river_splines);
        j["app"]["keymap"]["toggle_road_intersections"] = write_binding(app_prefs.keymap.toggle_road_intersections);
        j["app"]["keymap"]["reset_view"] = write_binding(app_prefs.keymap.reset_view);
        j["app"]["index_tabs"]["district"] = {{"list", app_prefs.district_tabs.show_list},
                                              {"details", app_prefs.district_tabs.show_details},
                                              {"settings", app_prefs.district_tabs.show_settings}};
        j["app"]["index_tabs"]["road"] = {{"list", app_prefs.road_tabs.show_list},
                                          {"details", app_prefs.road_tabs.show_details},
                                          {"settings", app_prefs.road_tabs.show_settings}};
        j["app"]["index_tabs"]["lot"] = {{"list", app_prefs.lot_tabs.show_list},
                                         {"details", app_prefs.lot_tabs.show_details},
                                         {"settings", app_prefs.lot_tabs.show_settings}};
        j["app"]["index_tabs"]["building"] = {{"list", app_prefs.building_tabs.show_list},
                                              {"details", app_prefs.building_tabs.show_details},
                                              {"settings", app_prefs.building_tabs.show_settings}};

        // Display preferences
        j["display"]["vsync"] = display_prefs.vsync;
        j["display"]["high_dpi"] = display_prefs.high_dpi;

        return j.dump(4);
    }

} // namespace RCG
