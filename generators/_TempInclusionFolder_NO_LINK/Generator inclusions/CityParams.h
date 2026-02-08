#pragma once

#include <array>
#include <cstdint>
#include "RoadTypes.hpp"

// Basic configuration values used throughout the generator pipeline.
enum class RoadDefinitionMode
{
    BySegment = 0,
    ByPolyline = 1
};

// Generator pipeline phases for debugging/verification.
enum class GeneratorPhase
{
    Roads = 0,
    Districts = 1,
    Blocks = 2,
    Lots = 3,
    Buildings = 4,
    Count = 5
};

// Block generation implementation mode.
enum class BlockGenMode
{
    Legacy = 0,
    GEOSOnly = 1
};

struct RoadTypeParams
{
    // Streamline params
    double dsep{20.0};
    double dtest{15.0};
    double dstep{1.0};
    double dlookahead{40.0};
    double dcirclejoin{5.0};
    double joinangle{0.1};
    int pathIterations{1000};
    int seedTries{300};
    double simplifyTolerance{0.5};
    double collideEarly{0.0};
    bool majorDirection{false};
    bool enabled{true};

    // Graph rules (future use)
    bool pruneDangling{true};
    bool allowDeadEnds{true};
    bool requireDeadEnd{false};
    double minEdgeLength{0.0};
    double maxEdgeLength{0.0};
    uint32_t allowIntersectionsMask{0xFFFFFFFFu};
    double intersectionSpacing{0.0};
};

struct CityParams
{
    double width{1000.0};
    double height{1000.0};
    unsigned int seed{1};

    // Building site generation
    bool randomizeSites{false};
    double bufferUtilityChance{0.35};

    // Lot generation
    int minLotsPerRoadSide{1};        // Minimum lots placed per road per side (1-5)
    double lotSpacingMultiplier{1.5}; // Multiplier for lot spacing (larger = more spread out)

    // Tensor noise
    bool tf_globalNoise{false};
    double tf_noiseSizePark{50.0};
    double tf_noiseAnglePark{0.0};
    double tf_noiseSizeGlobal{100.0};
    double tf_noiseAngleGlobal{0.0};

    // Water params (coastline/river)
    double water_dsep{20};
    double water_dtest{15};
    double water_dstep{1};
    double water_dlookahead{40};
    double water_dcirclejoin{5};
    double water_joinangle{0.1};
    int water_pathIterations{10000};
    int water_seedTries{300};
    double water_simplifyTolerance{10};
    double water_collideEarly{0};
    bool water_coastNoiseEnabled{true};
    double water_coastNoiseSize{30};
    double water_coastNoiseAngle{20};
    bool water_riverNoiseEnabled{true};
    double water_riverNoiseSize{30};
    double water_riverNoiseAngle{20};
    double water_riverBankSize{10};
    double water_riverSize{30};

    // Main road params
    double main_dsep{400};
    double main_dtest{200};
    double main_dstep{1};
    double main_dlookahead{500};
    double main_dcirclejoin{5};
    double main_joinangle{0.1};
    int main_pathIterations{1000};
    int main_seedTries{300};
    double main_simplifyTolerance{0.5};
    double main_collideEarly{0};

    // Major road params
    double major_dsep{100};
    double major_dtest{30};
    double major_dstep{1};
    double major_dlookahead{200};
    double major_dcirclejoin{5};
    double major_joinangle{0.1};
    int major_pathIterations{1000};
    int major_seedTries{300};
    double major_simplifyTolerance{0.5};
    double major_collideEarly{0};

    // Minor road params
    double minor_dsep{20};
    double minor_dtest{15};
    double minor_dstep{1};
    double minor_dlookahead{40};
    double minor_dcirclejoin{5};
    double minor_joinangle{0.1};
    int minor_pathIterations{1000};
    int minor_seedTries{300};
    double minor_simplifyTolerance{0.5};
    double minor_collideEarly{0};

    // Per-type road params (generated types + user types)
    std::array<RoadTypeParams, CityModel::road_type_count> road_type_params{};
    // Block extraction policy
    std::array<bool, CityModel::road_type_count> block_barrier{};
    std::array<bool, CityModel::road_type_count> block_closure{};
    bool debug_use_segment_roads_for_blocks{false};
    double block_snap_tolerance_factor{0.25};
    double merge_radius{20.0};
    bool verbose_geos_diagnostics{false};

    unsigned int maxMajorRoads{1200};
    unsigned int maxTotalRoads{4000};
    double majorToMinorRatio{0.3};
    RoadDefinitionMode roadDefinitionMode{RoadDefinitionMode::ByPolyline};

    // Block generation mode
    BlockGenMode block_gen_mode{BlockGenMode::Legacy};

    // Phase toggles: if false, skip that phase entirely
    std::array<bool, static_cast<int>(GeneratorPhase::Count)> phase_enabled{true, true, true, true, true};
};

// Provides a convenient starting set of parameters.
CityParams make_default_city_params();
