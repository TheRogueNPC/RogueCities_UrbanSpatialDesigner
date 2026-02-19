#include "RogueCity/Generators/Districts/AESPClassifier.hpp"

#include <cassert>

int main() {
    using RogueCity::Generators::AESPClassifier;
    using RogueCity::Generators::ScoringProfile;
    using RogueCity::Core::DistrictType;

    AESPClassifier::AESPScores scores{};
    scores.A = 0.9f;
    scores.E = 0.2f;
    scores.S = 0.1f;
    scores.P = 0.1f;

    ScoringProfile industrial_bias = ScoringProfile::Urban();
    industrial_bias.name = "IndustrialBias";
    industrial_bias.residential = { 0.0f, 0.0f, 0.0f, 1.0f };
    industrial_bias.industrial = { 1.0f, 0.0f, 0.0f, 0.0f };

    ScoringProfile residential_bias = ScoringProfile::Urban();
    residential_bias.name = "ResidentialBias";
    residential_bias.residential = { 1.0f, 0.0f, 0.0f, 0.0f };
    residential_bias.industrial = { 0.0f, 0.0f, 0.0f, 1.0f };

    const DistrictType industrial_choice = AESPClassifier::classifyDistrict(scores, industrial_bias);
    const DistrictType residential_choice = AESPClassifier::classifyDistrict(scores, residential_bias);
    assert(industrial_choice != residential_choice);

    const auto p0 = AESPClassifier::selectProfileForSeed(42u);
    const auto p1 = AESPClassifier::selectProfileForSeed(42u);
    const auto p2 = AESPClassifier::selectProfileForSeed(43u);
    assert(p0.name == p1.name);
    assert(!p2.name.empty());

    return 0;
}
