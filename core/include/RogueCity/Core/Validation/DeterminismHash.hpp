#pragma once

#include "RogueCity/Core/Editor/GlobalState.hpp"

#include <cstdint>
#include <string>

namespace RogueCity::Core::Validation {

/// Deterministic signature of editor-visible generation data.
struct DeterminismHash {
  uint64_t roads_hash{0};
  uint64_t districts_hash{0};
  uint64_t lots_hash{0};
  uint64_t buildings_hash{0};
  uint64_t tensor_field_hash{0};

  [[nodiscard]] bool operator==(const DeterminismHash &other) const noexcept;
  [[nodiscard]] bool operator!=(const DeterminismHash &other) const noexcept {
    return !(*this == other);
  }

  [[nodiscard]] std::string to_string() const;
};

/// Compute deterministic hash from a GlobalState snapshot.
[[nodiscard]] DeterminismHash
ComputeDeterminismHash(const Editor::GlobalState &gs);

/// Persist a baseline hash file used by determinism regression tests.
[[nodiscard]] bool SaveBaselineHash(const DeterminismHash &hash,
                                    const std::string &filepath);

/// Validate current hash against a previously recorded baseline hash file.
[[nodiscard]] bool ValidateAgainstBaseline(const DeterminismHash &hash,
                                           const std::string &filepath);

} // namespace RogueCity::Core::Validation
