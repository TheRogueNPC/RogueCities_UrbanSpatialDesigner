/**
 * @file EditorOverlayValidation.cpp
 * @brief Implements validation routines for urban spatial entities in the editor overlay.
 *
 * Contains functions to validate plan entities such as lots, buildings, and roads,
 * checking for issues like minimum lot area, building-lot relationships, and road intersections.
 *
 * Main functionalities:
 * - Computes signed polygon area for lot boundaries.
 * - Determines if a point lies within a polygon (lot boundary).
 * - Checks for segment intersections (road overlaps).
 * - Maps plan entity types to validation entity kinds.
 * - Collects validation errors from the editor's global state, including:
 *   - Plan violations
 *   - Lots below minimum area
 *   - Buildings outside their lots or referencing missing lots
 *   - Road intersections requiring review
 *
 * @namespace RogueCity::Core::Validation
 * @function CollectOverlayValidationErrors
 *   Collects validation errors from the editor's global state.
 *   @param gs Editor global state containing plan entities and violations.
 *   @param min_lot_area Minimum allowed area for lots.
 *   @return Vector of validation errors detected.
 */
 
#include "RogueCity/Core/Validation/EditorOverlayValidation.hpp"

#include <cmath>
#include <limits>
#include <unordered_map>

namespace RogueCity::Core::Validation {

namespace {

using RogueCity::Core::Vec2;
using RogueCity::Core::Editor::ValidationError;
using RogueCity::Core::Editor::ValidationSeverity;
using RogueCity::Core::Editor::VpEntityKind;

double SignedPolygonArea(const std::vector<Vec2>& points) {
    if (points.size() < 3) {
        return 0.0;
    }
    double area = 0.0;
    for (size_t i = 0; i < points.size(); ++i) {
        const Vec2& a = points[i];
        const Vec2& b = points[(i + 1) % points.size()];
        area += (a.x * b.y) - (b.x * a.y);
    }
    return area * 0.5;
}

bool PointInPolygon(const Vec2& p, const std::vector<Vec2>& polygon) {
    if (polygon.size() < 3) {
        return false;
    }
    bool inside = false;
    for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
        const Vec2& pi = polygon[i];
        const Vec2& pj = polygon[j];
        const bool intersects = ((pi.y > p.y) != (pj.y > p.y)) &&
            (p.x < (pj.x - pi.x) * (p.y - pi.y) / ((pj.y - pi.y) + 1e-12) + pi.x);
        if (intersects) {
            inside = !inside;
        }
    }
    return inside;
}

double Cross(const Vec2& a, const Vec2& b) {
    return (a.x * b.y) - (a.y * b.x);
}

bool SegmentIntersection(
    const Vec2& a1,
    const Vec2& a2,
    const Vec2& b1,
    const Vec2& b2,
    Vec2& out_point) {
    const Vec2 r = a2 - a1;
    const Vec2 s = b2 - b1;
    const double denom = Cross(r, s);
    if (std::fabs(denom) < 1e-8) {
        return false;
    }

    const Vec2 delta = b1 - a1;
    const double t = Cross(delta, s) / denom;
    const double u = Cross(delta, r) / denom;
    if (t <= 1e-5 || t >= 1.0 - 1e-5 || u <= 1e-5 || u >= 1.0 - 1e-5) {
        return false;
    }

    out_point = a1 + r * t;
    return true;
}

ValidationSeverity ToSeverity(float s) {
    if (s >= 0.8f) {
        return ValidationSeverity::Critical;
    }
    if (s >= 0.45f) {
        return ValidationSeverity::Error;
    }
    return ValidationSeverity::Warning;
}

size_t ToSizeT(uint64_t value) {
    constexpr uint64_t max = static_cast<uint64_t>(std::numeric_limits<size_t>::max());
    return static_cast<size_t>(value > max ? max : value);
}

VpEntityKind ToEntityKind(const RogueCity::Core::PlanEntityType entity_type) {
    switch (entity_type) {
    case RogueCity::Core::PlanEntityType::Road:
        return VpEntityKind::Road;
    case RogueCity::Core::PlanEntityType::Lot:
        return VpEntityKind::Lot;
    case RogueCity::Core::PlanEntityType::District:
        return VpEntityKind::District;
    default:
        return VpEntityKind::Unknown;
    }
}

void AppendValidationError(
    std::vector<ValidationError>& errors,
    const ValidationSeverity severity,
    const VpEntityKind entity_kind,
    const uint32_t entity_id,
    const Vec2& world_position,
    std::string message) {
    errors.emplace_back();
    ValidationError& error = errors.back();
    error.severity = severity;
    error.entity_kind = entity_kind;
    error.entity_id = entity_id;
    error.world_position = world_position;
    error.message = std::move(message);
}

} // namespace

std::vector<ValidationError> CollectOverlayValidationErrors(const Editor::GlobalState& gs, float min_lot_area) {
    std::vector<ValidationError> errors;
    errors.reserve(gs.plan_violations.size() + ToSizeT(gs.lots.size()) + gs.buildings.size());

    for (const auto& violation : gs.plan_violations) {
        AppendValidationError(
            errors,
            ToSeverity(violation.severity),
            ToEntityKind(violation.entity_type),
            violation.entity_id,
            violation.location,
            violation.message.empty() ? std::string("Plan validation issue") : violation.message);
    }

    const float lot_area_threshold = std::max(1.0f, min_lot_area);
    for (const auto& lot : gs.lots) {
        float area = lot.area;
        if (area <= 0.0f && lot.boundary.size() >= 3) {
            area = static_cast<float>(std::fabs(SignedPolygonArea(lot.boundary)));
        }
        if (area < lot_area_threshold) {
            AppendValidationError(
                errors,
                ValidationSeverity::Warning,
                VpEntityKind::Lot,
                lot.id,
                lot.centroid,
                "Lot area below minimum threshold");
        }
    }

    std::unordered_map<uint32_t, const RogueCity::Core::LotToken*> lots_by_id;
    lots_by_id.reserve(ToSizeT(gs.lots.size()));
    for (const auto& lot : gs.lots) {
        lots_by_id.emplace(lot.id, &lot);
    }

    for (const auto& building : gs.buildings) {
        const auto lot_it = lots_by_id.find(building.lot_id);
        if (lot_it == lots_by_id.end()) {
            AppendValidationError(
                errors,
                ValidationSeverity::Critical,
                VpEntityKind::Building,
                building.id,
                building.position,
                "Building references missing lot");
            continue;
        }

        const auto* lot = lot_it->second;
        if (lot->boundary.size() >= 3 && !PointInPolygon(building.position, lot->boundary)) {
            AppendValidationError(
                errors,
                ValidationSeverity::Error,
                VpEntityKind::Building,
                building.id,
                building.position,
                "Building footprint is outside its lot boundary");
        }
    }

    std::vector<const RogueCity::Core::Road*> roads;
    roads.reserve(ToSizeT(gs.roads.size()));
    for (const auto& road : gs.roads) {
        roads.push_back(&road);
    }

    constexpr size_t kMaxIntersectionErrors = 96;
    for (size_t i = 0; i < roads.size(); ++i) {
        const auto& road_a = *roads[i];
        if (road_a.points.size() < 2) {
            continue;
        }
        for (size_t j = i + 1; j < roads.size(); ++j) {
            const auto& road_b = *roads[j];
            if (road_b.points.size() < 2) {
                continue;
            }
            for (size_t sa = 1; sa < road_a.points.size(); ++sa) {
                for (size_t sb = 1; sb < road_b.points.size(); ++sb) {
                    Vec2 intersection{};
                    if (!SegmentIntersection(
                            road_a.points[sa - 1],
                            road_a.points[sa],
                            road_b.points[sb - 1],
                            road_b.points[sb],
                            intersection)) {
                        continue;
                    }

                    AppendValidationError(
                        errors,
                        ValidationSeverity::Warning,
                        VpEntityKind::Road,
                        road_a.id,
                        intersection,
                        "Road intersection requires review");

                    if (errors.size() >= kMaxIntersectionErrors) {
                        return errors;
                    }
                }
            }
        }
    }

    return errors;
}

} // namespace RogueCity::Core::Validation
