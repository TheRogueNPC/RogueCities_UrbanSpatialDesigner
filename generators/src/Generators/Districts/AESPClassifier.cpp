//File: AESPClassifier.cpp
// PURPOSE: Implementation of AESP-based district classification for lot zoning
// AI_INTEGRATION_TAG: V1_PASS1_TASK3_AESP_CLASSIFIER
// AGENT: Coder_Agent
//todo this implementation currently uses hardcoded lookup tables and formulas based on the research paper, but in future iterations we could consider training a machine learning model on real-world data to predict district types from AESP scores more accurately, especially as we gather more data from user-generated cities and can fine-tune the model to better reflect tune preferences and design patterns observed in successful city layouts. This would allow for more nuanced classifications that go beyond the simple formulas currently used, and could adapt over time as we collect more data on how players are using the generator and what kinds of districts they prefer in different contexts.

//todo we need to extend this classifier to become a universal scoring profiller that can also be applied dynamically to allow for more diverse/ personalized districts beyond the 5 hard coded archtypes. extending to a type of customizable tokenized system.
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
        float mixed       = 0.25f * (scores.A + scores.E + scores.S + scores.P);
        float residential = 0.60f * scores.P + 0.20f * scores.A + 0.10f * scores.S + 0.10f * scores.E;
        float commercial  = 0.60f * scores.E + 0.20f * scores.A + 0.10f * scores.S + 0.10f * scores.P;
        float civic       = 0.50f * scores.E + 0.20f * scores.A + 0.10f * scores.S + 0.20f * scores.P;
        float industrial  = 0.60f * scores.S + 0.25f * scores.A + 0.10f * scores.E + 0.05f * scores.P;

        // Return type with highest score
        float max_score = std::max({mixed, residential, commercial, civic, industrial});

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
