#pragma once

#include "RogueCity/Generators/Tensors/AxiomTerminalFeatures.hpp"
#include "RogueCity/Generators/Tensors/BasisFields.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace RogueCity::Generators {

struct AxiomTensorParams {
    Core::Vec2 center{};
    double radius{ 0.0 };
    double theta{ 0.0 };
    double decay{ 2.0 };

    float organic_curviness{ 0.5f };
    int radial_spokes{ 8 };
    float loose_grid_jitter{ 0.15f };
    float suburban_loop_strength{ 0.7f };
    float stem_branch_angle{ 0.7f };
    float superblock_block_size{ 250.0f };

    double radial_ring_rotation{ 0.0 };
    std::array<std::array<float, 4>, 3> radial_ring_knob_weights{{
        {{1.0f, 1.0f, 1.0f, 1.0f}},
        {{1.0f, 1.0f, 1.0f, 1.0f}},
        {{1.0f, 1.0f, 1.0f, 1.0f}}
    }};
};

struct TensorPostOp {
    enum class Kind : uint8_t {
        AngleNoiseWarp,
        AngularSnap,
        AxisLock,
        SpiralWarp,
        SmoothAxisBlend,
        AngleToleranceCull,
        OneWayBias,
        SinusoidalAxisWarp,
        EndTaper,
        RadiusPulse
    };

    Kind kind{ Kind::AngleNoiseWarp };
    TerminalFeature feature{ TerminalFeature::Organic_MeanderBias };
    Core::Vec2 center{};
    double radius{ 0.0 };
    double theta{ 0.0 };
    float a{ 0.0f };
    float b{ 0.0f };
    float c{ 0.0f };
    uint32_t seed{ 0u };

    [[nodiscard]] static constexpr int executionPriority(Kind k) noexcept {
        switch (k) {
            case Kind::AngleNoiseWarp:       return 10;
            case Kind::SinusoidalAxisWarp:   return 11;
            case Kind::SpiralWarp:           return 20;
            case Kind::AngularSnap:          return 30;
            case Kind::AxisLock:             return 31;
            case Kind::SmoothAxisBlend:      return 40;
            case Kind::OneWayBias:           return 41;
            case Kind::AngleToleranceCull:   return 50;
            case Kind::EndTaper:             return 60;
            case Kind::RadiusPulse:          return 61;
        }
        return 100;
    }
};

struct FeaturePlan {
    std::vector<std::unique_ptr<BasisField>> additive_basis_fields{};
    std::vector<std::unique_ptr<BasisField>> override_basis_fields{};
    std::vector<TensorPostOp> post_ops{};
    std::vector<std::string> warnings{};

    double additive_magnitude_budget{ 0.7 };
    double current_magnitude_total{ 0.0 };

    void sortPostOps() {
        std::sort(post_ops.begin(), post_ops.end(),
            [](const TensorPostOp& a, const TensorPostOp& b) {
                return TensorPostOp::executionPriority(a.kind) < TensorPostOp::executionPriority(b.kind);
            });
    }
};

[[nodiscard]] FeaturePlan buildFeaturePlanRaw(
    uint8_t axiom_type_raw,
    const AxiomTensorParams& params,
    const TerminalFeatureSet& features,
    uint32_t seed);

template <typename TAxiomType>
[[nodiscard]] FeaturePlan buildFeaturePlan(
    TAxiomType axiom_type,
    const AxiomTensorParams& params,
    const TerminalFeatureSet& features,
    uint32_t seed = 0u) {
    return buildFeaturePlanRaw(static_cast<uint8_t>(axiom_type), params, features, seed);
}

void applyTensorPostOps(
    Core::Tensor2D& tensor,
    const Core::Vec2& world_pos,
    const std::vector<TensorPostOp>& post_ops);

// ============================================================================
// FEATURE CONFLICT VALIDATION
// ============================================================================
// Validates a set of terminal features for internal conflicts.
// Returns a vector of human-readable conflict descriptions.
// Used by CityGenerator::ValidateAxioms to catch invalid feature combinations
// before tensor field generation.
// ============================================================================
[[nodiscard]] std::vector<std::string> validateFeatureConflicts(
    const TerminalFeatureSet& features);

} // namespace RogueCity::Generators
