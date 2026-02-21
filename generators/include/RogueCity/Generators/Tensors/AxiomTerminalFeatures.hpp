#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace RogueCity::Generators {

enum class TerminalFeature : uint8_t {
    Organic_TopologicalFlowConstraint = 0,
    Organic_MeanderBias,
    Organic_VoronoiRelaxation,
    Organic_CulDeSacPruning,

    Grid_AxisAlignmentLock,
    Grid_DiagonalSlicing,
    Grid_AlleywayBisection,
    Grid_BlockFusion,

    Radial_SpiralDominance,
    Radial_CoreVoiding,
    Radial_SpokePruning,
    Radial_ConcentricWaveDensity,

    Hex_HoneycombStrictness,
    Hex_TriangularSubdivision,
    Hex_OffsetStagger,
    Hex_OrganicEdgeBleed,

    Stem_FractalRecursion,
    Stem_DirectionalFlowBias,
    Stem_CanopyWeave,
    Stem_TerminalLoops,

    LooseGrid_HistoricalFaultLines,
    LooseGrid_JitterPersistence,
    LooseGrid_TJunctionForcing,
    LooseGrid_CenterWeightedDensity,

    Suburban_LollipopTerminals,
    Suburban_ArterialIsolation,
    Suburban_TerrainAvoidance,
    Suburban_HierarchicalStrictness,

    Superblock_PedestrianMicroField,
    Superblock_ArterialTrenching,
    Superblock_CourtyardVoid,
    Superblock_PermeableEdges,

    Linear_RibbonBraiding,
    Linear_ParallelCascading,
    Linear_PerpendicularRungs,
    Linear_TaperedTerminals,

    GridCorrective_AbsoluteOverride,
    GridCorrective_MagneticAlignment,
    GridCorrective_OrthogonalCull,
    GridCorrective_BoundaryStitching,
};

constexpr size_t kTerminalFeatureCount = 40;

struct TerminalFeatureSet {
    uint64_t bits{ 0 };

    [[nodiscard]] bool has(TerminalFeature feature) const {
        return (bits & (1ull << static_cast<uint8_t>(feature))) != 0;
    }

    void set(TerminalFeature feature, bool enabled) {
        const uint64_t mask = (1ull << static_cast<uint8_t>(feature));
        if (enabled) {
            bits |= mask;
        } else {
            bits &= ~mask;
        }
    }

    [[nodiscard]] bool empty() const {
        return bits == 0;
    }
};

[[nodiscard]] constexpr std::array<TerminalFeature, 4> featuresForAxiomTypeRaw(uint8_t type_raw) {
    switch (type_raw) {
        case 0:
            return {TerminalFeature::Organic_TopologicalFlowConstraint,
                TerminalFeature::Organic_MeanderBias,
                TerminalFeature::Organic_VoronoiRelaxation,
                TerminalFeature::Organic_CulDeSacPruning};
        case 1:
            return {TerminalFeature::Grid_AxisAlignmentLock,
                TerminalFeature::Grid_DiagonalSlicing,
                TerminalFeature::Grid_AlleywayBisection,
                TerminalFeature::Grid_BlockFusion};
        case 2:
            return {TerminalFeature::Radial_SpiralDominance,
                TerminalFeature::Radial_CoreVoiding,
                TerminalFeature::Radial_SpokePruning,
                TerminalFeature::Radial_ConcentricWaveDensity};
        case 3:
            return {TerminalFeature::Hex_HoneycombStrictness,
                TerminalFeature::Hex_TriangularSubdivision,
                TerminalFeature::Hex_OffsetStagger,
                TerminalFeature::Hex_OrganicEdgeBleed};
        case 4:
            return {TerminalFeature::Stem_FractalRecursion,
                TerminalFeature::Stem_DirectionalFlowBias,
                TerminalFeature::Stem_CanopyWeave,
                TerminalFeature::Stem_TerminalLoops};
        case 5:
            return {TerminalFeature::LooseGrid_HistoricalFaultLines,
                TerminalFeature::LooseGrid_JitterPersistence,
                TerminalFeature::LooseGrid_TJunctionForcing,
                TerminalFeature::LooseGrid_CenterWeightedDensity};
        case 6:
            return {TerminalFeature::Suburban_LollipopTerminals,
                TerminalFeature::Suburban_ArterialIsolation,
                TerminalFeature::Suburban_TerrainAvoidance,
                TerminalFeature::Suburban_HierarchicalStrictness};
        case 7:
            return {TerminalFeature::Superblock_PedestrianMicroField,
                TerminalFeature::Superblock_ArterialTrenching,
                TerminalFeature::Superblock_CourtyardVoid,
                TerminalFeature::Superblock_PermeableEdges};
        case 8:
            return {TerminalFeature::Linear_RibbonBraiding,
                TerminalFeature::Linear_ParallelCascading,
                TerminalFeature::Linear_PerpendicularRungs,
                TerminalFeature::Linear_TaperedTerminals};
        case 9:
            return {TerminalFeature::GridCorrective_AbsoluteOverride,
                TerminalFeature::GridCorrective_MagneticAlignment,
                TerminalFeature::GridCorrective_OrthogonalCull,
                TerminalFeature::GridCorrective_BoundaryStitching};
        default:
            return {TerminalFeature::Grid_AxisAlignmentLock,
                TerminalFeature::Grid_DiagonalSlicing,
                TerminalFeature::Grid_AlleywayBisection,
                TerminalFeature::Grid_BlockFusion};
    }
}

template <typename TAxiomType>
[[nodiscard]] constexpr std::array<TerminalFeature, 4> featuresForAxiomType(TAxiomType type) {
    return featuresForAxiomTypeRaw(static_cast<uint8_t>(type));
}

template <typename TAxiomType>
[[nodiscard]] constexpr bool featureAllowedForType(TAxiomType type, TerminalFeature feature) {
    const auto allowed = featuresForAxiomType(type);
    return allowed[0] == feature || allowed[1] == feature || allowed[2] == feature || allowed[3] == feature;
}

[[nodiscard]] inline std::string_view terminalFeatureShortName(TerminalFeature feature) {
    switch (feature) {
        case TerminalFeature::Organic_TopologicalFlowConstraint: return "Topological Flow";
        case TerminalFeature::Organic_MeanderBias: return "Meander Bias";
        case TerminalFeature::Organic_VoronoiRelaxation: return "Voronoi Relaxation";
        case TerminalFeature::Organic_CulDeSacPruning: return "Cul-de-sac Pruning";
        case TerminalFeature::Grid_AxisAlignmentLock: return "Axis Alignment Lock";
        case TerminalFeature::Grid_DiagonalSlicing: return "Diagonal Slicing";
        case TerminalFeature::Grid_AlleywayBisection: return "Alleyway Bisection";
        case TerminalFeature::Grid_BlockFusion: return "Block Fusion";
        case TerminalFeature::Radial_SpiralDominance: return "Spiral Dominance";
        case TerminalFeature::Radial_CoreVoiding: return "Core Voiding";
        case TerminalFeature::Radial_SpokePruning: return "Spoke Pruning";
        case TerminalFeature::Radial_ConcentricWaveDensity: return "Concentric Wave Density";
        case TerminalFeature::Hex_HoneycombStrictness: return "Honeycomb Strictness";
        case TerminalFeature::Hex_TriangularSubdivision: return "Triangular Subdivision";
        case TerminalFeature::Hex_OffsetStagger: return "Offset Stagger";
        case TerminalFeature::Hex_OrganicEdgeBleed: return "Organic Edge Bleed";
        case TerminalFeature::Stem_FractalRecursion: return "Fractal Recursion";
        case TerminalFeature::Stem_DirectionalFlowBias: return "Directional Flow Bias";
        case TerminalFeature::Stem_CanopyWeave: return "Canopy Weave";
        case TerminalFeature::Stem_TerminalLoops: return "Terminal Loops";
        case TerminalFeature::LooseGrid_HistoricalFaultLines: return "Historical Fault Lines";
        case TerminalFeature::LooseGrid_JitterPersistence: return "Jitter Persistence";
        case TerminalFeature::LooseGrid_TJunctionForcing: return "T-Junction Forcing";
        case TerminalFeature::LooseGrid_CenterWeightedDensity: return "Center Weighted Density";
        case TerminalFeature::Suburban_LollipopTerminals: return "Lollipop Terminals";
        case TerminalFeature::Suburban_ArterialIsolation: return "Arterial Isolation";
        case TerminalFeature::Suburban_TerrainAvoidance: return "Terrain Avoidance";
        case TerminalFeature::Suburban_HierarchicalStrictness: return "Hierarchical Strictness";
        case TerminalFeature::Superblock_PedestrianMicroField: return "Pedestrian Micro-Field";
        case TerminalFeature::Superblock_ArterialTrenching: return "Arterial Trenching";
        case TerminalFeature::Superblock_CourtyardVoid: return "Courtyard Void";
        case TerminalFeature::Superblock_PermeableEdges: return "Permeable Edges";
        case TerminalFeature::Linear_RibbonBraiding: return "Ribbon Braiding";
        case TerminalFeature::Linear_ParallelCascading: return "Parallel Cascading";
        case TerminalFeature::Linear_PerpendicularRungs: return "Perpendicular Rungs";
        case TerminalFeature::Linear_TaperedTerminals: return "Tapered Terminals";
        case TerminalFeature::GridCorrective_AbsoluteOverride: return "Absolute Override";
        case TerminalFeature::GridCorrective_MagneticAlignment: return "Magnetic Alignment";
        case TerminalFeature::GridCorrective_OrthogonalCull: return "Orthogonal Cull";
        case TerminalFeature::GridCorrective_BoundaryStitching: return "Boundary Stitching";
    }

    return "Unknown";
}

[[nodiscard]] inline std::string_view terminalFeatureInfluenceHint(TerminalFeature feature) {
    switch (feature) {
        case TerminalFeature::Organic_TopologicalFlowConstraint: return "Aligns flow to terrain-like gradients.";
        case TerminalFeature::Organic_MeanderBias: return "Adds low-frequency bend noise.";
        case TerminalFeature::Organic_VoronoiRelaxation: return "Quantizes organic flow into tri-directional strands.";
        case TerminalFeature::Organic_CulDeSacPruning: return "Suppresses lateral drift near boundaries.";
        case TerminalFeature::Grid_AxisAlignmentLock: return "Locks field to orthogonal axes.";
        case TerminalFeature::Grid_DiagonalSlicing: return "Injects a high-strength diagonal corridor.";
        case TerminalFeature::Grid_AlleywayBisection: return "Adds finer interior guidance lines.";
        case TerminalFeature::Grid_BlockFusion: return "Breaks strict regularity with patch variation.";
        case TerminalFeature::Radial_SpiralDominance: return "Rotates spokes into spiraling flow.";
        case TerminalFeature::Radial_CoreVoiding: return "Overrides the inner core with tangential flow.";
        case TerminalFeature::Radial_SpokePruning: return "Removes every second spoke direction.";
        case TerminalFeature::Radial_ConcentricWaveDensity: return "Adds radial wave modulation.";
        case TerminalFeature::Hex_HoneycombStrictness: return "Snaps orientation to 60-degree families.";
        case TerminalFeature::Hex_TriangularSubdivision: return "Adds cross-diagonal triangular bias.";
        case TerminalFeature::Hex_OffsetStagger: return "Offsets local orientation per cell band.";
        case TerminalFeature::Hex_OrganicEdgeBleed: return "Softens strictness near the border.";
        case TerminalFeature::Stem_FractalRecursion: return "Adds nested stem directions.";
        case TerminalFeature::Stem_DirectionalFlowBias: return "Biases direction along the stem axis.";
        case TerminalFeature::Stem_CanopyWeave: return "Adds tip-side weave curl.";
        case TerminalFeature::Stem_TerminalLoops: return "Injects loop tendencies toward endpoints.";
        case TerminalFeature::LooseGrid_HistoricalFaultLines: return "Shears grid orientation around a fault line.";
        case TerminalFeature::LooseGrid_JitterPersistence: return "Locks jitter as coherent long-wave drift.";
        case TerminalFeature::LooseGrid_TJunctionForcing: return "Suppresses crossings into T tendencies.";
        case TerminalFeature::LooseGrid_CenterWeightedDensity: return "Strengthens central guidance.";
        case TerminalFeature::Suburban_LollipopTerminals: return "Strengthens cul-de-sac loop behavior.";
        case TerminalFeature::Suburban_ArterialIsolation: return "Creates edge repulsion with entry slots.";
        case TerminalFeature::Suburban_TerrainAvoidance: return "Deflects from terrain-like gradients.";
        case TerminalFeature::Suburban_HierarchicalStrictness: return "Applies tiered distance guidance.";
        case TerminalFeature::Superblock_PedestrianMicroField: return "Adds fine-grain interior guidance.";
        case TerminalFeature::Superblock_ArterialTrenching: return "Boosts boundary axes and hollows interior.";
        case TerminalFeature::Superblock_CourtyardVoid: return "Zeroes center influence with override mask.";
        case TerminalFeature::Superblock_PermeableEdges: return "Keeps neighboring flow alive near edges.";
        case TerminalFeature::Linear_RibbonBraiding: return "Applies sinusoidal ribbon braiding.";
        case TerminalFeature::Linear_ParallelCascading: return "Adds parallel companion corridors.";
        case TerminalFeature::Linear_PerpendicularRungs: return "Injects periodic rung directions.";
        case TerminalFeature::Linear_TaperedTerminals: return "Tapers influence near corridor endpoints.";
        case TerminalFeature::GridCorrective_AbsoluteOverride: return "Overrides local tensors completely.";
        case TerminalFeature::GridCorrective_MagneticAlignment: return "Smoothly pulls angles toward corrective axis.";
        case TerminalFeature::GridCorrective_OrthogonalCull: return "Damps directions outside tolerance.";
        case TerminalFeature::GridCorrective_BoundaryStitching: return "Adds seam guidance around corrective edge.";
    }

    return "No influence hint.";
}

} // namespace RogueCity::Generators
