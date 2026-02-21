#include "RogueCity/Generators/Tensors/TerminalFeatureApplier.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
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

    const auto allowed = featuresForAxiomTypeRaw(axiom_type_raw);
    for (const TerminalFeature feature : allowed) {
        if (!features.has(feature)) {
            continue;
        }

        const uint32_t op_seed = seed ^ static_cast<uint32_t>(feature) * 2654435761u;
        const double jitter = hashToUnit(op_seed, 17u) * 2.0 - 1.0;

        switch (feature) {
            case TerminalFeature::Organic_TopologicalFlowConstraint:
                plan.additive_basis_fields.push_back(std::make_unique<NoisePatchField>(
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
                plan.additive_basis_fields.push_back(std::make_unique<StrongLinearCorridorField>(
                    params.center, params.radius, params.theta + std::numbers::pi * 0.25, params.radius * 0.2, params.decay));
                break;
            case TerminalFeature::Grid_AlleywayBisection:
                plan.additive_basis_fields.push_back(std::make_unique<StrongLinearCorridorField>(
                    params.center, params.radius * 0.95, params.theta, params.radius * 0.11, params.decay + 0.5));
                plan.additive_basis_fields.push_back(std::make_unique<StrongLinearCorridorField>(
                    params.center, params.radius * 0.95, params.theta + std::numbers::pi * 0.5, params.radius * 0.11, params.decay + 0.5));
                break;
            case TerminalFeature::Grid_BlockFusion:
                plan.additive_basis_fields.push_back(std::make_unique<NoisePatchField>(
                    params.center, params.radius, params.theta, 0.013, 0.45, params.decay + 0.3));
                break;

            case TerminalFeature::Radial_SpiralDominance:
                pushPostOp(plan, TensorPostOp::Kind::SpiralWarp, feature, params, 0.28f, 0.0f, 0.0f, op_seed);
                break;
            case TerminalFeature::Radial_CoreVoiding:
                plan.override_basis_fields.push_back(std::make_unique<CenterVoidOverrideField>(
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
                plan.additive_basis_fields.push_back(std::make_unique<StrongLinearCorridorField>(
                    params.center, params.radius, params.theta + std::numbers::pi / 6.0, params.radius * 0.17, params.decay + 0.2));
                break;
            case TerminalFeature::Hex_OffsetStagger:
                plan.additive_basis_fields.push_back(std::make_unique<ShearPlaneField>(
                    params.center, params.radius, params.theta, std::numbers::pi / 10.0, params.decay));
                break;
            case TerminalFeature::Hex_OrganicEdgeBleed:
                pushPostOp(plan, TensorPostOp::Kind::AngleNoiseWarp, feature, params, 0.20f, 0.0048f, 0.0f, op_seed);
                break;

            case TerminalFeature::Stem_FractalRecursion:
                plan.additive_basis_fields.push_back(std::make_unique<ParallelLinearFieldBundle>(
                    params.center, params.radius, params.theta, params.radius * 0.18, 2, params.decay + 0.4));
                break;
            case TerminalFeature::Stem_DirectionalFlowBias:
                pushPostOp(plan, TensorPostOp::Kind::OneWayBias, feature, params, 0.62f, 0.0f, 0.0f, op_seed);
                break;
            case TerminalFeature::Stem_CanopyWeave:
                pushPostOp(plan, TensorPostOp::Kind::SinusoidalAxisWarp, feature, params, 0.23f, 0.008f, 0.0f, op_seed);
                break;
            case TerminalFeature::Stem_TerminalLoops:
                plan.additive_basis_fields.push_back(std::make_unique<NoisePatchField>(
                    params.center, params.radius, params.theta + std::numbers::pi * 0.5, 0.0105, 0.62, params.decay + 0.4));
                break;

            case TerminalFeature::LooseGrid_HistoricalFaultLines:
                plan.additive_basis_fields.push_back(std::make_unique<ShearPlaneField>(
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
                plan.additive_basis_fields.push_back(std::make_unique<BoundarySeamField>(
                    params.center, params.radius, params.theta, 0.2, params.decay));
                break;
            case TerminalFeature::Suburban_TerrainAvoidance:
                pushPostOp(plan, TensorPostOp::Kind::AngleNoiseWarp, feature, params, 0.18f, 0.0045f, 0.0f, op_seed);
                break;
            case TerminalFeature::Suburban_HierarchicalStrictness:
                pushPostOp(plan, TensorPostOp::Kind::EndTaper, feature, params, 0.48f, 0.0f, 0.0f, op_seed);
                break;

            case TerminalFeature::Superblock_PedestrianMicroField:
                plan.additive_basis_fields.push_back(std::make_unique<NoisePatchField>(
                    params.center, params.radius, params.theta + std::numbers::pi * 0.25, 0.019, 0.52, params.decay + 0.5));
                break;
            case TerminalFeature::Superblock_ArterialTrenching:
                plan.additive_basis_fields.push_back(std::make_unique<BoundarySeamField>(
                    params.center, params.radius, params.theta, 0.15, params.decay));
                pushPostOp(plan, TensorPostOp::Kind::EndTaper, feature, params, 0.65f, 0.0f, 0.0f, op_seed);
                break;
            case TerminalFeature::Superblock_CourtyardVoid:
                plan.override_basis_fields.push_back(std::make_unique<CenterVoidOverrideField>(
                    params.center, params.radius, params.theta, 0.30, params.decay + 0.2));
                break;
            case TerminalFeature::Superblock_PermeableEdges:
                pushPostOp(plan, TensorPostOp::Kind::SmoothAxisBlend, feature, params, 0.28f, 0.65f, 0.0f, op_seed);
                break;

            case TerminalFeature::Linear_RibbonBraiding:
                pushPostOp(plan, TensorPostOp::Kind::SinusoidalAxisWarp, feature, params, 0.34f, 0.01f, 0.0f, op_seed);
                break;
            case TerminalFeature::Linear_ParallelCascading:
                plan.additive_basis_fields.push_back(std::make_unique<ParallelLinearFieldBundle>(
                    params.center, params.radius, params.theta, params.radius * 0.17, 3, params.decay));
                break;
            case TerminalFeature::Linear_PerpendicularRungs:
                plan.additive_basis_fields.push_back(std::make_unique<PeriodicRungField>(
                    params.center, params.radius, params.theta, params.radius * 0.2, 0.22, params.decay));
                break;
            case TerminalFeature::Linear_TaperedTerminals:
                pushPostOp(plan, TensorPostOp::Kind::EndTaper, feature, params, 0.75f, 0.0f, 0.0f, op_seed);
                break;

            case TerminalFeature::GridCorrective_AbsoluteOverride:
                plan.override_basis_fields.push_back(std::make_unique<StrongLinearCorridorField>(
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
                plan.additive_basis_fields.push_back(std::make_unique<BoundarySeamField>(
                    params.center, params.radius, params.theta, 0.18, params.decay + 0.1));
                break;
        }
    }

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

} // namespace RogueCity::Generators
