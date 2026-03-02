/**
 * @file EditorIntegrity.cpp
 * @brief Implements validation routines for editor entities in RogueCities Urban Spatial Designer.
 *
 * Contains functions to validate the integrity of roads, districts, lots, and buildings within the editor's global state.
 * These checks are intentionally lightweight and non-fatal, allowing for incomplete geometry during editing.
 * Future implementations may provide richer diagnostics and spatial integrity checks (such as overlaps, containment, and adjacency).
 *
 * Functions:
 * - ValidateRoads: Validates the collection of roads for basic integrity.
 * - ValidateDistricts: Validates the collection of districts.
 * - ValidateLots: Validates the collection of lot tokens.
 * - ValidateBuildings: Validates the collection of building sites.
 * - ValidateAll: Runs all validation routines on the editor's global state.
 * - SpatialCheckAll: Placeholder for spatial integrity checks on the editor's global state.
 */
 
#include "RogueCity/Core/Validation/EditorIntegrity.hpp"
#include "RogueCity/Core/Validation/EditorOverlayValidation.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <string_view>
#include <unordered_set>

namespace RogueCity::Core::Validation {

namespace {

constexpr std::string_view kEntityPrefix = "[Integrity/Entity] ";
constexpr std::string_view kSpatialPrefix = "[Integrity/Spatial] ";

[[nodiscard]] bool IsFiniteVec(const Vec2& v) {
    return std::isfinite(v.x) && std::isfinite(v.y);
}

[[nodiscard]] double SignedArea(const std::vector<Vec2>& points) {
    if (points.size() < 3u) {
        return 0.0;
    }
    double area = 0.0;
    for (size_t i = 0; i < points.size(); ++i) {
        const Vec2& a = points[i];
        const Vec2& b = points[(i + 1u) % points.size()];
        area += (a.x * b.y) - (b.x * a.y);
    }
    return area * 0.5;
}

[[nodiscard]] std::string PrefixMessage(std::string_view prefix, std::string message) {
    std::string tagged(prefix);
    tagged += std::move(message);
    return tagged;
}

void AddViolation(
    IntegrityReport& report,
    std::string_view prefix,
    PlanEntityType entity_type,
    uint32_t entity_id,
    float severity,
    const Vec2& location,
    std::string message) {
    PlanViolation violation{};
    violation.type = PlanViolationType::PolicyConflict;
    violation.entity_type = entity_type;
    violation.entity_id = entity_id;
    violation.severity = std::clamp(severity, 0.0f, 1.0f);
    violation.location = location;
    violation.message = PrefixMessage(prefix, std::move(message));
    report.violations.push_back(std::move(violation));
    if (severity >= 0.55f) {
        report.error_count += 1u;
    } else {
        report.warning_count += 1u;
    }
}

void PruneTaggedViolations(std::vector<PlanViolation>& violations, std::string_view prefix) {
    violations.erase(
        std::remove_if(
            violations.begin(),
            violations.end(),
            [prefix](const PlanViolation& violation) {
                return violation.message.rfind(prefix.data(), 0u) == 0u;
            }),
        violations.end());
}

[[nodiscard]] bool IsKnownSpatialOverlayMessage(const std::string& message) {
    return message == "Lot area below minimum threshold" ||
        message == "Building references missing lot" ||
        message == "Building footprint is outside its lot boundary" ||
        message == "Road intersection requires review";
}

[[nodiscard]] PlanEntityType ToPlanEntityType(Editor::VpEntityKind kind) {
    switch (kind) {
        case Editor::VpEntityKind::Road: return PlanEntityType::Road;
        case Editor::VpEntityKind::District: return PlanEntityType::District;
        case Editor::VpEntityKind::Lot: return PlanEntityType::Lot;
        default: return PlanEntityType::Global;
    }
}

[[nodiscard]] float ToPlanSeverity(Editor::ValidationSeverity severity) {
    switch (severity) {
        case Editor::ValidationSeverity::Critical: return 0.95f;
        case Editor::ValidationSeverity::Error: return 0.70f;
        case Editor::ValidationSeverity::Warning:
        default:
            return 0.35f;
    }
}

[[nodiscard]] IntegrityReport CollectRoadIntegrity(const fva::Container<Road>& roads, std::string_view prefix) {
    IntegrityReport report{};
    std::unordered_set<uint32_t> ids;
    ids.reserve(roads.size());
    for (const Road& road : roads) {
        const Vec2 location = road.points.empty() ? Vec2{} : road.points.front();
        if (road.id == 0u) {
            AddViolation(report, prefix, PlanEntityType::Road, road.id, 0.70f, location, "Road has zero id");
        } else if (!ids.insert(road.id).second) {
            AddViolation(report, prefix, PlanEntityType::Road, road.id, 0.95f, location, "Duplicate road id");
        }
        if (road.points.size() < 2u) {
            AddViolation(report, prefix, PlanEntityType::Road, road.id, 0.70f, location, "Road has fewer than two points");
        }
        for (const Vec2& point : road.points) {
            if (!IsFiniteVec(point)) {
                AddViolation(report, prefix, PlanEntityType::Road, road.id, 0.95f, location, "Road contains non-finite coordinates");
                break;
            }
        }
    }
    return report;
}

[[nodiscard]] IntegrityReport CollectDistrictIntegrity(
    const fva::Container<District>& districts,
    std::string_view prefix) {
    IntegrityReport report{};
    std::unordered_set<uint32_t> ids;
    ids.reserve(districts.size());
    for (const District& district : districts) {
        const Vec2 location = district.border.empty() ? Vec2{} : district.border.front();
        if (district.id == 0u) {
            AddViolation(report, prefix, PlanEntityType::District, district.id, 0.70f, location, "District has zero id");
        } else if (!ids.insert(district.id).second) {
            AddViolation(report, prefix, PlanEntityType::District, district.id, 0.95f, location, "Duplicate district id");
        }
        if (district.border.size() < 3u) {
            AddViolation(report, prefix, PlanEntityType::District, district.id, 0.70f, location, "District border has fewer than three points");
        }
        for (const Vec2& point : district.border) {
            if (!IsFiniteVec(point)) {
                AddViolation(report, prefix, PlanEntityType::District, district.id, 0.95f, location, "District contains non-finite coordinates");
                break;
            }
        }
        if (district.border.size() >= 3u && std::abs(SignedArea(district.border)) <= 1e-6) {
            AddViolation(report, prefix, PlanEntityType::District, district.id, 0.55f, location, "District border has near-zero area");
        }
    }
    return report;
}

[[nodiscard]] IntegrityReport CollectLotIntegrity(const fva::Container<LotToken>& lots, std::string_view prefix) {
    IntegrityReport report{};
    std::unordered_set<uint32_t> ids;
    ids.reserve(lots.size());
    for (const LotToken& lot : lots) {
        const Vec2 location = lot.boundary.empty() ? lot.centroid : lot.boundary.front();
        if (lot.id == 0u) {
            AddViolation(report, prefix, PlanEntityType::Lot, lot.id, 0.70f, location, "Lot has zero id");
        } else if (!ids.insert(lot.id).second) {
            AddViolation(report, prefix, PlanEntityType::Lot, lot.id, 0.95f, location, "Duplicate lot id");
        }
        if (lot.district_id == 0u) {
            AddViolation(report, prefix, PlanEntityType::Lot, lot.id, 0.35f, location, "Lot has no district assignment");
        }
        if (!IsFiniteVec(lot.centroid)) {
            AddViolation(report, prefix, PlanEntityType::Lot, lot.id, 0.95f, location, "Lot centroid is non-finite");
        }
        if (lot.boundary.size() < 3u) {
            AddViolation(report, prefix, PlanEntityType::Lot, lot.id, 0.55f, location, "Lot boundary has fewer than three points");
        } else {
            for (const Vec2& point : lot.boundary) {
                if (!IsFiniteVec(point)) {
                    AddViolation(report, prefix, PlanEntityType::Lot, lot.id, 0.95f, location, "Lot boundary contains non-finite coordinates");
                    break;
                }
            }
            if (std::abs(SignedArea(lot.boundary)) <= 1e-6) {
                AddViolation(report, prefix, PlanEntityType::Lot, lot.id, 0.55f, location, "Lot boundary has near-zero area");
            }
        }
    }
    return report;
}

[[nodiscard]] IntegrityReport CollectBuildingIntegrity(const siv::Vector<BuildingSite>& buildings, std::string_view prefix) {
    IntegrityReport report{};
    std::unordered_set<uint32_t> ids;
    ids.reserve(buildings.size());
    for (const BuildingSite& building : buildings) {
        if (building.id == 0u) {
            AddViolation(report, prefix, PlanEntityType::Global, building.id, 0.70f, building.position, "Building has zero id");
        } else if (!ids.insert(building.id).second) {
            AddViolation(report, prefix, PlanEntityType::Global, building.id, 0.95f, building.position, "Duplicate building id");
        }
        if (building.lot_id == 0u) {
            AddViolation(report, prefix, PlanEntityType::Global, building.id, 0.70f, building.position, "Building is missing lot_id linkage");
        }
        if (!IsFiniteVec(building.position)) {
            AddViolation(report, prefix, PlanEntityType::Global, building.id, 0.95f, building.position, "Building position is non-finite");
        }
    }
    return report;
}

void MergeReport(IntegrityReport& dst, IntegrityReport src) {
    dst.warning_count += src.warning_count;
    dst.error_count += src.error_count;
    dst.violations.insert(
        dst.violations.end(),
        std::make_move_iterator(src.violations.begin()),
        std::make_move_iterator(src.violations.end()));
}

} // namespace

void ValidateRoads(const fva::Container<Road>& roads) {
    (void)CollectRoadIntegrity(roads, kEntityPrefix);
}

void ValidateDistricts(const fva::Container<District>& districts) {
    (void)CollectDistrictIntegrity(districts, kEntityPrefix);
}

void ValidateLots(const fva::Container<LotToken>& lots) {
    (void)CollectLotIntegrity(lots, kEntityPrefix);
}

void ValidateBuildings(const siv::Vector<BuildingSite>& buildings) {
    (void)CollectBuildingIntegrity(buildings, kEntityPrefix);
}

IntegrityReport CollectEntityIntegrityReport(const Editor::GlobalState& gs) {
    IntegrityReport report{};
    MergeReport(report, CollectRoadIntegrity(gs.roads, kEntityPrefix));
    MergeReport(report, CollectDistrictIntegrity(gs.districts, kEntityPrefix));
    MergeReport(report, CollectLotIntegrity(gs.lots, kEntityPrefix));
    MergeReport(report, CollectBuildingIntegrity(gs.buildings, kEntityPrefix));
    return report;
}

IntegrityReport CollectSpatialIntegrityReport(const Editor::GlobalState& gs) {
    IntegrityReport report{};
    const float min_lot_area = gs.active_city_spec.has_value()
        ? std::max(1.0f, gs.active_city_spec->zoningConstraints.minLotArea)
        : 1.0f;
    const auto overlay_errors = CollectOverlayValidationErrors(gs, min_lot_area);

    std::unordered_set<uint64_t> dedupe;
    dedupe.reserve(overlay_errors.size());
    for (const auto& error : overlay_errors) {
        if (!IsKnownSpatialOverlayMessage(error.message)) {
            continue;
        }
        const uint64_t key = (static_cast<uint64_t>(static_cast<uint8_t>(error.entity_kind)) << 32ull) |
            static_cast<uint64_t>(error.entity_id);
        if (!dedupe.insert(key).second) {
            continue;
        }
        AddViolation(
            report,
            kSpatialPrefix,
            ToPlanEntityType(error.entity_kind),
            error.entity_id,
            ToPlanSeverity(error.severity),
            error.world_position,
            error.message);
    }
    return report;
}

void ValidateAll(Editor::GlobalState& gs) {
    PruneTaggedViolations(gs.plan_violations, kEntityPrefix);
    IntegrityReport report = CollectEntityIntegrityReport(gs);
    gs.plan_violations.insert(
        gs.plan_violations.end(),
        std::make_move_iterator(report.violations.begin()),
        std::make_move_iterator(report.violations.end()));
}

void SpatialCheckAll(Editor::GlobalState& gs) {
    PruneTaggedViolations(gs.plan_violations, kSpatialPrefix);
    IntegrityReport report = CollectSpatialIntegrityReport(gs);
    gs.plan_violations.insert(
        gs.plan_violations.end(),
        std::make_move_iterator(report.violations.begin()),
        std::make_move_iterator(report.violations.end()));
}

void ValidateAll(const Editor::GlobalState& gs) {
    (void)CollectEntityIntegrityReport(gs);
}

void SpatialCheckAll(const Editor::GlobalState& gs) {
    (void)CollectSpatialIntegrityReport(gs);
}

} // namespace RogueCity::Core::Validation
