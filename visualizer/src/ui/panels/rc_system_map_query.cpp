#include "ui/panels/rc_system_map_query.h"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/segment.hpp>

#include <algorithm>
#include <limits>

namespace RC_UI::Panels::SystemMap {

namespace {

using BoostPoint = boost::geometry::model::d2::point_xy<double>;
using BoostPolygon = boost::geometry::model::polygon<BoostPoint>;
using BoostSegment = boost::geometry::model::segment<BoostPoint>;

[[nodiscard]] float DistanceToSegment(
    const RogueCity::Core::Vec2& p,
    const RogueCity::Core::Vec2& a,
    const RogueCity::Core::Vec2& b) {
    const BoostPoint point(p.x, p.y);
    const BoostSegment segment(BoostPoint(a.x, a.y), BoostPoint(b.x, b.y));
    return static_cast<float>(boost::geometry::distance(point, segment));
}

[[nodiscard]] bool PointInPolygon(
    const RogueCity::Core::Vec2& point,
    const std::vector<RogueCity::Core::Vec2>& polygon) {
    if (polygon.size() < 3) {
        return false;
    }

    BoostPolygon boost_polygon{};
    auto& outer = boost_polygon.outer();
    outer.clear();
    outer.reserve(polygon.size() + 1);
    for (const auto& p : polygon) {
        outer.emplace_back(p.x, p.y);
    }
    if (!polygon.front().equals(polygon.back())) {
        outer.emplace_back(polygon.front().x, polygon.front().y);
    }
    boost::geometry::correct(boost_polygon);
    if (boost_polygon.outer().size() < 4) {
        return false;
    }
    return boost::geometry::covered_by(BoostPoint(point.x, point.y), boost_polygon);
}

[[nodiscard]] RogueCity::Core::Vec2 PolygonCentroid(const std::vector<RogueCity::Core::Vec2>& points) {
    RogueCity::Core::Vec2 centroid{};
    if (points.empty()) {
        return centroid;
    }
    for (const auto& p : points) {
        centroid += p;
    }
    centroid /= static_cast<double>(points.size());
    return centroid;
}

} // namespace

const char* SystemsMapKindLabel(RogueCity::Core::Editor::VpEntityKind kind) {
    using RogueCity::Core::Editor::VpEntityKind;
    switch (kind) {
    case VpEntityKind::Road: return "Road";
    case VpEntityKind::District: return "District";
    case VpEntityKind::Lot: return "Lot";
    case VpEntityKind::Building: return "Building";
    case VpEntityKind::Water: return "Water";
    default: return "Unknown";
    }
}

bool BuildSystemsMapBounds(
    const RogueCity::Core::Editor::GlobalState& gs,
    RogueCity::Core::Bounds& out_bounds) {
    bool has_bounds = false;
    if (gs.world_constraints.isValid()) {
        out_bounds.min = {0.0, 0.0};
        out_bounds.max = {
            static_cast<double>(gs.world_constraints.width) * gs.world_constraints.cell_size,
            static_cast<double>(gs.world_constraints.height) * gs.world_constraints.cell_size,
        };
        has_bounds = true;
    }

    auto include_point = [&](const RogueCity::Core::Vec2& p) {
        if (!has_bounds) {
            out_bounds.min = p;
            out_bounds.max = p;
            has_bounds = true;
            return;
        }
        out_bounds.min.x = std::min(out_bounds.min.x, p.x);
        out_bounds.min.y = std::min(out_bounds.min.y, p.y);
        out_bounds.max.x = std::max(out_bounds.max.x, p.x);
        out_bounds.max.y = std::max(out_bounds.max.y, p.y);
    };

    if (!has_bounds) {
        for (const auto& road : gs.roads) {
            for (const auto& p : road.points) {
                include_point(p);
            }
        }
    }

    if (!has_bounds) {
        for (const auto& district : gs.districts) {
            for (const auto& p : district.border) {
                include_point(p);
            }
        }
    }

    return has_bounds && out_bounds.width() > 1e-6 && out_bounds.height() > 1e-6;
}

SystemsMapQueryHit QuerySystemsMapEntity(
    const RogueCity::Core::Editor::GlobalState& gs,
    const RogueCity::Core::Editor::SystemsMapRuntimeState& runtime,
    const RogueCity::Core::Vec2& world_pos,
    float pick_radius) {
    using RogueCity::Core::Editor::VpEntityKind;

    SystemsMapQueryHit best{};
    best.distance = std::numeric_limits<float>::max();

    auto consider = [&](VpEntityKind kind,
                        uint32_t id,
                        const RogueCity::Core::Vec2& anchor,
                        float distance,
                        const char* label) {
        if (distance > pick_radius || distance >= best.distance) {
            return;
        }
        best.valid = true;
        best.kind = kind;
        best.id = id;
        best.anchor = anchor;
        best.distance = distance;
        best.label = label;
    };

    if (runtime.show_roads) {
        for (const auto& road : gs.roads) {
            if (road.points.size() < 2) {
                continue;
            }
            float min_distance = std::numeric_limits<float>::max();
            for (size_t i = 1; i < road.points.size(); ++i) {
                min_distance = std::min(min_distance, DistanceToSegment(world_pos, road.points[i - 1], road.points[i]));
            }
            consider(VpEntityKind::Road, road.id, road.points[road.points.size() / 2], min_distance, "Road");
        }
    }

    if (runtime.show_districts) {
        for (const auto& district : gs.districts) {
            if (district.border.empty()) {
                continue;
            }
            const RogueCity::Core::Vec2 centroid = PolygonCentroid(district.border);
            const bool inside = PointInPolygon(world_pos, district.border);
            const float distance = inside ? 0.0f : static_cast<float>(world_pos.distanceTo(centroid));
            consider(VpEntityKind::District, district.id, centroid, distance, "District");
        }
    }

    if (runtime.show_lots) {
        for (const auto& lot : gs.lots) {
            const RogueCity::Core::Vec2 anchor =
                lot.boundary.empty() ? lot.centroid : PolygonCentroid(lot.boundary);
            const bool inside = !lot.boundary.empty() && PointInPolygon(world_pos, lot.boundary);
            const float distance = inside ? 0.0f : static_cast<float>(world_pos.distanceTo(anchor));
            consider(VpEntityKind::Lot, lot.id, anchor, distance, "Lot");
        }
    }

    if (runtime.show_buildings) {
        for (const auto& building : gs.buildings) {
            const float distance = static_cast<float>(world_pos.distanceTo(building.position));
            consider(VpEntityKind::Building, building.id, building.position, distance, "Building");
        }
    }

    if (runtime.show_water) {
        for (const auto& water : gs.waterbodies) {
            if (water.boundary.size() < 2) {
                continue;
            }
            float min_distance = std::numeric_limits<float>::max();
            for (size_t i = 1; i < water.boundary.size(); ++i) {
                min_distance = std::min(min_distance, DistanceToSegment(world_pos, water.boundary[i - 1], water.boundary[i]));
            }
            const bool inside = water.boundary.size() >= 3 && PointInPolygon(world_pos, water.boundary);
            if (inside) {
                min_distance = 0.0f;
            }
            consider(VpEntityKind::Water, water.id, PolygonCentroid(water.boundary), min_distance, "Water");
        }
    }

    return best;
}

} // namespace RC_UI::Panels::SystemMap
