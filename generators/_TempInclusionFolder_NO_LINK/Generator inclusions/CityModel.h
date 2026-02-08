#pragma once

#include <vector>
#include <random>
#include <cstdint>
#include <array>

#include "CityParams.h"
#include "RoadTypes.hpp"

struct AxiomInput;

namespace CityModel
{

    // Fundamental 2D point type.
    struct Vec2
    {
        double x{0.0};
        double y{0.0};

        // Mutating helpers to mirror the TS Vector API.
        Vec2 &add(const Vec2 &v);
        Vec2 &sub(const Vec2 &v);
        Vec2 &multiply(const Vec2 &v);
        Vec2 &multiply(double s);
        Vec2 &divide(const Vec2 &v);
        Vec2 &divide(double s);
        Vec2 negated() const;
        Vec2 clone() const;
        Vec2 &set(const Vec2 &v);
        Vec2 &setLength(double len);
        Vec2 &normalize();
        Vec2 &rotateAround(const Vec2 &center, double angle);

        // Queries (non-mutating).
        double dot(const Vec2 &v) const;
        double cross(const Vec2 &v) const;
        double length() const;
        double lengthSquared() const;
        double angle() const;
        double distanceTo(const Vec2 &v) const;
        double distanceToSquared(const Vec2 &v) const;
        bool equals(const Vec2 &v) const;
    };

    // Ordered list of points forming a path or polyline.
    struct Polyline
    {
        std::vector<Vec2> points;
#if RCG_USE_GEOS
        std::vector<unsigned char> geos_wkb;
#endif
    };

    // Closed polygon ring (first point may equal last).
    struct Polygon
    {
        std::vector<Vec2> points;
        uint32_t district_id{0};
    };

    struct BlockPolygon
    {
        std::vector<Vec2> outer;
        std::vector<std::vector<Vec2>> holes;
        uint32_t district_id{0};
    };

    struct Road
    {
        std::vector<Vec2> points;
#if RCG_USE_GEOS
        std::vector<unsigned char> geos_wkb;
#endif
        RoadType type{RoadType::Street};
        uint32_t id{0};
        bool is_user_created{false}; // True for M_Major/M_Minor roads
    };

    enum class DistrictType : uint8_t
    {
        Mixed = 0,
        Residential,
        Commercial,
        Civic,
        Industrial
    };

    struct District
    {
        uint32_t id{0};
        int primary_axiom_id{-1};
        int secondary_axiom_id{-1};
        DistrictType type{DistrictType::Mixed};
        std::vector<Vec2> border;
        Vec2 orientation{};
    };

    enum class LotType : uint8_t
    {
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

    enum class BuildingType : uint8_t
    {
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

    struct LotToken
    {
        uint32_t id{0};
        uint32_t district_id{0};
        Vec2 centroid{};
        RoadType primary_road{RoadType::Street};
        RoadType secondary_road{RoadType::Street};
        float access{0.0f};
        float exposure{0.0f};
        float serviceability{0.0f};
        float privacy{0.0f};
        LotType lot_type{LotType::None};
        bool is_user_placed{false}; // True if placed by user
        bool locked_type{false};    // If true, generator won't override lot_type
    };

    struct BuildingSite
    {
        uint32_t id{0};
        uint32_t lot_id{0};
        uint32_t district_id{0};
        Vec2 position{};
        BuildingType type{BuildingType::None};
        bool is_user_placed{false}; // True if placed by user
        bool locked_type{false};    // If true, generator won't override type
    };

    struct BlockDebugStats
    {
        uint32_t road_inputs{0};
        uint32_t faces_found{0};
        uint32_t valid_blocks{0};
        uint32_t intersections{0};
        uint32_t segments{0};
        uint32_t input_lines{0};
        uint32_t snapped_lines{0};
        uint32_t healed_lines{0};
        uint32_t pruned_lines{0};
        uint32_t invalid_polygons{0};
        uint32_t repaired_polygons{0};
        uint32_t skipped_polygons{0};
    };

    // User-placed lot input for the generator
    struct UserLotInput
    {
        Vec2 position{};
        LotType lot_type{LotType::Residential};
        bool locked_type{false}; // If true, lot type is always preserved
    };

    // User-placed building input for the generator
    struct UserBuildingInput
    {
        Vec2 position{};
        BuildingType building_type{BuildingType::Residential};
        bool locked_type{false}; // If true, building type is always preserved
    };

    // User-placed road input for block extraction
    struct UserRoadInput
    {
        std::vector<Vec2> points;
        RoadType road_type{RoadType::Street};
        int source_generated_id{-1}; // Road index id if derived from generated geometry
    };

    // Axis-aligned bounding box for the generated city.
    struct Bounds
    {
        Vec2 min;
        Vec2 max;
    };

    // Simple deterministic RNG wrapper.
    struct RNG
    {
        explicit RNG(unsigned int seed = 1);
        double uniform();                       // [0,1)
        double uniform(double max);             // [0,max)
        double uniform(double min, double max); // [min,max)
        int uniformInt(int min, int max);       // inclusive min, inclusive max
    private:
        std::mt19937_64 gen;
        std::uniform_real_distribution<double> dist01;
    };

    // Aggregated result of the full pipeline.
    struct City
    {
        Bounds bounds;
        std::vector<Polyline> water;
        std::array<std::vector<Polyline>, road_type_count> roads_by_type;
        std::array<std::vector<Road>, road_type_count> segment_roads_by_type;
        std::vector<District> districts;
        std::vector<LotToken> lots;
        std::vector<BuildingSite> building_sites;
        std::vector<BlockPolygon> block_polygons;
        std::vector<Polygon> block_faces;
        BlockDebugStats block_stats;
    };

    // Input bundle for user-placed elements
    struct UserPlacedInputs
    {
        std::vector<UserLotInput> lots;
        std::vector<UserBuildingInput> buildings;
        std::vector<UserRoadInput> roads;
        bool lock_user_types{true}; // Global toggle: if false, generator may override types
    };

    // Utility math helpers.
    double distance(const Vec2 &a, const Vec2 &b);
    Vec2 lerp(const Vec2 &a, const Vec2 &b, double t);
    double angleBetween(const Vec2 &v1, const Vec2 &v2); // -pi to pi
    bool isLeft(const Vec2 &linePoint, const Vec2 &lineDir, const Vec2 &point);

    // High-level orchestration entry point.
    City generate_city(const CityParams &params,
                       const std::vector<AxiomInput> &axioms,
                       const UserPlacedInputs &user_inputs = {});

} // namespace CityModel
