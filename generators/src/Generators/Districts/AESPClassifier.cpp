#include "RogueCity/Generators/Districts/AESPClassifier.hpp"

#include <array>

namespace RogueCity::Generators {

namespace {

// Computes weighted A/E/S/P score for a given district archetype weight
// profile.
[[nodiscard]] float ScoreWithWeights(const AESPClassifier::AESPScores &scores,
                                     const DistrictScoreWeights &weights) {
  return (weights.access * scores.A) + (weights.exposure * scores.E) +
         (weights.serviceability * scores.S) + (weights.privacy * scores.P);
}

} // namespace

// Builds normalized AESP scores from road hierarchy pair.
AESPClassifier::AESPScores AESPClassifier::computeScores(RoadType primary,
                                                         RoadType secondary) {
  AESPScores scores;
  scores.A = roadTypeToAccess(primary);
  scores.E = roadTypeToExposure(primary);
  scores.S = roadTypeToServiceability(secondary);
  scores.P = roadTypeToPrivacy(primary);
  return scores;
}

// Builds enhanced AESP scores by analyzing ASAM semantic properties of the
// splines.
AESPClassifier::AESPScores
AESPClassifier::computeScores(const Core::Road &primary,
                              const Core::Road &secondary) {

  // Start with the baseline hierarchy scores
  AESPScores scores = computeScores(primary.type, secondary.type);

  // Physical Access Modifiers (Bridges / Tunnels)
  if (primary.layer_id < 0) {
    // Tunnels: completely physically and visually isolated from surface lots
    scores.A = 0.0f;
    scores.E = 0.0f;
    scores.S = 0.0f;
  } else if (primary.layer_id > 0) {
    // Bridges/Overpasses: visible, but completely physically isolated
    scores.A = 0.0f;
    scores.E = std::min(1.0f, scores.E + 0.20f);
  }

  if (secondary.layer_id < 0) {
    scores.S = 0.0f; // Can't take delivery from a tunnel
  }

  // Traffic Flow Modifiers (Signals / Crosswalks)
  if (primary.contains_signal || secondary.contains_signal) {
    // Controlled intersections increase walkability/service access, but hurt
    // privacy
    scores.S = std::min(1.0f, scores.S + 0.10f);
    scores.P = std::max(0.0f, scores.P - 0.10f);
  }

  return scores;
}

// Evaluates a junction archetype and returns its raw AESP profile.
AESPClassifier::AESPScores AESPClassifier::computeJunctionScores(
    const Core::IntersectionTemplate &junction) {
  AESPScores scores;
  // Baseline scores for any junction footprint
  scores.A = 0.5f;
  scores.E = 0.7f;
  scores.S = 0.6f;
  scores.P = 0.2f;

  if (junction.has_grade_separation) {
    scores.A = 0.0f; // Direct access blocked
    scores.E = 1.0f; // Highly visible
  }

  switch (junction.archetype) {
  case Core::JunctionArchetype::Cloverleaf:
  case Core::JunctionArchetype::Stack:
  case Core::JunctionArchetype::DirectionalT:
    scores.A = 0.0f;
    scores.E = 1.0f;
    scores.S = 0.8f;
    scores.P = 0.0f;
    break;
  case Core::JunctionArchetype::CompactRoundabout:
    scores.A = 0.7f;
    scores.E = 0.8f;
    scores.S = 0.9f;
    scores.P = 0.3f;
    break;
  case Core::JunctionArchetype::Diamond:
  case Core::JunctionArchetype::FoldedDiamond:
    scores.A = 0.2f;
    scores.E = 0.9f;
    scores.S = 0.9f;
    scores.P = 0.1f;
    break;
  case Core::JunctionArchetype::None:
  default:
    break;
  }
  return scores;
}

// Chooses the district type whose weighted AESP projection is highest.
DistrictType AESPClassifier::classifyDistrict(const AESPScores &scores,
                                              const ScoringProfile &profile) {
  struct Candidate {
    DistrictType type{DistrictType::Mixed};
    float score{0.0f};
  };

  std::array<Candidate, 5> candidates{
      {{DistrictType::Mixed, ScoreWithWeights(scores, profile.mixed)},
       {DistrictType::Residential,
        ScoreWithWeights(scores, profile.residential)},
       {DistrictType::Commercial, ScoreWithWeights(scores, profile.commercial)},
       {DistrictType::Civic, ScoreWithWeights(scores, profile.civic)},
       {DistrictType::Industrial,
        ScoreWithWeights(scores, profile.industrial)}}};

  Candidate best = candidates.front();
  for (size_t i = 1; i < candidates.size(); ++i) {
    if (candidates[i].score > best.score) {
      best = candidates[i];
    }
  }

  return best.type;
}

// Convenience path for lot tokens that already carry AESP-like scalar features.
DistrictType AESPClassifier::classifyLot(const LotToken &lot,
                                         const ScoringProfile &profile) {
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

// Table lookups for mapping road type into AESP axes, with neutral fallback on
// invalid enum.
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
