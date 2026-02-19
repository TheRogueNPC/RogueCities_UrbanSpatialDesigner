#pragma once
#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Generators/Scoring/ScoringProfile.hpp"
#include <array>

namespace RogueCity::Generators {

    using namespace Core;

    /// AESP-based district classification (from research paper)
    class AESPClassifier {
    public:
        /// AESP component scores
        struct AESPScores {
            float A{ 0.0f };  // Access
            float E{ 0.0f };  // Exposure
            float S{ 0.0f };  // Serviceability
            float P{ 0.0f };  // Privacy
        };

        /// Compute AESP scores from road types
        [[nodiscard]] static AESPScores computeScores(RoadType primary, RoadType secondary);

        /// Classify district type from AESP scores (research formulas)
        [[nodiscard]] static DistrictType classifyDistrict(
            const AESPScores& scores,
            const ScoringProfile& profile = ScoringProfile::Urban());

        /// Classify district directly from lot token
        [[nodiscard]] static DistrictType classifyLot(
            const LotToken& lot,
            const ScoringProfile& profile = ScoringProfile::Urban());

        /// Deterministically select scoring profile from seed.
        [[nodiscard]] static ScoringProfile selectProfileForSeed(uint32_t seed);

        /// Get road type → Access mapping (Table 2 from paper)
        [[nodiscard]] static float roadTypeToAccess(RoadType type);

        /// Get road type → Exposure mapping
        [[nodiscard]] static float roadTypeToExposure(RoadType type);

        /// Get road type → Serviceability mapping
        [[nodiscard]] static float roadTypeToServiceability(RoadType type);

        /// Get road type → Privacy mapping
        [[nodiscard]] static float roadTypeToPrivacy(RoadType type);

    private:
        /// AESP lookup tables (from research paper)
        static constexpr std::array<float, road_type_count> ACCESS_TABLE = {
            1.00f, // Highway
            0.90f, // Arterial
            0.80f, // Avenue
            0.70f, // Boulevard
            0.80f, // Street
            0.50f, // Lane
            0.30f, // Alleyway
            0.30f, // CulDeSac
            0.50f, // Drive
            0.20f, // Driveway
            0.90f, // M_Major
            0.50f  // M_Minor
        };

        static constexpr std::array<float, road_type_count> EXPOSURE_TABLE = {
            1.00f, // Highway
            0.90f, // Arterial
            0.80f, // Avenue
            0.90f, // Boulevard
            0.50f, // Street
            0.20f, // Lane
            0.10f, // Alleyway
            0.20f, // CulDeSac
            0.30f, // Drive
            0.05f, // Driveway
            0.90f, // M_Major
            0.20f  // M_Minor
        };

        static constexpr std::array<float, road_type_count> SERVICEABILITY_TABLE = {
            0.70f, // Highway
            0.90f, // Arterial
            0.80f, // Avenue
            0.50f, // Boulevard
            0.80f, // Street
            0.50f, // Lane
            1.00f, // Alleyway
            0.50f, // CulDeSac
            0.60f, // Drive
            0.70f, // Driveway
            0.80f, // M_Major
            0.70f  // M_Minor
        };

        static constexpr std::array<float, road_type_count> PRIVACY_TABLE = {
            0.00f, // Highway
            0.20f, // Arterial
            0.50f, // Avenue
            0.70f, // Boulevard
            0.80f, // Street
            1.00f, // Lane
            0.70f, // Alleyway
            1.00f, // CulDeSac
            0.90f, // Drive
            1.00f, // Driveway
            0.30f, // M_Major
            0.90f  // M_Minor
        };
    };

} // namespace RogueCity::Generators
