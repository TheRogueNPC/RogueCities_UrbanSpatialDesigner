#pragma once

#include "RogueCity/Core/Types.hpp"

#include <cstdint>
#include <vector>

/// @file GeometrySanitizer.hpp
/// @brief Ring-level geometry validity checking and repair.
///
/// Provides deterministic, Boost-free operations for the most common geometry
/// corruption scenarios encountered before offsetting or polygonization:
///   - Near-duplicate / zero-length segment removal (snap epsilon)
///   - Ring winding normalization to CCW (counter-clockwise)
///   - Self-intersection detection (Shamos-Hoey O(n²) for small rings)
///   - Optional Ramer-Douglas-Peucker simplification
///
/// ## Architecture position
///   generators/Geometry — no UI, no pipeline mutation
///
/// ## Determinism
/// All operations are deterministic (same input → same output).
/// No random state is used.  DeterminismHash is unaffected.
///
/// ## Integration points
/// Call `Sanitize()` after ROADG inner-network polygon extraction and
/// before Clipper2 offset / lot subdivision operations.

namespace RogueCity::Generators::Geometry {

    using Ring = std::vector<Core::Vec2>;

    /// Per-ring diagnostic result returned by Sanitize().
    struct SanitizeResult {
        Ring ring{};                   ///< Cleaned ring (may be empty if unrepairable)
        bool was_modified       = false; ///< True if any transformation was applied
        bool has_self_intersection = false; ///< True if self-intersection was detected
        bool is_valid           = false; ///< True if output ring is usable (≥ 3 pts)
    };

    /// Parameters controlling the sanitize pipeline.
    struct SanitizeOptions {
        /// Points closer than this (Euclidean distance in world units / metres)
        /// to a preceding point are merged.  Default conservative for city scale.
        double snap_epsilon = 1e-3;

        /// After deduplication, if an edge is shorter than this, it is considered
        /// degenerate and removed.  Must be ≥ snap_epsilon.
        double min_segment_length = 1e-3;

        /// If true, apply RDP simplification after winding normalization.
        /// Tolerance is `rdp_tolerance` below.
        bool simplify = false;

        /// RDP tolerance in world units, active only when simplify==true.
        double rdp_tolerance = 0.1;
    };

    class GeometrySanitizer {
    public:
        /// Full pipeline: snap → deduplicate degenerate segments →
        /// normalize winding → detect self-intersection.
        ///
        /// @param ring     Input ring (closing duplicate point allowed).
        /// @param options  Sanitization parameters.
        /// @return         SanitizeResult with the repaired ring and diagnostic flags.
        [[nodiscard]] static SanitizeResult Sanitize(
            const Ring& ring,
            const SanitizeOptions& options = {});

        /// Compute the signed area of a ring (positive = CCW, negative = CW).
        /// Degenerate rings (< 3 points) return 0.0.
        [[nodiscard]] static double SignedArea(const Ring& ring) noexcept;

        /// Return a copy of the ring with vertices in CCW order.
        /// If the ring has < 3 points it is returned unchanged.
        [[nodiscard]] static Ring NormalizeToCCW(const Ring& ring);

        /// Remove consecutive near-duplicate points (distance < snap_epsilon).
        /// Also removes the explicit closing duplicate if present.
        [[nodiscard]] static Ring SnapAndDeduplicate(
            const Ring& ring,
            double snap_epsilon);

        /// Remove edges shorter than min_length from a cleaned ring.
        [[nodiscard]] static Ring RemoveDegenerateSegments(
            const Ring& ring,
            double min_length);

        /// Detect self-intersection using O(n²) segment-pair test.
        /// Sufficient for the small rings (< 200 vertices) produced by the
        /// lot/block generators; replace with a sweep-line if rings grow larger.
        [[nodiscard]] static bool HasSelfIntersection(const Ring& ring) noexcept;

        /// Ramer-Douglas-Peucker simplification.
        [[nodiscard]] static Ring Simplify(const Ring& ring, double tolerance);
    };

} // namespace RogueCity::Generators::Geometry
