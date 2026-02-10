#pragma once

#include "RogueCity/Core/Math/Vec2.hpp"
#include <algorithm>
#include <vector>
#include <array>
#include <cstdint>
#include <random>
#include <string>
#include <magic_enum/magic_enum.hpp>

namespace RogueCity::Core {

    // ===== ENUMERATIONS =====

    /// Road classification hierarchy (from highways to driveways)
    enum class RoadType : uint8_t {
        Highway = 0,   // Major intercity routes
        Arterial,      // Major city arteries
        Avenue,        // Wide multi-lane roads
        Boulevard,     // Scenic wide roads
        Street,        // Standard city streets
        Lane,          // Narrow residential streets
        Alleyway,      // Service alleys
        CulDeSac,      // Dead-end streets
        Drive,         // Private roads
        Driveway,      // Property access
        M_Major,       // User-placed major road
        M_Minor,       // User-placed minor road
    };
    constexpr size_t road_type_count = static_cast<size_t>(RoadType::M_Minor) + 1;

    /// District classification based on AESP scoring (from research paper)
    enum class DistrictType : uint8_t {
        Mixed = 0,     // Mixed-use: 0.25(A + E + S + P)
        Residential,   // Residential: 0.60P + 0.20A + 0.10S + 0.10E
        Commercial,    // Commercial: 0.60E + 0.20A + 0.10S + 0.10P
        Civic,         // Civic: 0.50E + 0.20A + 0.10S + 0.20P
        Industrial     // Industrial: 0.60S + 0.25A + 0.10E + 0.05P
    };

    /// Lot type classification (building parcel types)
    enum class LotType : uint8_t {
        None = 0,
        Residential,
        RowhomeCompact,
        RetailStrip,
        MixedUse,
        LogisticsIndustrial,
        CivicCultural,
        LuxuryScenic,
        BufferStrip
    };

    /// Building type classification
    enum class BuildingType : uint8_t {
        None = 0,
        Residential,
        Rowhome,
        Retail,
        MixedUse,
        Industrial,
        Civic,
        Luxury,
        Utility
    };

    // ===== GEOMETRIC PRIMITIVES =====

    /// Ordered list of points forming a path or polyline
    struct Polyline {
        std::vector<Vec2> points;

        [[nodiscard]] size_t size() const { return points.size(); }
        [[nodiscard]] bool empty() const { return points.empty(); }
        void clear() { points.clear(); }
    };

    /// Closed polygon (simple ring, no holes)
    struct Polygon {
        std::vector<Vec2> points;
        uint32_t district_id{ 0 };

        [[nodiscard]] size_t size() const { return points.size(); }
        [[nodiscard]] bool empty() const { return points.empty(); }
    };

    /// Block polygon with holes (for city blocks with courtyards)
    struct BlockPolygon {
        std::vector<Vec2> outer;
        std::vector<std::vector<Vec2>> holes;
        uint32_t district_id{ 0 };

        [[nodiscard]] bool hasHoles() const { return !holes.empty(); }
    };

    /// Road segment with classification
    struct Road {
        std::vector<Vec2> points;
        RoadType type{ RoadType::Street };
        uint32_t id{ 0 };
        bool is_user_created{ false };  // True for M_Major/M_Minor

        [[nodiscard]] double length() const;
        [[nodiscard]] Vec2 startPoint() const { return points.empty() ? Vec2() : points.front(); }
        [[nodiscard]] Vec2 endPoint() const { return points.empty() ? Vec2() : points.back(); }
    };

    // ===== CITY ELEMENTS =====

    /// District (zone with AESP-based classification)
    struct District {
        uint32_t id{ 0 };
        int primary_axiom_id{ -1 };
        int secondary_axiom_id{ -1 };
        DistrictType type{ DistrictType::Mixed };
        std::vector<Vec2> border;
        Vec2 orientation{};
        float budget_allocated{ 0.0f };
        uint32_t projected_population{ 0 };
        float desirability{ 0.0f };

        [[nodiscard]] bool hasAxiom() const { return primary_axiom_id >= 0; }
    };

    /// Lot token (AESP-scored building parcel)
    struct LotToken {
        uint32_t id{ 0 };
        uint32_t district_id{ 0 };
        Vec2 centroid{};
        std::vector<Vec2> boundary;
        RoadType primary_road{ RoadType::Street };
        RoadType secondary_road{ RoadType::Street };

        // AESP scores (Access, Exposure, Serviceability, Privacy)
        float access{ 0.0f };
        float exposure{ 0.0f };
        float serviceability{ 0.0f };
        float privacy{ 0.0f };
        float area{ 0.0f };
        float budget_allocation{ 0.0f };

        LotType lot_type{ LotType::None };
        bool is_user_placed{ false };
        bool locked_type{ false };  // If true, generator won't override lot_type

        [[nodiscard]] float aespScore() const {
            return 0.25f * (access + exposure + serviceability + privacy);
        }
    };

    /// Building site (placed within a lot)
    struct BuildingSite {
        uint32_t id{ 0 };
        uint32_t lot_id{ 0 };
        uint32_t district_id{ 0 };
        Vec2 position{};
        BuildingType type{ BuildingType::None };
        bool is_user_placed{ false };
        bool locked_type{ false };
        float estimated_cost{ 0.0f };
    };

    // AI_INTEGRATION_TAG: V1_PASS1_TASK3_WATER_BODY
    /// Water body types
    enum class WaterType : uint8_t {
        Lake = 0,
        River,
        Ocean,
        Pond
    };

    /// Water body feature (lakes, rivers, oceans)
    struct WaterBody {
        uint32_t id{ 0 };
        WaterType type{ WaterType::Lake };
        std::vector<Vec2> boundary;  // Polygon for lakes, polyline for rivers
        float depth{ 5.0f };          // Meters
        bool generate_shore{ true };  // Generate coastal detail
        bool is_user_placed{ false };
        
        [[nodiscard]] bool empty() const { return boundary.empty(); }
        [[nodiscard]] size_t size() const { return boundary.size(); }
    };

    /// History/legacy overlays used to bias district classification and validation.
    enum class HistoryTag : uint8_t {
        None = 0,
        Brownfield = 1u << 0,   // Legacy dumps / industrial remnants
        SacredSite = 1u << 1,   // No-build cultural zones
        UtilityLegacy = 1u << 2, // Old buried lines/wells
        Contaminated = 1u << 3  // Hazard remediation zones
    };

    [[nodiscard]] constexpr uint8_t ToHistoryMask(HistoryTag tag) {
        return static_cast<uint8_t>(tag);
    }

    [[nodiscard]] constexpr bool HasHistoryTag(uint8_t mask, HistoryTag tag) {
        return (mask & static_cast<uint8_t>(tag)) != 0;
    }

    /// Runtime generation mode selected from terrain/policy diagnostics.
    enum class GenerationMode : uint8_t {
        Standard = 0,
        HillTown,
        ConservationOnly,
        BrownfieldCore,
        CompromisePlan,
        Patchwork
    };

    /// Validation issue type produced after pipeline generation.
    enum class PlanViolationType : uint8_t {
        None = 0,
        NoBuildEncroachment,
        SlopeTooHigh,
        FloodRisk,
        SoilTooWeak,
        PolicyConflict
    };

    /// Entity category attached to plan violations.
    enum class PlanEntityType : uint8_t {
        None = 0,
        Global,
        Road,
        Lot,
        District
    };

    /// Rasterized world constraints sampled by tensor/road and validation stages.
    struct WorldConstraintField {
        int width{ 0 };
        int height{ 0 };
        double cell_size{ 10.0 };

        std::vector<float> slope_degrees;      // Terrain slope in degrees.
        std::vector<uint8_t> flood_mask;       // 0 none, 1 minor, 2 severe.
        std::vector<float> soil_strength;      // 0..1.
        std::vector<uint8_t> no_build_mask;    // 0/1 hard no-build.
        std::vector<float> nature_score;       // 0..1 ecology intensity.
        std::vector<uint8_t> history_tags;     // Bitmask from HistoryTag.

        void resize(int w, int h, double cell) {
            width = std::max(0, w);
            height = std::max(0, h);
            cell_size = cell;
            const size_t cells = static_cast<size_t>(width) * static_cast<size_t>(height);
            slope_degrees.assign(cells, 0.0f);
            flood_mask.assign(cells, 0u);
            soil_strength.assign(cells, 1.0f);
            no_build_mask.assign(cells, 0u);
            nature_score.assign(cells, 0.0f);
            history_tags.assign(cells, 0u);
        }

        [[nodiscard]] bool isValid() const {
            if (width <= 0 || height <= 0 || cell_size <= 0.0) {
                return false;
            }
            const size_t cells = static_cast<size_t>(width) * static_cast<size_t>(height);
            return slope_degrees.size() == cells &&
                flood_mask.size() == cells &&
                soil_strength.size() == cells &&
                no_build_mask.size() == cells &&
                nature_score.size() == cells &&
                history_tags.size() == cells;
        }

        [[nodiscard]] size_t cellCount() const {
            return static_cast<size_t>(width) * static_cast<size_t>(height);
        }

        [[nodiscard]] bool worldToGrid(const Vec2& world, int& gx, int& gy) const {
            if (width <= 0 || height <= 0 || cell_size <= 0.0) {
                gx = 0;
                gy = 0;
                return false;
            }
            gx = static_cast<int>(world.x / cell_size);
            gy = static_cast<int>(world.y / cell_size);
            return gx >= 0 && gx < width && gy >= 0 && gy < height;
        }

        [[nodiscard]] int toIndex(int gx, int gy) const {
            return (gy * width) + gx;
        }

        [[nodiscard]] float sampleSlopeDegrees(const Vec2& world) const {
            int gx = 0;
            int gy = 0;
            if (!worldToGrid(world, gx, gy)) {
                return 0.0f;
            }
            return slope_degrees[static_cast<size_t>(toIndex(gx, gy))];
        }

        [[nodiscard]] uint8_t sampleFloodMask(const Vec2& world) const {
            int gx = 0;
            int gy = 0;
            if (!worldToGrid(world, gx, gy)) {
                return 0u;
            }
            return flood_mask[static_cast<size_t>(toIndex(gx, gy))];
        }

        [[nodiscard]] float sampleSoilStrength(const Vec2& world) const {
            int gx = 0;
            int gy = 0;
            if (!worldToGrid(world, gx, gy)) {
                return 1.0f;
            }
            return soil_strength[static_cast<size_t>(toIndex(gx, gy))];
        }

        [[nodiscard]] bool sampleNoBuild(const Vec2& world) const {
            int gx = 0;
            int gy = 0;
            if (!worldToGrid(world, gx, gy)) {
                return true;
            }
            return no_build_mask[static_cast<size_t>(toIndex(gx, gy))] != 0u;
        }

        [[nodiscard]] float sampleNatureScore(const Vec2& world) const {
            int gx = 0;
            int gy = 0;
            if (!worldToGrid(world, gx, gy)) {
                return 0.0f;
            }
            return nature_score[static_cast<size_t>(toIndex(gx, gy))];
        }

        [[nodiscard]] uint8_t sampleHistoryTags(const Vec2& world) const {
            int gx = 0;
            int gy = 0;
            if (!worldToGrid(world, gx, gy)) {
                return 0u;
            }
            return history_tags[static_cast<size_t>(toIndex(gx, gy))];
        }
    };

    /// Site diagnostics produced from world constraints before city generation.
    struct SiteProfile {
        float buildable_fraction{ 1.0f };
        float average_buildable_slope{ 0.0f };
        float buildable_fragmentation{ 0.0f };
        float policy_friction{ 0.0f };

        bool hostile_terrain{ false };
        bool policy_vs_physics{ false };
        bool awkward_geometry{ false };
        bool brownfield_pockets{ false };

        GenerationMode mode{ GenerationMode::Standard };
    };

    /// Validation finding from post-generation checks.
    struct PlanViolation {
        PlanViolationType type{ PlanViolationType::None };
        PlanEntityType entity_type{ PlanEntityType::None };
        uint32_t entity_id{ 0 };
        float severity{ 0.0f }; // 0..1
        Vec2 location{};
        std::string message;
    };

    // ===== UTILITY STRUCTURES =====

    /// Axis-aligned bounding box
    struct Bounds {
        Vec2 min;
        Vec2 max;

        [[nodiscard]] double width() const { return max.x - min.x; }
        [[nodiscard]] double height() const { return max.y - min.y; }
        [[nodiscard]] Vec2 center() const {
            return Vec2((min.x + max.x) * 0.5, (min.y + max.y) * 0.5);
        }
        [[nodiscard]] bool contains(const Vec2& p) const {
            return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y;
        }
    };

    /// Simple deterministic RNG wrapper
    struct RNG {
        explicit RNG(uint32_t seed = 1) : gen(seed) {}

        double uniform() { return dist01(gen); }
        double uniform(double max) { return dist01(gen) * max; }
        double uniform(double min, double max) { return min + dist01(gen) * (max - min); }
        int uniformInt(int min, int max) {
            return min + static_cast<int>(dist01(gen) * (max - min + 1));
        }

    private:
        std::mt19937_64 gen;
        std::uniform_real_distribution<double> dist01{ 0.0, 1.0 };
    };

    // ===== DEBUG STATISTICS =====

    /// Block extraction debug statistics
    struct BlockDebugStats {
        uint32_t road_inputs{ 0 };
        uint32_t faces_found{ 0 };
        uint32_t valid_blocks{ 0 };
        uint32_t intersections{ 0 };
        uint32_t segments{ 0 };
        uint32_t input_lines{ 0 };
        uint32_t snapped_lines{ 0 };
        uint32_t healed_lines{ 0 };
        uint32_t pruned_lines{ 0 };
        uint32_t invalid_polygons{ 0 };
        uint32_t repaired_polygons{ 0 };
        uint32_t skipped_polygons{ 0 };
    };

    /// Canonical runtime generation parameters (normalized from legacy CityParams)
    struct CityGenerationParams {
        uint32_t seed{ 1 };
        int width{ 2000 };
        int height{ 2000 };
        double cell_size{ 10.0 };

        int min_lot_width{ 10 };
        int max_lot_width{ 50 };
        int min_lot_depth{ 15 };
        int max_lot_depth{ 60 };
        int min_lots_per_road_side{ 1 };

        uint32_t max_road_segments{ 10000 };
        uint32_t max_districts{ 500 };
        uint32_t max_lots{ 50000 };
        uint32_t max_buildings{ 100000 };

        uint32_t rogueworker_threshold{ 100 };
    };

    struct GenerationStats {
        uint32_t roads_generated{ 0 };
        uint32_t districts_generated{ 0 };
        uint32_t lots_generated{ 0 };
        uint32_t buildings_generated{ 0 };
        float generation_time_ms{ 0.0f };
        bool used_parallelization{ false };
    };

} // namespace RogueCity::Core
