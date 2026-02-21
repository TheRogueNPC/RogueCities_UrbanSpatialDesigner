#include "RogueCity/Generators/Scoring/RogueProfiler.hpp"

#include "RogueCity/Generators/Districts/AESPClassifier.hpp"

namespace RogueCity::Generators {

// Adapter layer from legacy/public scoring API into AESP classifier primitives.
RogueProfiler::Scores RogueProfiler::computeScores(Core::RoadType primary, Core::RoadType secondary) {
    const AESPClassifier::AESPScores aesp = AESPClassifier::computeScores(primary, secondary);
    Scores scores{};
    scores.access = aesp.A;
    scores.exposure = aesp.E;
    scores.serviceability = aesp.S;
    scores.privacy = aesp.P;
    return scores;
}

// Converts profiler score struct into classifier score struct and delegates classification.
Core::DistrictType RogueProfiler::classifyDistrict(const Scores& scores) {
    AESPClassifier::AESPScores aesp{};
    aesp.A = scores.access;
    aesp.E = scores.exposure;
    aesp.S = scores.serviceability;
    aesp.P = scores.privacy;
    return AESPClassifier::classifyDistrict(aesp);
}

// Lot classification pass-through.
Core::DistrictType RogueProfiler::classifyLot(const Core::LotToken& lot) {
    return AESPClassifier::classifyLot(lot);
}

// Axis extraction helpers delegated to AESP mapping tables.
float RogueProfiler::roadTypeToAccess(Core::RoadType type) {
    return AESPClassifier::roadTypeToAccess(type);
}

float RogueProfiler::roadTypeToExposure(Core::RoadType type) {
    return AESPClassifier::roadTypeToExposure(type);
}

float RogueProfiler::roadTypeToServiceability(Core::RoadType type) {
    return AESPClassifier::roadTypeToServiceability(type);
}

float RogueProfiler::roadTypeToPrivacy(Core::RoadType type) {
    return AESPClassifier::roadTypeToPrivacy(type);
}

} // namespace RogueCity::Generators
