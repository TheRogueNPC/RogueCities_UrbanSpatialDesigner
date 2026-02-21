#pragma once

#include "RogueCity/Generators/Tensors/AxiomTerminalFeatures.hpp"
#include "RogueCity/Generators/Tensors/BasisFields.hpp"

#include <array>
#include <cstdint>
#include <memory>
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
};

struct FeaturePlan {
    std::vector<std::unique_ptr<BasisField>> additive_basis_fields{};
    std::vector<std::unique_ptr<BasisField>> override_basis_fields{};
    std::vector<TensorPostOp> post_ops{};
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

} // namespace RogueCity::Generators
