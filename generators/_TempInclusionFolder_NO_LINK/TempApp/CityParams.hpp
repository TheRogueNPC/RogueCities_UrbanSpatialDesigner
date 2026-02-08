#pragma once

#include <nlohmann/json.hpp>
#include <cstdint>
#include <array>
#include "RoadTypes.hpp"

namespace RCG
{

    /**
     * @struct CityParams
     * @brief Contains all procedural city generation parameters
     *
     * These parameters control how the city generator creates roads, buildings, and other structures.
     * Parameters are serializable to JSON for export and reproducibility.
     */
    struct CityParams
    {
        enum class RoadDefinitionMode
        {
            BySegment = 0,
            ByPolyline = 1
        };
        // Basic parameters
        float noise_scale = 1.0f;  ///< Perlin noise frequency (lower = smoother)
        int city_size = 1024;      ///< Canvas size in pixels
        float road_density = 0.5f; ///< Probability of road placement (0.0-1.0)

        // Road generation
        int major_road_iterations = 50;     ///< Number of major road growth iterations
        int minor_road_iterations = 25;     ///< Number of minor road growth iterations
        float vertex_snap_distance = 15.0f; ///< Distance threshold for snapping vertices to existing roads
        uint32_t maxMajorRoads = 1200;      ///< Maximum number of major roads
        uint32_t maxTotalRoads = 4000;      ///< Maximum number of total roads (major + minor)
        float majorToMinorRatio = 0.3f;     ///< Ratio of major roads when auto computing limits
        RoadDefinitionMode roadDefinitionMode = RoadDefinitionMode::ByPolyline;

        // Rivers and water
        bool generate_rivers = false; ///< Whether to generate natural water features
        int river_count = 3;          ///< Number of river systems to generate
        float river_width = 10.0f;    ///< Width of river channels in pixels

        // Building generation
        float building_density = 0.8f;      ///< Density of buildings (0.0-1.0)
        float max_building_height = 50.0f;  ///< Maximum height of buildings
        int lot_subdivision_iterations = 3; ///< How many times to subdivide building lots

        // Lot generation
        int minLotsPerRoadSide = 1;        ///< Minimum lots placed per road per side (1-5)
        float lotSpacingMultiplier = 1.5f; ///< Multiplier for lot spacing (0.5-3.0)

        // Growth rules (phase 2+)
        int radial_rule_weight = 1;  ///< Weight of radial growth rule
        int grid_rule_weight = 1;    ///< Weight of grid growth rule
        int organic_rule_weight = 1; ///< Weight of organic growth rule

        // Random seed for reproducibility
        unsigned int seed = 1; ///< Random seed for generation (1-999999)

        // Phase toggles: if false, skip that phase entirely (Roads, Districts, Blocks, Lots, Buildings)
        std::array<bool, 5> phase_enabled = {true, true, true, true, true};

        // Block generation mode (UI uses integer indices for ImGui controls)
        int block_gen_mode = 0; // 0 = Legacy, 1 = GEOSOnly

        // Block extraction policy
        std::array<bool, CityModel::road_type_count> block_barrier = []()
        {
            std::array<bool, CityModel::road_type_count> flags{};
            flags.fill(true);
            return flags;
        }();
        std::array<bool, CityModel::road_type_count> block_closure = []()
        {
            std::array<bool, CityModel::road_type_count> flags{};
            flags.fill(true);
            flags[CityModel::road_type_index(CityModel::RoadType::Highway)] = false;
            flags[CityModel::road_type_index(CityModel::RoadType::Arterial)] = false;
            return flags;
        }();

        // Debug toggles
        bool debug_use_segment_roads_for_blocks = false;
        float block_snap_tolerance_factor = 0.25f; ///< Multiplier for block snap rounding grid
        float merge_radius = 20.0f;                ///< Radius for merging nearby points in block generation
        bool verbose_geos_diagnostics = false;     ///< Enable verbose GEOS logging

        // Export settings
        std::string export_path = "build/Debug/city.json"; ///< Default export location
    };

    // JSON serialization for CityParams
    inline void to_json(nlohmann::json &j, const CityParams &p)
    {
        auto to_toggle_json = [](const std::array<bool, CityModel::road_type_count> &flags)
        {
            nlohmann::json out = nlohmann::json::object();
            for (std::size_t i = 0; i < CityModel::road_type_count; ++i)
            {
                auto type = static_cast<CityModel::RoadType>(i);
                out[CityModel::road_type_key(type)] = flags[i];
            }
            return out;
        };

        j = nlohmann::json{
            {"noise_scale", p.noise_scale},
            {"city_size", p.city_size},
            {"road_density", p.road_density},
            {"major_road_iterations", p.major_road_iterations},
            {"minor_road_iterations", p.minor_road_iterations},
            {"vertex_snap_distance", p.vertex_snap_distance},
            {"max_major_roads", p.maxMajorRoads},
            {"max_total_roads", p.maxTotalRoads},
            {"major_to_minor_ratio", p.majorToMinorRatio},
            {"road_definition_mode", static_cast<int>(p.roadDefinitionMode)},
            {"generate_rivers", p.generate_rivers},
            {"river_count", p.river_count},
            {"river_width", p.river_width},
            {"building_density", p.building_density},
            {"max_building_height", p.max_building_height},
            {"lot_subdivision_iterations", p.lot_subdivision_iterations},
            {"min_lots_per_road_side", p.minLotsPerRoadSide},
            {"lot_spacing_multiplier", p.lotSpacingMultiplier},
            {"radial_rule_weight", p.radial_rule_weight},
            {"grid_rule_weight", p.grid_rule_weight},
            {"organic_rule_weight", p.organic_rule_weight},
            {"seed", p.seed},
            {"block_barrier", to_toggle_json(p.block_barrier)},
            {"block_closure", to_toggle_json(p.block_closure)},
            {"debug_use_segment_roads_for_blocks", p.debug_use_segment_roads_for_blocks},
            {"block_snap_tolerance_factor", p.block_snap_tolerance_factor},
            {"merge_radius", p.merge_radius},
            {"verbose_geos_diagnostics", p.verbose_geos_diagnostics},
            {"export_path", p.export_path}};
    }

    inline void from_json(const nlohmann::json &j, CityParams &p)
    {
        auto read_toggle_json = [](std::array<bool, CityModel::road_type_count> &flags, const nlohmann::json &src)
        {
            if (!src.is_object())
                return;
            for (std::size_t i = 0; i < CityModel::road_type_count; ++i)
            {
                auto type = static_cast<CityModel::RoadType>(i);
                const char *key = CityModel::road_type_key(type);
                if (src.contains(key))
                {
                    flags[i] = src[key].get<bool>();
                }
            }
        };

        j.at("noise_scale").get_to(p.noise_scale);
        j.at("city_size").get_to(p.city_size);
        j.at("road_density").get_to(p.road_density);
        j.at("major_road_iterations").get_to(p.major_road_iterations);
        j.at("minor_road_iterations").get_to(p.minor_road_iterations);
        j.at("vertex_snap_distance").get_to(p.vertex_snap_distance);
        if (j.contains("max_major_roads"))
        {
            j.at("max_major_roads").get_to(p.maxMajorRoads);
        }
        if (j.contains("max_total_roads"))
        {
            j.at("max_total_roads").get_to(p.maxTotalRoads);
        }
        if (j.contains("major_to_minor_ratio"))
        {
            j.at("major_to_minor_ratio").get_to(p.majorToMinorRatio);
        }
        if (j.contains("road_definition_mode"))
        {
            int mode = 0;
            j.at("road_definition_mode").get_to(mode);
            p.roadDefinitionMode = (mode == 0) ? CityParams::RoadDefinitionMode::BySegment
                                               : CityParams::RoadDefinitionMode::ByPolyline;
        }
        j.at("generate_rivers").get_to(p.generate_rivers);
        j.at("river_count").get_to(p.river_count);
        j.at("river_width").get_to(p.river_width);
        j.at("building_density").get_to(p.building_density);
        j.at("max_building_height").get_to(p.max_building_height);
        j.at("lot_subdivision_iterations").get_to(p.lot_subdivision_iterations);
        if (j.contains("min_lots_per_road_side"))
        {
            j.at("min_lots_per_road_side").get_to(p.minLotsPerRoadSide);
        }
        if (j.contains("lot_spacing_multiplier"))
        {
            j.at("lot_spacing_multiplier").get_to(p.lotSpacingMultiplier);
        }
        j.at("radial_rule_weight").get_to(p.radial_rule_weight);
        j.at("grid_rule_weight").get_to(p.grid_rule_weight);
        j.at("organic_rule_weight").get_to(p.organic_rule_weight);
        if (j.contains("seed"))
        {
            j.at("seed").get_to(p.seed);
        }
        if (j.contains("block_barrier"))
        {
            read_toggle_json(p.block_barrier, j.at("block_barrier"));
        }
        if (j.contains("block_closure"))
        {
            read_toggle_json(p.block_closure, j.at("block_closure"));
        }
        if (j.contains("debug_use_segment_roads_for_blocks"))
        {
            j.at("debug_use_segment_roads_for_blocks").get_to(p.debug_use_segment_roads_for_blocks);
        }
        if (j.contains("block_snap_tolerance_factor"))
        {
            j.at("block_snap_tolerance_factor").get_to(p.block_snap_tolerance_factor);
        }
        if (j.contains("merge_radius"))
        {
            j.at("merge_radius").get_to(p.merge_radius);
        }
        if (j.contains("verbose_geos_diagnostics"))
        {
            j.at("verbose_geos_diagnostics").get_to(p.verbose_geos_diagnostics);
        }
        if (j.contains("export_path"))
        {
            j.at("export_path").get_to(p.export_path);
        }
    }

} // namespace RCG
