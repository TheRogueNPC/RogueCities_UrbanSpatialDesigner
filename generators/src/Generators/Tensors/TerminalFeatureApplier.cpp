#include "RogueCity/Generators/Tensors/TerminalFeatureApplier.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <numbers>

namespace RogueCity::Generators {
namespace {

[[nodiscard]] double smoothstep(double edge0, double edge1, double x) {
    const double t = std::clamp((x - edge0) / std::max(1e-9, edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

[[nodiscard]] double wrapPi(double angle) {
    constexpr double kPi = std::numbers::pi;
    while (angle > kPi * 0.5) {
        angle -= kPi;
    }
    while (angle < -kPi * 0.5) {
        angle += kPi;
    }
    return angle;
}

[[nodiscard]] double angleDistance(double a, double b) {
    return std::abs(wrapPi(a - b));
}

[[nodiscard]] double blendAngle(double from, double to, double t) {
    const double clamped_t = std::clamp(t, 0.0, 1.0);
    const Core::Vec2 u0(std::cos(2.0 * from), std::sin(2.0 * from));
    const Core::Vec2 u1(std::cos(2.0 * to), std::sin(2.0 * to));
    const Core::Vec2 mix = (u0 * (1.0 - clamped_t)) + (u1 * clamped_t);
    if (mix.lengthSquared() <= 1e-9) {
        return to;
    }
    return 0.5 * std::atan2(mix.y, mix.x);
}

[[nodiscard]] double hashToUnit(uint32_t seed, uint32_t salt) {
    uint32_t x = seed ^ (0x9e3779b9u + salt * 0x85ebca6bu);
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return static_cast<double>(x) / static_cast<double>(0xFFFFFFFFu);
}

[[nodiscard]] double coherentNoise(const Core::Vec2& p, double frequency, uint32_t seed) {
    const double sx = static_cast<double>(seed & 0xFFu) * 0.013;
    const double sy = static_cast<double>((seed >> 8) & 0xFFu) * 0.017;
    const double n =
        std::sin((p.x + sx * 73.0) * frequency) +
        std::cos((p.y - sy * 61.0) * frequency * 1.37) +
        0.5 * std::sin((p.x + p.y + sx * 13.0) * frequency * 0.73);
    return n / 2.5;
}

void pushPostOp(
    FeaturePlan& plan,
    TensorPostOp::Kind kind,
    TerminalFeature feature,
    const AxiomTensorParams& params,
    float a,
    float b,
    float c,
    uint32_t seed) {
    TensorPostOp op{};
    op.kind = kind;
    op.feature = feature;
    op.center = params.center;
    op.radius = params.radius;
    op.theta = params.theta;
    op.a = a;
    op.b = b;
    op.c = c;
    op.seed = seed;
    plan.post_ops.push_back(op);
}

// ============================================================================
// TERMINAL FEATURE CONFLICT DETECTION
// ============================================================================
// Some terminal features have opposing effects on the tensor field. When both
// are enabled, they can produce chaotic or undefined results. This table
// documents known conflicts and their resolution strategies.
//
// WHY: Conflict detection prevents users from creating axioms with mutually
// exclusive features, which would produce unpredictable road networks.
// ============================================================================

struct FeatureConflict {
    TerminalFeature a;
    TerminalFeature b;
    const char* reason;
    int priority_a;
    int priority_b;
};

static const FeatureConflict kFeatureConflicts[] = {
    { TerminalFeature::Grid_AxisAlignmentLock, TerminalFeature::Grid_DiagonalSlicing,
      "AxisAlignmentLock and DiagonalSlicing fight over axis orientation", 1, 0 },
    { TerminalFeature::Grid_AxisAlignmentLock, TerminalFeature::LooseGrid_TJunctionForcing,
      "AxisAlignmentLock and TJunctionForcing have conflicting alignment goals", 1, 0 },
    { TerminalFeature::Radial_CoreVoiding, TerminalFeature::LooseGrid_CenterWeightedDensity,
      "CoreVoiding removes center influence while CenterWeightedDensity strengthens it", 1, 0 },
    { TerminalFeature::Stem_DirectionalFlowBias, TerminalFeature::Linear_PerpendicularRungs,
      "DirectionalFlowBias enforces parallel flow while PerpendicularRungs adds cross-directions", 0, 1 },
    { TerminalFeature::Hex_HoneycombStrictness, TerminalFeature::Hex_OrganicEdgeBleed,
      "HoneycombStrictness enforces strict angles while OrganicEdgeBleed softens them", 1, 0 },
    { TerminalFeature::GridCorrective_AbsoluteOverride, TerminalFeature::GridCorrective_MagneticAlignment,
      "AbsoluteOverride fully replaces tensors while MagneticAlignment blends - override wins", 1, 0 },
    { TerminalFeature::Superblock_CourtyardVoid, TerminalFeature::Superblock_PedestrianMicroField,
      "CourtyardVoid zeros center while PedestrianMicroField adds interior guidance", 1, 0 },
};

[[nodiscard]] bool hasFeatureConflict(
    TerminalFeature a, TerminalFeature b, const FeatureConflict** out_conflict) {
    for (const auto& conflict : kFeatureConflicts) {
        if ((conflict.a == a && conflict.b == b) || (conflict.a == b && conflict.b == a)) {
            if (out_conflict) {
                *out_conflict = &conflict;
            }
            return true;
        }
    }
    return false;
}

[[nodiscard]] TerminalFeature resolveConflict(const FeatureConflict& conflict, TerminalFeature a, TerminalFeature b) {
    if (conflict.a == a) {
        return (conflict.priority_a >= conflict.priority_b) ? a : b;
    }
    return (conflict.priority_b >= conflict.priority_a) ? b : a;
}

// Approximate magnitude contribution for different basis field types.
// Used to track cumulative field strength and prevent overwhelming the base tensor.
[[nodiscard]] double estimateBasisFieldMagnitude(const BasisField* field) {
    if (!field) return 0.0;

    if (dynamic_cast<const StrongLinearCorridorField*>(field)) return 0.25;
    if (dynamic_cast<const CenterVoidOverrideField*>(field)) return 0.35;
    if (dynamic_cast<const BoundarySeamField*>(field)) return 0.15;
    if (dynamic_cast<const NoisePatchField*>(field)) return 0.12;
    if (dynamic_cast<const ParallelLinearFieldBundle*>(field)) return 0.18;
    if (dynamic_cast<const PeriodicRungField*>(field)) return 0.10;
    if (dynamic_cast<const ShearPlaneField*>(field)) return 0.08;

    return 0.10;
}

} // namespace

FeaturePlan buildFeaturePlanRaw(
    uint8_t axiom_type_raw,
    const AxiomTensorParams& params,
    const TerminalFeatureSet& features,
    uint32_t seed) {
    FeaturePlan plan{};
    if (features.empty()) {
        return plan;
    }

    // ===========================================================================
    // PHASE 1: COLLECT ENABLED FEATURES
    // ===========================================================================
    // We first gather all enabled features to detect conflicts before applying.
    // This prevents partial application when conflicts exist.
    
    std::vector<TerminalFeature> enabled_features;
    const auto allowed = featuresForAxiomTypeRaw(axiom_type_raw);
    for (const TerminalFeature feature : allowed) {
        if (features.has(feature)) {
            enabled_features.push_back(feature);
        }
    }

    // ===========================================================================
    // PHASE 2: CONFLICT DETECTION
    // ===========================================================================
    // Check for mutually exclusive feature combinations. When detected:
    // - Log a warning for diagnostics
    // - Skip the lower-priority feature (soft resolution)
    
    TerminalFeatureSet features_to_skip{};
    for (size_t i = 0; i < enabled_features.size(); ++i) {
        for (size_t j = i + 1; j < enabled_features.size(); ++j) {
            const TerminalFeature a = enabled_features[i];
            const TerminalFeature b = enabled_features[j];
            const FeatureConflict* conflict = nullptr;
            
            if (hasFeatureConflict(a, b, &conflict)) {
                const TerminalFeature loser = resolveConflict(*conflict, a, b);
                features_to_skip.set(loser, true);
                
                // Build warning message
                std::string warning = "Feature conflict: ";
                warning += terminalFeatureShortName(a);
                warning += " vs ";
                warning += terminalFeatureShortName(b);
                warning += " - ";
                warning += conflict->reason;
                warning += " (dropping ";
                warning += terminalFeatureShortName(loser);
                warning += ")";
                plan.warnings.push_back(warning);
            }
        }
    }

    // ===========================================================================
    // PHASE 3: APPLY NON-CONFLICTING FEATURES
    // ===========================================================================
    // Each feature contributes basis fields and/or post-ops. We track cumulative
    // magnitude to prevent overwhelming the base tensor field.
    
    for (const TerminalFeature feature : enabled_features) {
        // Skip features that lost conflict resolution
        if (features_to_skip.has(feature)) {
            continue;
        }

        const uint32_t op_seed = seed ^ static_cast<uint32_t>(feature) * 2654435761u;
        const double jitter = hashToUnit(op_seed, 17u) * 2.0 - 1.0;

        // Helper to add basis field with magnitude tracking
        auto add_additive = [&](std::unique_ptr<BasisField> field) {
            const double mag = estimateBasisFieldMagnitude(field.get());
            if (plan.current_magnitude_total + mag <= plan.additive_magnitude_budget) {
                plan.current_magnitude_total += mag;
                plan.additive_basis_fields.push_back(std::move(field));
            } else {
#ifndef NDEBUG
                plan.warnings.push_back("Magnitude budget exceeded, dropping field for " + 
                    std::string(terminalFeatureShortName(feature)));
#endif
            }
        };

        auto add_override = [&](std::unique_ptr<BasisField> field) {
            // Override fields don't count toward additive budget
            plan.override_basis_fields.push_back(std::move(field));
        };

        switch (feature) {
            case TerminalFeature::Organic_TopologicalFlowConstraint:
                add_additive(std::make_unique<NoisePatchField>(
                    params.center, params.radius, params.theta + jitter * 0.12, 0.0045, 0.35, params.decay + 0.25));
                break;
            case TerminalFeature::Organic_MeanderBias:
                pushPostOp(plan, TensorPostOp::Kind::AngleNoiseWarp, feature, params, 0.35f, 0.0032f, 0.0f, op_seed);
                break;
            case TerminalFeature::Organic_VoronoiRelaxation:
                pushPostOp(plan, TensorPostOp::Kind::AngularSnap, feature, params,
                    static_cast<float>(std::numbers::pi / 3.0), 0.65f, 0.0f, op_seed);
                break;
            case TerminalFeature::Organic_CulDeSacPruning:
                pushPostOp(plan, TensorPostOp::Kind::EndTaper, feature, params, 0.55f, 0.0f, 0.0f, op_seed);
                break;

            case TerminalFeature::Grid_AxisAlignmentLock:
                pushPostOp(plan, TensorPostOp::Kind::AxisLock, feature, params, 0.9f, 0.0f, 0.0f, op_seed);
                break;
            case TerminalFeature::Grid_DiagonalSlicing:
                add_additive(std::make_unique<StrongLinearCorridorField>(
                    params.center, params.radius, params.theta + std::numbers::pi * 0.25, params.radius * 0.2, params.decay));
                break;
            case TerminalFeature::Grid_AlleywayBisection:
                add_additive(std::make_unique<StrongLinearCorridorField>(
                    params.center, params.radius * 0.95, params.theta, params.radius * 0.11, params.decay + 0.5));
                add_additive(std::make_unique<StrongLinearCorridorField>(
                    params.center, params.radius * 0.95, params.theta + std::numbers::pi * 0.5, params.radius * 0.11, params.decay + 0.5));
                break;
            case TerminalFeature::Grid_BlockFusion:
                add_additive(std::make_unique<NoisePatchField>(
                    params.center, params.radius, params.theta, 0.013, 0.45, params.decay + 0.3));
                break;

            case TerminalFeature::Radial_SpiralDominance:
                pushPostOp(plan, TensorPostOp::Kind::SpiralWarp, feature, params, 0.28f, 0.0f, 0.0f, op_seed);
                break;
            case TerminalFeature::Radial_CoreVoiding:
                add_override(std::make_unique<CenterVoidOverrideField>(
                    params.center, params.radius, params.theta, 0.34, params.decay + 0.1));
                break;
            case TerminalFeature::Radial_SpokePruning:
                pushPostOp(plan, TensorPostOp::Kind::AngularSnap, feature, params,
                    static_cast<float>((2.0 * std::numbers::pi) / std::max(3.0, static_cast<double>(params.radial_spokes / 2))),
                    0.75f, 0.0f, op_seed);
                break;
            case TerminalFeature::Radial_ConcentricWaveDensity:
                pushPostOp(plan, TensorPostOp::Kind::RadiusPulse, feature, params, 0.22f, 6.0f, 0.0f, op_seed);
                break;

            case TerminalFeature::Hex_HoneycombStrictness:
                pushPostOp(plan, TensorPostOp::Kind::AngularSnap, feature, params,
                    static_cast<float>(std::numbers::pi / 3.0), 0.85f, 0.0f, op_seed);
                break;
            case TerminalFeature::Hex_TriangularSubdivision:
                add_additive(std::make_unique<StrongLinearCorridorField>(
                    params.center, params.radius, params.theta + std::numbers::pi / 6.0, params.radius * 0.17, params.decay + 0.2));
                break;
            case TerminalFeature::Hex_OffsetStagger:
                add_additive(std::make_unique<ShearPlaneField>(
                    params.center, params.radius, params.theta, std::numbers::pi / 10.0, params.decay));
                break;
            case TerminalFeature::Hex_OrganicEdgeBleed:
                pushPostOp(plan, TensorPostOp::Kind::AngleNoiseWarp, feature, params, 0.20f, 0.0048f, 0.0f, op_seed);
                break;

            case TerminalFeature::Stem_FractalRecursion:
                add_additive(std::make_unique<ParallelLinearFieldBundle>(
                    params.center, params.radius, params.theta, params.radius * 0.18, 2, params.decay + 0.4));
                break;
            case TerminalFeature::Stem_DirectionalFlowBias:
                pushPostOp(plan, TensorPostOp::Kind::OneWayBias, feature, params, 0.62f, 0.0f, 0.0f, op_seed);
                break;
            case TerminalFeature::Stem_CanopyWeave:
                pushPostOp(plan, TensorPostOp::Kind::SinusoidalAxisWarp, feature, params, 0.23f, 0.008f, 0.0f, op_seed);
                break;
            case TerminalFeature::Stem_TerminalLoops:
                add_additive(std::make_unique<NoisePatchField>(
                    params.center, params.radius, params.theta + std::numbers::pi * 0.5, 0.0105, 0.62, params.decay + 0.4));
                break;

            case TerminalFeature::LooseGrid_HistoricalFaultLines:
                add_additive(std::make_unique<ShearPlaneField>(
                    params.center, params.radius, params.theta, std::numbers::pi / 9.0, params.decay));
                break;
            case TerminalFeature::LooseGrid_JitterPersistence:
                pushPostOp(plan, TensorPostOp::Kind::AngleNoiseWarp, feature, params, 0.24f, 0.0075f, 0.0f, op_seed);
                break;
            case TerminalFeature::LooseGrid_TJunctionForcing:
                pushPostOp(plan, TensorPostOp::Kind::AxisLock, feature, params, 0.58f, 0.0f, 0.0f, op_seed);
                break;
            case TerminalFeature::LooseGrid_CenterWeightedDensity:
                pushPostOp(plan, TensorPostOp::Kind::SmoothAxisBlend, feature, params, 0.45f, 0.0f, 0.0f, op_seed);
                break;

            case TerminalFeature::Suburban_LollipopTerminals:
                pushPostOp(plan, TensorPostOp::Kind::SpiralWarp, feature, params, 0.12f, 0.0f, 0.0f, op_seed);
                break;
            case TerminalFeature::Suburban_ArterialIsolation:
                add_additive(std::make_unique<BoundarySeamField>(
                    params.center, params.radius, params.theta, 0.2, params.decay));
                break;
            case TerminalFeature::Suburban_TerrainAvoidance:
                pushPostOp(plan, TensorPostOp::Kind::AngleNoiseWarp, feature, params, 0.18f, 0.0045f, 0.0f, op_seed);
                break;
            case TerminalFeature::Suburban_HierarchicalStrictness:
                pushPostOp(plan, TensorPostOp::Kind::EndTaper, feature, params, 0.48f, 0.0f, 0.0f, op_seed);
                break;

            case TerminalFeature::Superblock_PedestrianMicroField:
                add_additive(std::make_unique<NoisePatchField>(
                    params.center, params.radius, params.theta + std::numbers::pi * 0.25, 0.019, 0.52, params.decay + 0.5));
                break;
            case TerminalFeature::Superblock_ArterialTrenching:
                add_additive(std::make_unique<BoundarySeamField>(
                    params.center, params.radius, params.theta, 0.15, params.decay));
                pushPostOp(plan, TensorPostOp::Kind::EndTaper, feature, params, 0.65f, 0.0f, 0.0f, op_seed);
                break;
            case TerminalFeature::Superblock_CourtyardVoid:
                add_override(std::make_unique<CenterVoidOverrideField>(
                    params.center, params.radius, params.theta, 0.30, params.decay + 0.2));
                break;
            case TerminalFeature::Superblock_PermeableEdges:
                pushPostOp(plan, TensorPostOp::Kind::SmoothAxisBlend, feature, params, 0.28f, 0.65f, 0.0f, op_seed);
                break;

            case TerminalFeature::Linear_RibbonBraiding:
                pushPostOp(plan, TensorPostOp::Kind::SinusoidalAxisWarp, feature, params, 0.34f, 0.01f, 0.0f, op_seed);
                break;
            case TerminalFeature::Linear_ParallelCascading:
                add_additive(std::make_unique<ParallelLinearFieldBundle>(
                    params.center, params.radius, params.theta, params.radius * 0.17, 3, params.decay));
                break;
            case TerminalFeature::Linear_PerpendicularRungs:
                add_additive(std::make_unique<PeriodicRungField>(
                    params.center, params.radius, params.theta, params.radius * 0.2, 0.22, params.decay));
                break;
            case TerminalFeature::Linear_TaperedTerminals:
                pushPostOp(plan, TensorPostOp::Kind::EndTaper, feature, params, 0.75f, 0.0f, 0.0f, op_seed);
                break;

            case TerminalFeature::GridCorrective_AbsoluteOverride:
                add_override(std::make_unique<StrongLinearCorridorField>(
                    params.center, params.radius, params.theta, params.radius * 0.55, params.decay + 0.2));
                break;
            case TerminalFeature::GridCorrective_MagneticAlignment:
                pushPostOp(plan, TensorPostOp::Kind::SmoothAxisBlend, feature, params, 0.88f, 0.0f, 0.0f, op_seed);
                break;
            case TerminalFeature::GridCorrective_OrthogonalCull:
                pushPostOp(plan, TensorPostOp::Kind::AngleToleranceCull, feature, params,
                    static_cast<float>(std::numbers::pi / 10.0), 0.0f, 0.0f, op_seed);
                break;
            case TerminalFeature::GridCorrective_BoundaryStitching:
                add_additive(std::make_unique<BoundarySeamField>(
                    params.center, params.radius, params.theta, 0.18, params.decay + 0.1));
                break;
        }
    }

    // ===========================================================================
    // PHASE 4: SORT POST-OPS BY EXECUTION PRIORITY
    // ===========================================================================
    // Post-ops must execute in a specific order to produce predictable results:
    // 1. Noise/warp operations (perturb raw angles)
    // 2. Snap/lock operations (discretize angles)
    // 3. Blend operations (smooth alignment)
    // 4. Filter operations (cull outliers)
    // 5. Edge effects (taper, pulse)
    
    plan.sortPostOps();

#ifndef NDEBUG
    // ===========================================================================
    // DIAGNOSTIC LOGGING (P2.3 - Debug Output for Feature Application)
    // ===========================================================================
    // Log feature plan details for debugging tensor field generation.
    // This helps diagnose why certain tensor fields produce specific results.
    // ===========================================================================
    if (!plan.additive_basis_fields.empty() || !plan.override_basis_fields.empty() || !plan.post_ops.empty()) {
        std::cerr << "[FeaturePlan] Built plan with "
                  << plan.additive_basis_fields.size() << " additive fields, "
                  << plan.override_basis_fields.size() << " override fields, "
                  << plan.post_ops.size() << " post-ops"
                  << " (magnitude=" << plan.current_magnitude_total << "/" << plan.additive_magnitude_budget << ")"
                  << std::endl;

        if (!plan.warnings.empty()) {
            std::cerr << "[FeaturePlan] Warnings:" << std::endl;
            for (const auto& warning : plan.warnings) {
                std::cerr << "  - " << warning << std::endl;
            }
        }

        if (!plan.post_ops.empty()) {
            std::cerr << "[FeaturePlan] Post-op execution order:" << std::endl;
            for (const auto& op : plan.post_ops) {
                std::cerr << "  - " << terminalFeatureShortName(op.feature)
                          << " (priority=" << TensorPostOp::executionPriority(op.kind) << ")"
                          << std::endl;
            }
        }
    }
#endif

    return plan;
}

void applyTensorPostOps(
    Core::Tensor2D& tensor,
    const Core::Vec2& world_pos,
    const std::vector<TensorPostOp>& post_ops) {
    double angle = tensor.angle();
    const double magnitude = std::max(1e-5, std::sqrt(tensor.m0 * tensor.m0 + tensor.m1 * tensor.m1));

    for (const auto& op : post_ops) {
        if (op.radius <= 1e-6) {
            continue;
        }

        const Core::Vec2 rel = world_pos - op.center;
        const double dist = rel.length();
        if (dist > op.radius) {
            continue;
        }

        const double radial = dist / op.radius;
        const double influence = 1.0 - smoothstep(0.0, 1.0, radial);

        switch (op.kind) {
            case TensorPostOp::Kind::AngleNoiseWarp: {
                const double noise = coherentNoise(world_pos, static_cast<double>(op.b), op.seed);
                angle += noise * static_cast<double>(op.a) * influence;
                break;
            }
            case TensorPostOp::Kind::AngularSnap: {
                const double step = std::max(1e-3, static_cast<double>(op.a));
                const double base = op.theta;
                const double snapped = base + std::round((angle - base) / step) * step;
                angle = blendAngle(angle, snapped, static_cast<double>(op.b) * influence);
                break;
            }
            case TensorPostOp::Kind::AxisLock: {
                const double a0 = op.theta;
                const double a1 = op.theta + std::numbers::pi * 0.5;
                const double d0 = angleDistance(angle, a0);
                const double d1 = angleDistance(angle, a1);
                const double target = (d0 <= d1) ? a0 : a1;
                angle = blendAngle(angle, target, static_cast<double>(op.a) * influence);
                break;
            }
            case TensorPostOp::Kind::SpiralWarp: {
                const double log_term = std::log1p(std::max(0.0, dist) / std::max(1.0, op.radius * 0.2));
                angle += static_cast<double>(op.a) * log_term * influence;
                break;
            }
            case TensorPostOp::Kind::SmoothAxisBlend: {
                double blend = static_cast<double>(op.a) * influence;
                if (op.b > 1e-4f) {
                    const double edge = smoothstep(0.65, 1.0, radial);
                    blend *= (1.0 - edge) + edge * static_cast<double>(op.b);
                }
                angle = blendAngle(angle, op.theta, blend);
                break;
            }
            case TensorPostOp::Kind::AngleToleranceCull: {
                const double tolerance = std::max(1e-4, static_cast<double>(op.a));
                if (angleDistance(angle, op.theta) > tolerance) {
                    angle = blendAngle(angle, op.theta, 0.7 * influence);
                }
                break;
            }
            case TensorPostOp::Kind::OneWayBias: {
                angle = blendAngle(angle, op.theta, static_cast<double>(op.a) * influence);
                break;
            }
            case TensorPostOp::Kind::SinusoidalAxisWarp: {
                const Core::Vec2 dir(std::cos(op.theta), std::sin(op.theta));
                const double axial = rel.dot(dir);
                const double oscillation = std::sin(axial * static_cast<double>(op.b));
                angle += oscillation * static_cast<double>(op.a) * influence;
                break;
            }
            case TensorPostOp::Kind::EndTaper: {
                const double edge = smoothstep(0.6, 1.0, radial);
                const double tangent = std::atan2(rel.y, rel.x) + std::numbers::pi * 0.5;
                angle = blendAngle(angle, tangent, edge * static_cast<double>(op.a));
                break;
            }
            case TensorPostOp::Kind::RadiusPulse: {
                const double phase = hashToUnit(op.seed, 97u) * 2.0 * std::numbers::pi;
                const double pulse = std::sin(radial * static_cast<double>(op.b) * 2.0 * std::numbers::pi + phase);
                angle += pulse * static_cast<double>(op.a) * influence;
                break;
            }
        }
    }

    tensor = Core::Tensor2D::fromAngle(angle);
    tensor.scale(magnitude);
}

// ============================================================================
// FEATURE CONFLICT VALIDATION FOR EXTERNAL USE
// ============================================================================
// Validates a set of terminal features for internal conflicts.
// Called by CityGenerator::ValidateAxioms to provide early error feedback
// before tensor field generation.
// ============================================================================
std::vector<std::string> validateFeatureConflicts(const TerminalFeatureSet& features) {
    std::vector<std::string> conflicts;
    if (features.empty()) {
        return conflicts;
    }

    // Collect enabled features
    std::vector<TerminalFeature> enabled;
    // Check all possible terminal features (40 total based on kTerminalFeatureCount)
    for (uint8_t i = 0; i < kTerminalFeatureCount; ++i) {
        const TerminalFeature feature = static_cast<TerminalFeature>(i);
        if (features.has(feature)) {
            enabled.push_back(feature);
        }
    }

    // Check each pair against conflict table
    for (size_t i = 0; i < enabled.size(); ++i) {
        for (size_t j = i + 1; j < enabled.size(); ++j) {
            const TerminalFeature a = enabled[i];
            const TerminalFeature b = enabled[j];
            const FeatureConflict* conflict = nullptr;

            if (hasFeatureConflict(a, b, &conflict)) {
                std::string msg = "Feature conflict: ";
                msg += terminalFeatureShortName(a);
                msg += " and ";
                msg += terminalFeatureShortName(b);
                msg += " - ";
                msg += conflict->reason;
                conflicts.push_back(msg);
            }
        }
    }

    return conflicts;
}

} // namespace RogueCity::Generators
