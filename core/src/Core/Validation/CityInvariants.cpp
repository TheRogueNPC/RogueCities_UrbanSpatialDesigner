#include "RogueCity/Core/Validation/CityInvariants.hpp"

#include "RogueCity/Core/Data/CityTypes.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_set>
#include <vector>

namespace RogueCity::Core::Validation {

namespace {

[[nodiscard]] bool IsFinite(const Vec2& v) {
    return std::isfinite(v.x) && std::isfinite(v.y);
}

[[nodiscard]] double SignedArea(const std::vector<Vec2>& pts) {
    if (pts.size() < 3u) {
        return 0.0;
    }
    double area = 0.0;
    for (size_t i = 0; i < pts.size(); ++i) {
        const Vec2& a = pts[i];
        const Vec2& b = pts[(i + 1u) % pts.size()];
        area += (a.x * b.y) - (b.x * a.y);
    }
    return area * 0.5;
}

void Append(std::vector<InvariantViolation>& dst, std::vector<InvariantViolation>&& src) {
    dst.insert(dst.end(),
               std::make_move_iterator(src.begin()),
               std::make_move_iterator(src.end()));
}

InvariantViolation MakeViolation(
    std::string category,
    std::string rule_id,
    std::string description,
    uint32_t entity_id,
    ViolationSeverity severity)
{
    InvariantViolation v{};
    v.category    = std::move(category);
    v.rule_id     = std::move(rule_id);
    v.description = std::move(description);
    v.entity_id   = entity_id;
    v.severity    = severity;
    return v;
}

} // namespace

// ---------------------------------------------------------------------------
// InvariantCheckResult helpers
// ---------------------------------------------------------------------------

size_t InvariantCheckResult::ErrorCount() const {
    return static_cast<size_t>(std::count_if(
        violations.begin(), violations.end(),
        [](const InvariantViolation& v) {
            return v.severity == ViolationSeverity::Error;
        }));
}

size_t InvariantCheckResult::WarningCount() const {
    return static_cast<size_t>(std::count_if(
        violations.begin(), violations.end(),
        [](const InvariantViolation& v) {
            return v.severity == ViolationSeverity::Warning;
        }));
}

// ---------------------------------------------------------------------------
// CheckRoads
// ---------------------------------------------------------------------------

std::vector<InvariantViolation> CityInvariantsChecker::CheckRoads(
    const std::vector<Core::Road>& roads,
    const InvariantCheckOptions& opts) const
{
    std::vector<InvariantViolation> out;

    for (const auto& road : roads) {
        if (road.points.size() < 2u) {
            out.push_back(MakeViolation(
                "Roads", "road.no_geometry",
                "Road has fewer than 2 points and no geometry",
                road.id, ViolationSeverity::Error));
            continue; // remaining checks require at least 2 points
        }

        for (size_t i = 0; i < road.points.size(); ++i) {
            if (!IsFinite(road.points[i])) {
                out.push_back(MakeViolation(
                    "Roads", "road.nonfinite_coord",
                    "Road contains a non-finite coordinate (NaN or Inf)",
                    road.id, ViolationSeverity::Error));
                break; // one violation per road for this category
            }
        }

        for (size_t i = 0; i + 1u < road.points.size(); ++i) {
            const double dx = road.points[i + 1u].x - road.points[i].x;
            const double dy = road.points[i + 1u].y - road.points[i].y;
            const double len = std::sqrt(dx * dx + dy * dy);
            if (len < opts.road_min_edge_length_m) {
                out.push_back(MakeViolation(
                    "Roads", "road.degenerate_edge",
                    "Road segment is shorter than the minimum allowed edge length",
                    road.id, ViolationSeverity::Warning));
                break; // one warning per road
            }
        }
    }

    return out;
}

// ---------------------------------------------------------------------------
// CheckDistricts
// ---------------------------------------------------------------------------

std::vector<InvariantViolation> CityInvariantsChecker::CheckDistricts(
    const std::vector<Core::District>& districts,
    const InvariantCheckOptions& opts) const
{
    std::vector<InvariantViolation> out;

    for (const auto& district : districts) {
        if (district.border.size() < opts.district_min_vertices) {
            out.push_back(MakeViolation(
                "Districts", "district.insufficient_vertices",
                "District polygon has fewer vertices than required minimum",
                district.id, ViolationSeverity::Error));
            continue;
        }

        bool has_nonfinite = false;
        for (const auto& pt : district.border) {
            if (!IsFinite(pt)) {
                has_nonfinite = true;
                break;
            }
        }
        if (has_nonfinite) {
            out.push_back(MakeViolation(
                "Districts", "district.nonfinite_coord",
                "District border contains a non-finite coordinate (NaN or Inf)",
                district.id, ViolationSeverity::Error));
            continue;
        }

        const double area = std::abs(SignedArea(district.border));
        if (area < 1e-4) {
            out.push_back(MakeViolation(
                "Districts", "district.zero_area",
                "District polygon has near-zero area",
                district.id, ViolationSeverity::Warning));
        }
    }

    return out;
}

// ---------------------------------------------------------------------------
// CheckLots
// ---------------------------------------------------------------------------

std::vector<InvariantViolation> CityInvariantsChecker::CheckLots(
    const std::vector<Core::LotToken>& lots,
    const std::vector<Core::District>& districts,
    const InvariantCheckOptions& opts) const
{
    std::vector<InvariantViolation> out;

    std::unordered_set<uint32_t> district_ids;
    if (opts.check_lot_district_membership) {
        district_ids.reserve(districts.size());
        for (const auto& d : districts) {
            district_ids.insert(d.id);
        }
    }

    for (const auto& lot : lots) {
        if (lot.boundary.size() < 3u) {
            out.push_back(MakeViolation(
                "Lots", "lot.degenerate_boundary",
                "Lot boundary has fewer than 3 vertices",
                lot.id, ViolationSeverity::Error));
            // still check membership even for degenerate boundaries
        } else {
            bool has_nonfinite = false;
            for (const auto& pt : lot.boundary) {
                if (!IsFinite(pt)) {
                    has_nonfinite = true;
                    break;
                }
            }
            if (has_nonfinite) {
                out.push_back(MakeViolation(
                    "Lots", "lot.nonfinite_coord",
                    "Lot boundary contains a non-finite coordinate (NaN or Inf)",
                    lot.id, ViolationSeverity::Error));
            }
        }

        if (opts.check_lot_district_membership &&
            district_ids.find(lot.district_id) == district_ids.end())
        {
            out.push_back(MakeViolation(
                "Lots", "lot.unknown_district",
                "Lot references a district ID that does not exist",
                lot.id, ViolationSeverity::Error));
        }
    }

    return out;
}

// ---------------------------------------------------------------------------
// CheckBuildings
// ---------------------------------------------------------------------------

std::vector<InvariantViolation> CityInvariantsChecker::CheckBuildings(
    const siv::Vector<Core::BuildingSite>& buildings,
    const std::vector<Core::LotToken>& lots,
    const InvariantCheckOptions& opts) const
{
    std::vector<InvariantViolation> out;

    std::unordered_set<uint32_t> lot_ids;
    lot_ids.reserve(lots.size());
    for (const auto& lot : lots) {
        lot_ids.insert(lot.id);
    }

    for (const auto& building : buildings) {

        if (!IsFinite(building.position)) {
            out.push_back(MakeViolation(
                "Buildings", "building.nonfinite_position",
                "Building has a non-finite position (NaN or Inf)",
                building.id, ViolationSeverity::Error));
        }

        if (lot_ids.find(building.lot_id) == lot_ids.end()) {
            out.push_back(MakeViolation(
                "Buildings", "building.unknown_lot",
                "Building references a lot ID that does not exist",
                building.id, ViolationSeverity::Error));
        }

        if (opts.check_building_fbcz && building.fbcz_height_max > 0.0f) {
            if (building.suggested_height < building.fbcz_height_min ||
                building.suggested_height > building.fbcz_height_max)
            {
                out.push_back(MakeViolation(
                    "Buildings", "building.fbcz_height_violation",
                    "Building suggested_height is outside the FBCZ allowed range",
                    building.id, ViolationSeverity::Warning));
            }
        }
    }

    return out;
}

// ---------------------------------------------------------------------------
// Check (full pass)
// ---------------------------------------------------------------------------

InvariantCheckResult CityInvariantsChecker::Check(
    const std::vector<Core::Road>&          roads,
    const std::vector<Core::District>&      districts,
    const std::vector<Core::LotToken>&      lots,
    const siv::Vector<Core::BuildingSite>&  buildings,
    const InvariantCheckOptions&            opts) const
{
    InvariantCheckResult result{};
    Append(result.violations, CheckRoads(roads, opts));
    Append(result.violations, CheckDistricts(districts, opts));
    Append(result.violations, CheckLots(lots, districts, opts));
    Append(result.violations, CheckBuildings(buildings, lots, opts));
    return result;
}

} // namespace RogueCity::Core::Validation
