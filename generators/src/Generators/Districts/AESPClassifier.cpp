#include "RogueCity/Generators/Districts/AESPClassifier.hpp"

#include <array>

namespace RogueCity::Generators {

namespace {

// Computes weighted A/E/S/P score for a given district archetype weight profile.
[[nodiscard]] float ScoreWithWeights(
    const AESPClassifier::AESPScores& scores,
    const DistrictScoreWeights& weights) {
    return (weights.access * scores.A) +
        (weights.exposure * scores.E) +
        (weights.serviceability * scores.S) +
        (weights.privacy * scores.P);
}

} // namespace

// Builds normalized AESP scores from road hierarchy pair.
AESPClassifier::AESPScores AESPClassifier::computeScores(
    RoadType primary,
    RoadType secondary) {
    AESPScores scores;
    scores.A = roadTypeToAccess(primary);
    scores.E = roadTypeToExposure(primary);
    scores.S = roadTypeToServiceability(secondary);
    scores.P = roadTypeToPrivacy(primary);
    return scores;
}

// Chooses the district type whose weighted AESP projection is highest.
DistrictType AESPClassifier::classifyDistrict(
    const AESPScores& scores,
    const ScoringProfile& profile) {
    struct Candidate {
        DistrictType type{ DistrictType::Mixed };
        float score{ 0.0f };
    };

    std::array<Candidate, 5> candidates{{
        { DistrictType::Mixed, ScoreWithWeights(scores, profile.mixed) },
        { DistrictType::Residential, ScoreWithWeights(scores, profile.residential) },
        { DistrictType::Commercial, ScoreWithWeights(scores, profile.commercial) },
        { DistrictType::Civic, ScoreWithWeights(scores, profile.civic) },
        { DistrictType::Industrial, ScoreWithWeights(scores, profile.industrial) }
    }};

    Candidate best = candidates.front();
    for (size_t i = 1; i < candidates.size(); ++i) {
        if (candidates[i].score > best.score) {
            best = candidates[i];
        }
    }

    return best.type;
}

// Convenience path for lot tokens that already carry AESP-like scalar features.
DistrictType AESPClassifier::classifyLot(
    const LotToken& lot,
    const ScoringProfile& profile) {
    AESPScores scores;
    scores.A = lot.access;
    scores.E = lot.exposure;
    scores.S = lot.serviceability;
    scores.P = lot.privacy;
    return classifyDistrict(scores, profile);
}

// Deterministically picks a scoring profile variant from seed.
ScoringProfile AESPClassifier::selectProfileForSeed(uint32_t seed) {
    return ScoringProfile::FromSeed(seed);
}

// Table lookups for mapping road type into AESP axes, with neutral fallback on invalid enum.
float AESPClassifier::roadTypeToAccess(RoadType type) {
    size_t idx = static_cast<size_t>(type);
    if (idx >= road_type_count) {
        return 0.5f;
    }
    return ACCESS_TABLE[idx];
}

float AESPClassifier::roadTypeToExposure(RoadType type) {
    size_t idx = static_cast<size_t>(type);
    if (idx >= road_type_count) {
        return 0.5f;
    }
    return EXPOSURE_TABLE[idx];
}

float AESPClassifier::roadTypeToServiceability(RoadType type) {
    size_t idx = static_cast<size_t>(type);
    if (idx >= road_type_count) {
        return 0.5f;
    }
    return SERVICEABILITY_TABLE[idx];
}

float AESPClassifier::roadTypeToPrivacy(RoadType type) {
    size_t idx = static_cast<size_t>(type);
    if (idx >= road_type_count) {
        return 0.5f;
    }
    return PRIVACY_TABLE[idx];
}

} // namespace RogueCity::Generators
