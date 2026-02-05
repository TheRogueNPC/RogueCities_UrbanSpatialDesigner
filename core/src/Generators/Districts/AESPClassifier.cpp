#include "RogueCity/Generators/Districts/AESPClassifier.hpp"
#include <algorithm>

namespace RogueCity::Generators {

    AESPClassifier::AESPScores AESPClassifier::computeScores(
        RoadType primary,
        RoadType secondary
    ) {
        AESPScores scores;
        scores.A = roadTypeToAccess(primary);
        scores.E = roadTypeToExposure(primary);
        scores.S = roadTypeToServiceability(secondary);
        scores.P = roadTypeToPrivacy(primary);
        return scores;
    }

    DistrictType AESPClassifier::classifyDistrict(const AESPScores& scores) {
        // Apply district classification formulas from research paper
        float mixed = 0.25f * (scores.A + scores.E + scores.S + scores.P);
        float residential = 0.60f * scores.P + 0.20f * scores.A + 0.10f * scores.S + 0.10f * scores.E;
        float commercial = 0.60f * scores.E + 0.20f * scores.A + 0.10f * scores.S + 0.10f * scores.P;
        float civic = 0.50f * scores.E + 0.20f * scores.A + 0.10f * scores.S + 0.20f * scores.P;
        float industrial = 0.60f * scores.S + 0.25f * scores.A + 0.10f * scores.E + 0.05f * scores.P;

        // Return type with highest score
        float max_score = std::max({ mixed, residential, commercial, civic, industrial });

        if (max_score == residential) return DistrictType::Residential;
        if (max_score == commercial)  return DistrictType::Commercial;
        if (max_score == civic)       return DistrictType::Civic;
        if (max_score == industrial)  return DistrictType::Industrial;
        return DistrictType::Mixed;
    }

    DistrictType AESPClassifier::classifyLot(const LotToken& lot) {
        AESPScores scores;
        scores.A = lot.access;
        scores.E = lot.exposure;
        scores.S = lot.serviceability;
        scores.P = lot.privacy;
        return classifyDistrict(scores);
    }

    float AESPClassifier::roadTypeToAccess(RoadType type) {
        size_t idx = static_cast<size_t>(type);
        if (idx >= road_type_count) return 0.5f;
        return ACCESS_TABLE[idx];
    }

    float AESPClassifier::roadTypeToExposure(RoadType type) {
        size_t idx = static_cast<size_t>(type);
        if (idx >= road_type_count) return 0.5f;
        return EXPOSURE_TABLE[idx];
    }

    float AESPClassifier::roadTypeToServiceability(RoadType type) {
        size_t idx = static_cast<size_t>(type);
        if (idx >= road_type_count) return 0.5f;
        return SERVICEABILITY_TABLE[idx];
    }

    float AESPClassifier::roadTypeToPrivacy(RoadType type) {
        size_t idx = static_cast<size_t>(type);
        if (idx >= road_type_count) return 0.5f;
        return PRIVACY_TABLE[idx];
    }

} // namespace RogueCity::Generators
