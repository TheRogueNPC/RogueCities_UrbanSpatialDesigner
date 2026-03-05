#pragma once

#include "RogueCity/Core/Data/CityTypes.hpp"

#include <string>
#include <vector>

/// @file CityInvariants.hpp
/// @brief Cross-stage semantic invariant checks for pipeline output.
///
/// These checks express the "thesis contracts" the pipeline must satisfy:
///   - Roads:     connected (each non-isolated vertex reached), no degenerate edges
///   - Districts: polygons are closed (≥ 3 pts), no per-pair requirement (optional)
///   - Lots:      within a known district, boundary is closed
///   - Buildings: within a known lot, FBCZ height constraints satisfied if present
///
/// ## Architecture position
///   core/Validation — pure data, no generators/, no UI
///
/// ## Usage
/// ```cpp
/// CityInvariantsChecker checker{};
/// auto result = checker.Check(output.roads, output.districts,
///                             output.lots, output.buildings);
/// if (!result.IsValid()) { /* log violations */ }
/// ```
///
/// ## Determinism
/// All checks are read-only.  No data is mutated.  DeterminismHash unaffected.

namespace RogueCity::Core::Validation {

    /// Severity distinguishes informational warnings from hard failures.
    enum class ViolationSeverity : uint8_t {
        Warning = 0,  ///< Suspicious but may be intentional
        Error         ///< Contract broken; downstream stages may fail
    };

    /// A single detected invariant violation.
    struct InvariantViolation {
        std::string category{};   ///< "Roads" | "Districts" | "Lots" | "Buildings"
        std::string rule_id{};    ///< Short machine-readable key, e.g. "road.degenerate_edge"
        std::string description{};///< Human-readable description
        uint32_t entity_id{0};   ///< ID of the offending entity (road.id / lot.id / …)
        ViolationSeverity severity = ViolationSeverity::Error;
    };

    /// Aggregated result of a full invariant check pass.
    struct InvariantCheckResult {
        std::vector<InvariantViolation> violations{};

        [[nodiscard]] bool IsValid() const { return violations.empty(); }
        [[nodiscard]] size_t ErrorCount() const;
        [[nodiscard]] size_t WarningCount() const;
    };

    /// Options controlling which checks run and their thresholds.
    struct InvariantCheckOptions {
        /// Minimum road segment length (metres) before flagging as degenerate.
        double road_min_edge_length_m = 0.01;

        /// Minimum spacing (metres) between parallel roads before a warning.
        double road_min_spacing_m = 0.5;

        /// Minimum vertices for a closed district polygon.
        size_t district_min_vertices = 3;

        /// If true, also check that lot centroids fall within known district IDs.
        bool check_lot_district_membership = true;

        /// If true, check buildings against FBCZ height fields (if non-zero).
        bool check_building_fbcz = true;
    };

    /// Stateless invariant checker. All methods take const references.
    class CityInvariantsChecker {
    public:
        /// Run all enabled checks over the given pipeline outputs.
        [[nodiscard]] InvariantCheckResult Check(
            const std::vector<Core::Road>&          roads,
            const std::vector<Core::District>&      districts,
            const std::vector<Core::LotToken>&      lots,
            const siv::Vector<Core::BuildingSite>&  buildings,
            const InvariantCheckOptions&            opts = {}) const;

        // ---- Individual check methods (public for targeted unit tests) ----

        [[nodiscard]] std::vector<InvariantViolation> CheckRoads(
            const std::vector<Core::Road>& roads,
            const InvariantCheckOptions& opts) const;

        [[nodiscard]] std::vector<InvariantViolation> CheckDistricts(
            const std::vector<Core::District>& districts,
            const InvariantCheckOptions& opts) const;

        [[nodiscard]] std::vector<InvariantViolation> CheckLots(
            const std::vector<Core::LotToken>& lots,
            const std::vector<Core::District>& districts,
            const InvariantCheckOptions& opts) const;

        [[nodiscard]] std::vector<InvariantViolation> CheckBuildings(
            const siv::Vector<Core::BuildingSite>& buildings,
            const std::vector<Core::LotToken>& lots,
            const InvariantCheckOptions& opts) const;
    };

} // namespace RogueCity::Core::Validation
