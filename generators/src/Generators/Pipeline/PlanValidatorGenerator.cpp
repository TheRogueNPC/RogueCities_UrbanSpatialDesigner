#include "RogueCity/Generators/Pipeline/PlanValidatorGenerator.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace RogueCity::Generators {

namespace {

// Utility clamp used to keep computed violation severities in [0, 1].
[[nodiscard]] float Clamp01(float v) {
    return std::clamp(v, 0.0f, 1.0f);
}

// Appends one violation record and updates aggregate validation state.
// "hard_fail" is elevated for violations considered non-negotiable, or for near-max severity.
void PushViolation(
    PlanValidatorGenerator::Output& out,
    const Core::PlanViolationType type,
    const Core::PlanEntityType entity_type,
    const uint32_t entity_id,
    const float severity,
    const Core::Vec2& location,
    const char* message,
    bool& hard_fail) {
    Core::PlanViolation violation{};
    violation.type = type;
    violation.entity_type = entity_type;
    violation.entity_id = entity_id;
    violation.severity = Clamp01(severity);
    violation.location = location;
    violation.message = message;
    out.violations.push_back(std::move(violation));
    out.max_severity = std::max(out.max_severity, Clamp01(severity));

    if (type == Core::PlanViolationType::NoBuildEncroachment ||
        type == Core::PlanViolationType::PolicyConflict) {
        hard_fail = true;
    }
    if (severity >= 0.95f) {
        hard_fail = true;
    }
}

} // namespace

// Convenience overload that validates using default thresholds.
PlanValidatorGenerator::Output PlanValidatorGenerator::validate(
    const Input& input) const {
    return validate(input, Config{});
}

// Validates roads/lots/global policy against world constraints and policy config.
// Strategy:
// - inspect each entity at sampled points
// - keep only the most severe issue per entity
// - aggregate all violations and compute approval via hard-fail gates
PlanValidatorGenerator::Output PlanValidatorGenerator::validate(
    const Input& input,
    const Config& config) const {
    Output output{};
    bool hard_fail = false;

    // Without a valid constraint field there is nothing authoritative to validate against.
    // Return clean output rather than inventing synthetic failures.
    if (input.constraints == nullptr || !input.constraints->isValid()) {
        return output;
    }

    const Core::WorldConstraintField& constraints = *input.constraints;

    // Road validation:
    // evaluate each polyline point and retain the worst single violation per road.
    if (input.roads != nullptr) {
        for (const auto& road : *input.roads) {
            Core::PlanViolationType worst_type = Core::PlanViolationType::None;
            Core::Vec2 worst_pos{};
            float worst_severity = 0.0f;
            const char* message = "";

            for (const auto& point : road.points) {
                // No-build encroachment is a hard stop; no need to evaluate lower-priority checks.
                if (constraints.sampleNoBuild(point)) {
                    worst_type = Core::PlanViolationType::NoBuildEncroachment;
                    worst_pos = point;
                    worst_severity = 1.0f;
                    message = "Road enters a no-build cell.";
                    break;
                }

                const uint8_t flood = constraints.sampleFloodMask(point);
                if (flood > config.max_road_flood_level) {
                    // Severity increases with excess flood band above allowed threshold.
                    const float severity = 0.75f + 0.15f * static_cast<float>(flood - config.max_road_flood_level);
                    if (severity > worst_severity) {
                        worst_type = Core::PlanViolationType::FloodRisk;
                        worst_pos = point;
                        worst_severity = Clamp01(severity);
                        message = "Road crosses high flood-risk cells.";
                    }
                }

                const float slope = constraints.sampleSlopeDegrees(point);
                if (slope > config.max_road_slope_deg) {
                    // Linear overrun model: mild penalties near threshold, stronger as grade increases.
                    const float severity = 0.35f + (slope - config.max_road_slope_deg) / 20.0f;
                    if (severity > worst_severity) {
                        worst_type = Core::PlanViolationType::SlopeTooHigh;
                        worst_pos = point;
                        worst_severity = Clamp01(severity);
                        message = "Road exceeds maximum allowed slope.";
                    }
                }

                const float soil = constraints.sampleSoilStrength(point);
                if (soil < config.min_soil_strength) {
                    // Weak soil risk grows with deficit below minimum strength target.
                    const float severity = 0.4f + (config.min_soil_strength - soil) * 2.4f;
                    if (severity > worst_severity) {
                        worst_type = Core::PlanViolationType::SoilTooWeak;
                        worst_pos = point;
                        worst_severity = Clamp01(severity);
                        message = "Road crosses low soil-strength cells.";
                    }
                }
            }

            // Emit at most one violation per road: the strongest observed signal.
            if (worst_type != Core::PlanViolationType::None) {
                PushViolation(
                    output,
                    worst_type,
                    Core::PlanEntityType::Road,
                    road.id,
                    worst_severity,
                    worst_pos,
                    message,
                    hard_fail);
            }
        }
    }

    // Lot validation:
    // sample the centroid plus boundary vertices to approximate lot footprint exposure.
    if (input.lots != nullptr) {
        for (const auto& lot : *input.lots) {
            std::vector<Core::Vec2> samples;
            samples.reserve(lot.boundary.size() + 1u);
            samples.push_back(lot.centroid);
            for (const auto& p : lot.boundary) {
                samples.push_back(p);
            }

            Core::PlanViolationType worst_type = Core::PlanViolationType::None;
            Core::Vec2 worst_pos = lot.centroid;
            float worst_severity = 0.0f;
            const char* message = "";

            for (const auto& point : samples) {
                // As with roads, no-build overlap is decisive for this lot.
                if (constraints.sampleNoBuild(point)) {
                    worst_type = Core::PlanViolationType::NoBuildEncroachment;
                    worst_pos = point;
                    worst_severity = 1.0f;
                    message = "Lot overlaps a no-build cell.";
                    break;
                }

                const uint8_t flood = constraints.sampleFloodMask(point);
                if (flood > config.max_lot_flood_level) {
                    // Lots are penalized slightly more aggressively for flood exposure.
                    const float severity = 0.7f + 0.2f * static_cast<float>(flood - config.max_lot_flood_level);
                    if (severity > worst_severity) {
                        worst_type = Core::PlanViolationType::FloodRisk;
                        worst_pos = point;
                        worst_severity = Clamp01(severity);
                        message = "Lot intersects flood-risk cells.";
                    }
                }

                const float slope = constraints.sampleSlopeDegrees(point);
                if (slope > config.max_lot_slope_deg) {
                    // Lot slope overrun weighting differs from roads to reflect siting sensitivity.
                    const float severity = 0.3f + (slope - config.max_lot_slope_deg) / 18.0f;
                    if (severity > worst_severity) {
                        worst_type = Core::PlanViolationType::SlopeTooHigh;
                        worst_pos = point;
                        worst_severity = Clamp01(severity);
                        message = "Lot exceeds maximum slope target.";
                    }
                }

                const float soil = constraints.sampleSoilStrength(point);
                if (soil < config.min_soil_strength) {
                    // Soil penalty tuned for static parcel stability risk.
                    const float severity = 0.45f + (config.min_soil_strength - soil) * 2.5f;
                    if (severity > worst_severity) {
                        worst_type = Core::PlanViolationType::SoilTooWeak;
                        worst_pos = point;
                        worst_severity = Clamp01(severity);
                        message = "Lot is on weak soil-strength cells.";
                    }
                }
            }

            // Emit only the dominant issue for each lot to keep reports actionable.
            if (worst_type != Core::PlanViolationType::None) {
                PushViolation(
                    output,
                    worst_type,
                    Core::PlanEntityType::Lot,
                    lot.id,
                    worst_severity,
                    worst_pos,
                    message,
                    hard_fail);
            }
        }
    }

    // Global policy-level validation not tied to a specific geometry entity.
    if (input.site_profile != nullptr && input.site_profile->policy_friction > config.max_policy_friction) {
        const float severity = Clamp01(
            0.55f + (input.site_profile->policy_friction - config.max_policy_friction) * 2.0f);
        PushViolation(
            output,
            Core::PlanViolationType::PolicyConflict,
            Core::PlanEntityType::Global,
            0u,
            severity,
            Core::Vec2{},
            "Policy friction exceeds the configured threshold.",
            hard_fail);
    }

    // Approval is true only when no hard-fail condition has been triggered.
    output.approved = !hard_fail;
    return output;
}

} // namespace RogueCity::Generators
