#include "RogueCity/Generators/Scoring/RogueProfiler.hpp"

#include "RogueCity/Generators/Districts/AESPClassifier.hpp"

namespace RogueCity::Generators {

RogueProfiler::Scores RogueProfiler::computeScores(Core::RoadType primary, Core::RoadType secondary) {
    const AESPClassifier::AESPScores aesp = AESPClassifier::computeScores(primary, secondary);
    Scores scores{};
    scores.access = aesp.A;
    scores.exposure = aesp.E;
    scores.serviceability = aesp.S;
    scores.privacy = aesp.P;
    return scores;
}

Core::DistrictType RogueProfiler::classifyDistrict(const Scores& scores) {
    AESPClassifier::AESPScores aesp{};
    aesp.A = scores.access;
    aesp.E = scores.exposure;
    aesp.S = scores.serviceability;
    aesp.P = scores.privacy;
    return AESPClassifier::classifyDistrict(aesp);
}

Core::DistrictType RogueProfiler::classifyLot(const Core::LotToken& lot) {
    return AESPClassifier::classifyLot(lot);
}

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
