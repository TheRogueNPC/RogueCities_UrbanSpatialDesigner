#pragma once

#include <cstddef>
#include <vector>

#include "CityModel.h"
#include "CityParams.h"
#include "DistrictGenerator.h"

namespace BlockGenerator
{
    // Block generation implementation mode.
    enum class Mode
    {
        Legacy = 0,  // Original PolygonFinder-based implementation
        GEOSOnly = 1 // GEOS polygonize-based implementation
    };

    struct Settings
    {
        // Minimum polygon area to accept as a face/block.
        // Lowered temporarily for testing to allow inspection of tiny GEOS polygons.
        double min_area{1e-6};
        // Maximum polygon area to accept as a face/block.
        double max_area{1e8};
        // Merge radius for nearby nodes/intersections during graph building.
        double merge_radius{20.0};
        // Snap rounding factor applied to merge_radius for GEOS cleanup.
        double snap_tolerance_factor{0.25};
        double near_miss_tolerance{5.0}; // For T-junction detection
        bool enable_near_miss_splitting{true};
        bool guard_largest_face_removal{true}; // Only remove if touches bounds
        double largest_face_threshold{5.0};    // Size ratio before removal
        bool add_district_borders{true};
        bool verbose_geos_diagnostics{false}; // Detailed GEOS stage logging
    };

    struct Stats
    {
        // Number of road polylines considered as inputs.
        std::size_t road_inputs{0};
        // Number of line segments generated from input roads.
        std::size_t segments{0};
        // Count of unique segment intersections found.
        std::size_t intersections{0};
        // Number of face polygons discovered in planar graph.
        std::size_t faces_found{0};
        // Number of faces that pass closure checks and become blocks.
        std::size_t valid_blocks{0};
        // Cleanup stage stats.
        std::size_t input_lines{0};
        std::size_t snapped_lines{0};
        std::size_t healed_lines{0};
        std::size_t pruned_lines{0};
        std::size_t invalid_polygons{0};
        std::size_t repaired_polygons{0};
        std::size_t skipped_polygons{0};
    };

    // Builds block polygons from city roads and user inputs, optionally returning faces and stats.
    void generate(const CityParams &params,
                  const CityModel::City &city,
                  const CityModel::UserPlacedInputs &user_inputs,
                  const DistrictGenerator::DistrictField &field,
                  std::vector<CityModel::BlockPolygon> &out_polygons,
                  std::vector<CityModel::Polygon> *out_faces,
                  Stats *out_stats = nullptr,
                  const Settings &settings = {});
} // namespace BlockGenerator
