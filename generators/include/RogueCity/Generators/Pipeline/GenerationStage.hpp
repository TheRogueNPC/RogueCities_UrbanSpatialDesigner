#pragma once

#include <bitset>
#include <cstddef>
#include <cstdint>

namespace RogueCity::Generators {

/// Ordered generation stages used by full and incremental pipeline execution.
enum class GenerationStage : uint8_t {
  Terrain = 0,
  TensorField,
  Roads,
  Districts,
  Blocks,
  Lots,
  Buildings,
  Validation,
  Count
};

inline constexpr size_t kGenerationStageCount =
    static_cast<size_t>(GenerationStage::Count);
using StageMask = std::bitset<kGenerationStageCount>;

/// Convert stage enum to bitset index.
[[nodiscard]] inline constexpr size_t StageIndex(GenerationStage stage) {
  return static_cast<size_t>(stage);
}

/// Mark a stage as dirty in the stage mask.
inline void MarkStageDirty(StageMask &mask, GenerationStage stage) {
  mask.set(StageIndex(stage), true);
}

/// Check whether a stage is currently marked dirty.
[[nodiscard]] inline bool IsStageDirty(const StageMask &mask,
                                       GenerationStage stage) {
  return mask.test(StageIndex(stage));
}

/// Build a mask with all stages marked dirty.
[[nodiscard]] inline StageMask FullStageMask() {
  StageMask mask{};
  mask.set();
  return mask;
}

// Dirty stage cascades to all downstream stages.
inline void CascadeDirty(StageMask &mask) {
  bool downstream_dirty = false;
  for (size_t i = 0; i < kGenerationStageCount; ++i) {
    downstream_dirty = downstream_dirty || mask.test(i);
    if (downstream_dirty) {
      mask.set(i, true);
    }
  }
}

} // namespace RogueCity::Generators
