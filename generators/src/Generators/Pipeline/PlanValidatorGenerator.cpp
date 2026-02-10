#include "RogueCity/Generators/Pipeline/PlanValidatorGenerator.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace RogueCity::Generators {

namespace {

[[nodiscard]] float Clamp01(float v) {
    return std::clamp(v, 0.0f, 1.0f);
}

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

PlanValidatorGenerator::Output PlanValidatorGenerator::validate(
    const Input& input,
    const Config& config) const {
    Output output{};
    bool hard_fail = false;

    if (input.constraints == nullptr || !input.constraints->isValid()) {
        return output;
    }

    const Core::WorldConstraintField& constraints = *input.constraints;

    if (input.roads != nullptr) {
        for (const auto& road : *input.roads) {
            Core::PlanViolationType worst_type = Core::PlanViolationType::None;
            Core::Vec2 worst_pos{};
            float worst_severity = 0.0f;
            const char* message = "";

            for (const auto& point : road.points) {
                if (constraints.sampleNoBuild(point)) {
                    worst_type = Core::PlanViolationType::NoBuildEncroachment;
                    worst_pos = point;
                    worst_severity = 1.0f;
                    message = "Road enters a no-build cell.";
                    break;
                }

                const uint8_t flood = constraints.sampleFloodMask(point);
                if (flood > config.max_road_flood_level) {
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
                    const float severity = 0.4f + (config.min_soil_strength - soil) * 2.4f;
                    if (severity > worst_severity) {
                        worst_type = Core::PlanViolationType::SoilTooWeak;
                        worst_pos = point;
                        worst_severity = Clamp01(severity);
                        message = "Road crosses low soil-strength cells.";
                    }
                }
            }

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
                if (constraints.sampleNoBuild(point)) {
                    worst_type = Core::PlanViolationType::NoBuildEncroachment;
                    worst_pos = point;
                    worst_severity = 1.0f;
                    message = "Lot overlaps a no-build cell.";
                    break;
                }

                const uint8_t flood = constraints.sampleFloodMask(point);
                if (flood > config.max_lot_flood_level) {
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
                    const float severity = 0.45f + (config.min_soil_strength - soil) * 2.5f;
                    if (severity > worst_severity) {
                        worst_type = Core::PlanViolationType::SoilTooWeak;
                        worst_pos = point;
                        worst_severity = Clamp01(severity);
                        message = "Lot is on weak soil-strength cells.";
                    }
                }
            }

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

    output.approved = !hard_fail;
    return output;
}

} // namespace RogueCity::Generators
