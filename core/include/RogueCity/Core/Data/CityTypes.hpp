#pragma once

#include "RogueCity/Core/Math/Vec2.hpp"
#include <vector>
#include <array>
#include <cstdint>
#include <random>
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
