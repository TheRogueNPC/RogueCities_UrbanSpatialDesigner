#pragma once

/// @file RoutingPolicy.hpp
/// @brief Configurable routing policy façade for the RogueCity generator stack.
///
/// Provides a single `RoutingPolicy::compute()` entry-point that selects between
/// three weight modes (LengthOnly, LengthPlusTurnPenalty, HierarchyBiased) and
/// optionally records stage timing via a user-supplied callback.
///
/// ## Architecture position
/// ```
/// generators/Urban/Graph + GraphAlgorithms     (base graph layer)
///        ↓
/// generators/Policy/RoutingPolicy              (this file – policy façade)
///        ↓
/// generators/Pipeline/CityGenerator            (consumer)
/// ```
///
/// ## Invariants preserved
/// - All modes return a valid `PathResult` (possibly empty if disconnected).
/// - No Graph or GraphAlgorithms signatures are modified.
/// - Edge weight is always ≥ 0.001 m; hierarchy multipliers never make it zero.
/// - DeterminismHash is not affected: policy is stateless, data unchanged.
///
/// ## Weight mode semantics
/// | Mode                  | Formula (per edge)                         |
/// |-----------------------|--------------------------------------------|
/// | LengthOnly            | `max(0.001, edge.length)`                  |
/// | LengthPlusTurnPenalty | length + turn_penalty_meters at each turn  |
/// | HierarchyBiased       | `length / road_type_multiplier(edge.type)` |
///
/// HierarchyBiased multipliers (lower weight → router prefers higher roads):
/// - Highway=4.0, Arterial=3.0, Avenue=2.5, Boulevard=2.0, Street/others=1.0
///
/// ## Profiling hook
/// Supply a non-null `RoutingTimingCallback` to receive `(label, nanoseconds)`
/// after each `compute()` call.  Pass `nullptr` for zero overhead.

#include "RogueCity/Generators/Urban/Graph.hpp"
#include "RogueCity/Generators/Urban/GraphAlgorithms.hpp"

#include <functional>

namespace RogueCity::Generators::Policy {

    // =========================================================================
    // WeightMode
    // =========================================================================

    /// Selects the edge-weight function used during routing.
    enum class WeightMode : uint8_t {
        /// Pure geometric length (identical to GraphAlgorithms::shortestPath).
        LengthOnly = 0,

        /// Length plus a fixed penalty added at every intermediate turn.
        /// Encourages routes with fewer direction changes, e.g. arterials.
        LengthPlusTurnPenalty,

        /// Length discounted by road-class multiplier.
        /// Routes favour higher-class roads even when geometrically longer.
        HierarchyBiased,
    };

    // =========================================================================
    // RoutingPolicyConfig
    // =========================================================================

    /// Parameters controlling a single routing query.
    struct RoutingPolicyConfig {
        /// Weight function to apply.
        WeightMode mode = WeightMode::LengthOnly;

        /// Added cost (meters equivalent) at each intermediate vertex where the
        /// direction changes, used only in LengthPlusTurnPenalty mode.
        /// Invariant: must be ≥ 0.
        double turn_penalty_meters = 10.0;
    };

    // =========================================================================
    // Timing callback
    // =========================================================================

    /// Optional profiling hook.  Called once per compute() with:
    ///   @p label  – short human-readable tag (e.g. "RoutingPolicy::LengthOnly").
    ///   @p ns     – wall-clock elapsed nanoseconds for the query.
    /// Pass as nullptr (or default-constructed std::function) to disable.
    using RoutingTimingCallback =
        std::function<void(const char* label, long long ns)>;

    // =========================================================================
    // RoutingPolicy
    // =========================================================================

    /// Stateless policy façade. All methods are static; no state is mutated.
    class RoutingPolicy {
    public:
        /// Compute the route from @p src to @p dst under the given policy.
        ///
        /// @param g          Read-only road graph.
        /// @param src        Source vertex ID.
        /// @param dst        Destination vertex ID.
        /// @param cfg        Weight mode and parameters.
        /// @param timing_cb  Optional timing callback; nullable.
        /// @return           PathResult (reachable() == false if disconnected).
        [[nodiscard]] static Urban::PathResult compute(
            const Urban::Graph&         g,
            Urban::VertexID             src,
            Urban::VertexID             dst,
            const RoutingPolicyConfig&  cfg,
            const RoutingTimingCallback& timing_cb = nullptr);

        /// Returns the hierarchy multiplier for a given road type.
        /// Higher multiplier → lower effective weight in HierarchyBiased mode.
        /// Public to allow callers to reason about the discount schedule.
        [[nodiscard]] static double hierarchyMultiplier(
            Core::RoadType type) noexcept;
    };

} // namespace RogueCity::Generators::Policy
