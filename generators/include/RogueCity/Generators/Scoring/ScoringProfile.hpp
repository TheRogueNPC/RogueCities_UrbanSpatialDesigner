#pragma once

#include <cstdint>
#include <string>

namespace RogueCity::Generators {

/// Weight set for AESP component blending.
struct DistrictScoreWeights {
  float access{0.25f};
  float exposure{0.25f};
  float serviceability{0.25f};
  float privacy{0.25f};
};

/// Tunable profile used by district scoring/classification.
struct ScoringProfile {
  std::string name{"Urban"};
  DistrictScoreWeights mixed{0.25f, 0.25f, 0.25f, 0.25f};
  DistrictScoreWeights residential{0.20f, 0.10f, 0.10f, 0.60f};
  DistrictScoreWeights commercial{0.20f, 0.60f, 0.10f, 0.10f};
  DistrictScoreWeights civic{0.20f, 0.50f, 0.10f, 0.20f};
  DistrictScoreWeights industrial{0.25f, 0.10f, 0.60f, 0.05f};

  /// Canonical high-density urban profile.
  [[nodiscard]] static ScoringProfile Urban() { return ScoringProfile{}; }

  /// Canonical suburban profile.
  [[nodiscard]] static ScoringProfile Suburban() {
    ScoringProfile profile{};
    profile.name = "Suburban";
    profile.residential = {0.18f, 0.08f, 0.14f, 0.60f};
    profile.commercial = {0.16f, 0.52f, 0.18f, 0.14f};
    profile.civic = {0.18f, 0.44f, 0.20f, 0.18f};
    profile.industrial = {0.28f, 0.10f, 0.52f, 0.10f};
    return profile;
  }

  /// Canonical rural profile.
  [[nodiscard]] static ScoringProfile Rural() {
    ScoringProfile profile{};
    profile.name = "Rural";
    profile.residential = {0.16f, 0.08f, 0.18f, 0.58f};
    profile.commercial = {0.12f, 0.46f, 0.22f, 0.20f};
    profile.civic = {0.14f, 0.40f, 0.22f, 0.24f};
    profile.industrial = {0.30f, 0.08f, 0.50f, 0.12f};
    return profile;
  }

  /// Canonical industrial-biased profile.
  [[nodiscard]] static ScoringProfile Industrial() {
    ScoringProfile profile{};
    profile.name = "Industrial";
    profile.residential = {0.24f, 0.06f, 0.22f, 0.48f};
    profile.commercial = {0.24f, 0.48f, 0.18f, 0.10f};
    profile.civic = {0.22f, 0.42f, 0.20f, 0.16f};
    profile.industrial = {0.28f, 0.12f, 0.56f, 0.04f};
    return profile;
  }

  /// Deterministically choose a profile from seed value.
  [[nodiscard]] static ScoringProfile FromSeed(uint32_t seed) {
    switch (seed % 4u) {
    case 0u:
      return Urban();
    case 1u:
      return Suburban();
    case 2u:
      return Rural();
    case 3u:
    default:
      return Industrial();
    }
  }
};

} // namespace RogueCity::Generators
