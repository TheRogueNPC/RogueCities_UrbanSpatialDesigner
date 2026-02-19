#pragma once

#include "RogueCity/Core/Editor/ViewportIndex.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace RogueCity::Core::Editor {

/// Stable identifier persisted across viewport id regeneration.
struct StableID {
  uint64_t id{0};
  uint32_t version{1};

  [[nodiscard]] bool operator==(const StableID &other) const noexcept {
    return id == other.id && version == other.version;
  }
  [[nodiscard]] bool operator!=(const StableID &other) const noexcept {
    return !(*this == other);
  }
};

/// Runtime viewport entity key used by editor indices.
struct ViewportEntityID {
  VpEntityKind kind{VpEntityKind::Unknown};
  uint32_t id{0};

  [[nodiscard]] bool operator==(const ViewportEntityID &other) const noexcept {
    return kind == other.kind && id == other.id;
  }
};

/// Bidirectional mapping between transient viewport ids and stable ids.
class StableIDRegistry {
public:
  StableIDRegistry() = default;

  /// Allocate and register a stable id for a viewport entity.
  [[nodiscard]] StableID AllocateStableID(VpEntityKind kind,
                                          uint32_t viewport_id);
  /// Lookup stable id for a viewport entity.
  [[nodiscard]] std::optional<StableID> GetStableID(VpEntityKind kind,
                                                    uint32_t viewport_id) const;
  /// Reverse-lookup viewport entity id from a stable id.
  [[nodiscard]] std::optional<ViewportEntityID>
  GetViewportID(const StableID &stable_id) const;

  /// Rebuild mapping from active viewport entities, preserving aliases when
  /// possible.
  void RebuildMapping(const std::vector<ViewportEntityID> &active_viewport_ids);

  /// Serialize registry state for persistence.
  [[nodiscard]] std::string Serialize() const;
  /// Deserialize registry state from persisted text.
  [[nodiscard]] bool Deserialize(std::string_view serialized);

  /// Clear all mappings and reset id allocation state.
  void Clear();

private:
  struct ViewportEntityIDHasher {
    [[nodiscard]] size_t
    operator()(const ViewportEntityID &value) const noexcept {
      return (static_cast<size_t>(static_cast<uint8_t>(value.kind)) << 32u) ^
             static_cast<size_t>(value.id);
    }
  };

  struct StableIDHasher {
    [[nodiscard]] size_t operator()(const StableID &value) const noexcept {
      return static_cast<size_t>(value.id) ^
             (static_cast<size_t>(value.version) << 1u);
    }
  };

  uint64_t next_stable_id_{1};
  std::unordered_map<ViewportEntityID, StableID, ViewportEntityIDHasher>
      viewport_to_stable_{};
  std::unordered_map<StableID, ViewportEntityID, StableIDHasher>
      stable_to_viewport_{};
  std::unordered_map<uint64_t, uint64_t>
      aliases_{}; // legacy_id -> canonical_id
};

/// Access process-wide registry instance used by viewport/editor systems.
[[nodiscard]] StableIDRegistry &GetStableIDRegistry();

} // namespace RogueCity::Core::Editor
