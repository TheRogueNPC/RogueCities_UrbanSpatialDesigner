#include "Application.hpp"
#include "ImGuiCompat.hpp"
#include "../third_party/icons/IconsFontAwesome6.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <cstring>
#include <algorithm>
#include <unordered_set>
#include <cmath>
#include <functional>
#include <random>
#include <nlohmann/json.hpp>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#pragma comment(lib, "Comdlg32.lib")
#endif
#include "TensorField.h"
#include "RoadGenerator.h"
#include "WaterGenerator.h"
#include "Integrator.h"
#include "ExportSchema.h"
#include "AxiomInput.h"
#include "DistrictGenerator.h"
#include "LotGenerator.h"
#include "SiteGenerator.h"
#include "DebugLog.hpp"

namespace RCG
{
    namespace fs = std::filesystem;
    using json = nlohmann::json;

    // GLFW error callback
    static void glfw_error_callback(int error, const char *description)
    {
        fprintf(stderr, "GLFW Error %d: %s\n", error, description);
    }

    static const char *axiom_type_label(int type)
    {
        switch (type)
        {
        case 0:
            return "Radial";
        case 1:
            return "Grid";
        case 2:
        default:
            return "Delta";
        }
    }

    static std::string make_timestamp()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm{};
#ifdef _WIN32
        localtime_s(&local_tm, &now_time);
#else
        localtime_r(&now_time, &local_tm);
#endif
        std::ostringstream oss;
        oss << std::put_time(&local_tm, "%Y%m%d_%H%M%S");
        return oss.str();
    }

    static uint64_t hash_combine(uint64_t seed, uint64_t value)
    {
        seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        return seed;
    }

    static bool vec2_near(const CityModel::Vec2 &a, const CityModel::Vec2 &b, double eps)
    {
        return a.distanceToSquared(b) <= eps * eps;
    }

    static bool merge_polyline(CityModel::Polyline &base, const CityModel::Polyline &other, double eps)
    {
        if (base.points.size() < 2 || other.points.size() < 2)
        {
            return false;
        }

        const auto &a_start = base.points.front();
        const auto &a_end = base.points.back();
        const auto &b_start = other.points.front();
        const auto &b_end = other.points.back();

        if (vec2_near(a_end, b_start, eps))
        {
            base.points.insert(base.points.end(), other.points.begin() + 1, other.points.end());
            return true;
        }
        if (vec2_near(a_end, b_end, eps))
        {
            for (auto it = other.points.rbegin() + 1; it != other.points.rend(); ++it)
            {
                base.points.push_back(*it);
            }
            return true;
        }
        if (vec2_near(a_start, b_end, eps))
        {
            base.points.insert(base.points.begin(), other.points.begin(), other.points.end() - 1);
            return true;
        }
        if (vec2_near(a_start, b_start, eps))
        {
            std::vector<CityModel::Vec2> prefix;
            prefix.reserve(other.points.size() - 1);
            for (auto it = other.points.rbegin(); it != other.points.rend() - 1; ++it)
            {
                prefix.push_back(*it);
            }
            base.points.insert(base.points.begin(), prefix.begin(), prefix.end());
            return true;
        }

        return false;
    }

    static std::vector<CityModel::Polyline> merge_polylines(const std::vector<CityModel::Polyline> &input, double eps)
    {
        std::vector<CityModel::Polyline> out;
        out.reserve(input.size());

        for (const auto &poly : input)
        {
            if (poly.points.size() < 2)
            {
                continue;
            }

            bool merged = false;
            for (auto &existing : out)
            {
                if (merge_polyline(existing, poly, eps))
                {
                    merged = true;
                    break;
                }
            }

            if (!merged)
            {
                out.push_back(poly);
            }
        }

        bool changed = true;
        while (changed)
        {
            changed = false;
            for (std::size_t i = 0; i < out.size() && !changed; ++i)
            {
                for (std::size_t j = i + 1; j < out.size(); ++j)
                {
                    if (merge_polyline(out[i], out[j], eps))
                    {
                        out.erase(out.begin() + static_cast<std::ptrdiff_t>(j));
                        changed = true;
                        break;
                    }
                    if (merge_polyline(out[j], out[i], eps))
                    {
                        out[i] = out[j];
                        out.erase(out.begin() + static_cast<std::ptrdiff_t>(j));
                        changed = true;
                        break;
                    }
                }
            }
        }

        return out;
    }

    static uint64_t hash_city_params(const CityParams &params)
    {
        uint64_t h = 0;
        h = hash_combine(h, std::hash<int>{}(params.city_size));
        h = hash_combine(h, std::hash<float>{}(params.noise_scale));
        h = hash_combine(h, std::hash<float>{}(params.road_density));
        h = hash_combine(h, std::hash<int>{}(params.major_road_iterations));
        h = hash_combine(h, std::hash<int>{}(params.minor_road_iterations));
        h = hash_combine(h, std::hash<float>{}(params.vertex_snap_distance));
        h = hash_combine(h, std::hash<bool>{}(params.generate_rivers));
        h = hash_combine(h, std::hash<int>{}(params.river_count));
        h = hash_combine(h, std::hash<float>{}(params.river_width));
        h = hash_combine(h, std::hash<float>{}(params.building_density));
        h = hash_combine(h, std::hash<float>{}(params.max_building_height));
        h = hash_combine(h, std::hash<int>{}(params.lot_subdivision_iterations));
        h = hash_combine(h, std::hash<int>{}(params.radial_rule_weight));
        h = hash_combine(h, std::hash<int>{}(params.grid_rule_weight));
        h = hash_combine(h, std::hash<int>{}(params.organic_rule_weight));
        h = hash_combine(h, std::hash<uint32_t>{}(params.maxMajorRoads));
        h = hash_combine(h, std::hash<uint32_t>{}(params.maxTotalRoads));
        h = hash_combine(h, std::hash<float>{}(params.majorToMinorRatio));
        h = hash_combine(h, std::hash<int>{}(static_cast<int>(params.roadDefinitionMode)));
        h = hash_combine(h, std::hash<float>{}(params.block_snap_tolerance_factor));
        h = hash_combine(h, std::hash<float>{}(params.merge_radius));
        h = hash_combine(h, std::hash<bool>{}(params.verbose_geos_diagnostics));
        return h;
    }

    void Application::record_generation_state()
    {
        if (viewport_window)
        {
            last_generated_axiom_revision = viewport_window->get_axiom_revision();
            last_user_lot_revision = viewport_window->get_user_lot_revision();
            last_user_road_revision = viewport_window->get_user_road_revision();
        }
        last_city_params_hash = hash_city_params(city_params);
    }

    static bool nearly_equal(float a, float b, float eps = 1e-4f)
    {
        return std::fabs(a - b) <= eps;
    }

    static void open_in_file_explorer(const std::string &path)
    {
#ifdef _WIN32
        if (path.empty())
        {
            return;
        }
        ShellExecuteA(nullptr, "open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
        (void)path;
#endif
    }

    static void apply_ui_params_to_generator(const RCG::CityParams &ui_params, ::CityParams &gen_params)
    {
        // Pass seed for reproducibility
        gen_params.seed = ui_params.seed;

        double density = std::clamp(static_cast<double>(ui_params.road_density), 0.05, 1.0);
        double density_scale = std::clamp(1.6 - density, 0.5, 1.6);
        gen_params.main_dsep = 400.0 * density_scale;
        gen_params.major_dsep = 100.0 * density_scale;
        gen_params.minor_dsep = 20.0 * density_scale;

        int major_iters = std::clamp(ui_params.major_road_iterations, 10, 200);
        int minor_iters = std::clamp(ui_params.minor_road_iterations, 10, 200);
        gen_params.main_pathIterations = std::clamp(major_iters * 20, 200, 6000);
        gen_params.major_pathIterations = std::clamp(major_iters * 20, 200, 6000);
        gen_params.minor_pathIterations = std::clamp(minor_iters * 20, 200, 6000);

        double snap = std::clamp(static_cast<double>(ui_params.vertex_snap_distance), 1.0, 100.0);
        gen_params.main_dcirclejoin = snap;
        gen_params.major_dcirclejoin = snap;
        gen_params.minor_dcirclejoin = snap;

        double noise_scale = std::max(0.1, static_cast<double>(ui_params.noise_scale));
        if (std::abs(noise_scale - 1.0) > 0.001)
        {
            gen_params.tf_globalNoise = true;
        }
        gen_params.tf_noiseSizeGlobal = 100.0 / noise_scale;
        gen_params.tf_noiseSizePark = 50.0 / noise_scale;

        double river_width = std::max(1.0, static_cast<double>(ui_params.river_width));
        gen_params.water_riverSize = river_width;
        gen_params.water_riverBankSize = river_width * 0.5;
        gen_params.water_seedTries = std::clamp(ui_params.river_count * 100, 100, 1200);

        gen_params.maxMajorRoads = ui_params.maxMajorRoads;
        gen_params.maxTotalRoads = ui_params.maxTotalRoads;
        gen_params.majorToMinorRatio = ui_params.majorToMinorRatio;
        gen_params.roadDefinitionMode = (ui_params.roadDefinitionMode == CityParams::RoadDefinitionMode::BySegment)
                                            ? RoadDefinitionMode::BySegment
                                            : RoadDefinitionMode::ByPolyline;
        gen_params.debug_use_segment_roads_for_blocks = ui_params.debug_use_segment_roads_for_blocks ||
                                                        (gen_params.roadDefinitionMode == RoadDefinitionMode::BySegment);
        gen_params.block_snap_tolerance_factor = std::clamp(static_cast<double>(ui_params.block_snap_tolerance_factor), 0.05, 2.0);
        gen_params.merge_radius = ui_params.merge_radius;
        gen_params.verbose_geos_diagnostics = ui_params.verbose_geos_diagnostics;

        auto set_type = [&](CityModel::RoadType type,
                            double base_dsep,
                            double base_dtest,
                            double base_lookahead,
                            int base_iters,
                            bool major_dir,
                            bool prune_dangling,
                            bool enabled)
        {
            auto &tp = gen_params.road_type_params[CityModel::road_type_index(type)];
            tp.dsep = base_dsep * density_scale;
            tp.dtest = base_dtest * density_scale;
            tp.dstep = 1.0;
            tp.dlookahead = base_lookahead;
            tp.dcirclejoin = snap;
            tp.joinangle = 0.1;
            tp.pathIterations = std::clamp(base_iters * 20, 200, 6000);
            tp.seedTries = 300;
            tp.simplifyTolerance = 0.5;
            tp.collideEarly = 0.0;
            tp.majorDirection = major_dir;
            tp.pruneDangling = prune_dangling;
            tp.enabled = enabled;
        };

        set_type(CityModel::RoadType::Highway, 600.0, 250.0, 600.0, major_iters, true, true, true);
        set_type(CityModel::RoadType::Arterial, 350.0, 180.0, 400.0, major_iters, true, true, true);
        set_type(CityModel::RoadType::Avenue, 250.0, 140.0, 300.0, major_iters, true, true, true);
        set_type(CityModel::RoadType::Boulevard, 200.0, 120.0, 240.0, major_iters, true, true, true);
        set_type(CityModel::RoadType::Street, 120.0, 60.0, 140.0, minor_iters, false, true, true);
        set_type(CityModel::RoadType::Lane, 80.0, 45.0, 100.0, minor_iters, false, true, true);
        set_type(CityModel::RoadType::Alleyway, 50.0, 30.0, 70.0, minor_iters, false, true, true);
        set_type(CityModel::RoadType::CulDeSac, 40.0, 25.0, 50.0, minor_iters, false, false, true);
        set_type(CityModel::RoadType::Drive, 60.0, 35.0, 80.0, minor_iters, false, true, true);
        set_type(CityModel::RoadType::Driveway, 25.0, 15.0, 30.0, minor_iters, false, false, false);

        gen_params.road_type_params[CityModel::road_type_index(CityModel::RoadType::CulDeSac)].requireDeadEnd = true;
        gen_params.road_type_params[CityModel::road_type_index(CityModel::RoadType::Driveway)].requireDeadEnd = true;
        gen_params.road_type_params[CityModel::road_type_index(CityModel::RoadType::Highway)].allowDeadEnds = false;

        auto set_mask = [&](CityModel::RoadType type, uint32_t mask)
        {
            gen_params.road_type_params[CityModel::road_type_index(type)].allowIntersectionsMask = mask;
        };

        const uint32_t all = 0xFFFFFFFFu;
        const uint32_t allow_highway = CityModel::road_type_bit(CityModel::RoadType::Highway) |
                                       CityModel::road_type_bit(CityModel::RoadType::Arterial) |
                                       CityModel::road_type_bit(CityModel::RoadType::Avenue) |
                                       CityModel::road_type_bit(CityModel::RoadType::Boulevard) |
                                       CityModel::road_type_bit(CityModel::RoadType::Street) |
                                       CityModel::road_type_bit(CityModel::RoadType::Drive);
        set_mask(CityModel::RoadType::Highway, allow_highway);
        set_mask(CityModel::RoadType::Arterial, all);
        set_mask(CityModel::RoadType::Avenue, all);
        set_mask(CityModel::RoadType::Boulevard, all);
        set_mask(CityModel::RoadType::Street, all);
        set_mask(CityModel::RoadType::Lane, all);
        set_mask(CityModel::RoadType::Alleyway, all);
        set_mask(CityModel::RoadType::CulDeSac, all);
        set_mask(CityModel::RoadType::Drive, all);
        set_mask(CityModel::RoadType::Driveway, all);

        for (std::size_t i = 0; i < CityModel::road_type_count; ++i)
        {
            gen_params.block_barrier[i] = ui_params.block_barrier[i];
            gen_params.block_closure[i] = ui_params.block_closure[i];
        }
        gen_params.debug_use_segment_roads_for_blocks = ui_params.debug_use_segment_roads_for_blocks ||
                                                        (gen_params.roadDefinitionMode == RoadDefinitionMode::BySegment);

        // Block generation mode
        gen_params.block_gen_mode = (ui_params.block_gen_mode == 0) ? BlockGenMode::Legacy : BlockGenMode::GEOSOnly;

        // Phase toggles
        for (std::size_t i = 0; i < static_cast<std::size_t>(GeneratorPhase::Count); ++i)
        {
            gen_params.phase_enabled[i] = ui_params.phase_enabled[i];
        }

        // Lot generation params
        gen_params.minLotsPerRoadSide = ui_params.minLotsPerRoadSide;
        gen_params.lotSpacingMultiplier = static_cast<double>(ui_params.lotSpacingMultiplier);
    }

    static bool browse_save_path(std::string &path, const char *title)
    {
#ifdef _WIN32
        char buffer[MAX_PATH] = {};
        if (!path.empty())
        {
            std::snprintf(buffer, sizeof(buffer), "%s", path.c_str());
        }
        OPENFILENAMEA ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile = buffer;
        ofn.nMaxFile = sizeof(buffer);
        ofn.lpstrFilter = "JSON Files\0*.json\0All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrTitle = title;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        if (GetSaveFileNameA(&ofn))
        {
            path = buffer;
            return true;
        }
#else
        (void)title;
        (void)path;
#endif
        return false;
    }

    Application::Application()
        : user_preferences(),
          city_params(),
          generated_city(),
          has_generated_city(false),
          ui_manager(nullptr),
          tools_window(nullptr),
          parameters_window(nullptr),
          export_window(nullptr),
          view_options_window(nullptr),
          roads_window(nullptr),
          rivers_window(nullptr),
          district_index_window(nullptr),
          river_index_window(nullptr),
          event_log_window(nullptr),
          road_index_window(nullptr),
          lot_index_window(nullptr),
          building_index_window(nullptr),
          presets_window(nullptr),
          progress_window(nullptr),
          color_settings_window(nullptr),
          viewport_window(nullptr),
          window(nullptr),
          show_preferences(false),
          last_axiom_count(0),
          last_axiom_revision(0),
          last_generated_axiom_revision(0),
          last_city_params_hash(0),
          last_user_lot_revision(0),
          last_user_road_revision(0),
          show_exit_prompt(false),
          exit_confirmed(false)
    {
    }

    Application::~Application()
    {
        shutdown();
    }

    bool Application::initialize()
    {
        // Set GLFW error callback
        glfwSetErrorCallback(glfw_error_callback);

        // Initialize GLFW
        if (!glfwInit())
        {
            fprintf(stderr, "Failed to initialize GLFW\n");
            return false;
        }

        // Create window
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

        window = glfwCreateWindow(1280, 720, "RogueCityVisualizer [test build]", nullptr, nullptr);
        if (!window)
        {
            fprintf(stderr, "Failed to create GLFW window\n");
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Enable vsync

        // Initialize GLEW
        glewExperimental = GL_TRUE;
        GLenum err = glewInit();
        if (err != GLEW_OK)
        {
            fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
            glfwDestroyWindow(window);
            glfwTerminate();
            return false;
        }

        // Initialize ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();

        // Load fonts - default plus Font Awesome icons
        io.Fonts->AddFontDefault();

        // Merge Font Awesome icons into the default font
        static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        icons_config.GlyphMinAdvanceX = 13.0f; // Monospace icons

        // Try to find the font file relative to executable or workspace
        std::string icon_font_path = "third_party/icons/fa-solid-900.ttf";
        if (!fs::exists(icon_font_path))
        {
            // Try from parent directory (in case running from build folder)
            icon_font_path = "../third_party/icons/fa-solid-900.ttf";
        }
        if (fs::exists(icon_font_path))
        {
            io.Fonts->AddFontFromFileTTF(icon_font_path.c_str(), 13.0f, &icons_config, icons_ranges);
            std::cout << "Font Awesome icons loaded from: " << icon_font_path << std::endl;
        }
        else
        {
            std::cerr << "Warning: Could not find Font Awesome font file" << std::endl;
        }

#if RCG_IMGUI_HAS_DOCKING
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif
#if RCG_IMGUI_HAS_VIEWPORTS
        if (user_preferences.app().enable_viewports)
        {
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        }
#endif

        // Setup ImGui style
        ImGui::StyleColorsDark();

        // Setup platform/renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 130");

        // Create UI components
        ui_manager = std::make_unique<UIManager>(user_preferences);
        tools_window = std::make_unique<ToolsWindow>(user_preferences);
        parameters_window = std::make_unique<ParametersWindow>(user_preferences);
        export_window = std::make_unique<ExportWindow>(user_preferences);
        view_options_window = std::make_unique<ViewOptionsWindow>(user_preferences);
        roads_window = std::make_unique<RoadsWindow>(user_preferences);
        rivers_window = std::make_unique<RiversWindow>(user_preferences);
        district_index_window = std::make_unique<DistrictIndexWindow>(user_preferences);
        river_index_window = std::make_unique<RiverIndexWindow>(user_preferences);
        event_log_window = std::make_unique<EventLogWindow>(user_preferences);
        road_index_window = std::make_unique<RoadIndexWindow>(user_preferences);
        lot_index_window = std::make_unique<LotIndexWindow>(user_preferences);
        building_index_window = std::make_unique<BuildingIndexWindow>(user_preferences);
        presets_window = std::make_unique<PresetsWindow>(user_preferences);
        progress_window = std::make_unique<ProgressWindow>(user_preferences);
        color_settings_window = std::make_unique<ColorSettingsWindow>(user_preferences);
        viewport_window = std::make_unique<ViewportWindow>(user_preferences);

        register_docks();

        // Reset generation modes to Manual each session (safety default)
        user_preferences.app().generation_mode = 1;       // Manual
        user_preferences.app().river_generation_mode = 1; // Manual

        if (event_log_window)
        {
            event_log_window->add_entry("Welcome to RogueCityVisualizer.");
            event_log_window->add_entry("Event log initialized.");
            event_log_window->add_entry("Generation mode reset to Manual.");
            event_log_window->add_entry("Axiom autosave active (4-slot rotation).");
        }
        if (viewport_window)
        {
            last_axiom_revision = viewport_window->get_axiom_revision();
            last_generated_axiom_revision = last_axiom_revision;
        }
        last_city_params_hash = hash_city_params(city_params);

        // Load city parameters from last session if available
        const fs::path autosave_path = fs::path(get_temp_dir()) / "autosave_0.json";
        if (fs::exists(autosave_path))
        {
            std::ifstream in(autosave_path.string());
            if (in)
            {
                try
                {
                    json j;
                    in >> j;
                    std::vector<ViewportWindow::AxiomData> saved;
                    if (j.contains("axioms") && j["axioms"].is_array())
                    {
                        for (const auto &entry : j["axioms"])
                        {
                            ViewportWindow::AxiomData axiom{};
                            axiom.id = entry.value("id", 0);
                            axiom.position.x = entry.value("x", 0.0f);
                            axiom.position.y = entry.value("y", 0.0f);
                            axiom.type = entry.value("type", 0);
                            axiom.size = entry.value("size", 10.0f);
                            axiom.opacity = entry.value("opacity", 1.0f);
                            axiom.terminal_type = entry.value("terminal", 0);
                            axiom.mode = entry.value("mode", 0);
                            axiom.name = entry.value("name", std::string("Axiom ") + std::to_string(axiom.id));
                            axiom.tag_id = entry.value("tag_id", 0);
                            axiom.major_road_count = entry.value("major_road_count", 5);
                            axiom.minor_road_count = entry.value("minor_road_count", 10);
                            axiom.fuzzy_mode = entry.value("fuzzy_mode", false);
                            axiom.min_roads = entry.value("min_roads", 5);
                            axiom.max_roads = entry.value("max_roads", 20);
                            axiom.influencer_type = entry.value("influencer_type", 0);
                            saved.push_back(axiom);
                        }
                    }

                    if (!saved.empty() && viewport_window)
                    {
                        viewport_window->set_axioms(saved);
                        if (event_log_window)
                        {
                            event_log_window->add_entry("Loaded autosave from previous session.");
                        }
                        update_city_generation();
                        record_generation_state();
                    }
                }
                catch (const std::exception &ex)
                {
                    if (event_log_window)
                    {
                        event_log_window->add_entry(
                            std::string("Failed to load autosave: ") + ex.what());
                    }
                }
            }
            else if (event_log_window)
            {
                event_log_window->add_entry("Autosave file found but could not be opened.");
            }
        }
        // (TODO: integrate with CityGenerator when ready)

        return true;
    }

    void Application::shutdown()
    {
        // Save preferences before shutdown
        user_preferences.save_to_file();
        cleanup_temp_state();

        // Cleanup UI components
        tools_window.reset();
        parameters_window.reset();
        export_window.reset();
        view_options_window.reset();
        roads_window.reset();
        rivers_window.reset();
        district_index_window.reset();
        river_index_window.reset();
        event_log_window.reset();
        road_index_window.reset();
        lot_index_window.reset();
        building_index_window.reset();
        viewport_window.reset();
        ui_manager.reset();

        // Cleanup ImGui
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        // Cleanup GLFW
        if (window)
        {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }

    void Application::register_docks()
    {
        dock_manager.clear();
        auto &win = user_preferences.windows();

        auto add = [&](const char *id, const char *label, const char *group, bool *visible, bool *locked, bool always_visible, std::function<void()> render)
        {
            DockManager::DockSpec spec;
            spec.id = id;
            spec.label = label;
            spec.group = group;
            spec.default_visible = visible ? *visible : true;
            spec.visible = visible;
            spec.locked = locked;
            spec.always_visible = always_visible;
            spec.render = render;
            dock_manager.register_dock(spec);
        };

        add("tools", "Tools", "Core", &win.tools_visible, &win.tools_locked, false, [this]()
            {
                if (tools_window)
                {
                    tools_window->render();
                } });
        add("parameters", "Parameters", "Core", &win.parameters_visible, &win.parameters_locked, false, [this]()
            {
                if (parameters_window)
                {
                    parameters_window->render(city_params);
                } });
        add("view_options", "View Options", "Core", &win.view_options_visible, &win.view_options_locked, false, [this]()
            {
                if (view_options_window)
                {
                    view_options_window->render();
                } });
        add("viewport", "City Viewport", "Core", &win.viewport_visible, nullptr, true, [this]()
            {
                if (viewport_window)
                {
                    viewport_window->render();
                } });
        add("roads", "Roads", "Editors", &win.roads_visible, &win.roads_locked, false, [this]()
            {
                if (roads_window)
                {
                    roads_window->render(city_params, has_generated_city);
                } });
        add("rivers", "Rivers", "Editors", &win.rivers_visible, &win.rivers_locked, false, [this]()
            {
                if (rivers_window && viewport_window)
                {
                    rivers_window->render(city_params, !viewport_window->get_rivers().empty());
                } });
        add("export", "Export & Status", "Output", &win.export_visible, &win.export_locked, false, [this]()
            {
                if (export_window)
                {
                    export_window->render();
                } });
        add("district_index", "District Index", "Indexes", &win.district_visible, &win.district_locked, false, [this]()
            {
                if (district_index_window && viewport_window)
                {
                        district_index_window->render(*viewport_window, city_params, generated_city);
                } });
        add("river_index", "River Index", "Indexes", &win.river_index_visible, &win.river_index_locked, false, [this]()
            {
                if (river_index_window && viewport_window)
                {
                    river_index_window->render(*viewport_window);
                } });
        add("road_index", "Road Index", "Indexes", &win.road_index_visible, &win.road_index_locked, false, [this]()
            {
                if (road_index_window && viewport_window)
                {
                    road_index_window->render(*viewport_window, city_params, road_index, city_params.maxTotalRoads,
                                              static_cast<uint32_t>(road_index.size()));
                } });
        add("lot_index", "Lot Index", "Indexes", &win.lot_index_visible, &win.lot_index_locked, false, [this]()
            {
                if (lot_index_window && viewport_window)
                {
                    const uint32_t max_lots = city_params.maxTotalRoads > 0 ? (city_params.maxTotalRoads / 2) : 0;
                    lot_index_window->render(*viewport_window, generated_city, max_lots);
                } });
        add("building_index", "Building Index", "Indexes", &win.building_index_visible, &win.building_index_locked, false, [this]()
            {
                if (building_index_window)
                {
                    const uint32_t max_lots = city_params.maxTotalRoads > 0 ? (city_params.maxTotalRoads / 2) : 0;
                    const uint32_t max_building_sites = max_lots * 6;
                    building_index_window->render(generated_city, max_building_sites);
                } });
        add("event_log", "Event Log", "Output", &win.event_log_visible, &win.event_log_locked, false, [this]()
            {
                if (event_log_window)
                {
                    event_log_window->render();
                } });
        add("presets", "Presets", "Output", &win.presets_visible, &win.presets_locked, false, [this]()
            {
                if (presets_window)
                {
                    presets_window->render(city_params);
                } });
        add("progress", "Progress", "Output", &win.progress_visible, &win.progress_locked, false, [this]()
            {
                if (progress_window)
                {
                    progress_window->set_visible(user_preferences.windows().progress_visible);
                    progress_window->render();
                } });
        add("color_settings", "Color Settings", "Output", &win.color_settings_visible, &win.color_settings_locked, false, [this]()
            {
                if (color_settings_window)
                {
                    color_settings_window->render();
                } });
    }

    bool Application::update()
    {
        // Process GLFW events
        glfwPollEvents();

        // Check if window should close
        if (glfwWindowShouldClose(window) && !exit_confirmed)
        {
            glfwSetWindowShouldClose(window, GLFW_FALSE);
            if (!show_exit_prompt)
            {
                show_exit_prompt = true;
                if (event_log_window)
                {
                    event_log_window->add_entry("Exit requested.");
                }
            }
        }

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Begin docking layout
        ui_manager->begin_docking();

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open Config Folder"))
                {
                    open_in_file_explorer(UserPreferences::get_config_dir());
                }
                if (ImGui::MenuItem("Open Export Folder"))
                {
                    fs::path export_path = city_params.export_path;
                    if (export_path.has_filename())
                    {
                        export_path = export_path.parent_path();
                    }
                    if (!export_path.empty())
                    {
                        open_in_file_explorer(export_path.string());
                    }
                }
                if (ImGui::MenuItem("Save Preferences Now"))
                {
                    user_preferences.save_to_file();
                    user_preferences.save_to_ini();
                    if (event_log_window)
                    {
                        event_log_window->add_entry("Preferences saved.");
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                const bool canUndo = can_undo();
                const bool canRedo = can_redo();
                if (ImGui::MenuItem("Undo", "Ctrl+Z", false, canUndo))
                {
                    undo();
                }
                if (ImGui::MenuItem("Redo", "Ctrl+Y", false, canRedo))
                {
                    redo();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Options"))
            {
                ImGui::MenuItem("Preferences", nullptr, &show_preferences);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View"))
            {
                dock_manager.draw_view_menu();
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        ImGuiIO &io = ImGui::GetIO();
        if (!io.WantTextInput && !io.WantCaptureKeyboard)
        {
            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z))
            {
                if (io.KeyShift)
                {
                    redo();
                }
                else
                {
                    undo();
                }
            }
            else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y))
            {
                redo();
            }

            auto binding_pressed = [&](const UserPreferences::AppPrefs::KeyBinding &binding)
            {
                if (binding.key == ImGuiKey_None)
                {
                    return false;
                }
                if (binding.ctrl != io.KeyCtrl)
                    return false;
                if (binding.alt != io.KeyAlt)
                    return false;
                if (binding.shift != io.KeyShift)
                    return false;
                return ImGui::IsKeyPressed(static_cast<ImGuiKey>(binding.key));
            };

            const auto &keymap = user_preferences.app().keymap;
            if (binding_pressed(keymap.toggle_grid))
                user_preferences.app().show_grid = !user_preferences.app().show_grid;
            if (binding_pressed(keymap.toggle_axioms))
                user_preferences.app().show_axioms = !user_preferences.app().show_axioms;
            if (binding_pressed(keymap.toggle_roads))
                user_preferences.app().show_roads = !user_preferences.app().show_roads;
            if (binding_pressed(keymap.toggle_river_splines))
                user_preferences.app().show_river_splines = !user_preferences.app().show_river_splines;
            if (binding_pressed(keymap.toggle_road_intersections))
                user_preferences.app().show_road_intersections = !user_preferences.app().show_road_intersections;
            if (binding_pressed(keymap.reset_view) && viewport_window)
                viewport_window->reset_view();
        }

        if (event_log_window && viewport_window)
        {
            const auto &axioms = viewport_window->get_axioms();
            for (size_t i = last_axiom_count; i < axioms.size(); ++i)
            {
                const auto &axiom = axioms[i];
                char buffer[160];
                std::snprintf(buffer, sizeof(buffer), "Axiom %d placed: (%.2f, %.2f) type=%s",
                              axiom.id, axiom.position.x, axiom.position.y, axiom_type_label(axiom.type));
                event_log_window->add_entry(buffer);
            }
            last_axiom_count = axioms.size();
        }
        dock_manager.render_visible();

        if (show_exit_prompt)
        {
            ImGui::OpenPopup("Exit");
        }
        if (ImGui::BeginPopupModal("Exit", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Save a snapshot before exiting?");
            ImGui::Separator();
            if (ImGui::Button("Save & Exit"))
            {
                save_snapshot("user_exit");
                cleanup_temp_state();
                exit_confirmed = true;
                show_exit_prompt = false;
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                if (event_log_window)
                {
                    event_log_window->add_entry("Exit confirmed (snapshot saved).");
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Exit Without Save"))
            {
                cleanup_temp_state();
                exit_confirmed = true;
                show_exit_prompt = false;
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                if (event_log_window)
                {
                    event_log_window->add_entry("Exit confirmed (no snapshot).");
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                show_exit_prompt = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (show_preferences)
        {
            if (ImGui::Begin("Preferences", &show_preferences))
            {
                ImGui::Text("Theme");
                bool light_theme = user_preferences.app().use_light_theme;
                if (ImGui::RadioButton("Dark", !light_theme))
                {
                    user_preferences.app().use_light_theme = false;
                    ui_manager->apply_theme(false);
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Light", light_theme))
                {
                    user_preferences.app().use_light_theme = true;
                    ui_manager->apply_theme(true);
                }

                ImGui::Separator();
                ImGui::Text("Viewport");
                ImGui::SliderFloat("Brightness", &user_preferences.app().viewport_brightness, 0.3f, 2.5f);
                ImGui::Checkbox("Enable Culling", &user_preferences.app().enable_culling);
                ImGui::SliderFloat("Near Clip", &user_preferences.app().cull_near, 0.01f, 10.0f);
                ImGui::SliderFloat("Far Clip", &user_preferences.app().cull_far, 50.0f, 5000.0f);
                if (user_preferences.app().cull_far <= user_preferences.app().cull_near + 0.1f)
                {
                    user_preferences.app().cull_far = user_preferences.app().cull_near + 0.1f;
                }

                ImGui::Separator();
                ImGui::Text("Index Tabs");
                auto draw_index_tabs = [](const char *label, UserPreferences::AppPrefs::IndexTabs &tabs)
                {
                    ImGui::Text("%s", label);
                    ImGui::Indent();
                    ImGui::PushID(label);
                    ImGui::Checkbox("List", &tabs.show_list);
                    ImGui::SameLine();
                    ImGui::Checkbox("Details", &tabs.show_details);
                    ImGui::SameLine();
                    ImGui::Checkbox("Settings", &tabs.show_settings);
                    ImGui::PopID();
                    ImGui::Unindent();
                    ImGui::Spacing();
                    if (!tabs.show_list && !tabs.show_details && !tabs.show_settings)
                    {
                        tabs.show_list = true;
                    }
                };
                draw_index_tabs("District Index", user_preferences.app().district_tabs);
                draw_index_tabs("Road Index", user_preferences.app().road_tabs);
                draw_index_tabs("Lot Index", user_preferences.app().lot_tabs);
                draw_index_tabs("Building Index", user_preferences.app().building_tabs);

                ImGui::Separator();
                ImGui::Text("Export Preferences");
                static char export_path_buffer[260] = {};
                static bool export_path_init = false;
                if (!export_path_init)
                {
                    std::snprintf(export_path_buffer, sizeof(export_path_buffer), "%s",
                                  city_params.export_path.c_str());
                    export_path_init = true;
                }
                if (ImGui::InputText("Export Path", export_path_buffer, sizeof(export_path_buffer)))
                {
                    city_params.export_path = export_path_buffer;
                }
                if (ImGui::Button("Browse Export Path"))
                {
                    std::string path = city_params.export_path;
                    if (browse_save_path(path, "Select Export Path"))
                    {
                        city_params.export_path = path;
                        std::snprintf(export_path_buffer, sizeof(export_path_buffer), "%s",
                                      city_params.export_path.c_str());
                        if (event_log_window)
                        {
                            event_log_window->add_entry("Export path updated.");
                        }
                    }
                }
                if (ImGui::Button("Export Preferences File"))
                {
                    std::string pref_path = UserPreferences::get_config_dir() + "/preferences_export.json";
                    if (browse_save_path(pref_path, "Export Preferences"))
                    {
                        user_preferences.save_to_file(pref_path);
                        if (event_log_window)
                        {
                            event_log_window->add_entry("Preferences exported.");
                        }
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Save Preferences"))
                {
                    user_preferences.save_to_file();
                    user_preferences.save_to_ini();
                    if (event_log_window)
                    {
                        event_log_window->add_entry("Preferences saved.");
                    }
                }
            }
            ImGui::End();
        }

        // Handle input and interactions
        handle_input();

        // Update city generation if needed
        if (user_preferences.app().generation_mode == 0 && viewport_window)
        {
            const auto &axioms = viewport_window->get_axioms();
            size_t revision = viewport_window->get_axiom_revision();
            size_t lot_revision = viewport_window->get_user_lot_revision();
            size_t road_revision = viewport_window->get_user_road_revision();
            uint64_t params_hash = hash_city_params(city_params);
            bool user_inputs_changed = (lot_revision != last_user_lot_revision) ||
                                       (road_revision != last_user_road_revision);
            bool params_changed = revision != last_generated_axiom_revision ||
                                  params_hash != last_city_params_hash;
            if ((!axioms.empty() && params_changed) || user_inputs_changed)
            {
                if (event_log_window)
                {
                    if (user_inputs_changed)
                    {
                        event_log_window->add_entry("Live generation triggered (user edit).");
                    }
                    else
                    {
                        event_log_window->add_entry("Live generation triggered.");
                    }
                }
                update_city_generation();
                record_generation_state();
            }
        }

        const bool regen_requested =
            (viewport_window && viewport_window->consume_regeneration_request());
        if (regen_requested && user_preferences.app().generation_mode == 1 &&
            user_preferences.app().auto_regenerate)
        {
            if (event_log_window)
            {
                event_log_window->add_entry("Manual edit triggered auto regeneration.");
            }
            update_city_generation();
            record_generation_state();
        }

        // Handle export requests
        handle_export();

        // Update undo/redo history after input and exports are processed
        update_history();

        // Auto-save axiom state when changed
        if (viewport_window)
        {
            size_t revision = viewport_window->get_axiom_revision();
            if (revision != last_axiom_revision)
            {
                autosave_state();
                last_axiom_revision = revision;
            }
        }

        // Finish ImGui frame
        ImGui::Render();

        // Render to OpenGL
        render();

        if (exit_confirmed)
        {
            return false;
        }

        return true;
    }

    Application::EditorState Application::capture_state() const
    {
        EditorState state;
        state.params = city_params;
        if (viewport_window)
        {
            state.axioms = viewport_window->get_axioms();
            state.rivers = viewport_window->get_rivers();
            state.selected_axiom_id = viewport_window->get_selected_axiom_id();
            state.selected_river_id = viewport_window->get_selected_river_id();
        }
        return state;
    }

    void Application::apply_state(const EditorState &state)
    {
        applying_history = true;
        city_params = state.params;
        if (viewport_window)
        {
            viewport_window->set_axioms(state.axioms, state.selected_axiom_id);
            viewport_window->set_rivers(state.rivers, state.selected_river_id);
            last_axiom_count = viewport_window->get_axioms().size();
            last_axiom_revision = viewport_window->get_axiom_revision();
        }
        last_generated_axiom_revision = 0;
        last_city_params_hash = 0;
        applying_history = false;
    }

    void Application::push_undo_state(const EditorState &state)
    {
        if (undo_stack.size() >= kHistoryLimit)
        {
            undo_stack.erase(undo_stack.begin());
        }
        undo_stack.push_back(state);
    }

    void Application::update_history()
    {
        if (!viewport_window)
        {
            return;
        }

        EditorState current = capture_state();
        if (!history_initialized)
        {
            last_state = current;
            history_initialized = true;
            return;
        }

        if (applying_history)
        {
            last_state = current;
            return;
        }

        if (!states_equal(current, last_state))
        {
            push_undo_state(last_state);
            redo_stack.clear();
            last_state = current;
        }
    }

    bool Application::can_undo() const
    {
        return !undo_stack.empty();
    }

    bool Application::can_redo() const
    {
        return !redo_stack.empty();
    }

    void Application::undo()
    {
        if (!can_undo())
        {
            return;
        }
        if (redo_stack.size() >= kHistoryLimit)
        {
            redo_stack.erase(redo_stack.begin());
        }
        redo_stack.push_back(capture_state());
        EditorState state = undo_stack.back();
        undo_stack.pop_back();
        apply_state(state);
        last_state = state;
        if (event_log_window)
        {
            event_log_window->add_entry("Undo applied.");
        }
    }

    void Application::redo()
    {
        if (!can_redo())
        {
            return;
        }
        push_undo_state(capture_state());
        EditorState state = redo_stack.back();
        redo_stack.pop_back();
        apply_state(state);
        last_state = state;
        if (event_log_window)
        {
            event_log_window->add_entry("Redo applied.");
        }
    }

    bool Application::states_equal(const EditorState &a, const EditorState &b)
    {
        if (!nearly_equal(a.params.noise_scale, b.params.noise_scale) ||
            a.params.city_size != b.params.city_size ||
            !nearly_equal(a.params.road_density, b.params.road_density) ||
            a.params.major_road_iterations != b.params.major_road_iterations ||
            a.params.minor_road_iterations != b.params.minor_road_iterations ||
            !nearly_equal(a.params.vertex_snap_distance, b.params.vertex_snap_distance) ||
            a.params.maxMajorRoads != b.params.maxMajorRoads ||
            a.params.maxTotalRoads != b.params.maxTotalRoads ||
            !nearly_equal(a.params.majorToMinorRatio, b.params.majorToMinorRatio) ||
            a.params.roadDefinitionMode != b.params.roadDefinitionMode ||
            a.params.generate_rivers != b.params.generate_rivers ||
            a.params.river_count != b.params.river_count ||
            !nearly_equal(a.params.river_width, b.params.river_width) ||
            !nearly_equal(a.params.building_density, b.params.building_density) ||
            !nearly_equal(a.params.max_building_height, b.params.max_building_height) ||
            a.params.lot_subdivision_iterations != b.params.lot_subdivision_iterations ||
            a.params.radial_rule_weight != b.params.radial_rule_weight ||
            a.params.grid_rule_weight != b.params.grid_rule_weight ||
            a.params.organic_rule_weight != b.params.organic_rule_weight ||
            a.params.export_path != b.params.export_path)
        {
            return false;
        }

        if (a.selected_axiom_id != b.selected_axiom_id || a.selected_river_id != b.selected_river_id)
        {
            return false;
        }

        if (a.axioms.size() != b.axioms.size() || a.rivers.size() != b.rivers.size())
        {
            return false;
        }

        for (size_t i = 0; i < a.axioms.size(); ++i)
        {
            const auto &lhs = a.axioms[i];
            const auto &rhs = b.axioms[i];
            if (lhs.id != rhs.id || lhs.type != rhs.type ||
                !nearly_equal(lhs.position.x, rhs.position.x) ||
                !nearly_equal(lhs.position.y, rhs.position.y) ||
                !nearly_equal(lhs.size, rhs.size) ||
                !nearly_equal(lhs.opacity, rhs.opacity) ||
                lhs.terminal_type != rhs.terminal_type ||
                lhs.mode != rhs.mode ||
                lhs.name != rhs.name ||
                lhs.tag_id != rhs.tag_id)
            {
                return false;
            }
        }

        for (size_t i = 0; i < a.rivers.size(); ++i)
        {
            const auto &lhs = a.rivers[i];
            const auto &rhs = b.rivers[i];
            if (lhs.id != rhs.id || lhs.type != rhs.type ||
                !nearly_equal(lhs.width, rhs.width) ||
                !nearly_equal(lhs.opacity, rhs.opacity) ||
                lhs.name != rhs.name ||
                lhs.control_points.size() != rhs.control_points.size())
            {
                return false;
            }
            for (size_t p = 0; p < lhs.control_points.size(); ++p)
            {
                if (!nearly_equal(lhs.control_points[p].x, rhs.control_points[p].x) ||
                    !nearly_equal(lhs.control_points[p].y, rhs.control_points[p].y))
                {
                    return false;
                }
            }
        }

        return true;
    }

    void Application::render()
    {
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (viewport_window)
        {
            viewport_window->render_gl();
        }

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

#if RCG_IMGUI_HAS_VIEWPORTS
        // Handle multi-viewport
        ImGuiIO &io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow *backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
#endif

        glfwSwapBuffers(window);
    }

    void Application::handle_input()
    {
        // Road generation controls
        if (roads_window && roads_window->generate_clicked())
        {
            if (event_log_window)
            {
                event_log_window->add_entry(has_generated_city ? "Regenerate Roads requested."
                                                               : "Generate Roads requested.");
            }
            update_city_generation();
            record_generation_state();
        }
        if (roads_window && roads_window->clear_clicked())
        {
            if (viewport_window)
            {
                viewport_window->clear_city();
            }
            has_generated_city = false;
            generated_city = CityModel::City{};
            road_index.clear();
            merge_user_roads_to_index();
            sync_user_roads_to_viewport();
            if (event_log_window)
            {
                event_log_window->add_entry("Roads cleared (user roads preserved).");
            }
        }

        if (parameters_window && parameters_window->generate_city_clicked())
        {
            if (event_log_window)
            {
                event_log_window->add_entry("Generate City requested.");
            }
            update_city_generation();
            record_generation_state();
        }

        if (parameters_window && parameters_window->regenerate_lots_clicked())
        {
            if (event_log_window)
            {
                event_log_window->add_entry("Regenerate Lots requested (full city regeneration).");
            }
            update_city_generation();
            record_generation_state();
        }

        // Handle Clear All - removes everything including user roads
        if (roads_window && roads_window->clear_all_clicked())
        {
            if (viewport_window)
            {
                viewport_window->clear_city();
                viewport_window->clear_user_roads();
            }
            has_generated_city = false;
            generated_city = CityModel::City{};
            road_index.clear();
            user_roads.clear();
            if (event_log_window)
            {
                event_log_window->add_entry("All roads cleared (including user roads).");
            }
        }

        // Handle +Major/+Minor buttons from Road Index
        if (road_index_window && viewport_window)
        {
            int selected_axiom = viewport_window->get_selected_axiom_id();
            if (road_index_window->add_major_clicked() && selected_axiom >= 0)
            {
                // Create user M_Major road at selected axiom
                add_user_road(selected_axiom, true);
                if (event_log_window)
                {
                    event_log_window->add_entry("M_Major road added at axiom " + std::to_string(selected_axiom) + ".");
                }
            }
            if (road_index_window->add_minor_clicked() && selected_axiom >= 0)
            {
                // Create user M_Minor road at selected axiom
                add_user_road(selected_axiom, false);
                if (event_log_window)
                {
                    event_log_window->add_entry("M_Minor road added at axiom " + std::to_string(selected_axiom) + ".");
                }
            }
            if (road_index_window->remove_road_clicked())
            {
                int road_to_remove = road_index_window->get_road_to_remove();
                remove_user_road(road_to_remove);
                if (event_log_window)
                {
                    event_log_window->add_entry("User road " + std::to_string(road_to_remove) + " removed.");
                }
            }
            if (road_index_window->lock_road_clicked())
            {
                int road_to_lock = road_index_window->get_road_to_lock();
                auto it = std::find_if(road_index.begin(), road_index.end(),
                                       [road_to_lock](const RoadIndexEntry &r)
                                       { return r.id == road_to_lock; });
                if (it != road_index.end())
                {
                    RoadIndexEntry locked = *it;
                    locked.category = RoadCategory::User;
                    locked.locked = true;
                    locked.from_generated = true;
                    locked.intersections.clear();
                    locked.intersections_computed = false;
                    if (locked.nodes.empty())
                    {
                        locked.nodes = {ImVec2(locked.start_x, locked.start_y), ImVec2(locked.end_x, locked.end_y)};
                    }
                    auto uit = std::find_if(user_roads.begin(), user_roads.end(),
                                            [road_to_lock](const RoadIndexEntry &r)
                                            { return r.id == road_to_lock; });
                    if (uit != user_roads.end())
                    {
                        *uit = locked;
                    }
                    else
                    {
                        user_roads.push_back(locked);
                    }
                    merge_user_roads_to_index();
                    sync_user_roads_to_viewport();
                    if (event_log_window)
                    {
                        event_log_window->add_entry("Road " + std::to_string(road_to_lock) + " locked to user.");
                    }
                }
            }
            if (road_index_window->unlock_road_clicked())
            {
                int road_to_unlock = road_index_window->get_road_to_unlock();
                auto uit = std::find_if(user_roads.begin(), user_roads.end(),
                                        [road_to_unlock](const RoadIndexEntry &r)
                                        { return r.id == road_to_unlock; });
                if (uit != user_roads.end())
                {
                    uit->category = RoadCategory::Generated;
                    uit->locked = false;
                    uit->from_generated = false;
                    merge_user_roads_to_index();
                    sync_user_roads_to_viewport();
                    if (event_log_window)
                    {
                        event_log_window->add_entry("Road " + std::to_string(road_to_unlock) + " unlocked to generated.");
                    }
                }
            }
        }

        bool manual_road_generation_requested = false;
        if (viewport_window)
        {
            ViewportWindow::RoadEditEvent edit;
            if (viewport_window->consume_road_edit(edit))
            {
                auto it = std::find_if(road_index.begin(), road_index.end(),
                                       [edit](const RoadIndexEntry &r)
                                       { return r.id == edit.road_id; });
                if (it != road_index.end())
                {
                    RoadIndexEntry updated = *it;
                    updated.road_type = edit.type;
                    updated.nodes = edit.points;
                    if (!updated.nodes.empty())
                    {
                        updated.start_x = updated.nodes.front().x;
                        updated.start_y = updated.nodes.front().y;
                        updated.end_x = updated.nodes.back().x;
                        updated.end_y = updated.nodes.back().y;
                    }
                    updated.category = RoadCategory::User;
                    updated.locked = true;
                    updated.from_generated = updated.from_generated || !edit.from_user;
                    updated.intersections.clear();
                    updated.intersections_computed = false;

                    auto uit = std::find_if(user_roads.begin(), user_roads.end(),
                                            [edit](const RoadIndexEntry &r)
                                            { return r.id == edit.road_id; });
                    if (uit != user_roads.end())
                    {
                        *uit = updated;
                    }
                    else
                    {
                        user_roads.push_back(updated);
                    }
                    merge_user_roads_to_index();
                    sync_user_roads_to_viewport();
                    if (event_log_window)
                    {
                        event_log_window->add_entry("Road " + std::to_string(edit.road_id) + " edited (locked to user).");
                    }
                    manual_road_generation_requested = true;
                }
            }
        }

        if (manual_road_generation_requested && user_preferences.app().generation_mode == 0)
        {
            if (event_log_window)
            {
                event_log_window->add_entry("Live generation triggered (road edit).");
            }
            update_city_generation();
            record_generation_state();
        }

        // Handle preset application
        if (presets_window && presets_window->preset_applied())
        {
            // Preset has already updated city_params in PresetsWindow::render
            // Force regeneration
            if (user_preferences.app().generation_mode == 0)
            {
                // Live mode - will auto-regenerate
                last_city_params_hash = 0; // Force update
            }
            if (event_log_window)
            {
                event_log_window->add_entry("Preset applied: " + presets_window->get_selected_preset());
            }
        }

        if (rivers_window && rivers_window->generate_clicked())
        {
            if (viewport_window)
            {
                viewport_window->rebuild_rivers();
            }
            if (event_log_window)
            {
                event_log_window->add_entry("Rivers generated from control points.");
            }
        }
        if (rivers_window && rivers_window->clear_clicked())
        {
            if (viewport_window)
            {
                viewport_window->clear_rivers();
            }
            if (event_log_window)
            {
                event_log_window->add_entry("Rivers cleared.");
            }
        }

        if (viewport_window)
        {
            if (viewport_window->consume_clear_axioms_request())
            {
                viewport_window->clear_axioms();
                if (event_log_window)
                {
                    event_log_window->add_entry("Axioms cleared.");
                }
            }
            if (viewport_window->consume_clear_city_request())
            {
                viewport_window->clear_city();
                has_generated_city = false;
                generated_city = CityModel::City{};
                if (event_log_window)
                {
                    event_log_window->add_entry("City cleared.");
                }
            }
        }

        // Let viewport handle its own pan/zoom input
        // (handled in ViewportWindow::render())

        // Handle Delete key for removing selected objects
        if (ImGui::IsKeyPressed(ImGuiKey_Delete) && viewport_window)
        {
            int selected_axiom = viewport_window->get_selected_axiom_id();
            int selected_road = viewport_window->get_selected_road_id();

            if (selected_axiom >= 0)
            {
                // Delete selected axiom
                auto axioms = viewport_window->get_axioms();
                auto it = std::find_if(axioms.begin(), axioms.end(),
                                       [selected_axiom](const ViewportWindow::AxiomData &a)
                                       { return a.id == selected_axiom; });
                if (it != axioms.end())
                {
                    axioms.erase(it);
                    viewport_window->set_axioms(axioms, -1);
                    if (event_log_window)
                    {
                        event_log_window->add_entry("Axiom " + std::to_string(selected_axiom) + " deleted.");
                    }
                    // In live mode, regenerate roads
                    if (user_preferences.app().generation_mode == 0 && !axioms.empty())
                    {
                        update_city_generation();
                        record_generation_state();
                    }
                    else if (axioms.empty())
                    {
                        viewport_window->clear_city();
                        has_generated_city = false;
                        road_index.clear();
                    }
                }
            }
            else if (selected_road >= 0)
            {
                // Delete selected road from index (won't affect generated geometry until regen)
                auto it = std::find_if(road_index.begin(), road_index.end(),
                                       [selected_road](const RoadIndexEntry &r)
                                       { return r.id == selected_road; });
                if (it != road_index.end())
                {
                    road_index.erase(it);
                    viewport_window->set_selected_road_id(-1);
                    if (event_log_window)
                    {
                        event_log_window->add_entry("Road " + std::to_string(selected_road) + " removed from index.");
                    }
                }
            }
        }
    }

    void Application::update_city_generation()
    {
        DebugLog::set_enabled(user_preferences.app().enable_debug_logging);
        if (user_preferences.app().enable_debug_logging)
        {
            DebugLog::set_log_file("tests/Debug.log.txt");
            if (event_log_window)
            {
                DebugLog::set_sink([this](const std::string &message)
                                   {
                                       std::string trimmed = message;
                                       while (!trimmed.empty() && (trimmed.back() == '\n' || trimmed.back() == '\r'))
                                       {
                                           trimmed.pop_back();
                                       }
                                       if (!trimmed.empty())
                                       {
                                           event_log_window->add_entry(trimmed);
                                       } });
            }
            else
            {
                DebugLog::set_sink(nullptr);
            }
        }
        else
        {
            DebugLog::set_sink(nullptr);
        }
        DebugLog::printf("[Gen] Start city generation.\n");
        ::CityParams gen_params = make_default_city_params();
        gen_params.width = static_cast<double>(city_params.city_size);
        gen_params.height = static_cast<double>(city_params.city_size);
        apply_ui_params_to_generator(city_params, gen_params);

        // Build tensor field from axioms
        auto field = TensorField::make_tensor_field(gen_params);
        static const std::vector<ViewportWindow::AxiomData> empty_axioms;
        const auto &axioms = viewport_window ? viewport_window->get_axioms() : empty_axioms;

        // Compute effective road limits from per-axiom counts
        uint32_t total_major_roads = 0;
        uint32_t total_minor_roads = 0;
        for (const auto &axiom : axioms)
        {
            if (axiom.fuzzy_mode)
            {
                // Fuzzy mode: use max_roads as upper limit, split 30% major / 70% minor
                int max_r = std::clamp(axiom.max_roads, 1, 500);
                total_major_roads += static_cast<uint32_t>(max_r * 0.3);
                total_minor_roads += static_cast<uint32_t>(max_r * 0.7);
            }
            else
            {
                // Exact mode: use specified counts
                total_major_roads += static_cast<uint32_t>(std::clamp(axiom.major_road_count, 0, 500));
                total_minor_roads += static_cast<uint32_t>(std::clamp(axiom.minor_road_count, 0, 500));
            }
        }

        // Apply per-axiom limits if specified, otherwise use global UI limits
        if (total_major_roads > 0 || total_minor_roads > 0)
        {
            gen_params.maxMajorRoads = std::max(total_major_roads, 1u);
            gen_params.maxTotalRoads = std::max(total_major_roads + total_minor_roads, 1u);
        }
        // When there are no axioms and generate city is pressed, randomly spawn 1 of each axiom type
        if (axioms.empty())
        {
            if (viewport_window)
            {
                // Create random number generator for positioning
                static std::random_device rd;
                static std::mt19937 gen(rd());

                // Define bounds for random placement (within city bounds, avoiding edges)
                const double city_size = static_cast<double>(city_params.city_size);
                const double margin = city_size * 0.2; // 20% margin from edges
                const double placement_range = city_size - 2 * margin;

                std::uniform_real_distribution<double> pos_dist(-placement_range * 0.5, placement_range * 0.5);
                std::uniform_real_distribution<float> size_dist(15.0f, 25.0f); // Random size between 15-25

                std::vector<ViewportWindow::AxiomData> new_axioms;

                // Spawn one axiom of each type: Radial(0), Grid(1), Delta(2), GridCorrective(3)
                for (int axiom_type = 0; axiom_type < 4; ++axiom_type)
                {
                    ImVec2 random_pos{static_cast<float>(pos_dist(gen)), static_cast<float>(pos_dist(gen))};
                    float random_size = size_dist(gen);

                    // Generate a unique ID for the axiom (starting from 1)
                    int next_id = axiom_type + 1;

                    // Create axiom with default settings
                    ViewportWindow::AxiomData new_axiom{
                        next_id,                                        // id
                        random_pos,                                     // position
                        axiom_type,                                     // type (0-3)
                        random_size,                                    // size
                        0.8f,                                           // opacity
                        0,                                              // terminal_type (for Delta fields)
                        0,                                              // mode (for Radial/Block modes)
                        std::string("Auto ") + std::to_string(next_id), // name
                        100 + axiom_type,                               // tag_id
                        5,                                              // major_road_count
                        10,                                             // minor_road_count
                        false,                                          // fuzzy_mode
                        10,                                             // min_roads
                        20,                                             // max_roads
                        0                                               // influencer_type (None)
                    };

                    new_axioms.push_back(new_axiom);
                }

                // Set the axioms in viewport
                viewport_window->set_axioms(new_axioms);

                // Log the auto-generation
                if (event_log_window)
                {
                    event_log_window->add_entry("Auto-generated 4 axioms (1 of each type) for city generation.");
                }

                // Re-fetch the axioms reference since we just updated them
                const auto &axioms_after_spawn = viewport_window->get_axioms();
                if (axioms_after_spawn.empty())
                {
                    // Still no axioms after auto-generation attempt
                    viewport_window->clear_city();
                    has_generated_city = false;
                    generated_city = CityModel::City{};
                    export_window->set_status("City generation skipped (failed to create axioms)");
                    if (event_log_window)
                    {
                        event_log_window->add_entry("City generation skipped (failed to create axioms).");
                    }
                    return;
                }
            }
            else
            {
                // No viewport window, can't add axioms
                has_generated_city = false;
                generated_city = CityModel::City{};
                export_window->set_status("City generation skipped (no viewport)");
                if (event_log_window)
                {
                    event_log_window->add_entry("City generation skipped (no viewport window).");
                }
                return;
            }
        }

        // Re-fetch axioms after potential auto-generation to ensure we use the updated list
        const auto &axioms_for_generation = viewport_window ? viewport_window->get_axioms() : empty_axioms;

        if (event_log_window)
        {
            event_log_window->add_entry("City generation started.");
        }
        std::vector<AxiomInput> axiom_inputs;
        axiom_inputs.reserve(axioms_for_generation.size());

        if (!axioms_for_generation.empty())
        {
            field.clear();
            const double base_size = std::min(gen_params.width, gen_params.height) * 0.25;
            bool has_primary = false;
            bool has_flow = false;
            for (const auto &axiom : axioms_for_generation)
            {
                if (axiom.type == 0 || axiom.type == 1)
                {
                    has_flow = true;
                }
                if (axiom.type == 0 || axiom.type == 1 || axiom.type == 2)
                {
                    has_primary = true;
                }
            }

            if (!has_primary)
            {
                if (viewport_window)
                {
                    viewport_window->clear_city();
                }
                has_generated_city = false;
                generated_city = CityModel::City{};
                export_window->set_status("City generation skipped (no primary axioms)");
                if (event_log_window)
                {
                    event_log_window->add_entry("City generation skipped (no primary axioms).");
                }
                return;
            }

            for (const auto &axiom : axioms_for_generation)
            {
                const double size = std::clamp(static_cast<double>(axiom.size), 2.0, 4096.0);
                const double size_scale = std::clamp(size / 30.0, 0.1, 10.0);
                const double field_size = std::max(20.0, base_size * size_scale);
                const double opacity = std::clamp(static_cast<double>(axiom.opacity), 0.05, 1.0);
                const double decay = std::clamp(0.25 + (1.0 - opacity) * 1.75, 0.25, 2.0);
                CityModel::Vec2 pos{axiom.position.x + gen_params.width * 0.5,
                                    axiom.position.y + gen_params.height * 0.5};

                AxiomInput input{axiom.id, axiom.type, pos, field_size,
                                 static_cast<InfluencerType>(axiom.influencer_type)};
                axiom_inputs.push_back(input);
                if (axiom.type == 0)
                {
                    if (axiom.mode == 0)
                    {
                        field.addRadial(pos, field_size, decay);
                    }
                    else if (axiom.mode == 1)
                    {
                        field.addRadial(pos, field_size, decay * 0.6);
                        field.addGridCorrective(pos, field_size * 0.5, decay * 0.8, 0.0);
                    }
                    else
                    {
                        field.addRadial(pos, field_size, decay);
                        field.addGridCorrective(pos, field_size * 0.6, decay * 0.8, 0.0);
                    }
                }
                else if (axiom.type == 1)
                {
                    TensorField::DeltaTerminal terminal = TensorField::DeltaTerminal::Top;
                    if (axiom.terminal_type == 1)
                        terminal = TensorField::DeltaTerminal::BottomLeft;
                    else if (axiom.terminal_type == 2)
                        terminal = TensorField::DeltaTerminal::BottomRight;
                    field.addDelta(pos, field_size, decay, terminal);
                }
                else if (axiom.type == 2)
                {
                    if (axiom.mode == 0)
                    {
                        field.addSquare(pos, field_size, decay);
                    }
                    else if (axiom.mode == 1)
                    {
                        field.addGridCorrective(pos, field_size, decay, 3.14159265358979323846 / 4.0);
                    }
                    else
                    {
                        field.addSquare(pos, field_size, decay * 0.8);
                        field.addGridCorrective(pos, field_size, decay * 0.6, 0.0);
                    }
                }
                else if (axiom.type == 3 && has_flow)
                {
                    field.addGridCorrective(pos, field_size, decay, 0.0);
                }
            }

            // Detect 4-corner block configuration: when 4 blocks have overlapping influence,
            // create connecting major roads between their centers
            std::vector<std::pair<CityModel::Vec2, double>> block_centers;
            for (const auto &axiom : axioms_for_generation)
            {
                if (axiom.type == 2) // Block type
                {
                    const double size = std::clamp(static_cast<double>(axiom.size), 2.0, 30.0);
                    const double size_scale = std::clamp(size / 30.0, 0.1, 10.0);
                    const double field_size = std::max(20.0, base_size * size_scale);
                    CityModel::Vec2 pos{axiom.position.x + gen_params.width * 0.5,
                                        axiom.position.y + gen_params.height * 0.5};
                    block_centers.push_back({pos, field_size});
                }
            }

            // If we have exactly 4 blocks, check if they form a corner configuration
            if (block_centers.size() == 4)
            {
                // Check if all blocks overlap with at least one other
                bool forms_grid = true;
                for (size_t i = 0; i < 4 && forms_grid; ++i)
                {
                    int overlap_count = 0;
                    for (size_t j = 0; j < 4; ++j)
                    {
                        if (i == j)
                            continue;
                        double dx = block_centers[i].first.x - block_centers[j].first.x;
                        double dy = block_centers[i].first.y - block_centers[j].first.y;
                        double dist = std::sqrt(dx * dx + dy * dy);
                        double combined_radius = block_centers[i].second + block_centers[j].second;
                        if (dist < combined_radius * 1.2)
                        {
                            overlap_count++;
                        }
                    }
                    if (overlap_count < 1)
                        forms_grid = false;
                }

                if (forms_grid)
                {
                    // Add grid corrective spanning all 4 blocks to merge their roads
                    double cx = 0, cy = 0;
                    double max_extent = 0;
                    for (const auto &bc : block_centers)
                    {
                        cx += bc.first.x;
                        cy += bc.first.y;
                        max_extent = std::max(max_extent, bc.second);
                    }
                    cx /= 4.0;
                    cy /= 4.0;
                    // Add a large grid corrective at center to unify the 4 blocks
                    field.addGridCorrective({cx, cy}, max_extent * 2.0, 0.6, 0.0);
                }
            }
        }

        CityModel::City city;
        city.bounds.min = {0.0, 0.0};
        city.bounds.max = {gen_params.width, gen_params.height};

        if (city_params.generate_rivers)
        {
            RK4Integrator waterIntegrator(field, gen_params.water_dstep);
            DebugLog::printf("[Gen] Water generation start.\n");
            city.water = WaterGenerator::generate_water(gen_params, field, waterIntegrator);
            DebugLog::printf("[Gen] Water generation done. water_polylines=%zu\n", city.water.size());
        }
        else
        {
            city.water.clear();
        }
        DebugLog::printf("[Gen] Road generation start.\n");
        RoadGenerator::generate_roads(gen_params, field, city.water, city);
        std::size_t pre_clip_polylines = 0;
        std::size_t pre_clip_segments = 0;
        for (std::size_t i = 0; i < CityModel::road_type_count; ++i)
        {
            pre_clip_polylines += city.roads_by_type[i].size();
            pre_clip_segments += city.segment_roads_by_type[i].size();
        }
        DebugLog::printf("[Gen] Road generation done. polylines=%zu segments=%zu\n",
                         pre_clip_polylines, pre_clip_segments);
        DistrictGenerator::DistrictField district_field;
        DistrictGenerator::Settings district_settings;
        district_settings.grid_resolution = user_preferences.app().district_grid_resolution;
        district_settings.secondary_threshold = user_preferences.app().district_secondary_threshold;
        district_settings.weight_scale = user_preferences.app().district_weight_scale;
        district_settings.use_reaction_diffusion = user_preferences.app().district_rd_mode;
        district_settings.rd_mix = user_preferences.app().district_rd_mix;
        district_settings.desire_weight_axiom = user_preferences.app().district_desire_weight_axiom;
        district_settings.desire_weight_frontage = user_preferences.app().district_desire_weight_frontage;
        district_settings.disable_weight_normalization = true;
        district_settings.desire_score_epsilon = 0.05;
        district_settings.enable_desire_geometry_factor = true;
        district_settings.desire_density_radius = 200.0;
        district_settings.debug_log_desire_scores = user_preferences.app().enable_debug_logging;
        DebugLog::printf("[Gen] District generation start.\n");
        DistrictGenerator::generate(gen_params, axiom_inputs, city, district_field, district_settings, &field);
        DebugLog::printf("[Gen] District generation done. districts=%zu\n", city.districts.size());
        DebugLog::printf("[Gen] Clip roads to districts start.\n");
        DistrictGenerator::clip_roads_to_districts(city, district_field);
        std::size_t post_clip_polylines = 0;
        std::size_t post_clip_segments = 0;
        for (std::size_t i = 0; i < CityModel::road_type_count; ++i)
        {
            post_clip_polylines += city.roads_by_type[i].size();
            post_clip_segments += city.segment_roads_by_type[i].size();
        }
        DebugLog::printf("[Gen] Clip roads to districts done. polylines=%zu segments=%zu\n",
                         post_clip_polylines, post_clip_segments);

        // Build user-placed inputs from viewport data
        CityModel::UserPlacedInputs user_inputs;
        user_inputs.lock_user_types = user_preferences.tools().lock_user_placed_types;
        if (viewport_window)
        {
            const auto &user_lots = viewport_window->get_user_lots();
            for (const auto &lot : user_lots)
            {
                CityModel::UserLotInput lot_input;
                lot_input.position = {lot.position.x + gen_params.width * 0.5,
                                      lot.position.y + gen_params.height * 0.5};
                lot_input.lot_type = lot.lot_type;
                lot_input.locked_type = lot.locked_type;
                user_inputs.lots.push_back(lot_input);
            }
            const auto &user_buildings = viewport_window->get_user_buildings();
            for (const auto &building : user_buildings)
            {
                CityModel::UserBuildingInput building_input;
                building_input.position = {building.position.x + gen_params.width * 0.5,
                                           building.position.y + gen_params.height * 0.5};
                building_input.building_type = building.building_type;
                building_input.locked_type = building.locked_type;
                user_inputs.buildings.push_back(building_input);
            }
        }

        const double half_x_world = gen_params.width * 0.5;
        const double half_y_world = gen_params.height * 0.5;
        for (const auto &road : user_roads)
        {
            CityModel::UserRoadInput road_input;
            road_input.road_type = road.road_type;
            road_input.source_generated_id = road.from_generated ? road.id : -1;

            if (!road.nodes.empty())
            {
                road_input.points.reserve(road.nodes.size());
                for (const auto &pt : road.nodes)
                {
                    road_input.points.push_back({pt.x + half_x_world, pt.y + half_y_world});
                }
            }
            else
            {
                road_input.points.push_back({road.start_x + half_x_world, road.start_y + half_y_world});
                road_input.points.push_back({road.end_x + half_x_world, road.end_y + half_y_world});
            }

            if (road_input.points.size() >= 2)
            {
                user_inputs.roads.push_back(std::move(road_input));
            }
        }

        DebugLog::printf("[Gen] Lot generation start.\n");
        LotGenerator::generate(gen_params, axiom_inputs, city.districts, district_field, field, city, user_inputs);
        DebugLog::printf("[Gen] Lot generation done. lots=%zu blocks=%zu faces=%zu\n",
                         city.lots.size(), city.block_polygons.size(), city.block_faces.size());
        DebugLog::printf("[Gen] Site generation start.\n");
        SiteGenerator::generate(gen_params, city, user_inputs);
        DebugLog::printf("[Gen] Site generation done. sites=%zu\n", city.building_sites.size());

        generated_city = city;
        has_generated_city = true;

        // Populate road_index from generated roads
        road_index.clear();
        int road_id = 0;
        const float half_x = static_cast<float>(gen_params.width * 0.5);
        const float half_y = static_cast<float>(gen_params.height * 0.5);
        auto populate_roads = [&](const std::vector<CityModel::Polyline> &roads, CityModel::RoadType type)
        {
            for (const auto &road : roads)
            {
                RoadIndexEntry entry;
                entry.id = road_id++;
                entry.road_type = type;
                entry.category = RoadCategory::Generated;
                entry.locked = false;
                entry.from_generated = false;
                entry.intersections.clear();
                entry.intersections_computed = false;
                entry.nodes.clear();
                entry.axiom_origin_id = -1;
                entry.axiom_end_id = -1;
                entry.start_x = 0.0f;
                entry.start_y = 0.0f;
                entry.end_x = 0.0f;
                entry.end_y = 0.0f;

                // Determine origin/end axiom by proximity to road endpoints
                if (!road.points.empty())
                {
                    const auto &start = road.points.front();
                    const auto &end = road.points.back();
                    entry.start_x = static_cast<float>(start.x - half_x);
                    entry.start_y = static_cast<float>(start.y - half_y);
                    entry.end_x = static_cast<float>(end.x - half_x);
                    entry.end_y = static_cast<float>(end.y - half_y);
                    float best_start_dist = 1e9f;
                    float best_end_dist = 1e9f;

                    for (const auto &axiom : axioms)
                    {
                        float ax = axiom.position.x + static_cast<float>(gen_params.width * 0.5);
                        float ay = axiom.position.y + static_cast<float>(gen_params.height * 0.5);
                        float field_radius = std::max(20.0f, static_cast<float>(std::min(gen_params.width, gen_params.height) * 0.25 * std::clamp(axiom.size / 30.0f, 0.1f, 10.0f)));

                        float dx_s = static_cast<float>(start.x) - ax;
                        float dy_s = static_cast<float>(start.y) - ay;
                        float dist_s = std::sqrt(dx_s * dx_s + dy_s * dy_s);

                        float dx_e = static_cast<float>(end.x) - ax;
                        float dy_e = static_cast<float>(end.y) - ay;
                        float dist_e = std::sqrt(dx_e * dx_e + dy_e * dy_e);

                        // Check if within influence
                        if (dist_s < field_radius && dist_s < best_start_dist)
                        {
                            best_start_dist = dist_s;
                            entry.axiom_origin_id = axiom.id;
                        }
                        if (dist_e < field_radius && dist_e < best_end_dist)
                        {
                            best_end_dist = dist_e;
                            entry.axiom_end_id = axiom.id;
                        }

                        // Check influence on any point of road
                        for (const auto &pt : road.points)
                        {
                            float px = static_cast<float>(pt.x) - ax;
                            float py = static_cast<float>(pt.y) - ay;
                            float d = std::sqrt(px * px + py * py);
                            if (d < field_radius)
                            {
                                // Check if already in influenced list
                                bool found = false;
                                for (int id : entry.influenced_by_axioms)
                                {
                                    if (id == axiom.id)
                                    {
                                        found = true;
                                        break;
                                    }
                                }
                                if (!found)
                                {
                                    entry.influenced_by_axioms.push_back(axiom.id);
                                }
                                break;
                            }
                        }
                    }
                }

                if (!road.points.empty())
                {
                    entry.nodes.reserve(road.points.size());
                    for (const auto &pt : road.points)
                    {
                        entry.nodes.emplace_back(static_cast<float>(pt.x - half_x),
                                                 static_cast<float>(pt.y - half_y));
                    }
                }

                road_index.push_back(entry);
            }
        };

        const double merge_eps = 1e-3;
        for (auto type : CityModel::generated_road_order)
        {
            const auto &roads = city.roads_by_type[CityModel::road_type_index(type)];
            auto merged = merge_polylines(roads, merge_eps);
            populate_roads(merged, type);
        }

        merge_user_roads_to_index();

        if (viewport_window)
        {
            viewport_window->set_city(generated_city, city_params.maxMajorRoads, city_params.maxTotalRoads);
            viewport_window->set_district_field(district_field);
            sync_user_roads_to_viewport();
        }
        export_window->set_status("City generation complete");
        if (event_log_window)
        {
            event_log_window->add_entry("City generation complete.");
        }
    }

    void Application::handle_export()
    {
        bool export_requested = false;
        int format = export_window->get_export_format();

        if (parameters_window->export_clicked())
        {
            export_requested = true;
            format = 0;
        }

        if (export_window->export_triggered())
        {
            export_requested = true;
            format = export_window->get_export_format();
        }

        if (!export_requested)
        {
            return;
        }

        switch (format)
        {
        case 0: // JSON
            export_json();
            break;
        case 1: // OBJ
            export_obj();
            break;
        case 2: // SVG
            export_svg();
            break;
        }
    }

    void Application::export_json()
    {
        if (!has_generated_city)
        {
            export_window->set_status("JSON export: no city generated");
            if (event_log_window)
            {
                event_log_window->add_entry("JSON export failed: no city generated.");
            }
            return;
        }

        const std::string output_path = city_params.export_path;
        const bool ok = export_city_to_json(generated_city, output_path);
        export_window->set_status(ok ? ("JSON exported to " + output_path)
                                     : ("JSON export failed: " + output_path));
        if (event_log_window)
        {
            event_log_window->add_entry(ok ? ("JSON export: " + output_path)
                                           : ("JSON export failed: " + output_path));
        }
    }

    void Application::export_obj()
    {
        // TODO: Export 3D model to OBJ format
        export_window->set_status("OBJ export: Not yet implemented");
        if (event_log_window)
        {
            event_log_window->add_entry("OBJ export requested (not implemented).");
        }
    }

    void Application::export_svg()
    {
        // TODO: Export 2D map to SVG format
        export_window->set_status("SVG export: Not yet implemented");
        if (event_log_window)
        {
            event_log_window->add_entry("SVG export requested (not implemented).");
        }
    }

    std::string Application::get_temp_dir() const
    {
        return UserPreferences::get_config_dir() + "/temp_state";
    }

    std::string Application::get_snapshot_dir() const
    {
        return UserPreferences::get_config_dir() + "/snapshots";
    }

    void Application::autosave_state()
    {
        if (!viewport_window)
        {
            return;
        }

        const auto &axioms = viewport_window->get_axioms();
        json j;
        j["version"] = 1;
        j["timestamp"] = make_timestamp();
        j["axioms"] = json::array();
        for (const auto &axiom : axioms)
        {
            json entry;
            entry["id"] = axiom.id;
            entry["name"] = axiom.name;
            entry["tag_id"] = axiom.tag_id;
            entry["type"] = axiom.type;
            entry["terminal"] = axiom.terminal_type;
            entry["mode"] = axiom.mode;
            entry["x"] = axiom.position.x;
            entry["y"] = axiom.position.y;
            entry["size"] = axiom.size;
            entry["opacity"] = axiom.opacity;
            j["axioms"].push_back(entry);
        }

        const std::string temp_dir = get_temp_dir();
        std::error_code ec;
        fs::create_directories(temp_dir, ec);

        const int max_versions = 4;
        for (int i = max_versions - 1; i >= 1; --i)
        {
            fs::path from = fs::path(temp_dir) / ("autosave_" + std::to_string(i - 1) + ".json");
            fs::path to = fs::path(temp_dir) / ("autosave_" + std::to_string(i) + ".json");
            if (fs::exists(from, ec))
            {
                fs::remove(to, ec);
                fs::rename(from, to, ec);
            }
        }

        fs::path out_path = fs::path(temp_dir) / "autosave_0.json";
        std::ofstream out(out_path.string(), std::ios::trunc);
        if (out.is_open())
        {
            out << j.dump(2);
            out.close();
            if (event_log_window)
            {
                event_log_window->add_entry("Autosave updated.");
            }
        }
        else if (event_log_window)
        {
            event_log_window->add_entry("Autosave failed to write temp_state.");
        }
    }

    bool Application::save_snapshot(const std::string &reason)
    {
        if (!viewport_window)
        {
            return false;
        }

        const auto &axioms = viewport_window->get_axioms();
        json j;
        j["version"] = 1;
        j["timestamp"] = make_timestamp();
        j["reason"] = reason;
        j["axioms"] = json::array();
        for (const auto &axiom : axioms)
        {
            json entry;
            entry["id"] = axiom.id;
            entry["name"] = axiom.name;
            entry["tag_id"] = axiom.tag_id;
            entry["type"] = axiom.type;
            entry["terminal"] = axiom.terminal_type;
            entry["mode"] = axiom.mode;
            entry["x"] = axiom.position.x;
            entry["y"] = axiom.position.y;
            entry["size"] = axiom.size;
            entry["opacity"] = axiom.opacity;
            j["axioms"].push_back(entry);
        }

        const std::string snapshot_dir = get_snapshot_dir();
        std::error_code ec;
        fs::create_directories(snapshot_dir, ec);

        fs::path out_path = fs::path(snapshot_dir) / ("snapshot_" + make_timestamp() + ".json");
        std::ofstream out(out_path.string(), std::ios::trunc);
        if (out.is_open())
        {
            out << j.dump(2);
            out.close();
            if (event_log_window)
            {
                event_log_window->add_entry("Snapshot saved.");
            }
            return true;
        }

        if (event_log_window)
        {
            event_log_window->add_entry("Snapshot save failed.");
        }
        return false;
    }

    void Application::cleanup_temp_state()
    {
        std::error_code ec;
        fs::remove_all(get_temp_dir(), ec);
        if (event_log_window)
        {
            event_log_window->add_entry("Temp state cleared.");
        }
    }

    void Application::add_user_road(int axiom_id, bool is_major)
    {
        if (!viewport_window)
            return;

        const auto &axioms = viewport_window->get_axioms();
        const ViewportWindow::AxiomData *source_axiom = nullptr;
        for (const auto &ax : axioms)
        {
            if (ax.id == axiom_id)
            {
                source_axiom = &ax;
                break;
            }
        }

        if (!source_axiom)
            return;

        // Create a new user road starting at axiom center
        RoadIndexEntry road;
        road.id = next_user_road_id++;
        road.road_type = is_major ? CityModel::RoadType::M_Major : CityModel::RoadType::M_Minor;
        road.category = RoadCategory::User;
        road.locked = true;
        road.from_generated = false;
        road.intersections.clear();
        road.intersections_computed = false;
        road.nodes.clear();
        road.axiom_origin_id = axiom_id;
        road.axiom_end_id = -1;

        // Position road at axiom with default length extending outward
        float ax = source_axiom->position.x;
        float ay = source_axiom->position.y;
        float road_length = source_axiom->size * 3.0f; // Default length based on axiom size

        road.start_x = ax;
        road.start_y = ay;
        road.end_x = ax + road_length;
        road.end_y = ay;
        road.nodes.push_back(ImVec2(road.start_x, road.start_y));
        road.nodes.push_back(ImVec2(road.end_x, road.end_y));
        road.influenced_by_axioms.push_back(axiom_id);

        user_roads.push_back(road);
        merge_user_roads_to_index();
        sync_user_roads_to_viewport();
    }

    void Application::remove_user_road(int road_id)
    {
        auto it = std::find_if(user_roads.begin(), user_roads.end(),
                               [road_id](const RoadIndexEntry &r)
                               { return r.id == road_id; });
        if (it != user_roads.end())
        {
            bool was_from_generated = it->from_generated;
            user_roads.erase(it);
            // If this was a locked Generated road, also remove it from road_index
            // to prevent it from reappearing when merge_user_roads_to_index() is called
            if (was_from_generated)
            {
                auto gen_it = std::find_if(road_index.begin(), road_index.end(),
                                           [road_id](const RoadIndexEntry &r)
                                           { return r.id == road_id && r.category == RoadCategory::Generated; });
                if (gen_it != road_index.end())
                {
                    road_index.erase(gen_it);
                }
            }
            merge_user_roads_to_index();
            sync_user_roads_to_viewport();
        }
    }

    void Application::merge_user_roads_to_index()
    {
        std::unordered_set<int> user_ids;
        user_ids.reserve(user_roads.size());
        for (const auto &ur : user_roads)
        {
            user_ids.insert(ur.id);
        }

        std::vector<RoadIndexEntry> generated;
        generated.reserve(road_index.size());
        for (const auto &r : road_index)
        {
            if (r.category == RoadCategory::Generated && user_ids.find(r.id) == user_ids.end())
            {
                generated.push_back(r);
            }
        }

        road_index.clear();
        for (const auto &ur : user_roads)
        {
            road_index.push_back(ur);
        }
        for (const auto &gr : generated)
        {
            road_index.push_back(gr);
        }
    }

    void Application::sync_user_roads_to_viewport()
    {
        if (!viewport_window)
        {
            return;
        }
        std::vector<ViewportWindow::RoadOverlay> overlays;
        overlays.reserve(user_roads.size());
        for (const auto &road : user_roads)
        {
            ViewportWindow::RoadOverlay overlay;
            overlay.id = road.id;
            overlay.type = road.road_type;
            overlay.from_generated = road.from_generated;
            if (!road.nodes.empty())
            {
                overlay.points = road.nodes;
            }
            else
            {
                overlay.points = {ImVec2(road.start_x, road.start_y), ImVec2(road.end_x, road.end_y)};
            }
            overlays.push_back(std::move(overlay));
        }
        viewport_window->set_user_roads(overlays);
    }

    bool Application::should_close() const
    {
        return glfwWindowShouldClose(window);
    }

} // namespace RCG
