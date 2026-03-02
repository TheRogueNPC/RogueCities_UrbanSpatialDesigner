/**
 * @file StableIDRegistry.cpp
 * @brief Implements the StableIDRegistry for mapping between viewport entity IDs and stable IDs.
 *
 * This file provides the implementation for the StableIDRegistry class, which manages the allocation,
 * mapping, serialization, and deserialization of stable IDs for entities in the viewport. It ensures
 * that entities retain consistent identifiers across sessions and supports migration via aliasing.
 *
 * Key Features:
 * - Allocation of stable IDs for viewport entities.
 * - Bidirectional mapping between viewport entity IDs and stable IDs.
 * - Serialization and deserialization of registry state for persistence.
 * - Support for aliasing legacy stable IDs to canonical IDs.
 * - Utility functions for parsing and trimming input data.
 *
 * Internal Functions:
 * - Trim: Removes leading and trailing whitespace from strings.
 * - ParseU64/U32/U8: Safely parses unsigned integers from strings.
 * - IsMappableKind: Determines if an entity kind is eligible for mapping.
 *
 * Main Methods:
 * - AllocateStableID: Assigns a new stable ID to a viewport entity.
 * - GetStableID: Retrieves the stable ID for a given viewport entity.
 * - GetViewportID: Retrieves the viewport entity ID for a given stable ID.
 * - RebuildMapping: Reconstructs the mapping based on active viewport IDs.
 * - Serialize: Outputs the registry state as a string.
 * - Deserialize: Loads the registry state from a string.
 * - Clear: Resets the registry to its initial state.
 *
 * Singleton Access:
 * - GetStableIDRegistry: Provides access to a singleton instance of the registry.
 */
 
#include "RogueCity/Core/Editor/StableIDRegistry.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_set>

namespace RogueCity::Core::Editor {

namespace {

[[nodiscard]] std::string Trim(std::string value) {
    const auto is_space = [](unsigned char c) {
        return std::isspace(c) != 0;
    };

    while (!value.empty() && is_space(static_cast<unsigned char>(value.front()))) {
        value.erase(value.begin());
    }
    while (!value.empty() && is_space(static_cast<unsigned char>(value.back()))) {
        value.pop_back();
    }
    return value;
}

[[nodiscard]] bool ParseU64(const std::string& text, uint64_t& out) {
    try {
        size_t consumed = 0;
        out = std::stoull(text, &consumed, 0);
        return consumed == text.size();
    } catch (...) {
        return false;
    }
}

[[nodiscard]] bool ParseU32(const std::string& text, uint32_t& out) {
    uint64_t tmp = 0;
    if (!ParseU64(text, tmp) || tmp > 0xFFFFFFFFull) {
        return false;
    }
    out = static_cast<uint32_t>(tmp);
    return true;
}

[[nodiscard]] bool ParseU8(const std::string& text, uint8_t& out) {
    uint64_t tmp = 0;
    if (!ParseU64(text, tmp) || tmp > 0xFFull) {
        return false;
    }
    out = static_cast<uint8_t>(tmp);
    return true;
}

[[nodiscard]] bool IsMappableKind(VpEntityKind kind) {
    return kind != VpEntityKind::Unknown;
}

[[nodiscard]] bool IsKnownKind(uint8_t kind_u8) {
    return kind_u8 <= static_cast<uint8_t>(VpEntityKind::Block);
}

} // namespace

StableID StableIDRegistry::AllocateStableID(VpEntityKind kind, uint32_t viewport_id) {
    const ViewportEntityID key{ kind, viewport_id };
    if (const auto it = viewport_to_stable_.find(key); it != viewport_to_stable_.end()) {
        return it->second;
    }

    StableID stable{};
    stable.id = next_stable_id_++;
    stable.version = 1u;

    viewport_to_stable_[key] = stable;
    stable_to_viewport_[stable] = key;
    return stable;
}

std::optional<StableID> StableIDRegistry::GetStableID(VpEntityKind kind, uint32_t viewport_id) const {
    const ViewportEntityID key{ kind, viewport_id };
    if (const auto it = viewport_to_stable_.find(key); it != viewport_to_stable_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<ViewportEntityID> StableIDRegistry::GetViewportID(const StableID& stable_id) const {
    if (const auto it = stable_to_viewport_.find(stable_id); it != stable_to_viewport_.end()) {
        return it->second;
    }

    uint64_t canonical_id = stable_id.id;
    std::unordered_set<uint64_t> visited;
    visited.reserve(8u);
    while (true) {
        if (!visited.insert(canonical_id).second) {
            break;
        }
        const auto alias_it = aliases_.find(canonical_id);
        if (alias_it == aliases_.end() || alias_it->second == canonical_id) {
            break;
        }
        canonical_id = alias_it->second;
    }

    if (canonical_id != stable_id.id) {
        const StableID canonical{ canonical_id, stable_id.version };
        if (const auto it = stable_to_viewport_.find(canonical); it != stable_to_viewport_.end()) {
            return it->second;
        }

        for (const auto& [stable, viewport] : stable_to_viewport_) {
            if (stable.id == canonical_id) {
                return viewport;
            }
        }
    }

    return std::nullopt;
}

void StableIDRegistry::RebuildMapping(const std::vector<ViewportEntityID>& active_viewport_ids) {
    std::vector<ViewportEntityID> ordered;
    ordered.reserve(active_viewport_ids.size());
    for (const auto& id : active_viewport_ids) {
        if (IsMappableKind(id.kind)) {
            ordered.push_back(id);
        }
    }

    std::sort(ordered.begin(), ordered.end(), [](const ViewportEntityID& a, const ViewportEntityID& b) {
        if (a.kind != b.kind) {
            return static_cast<uint8_t>(a.kind) < static_cast<uint8_t>(b.kind);
        }
        return a.id < b.id;
    });

    std::unordered_map<ViewportEntityID, StableID, ViewportEntityIDHasher> new_viewport_to_stable;
    new_viewport_to_stable.reserve(ordered.size());
    std::unordered_map<StableID, ViewportEntityID, StableIDHasher> new_stable_to_viewport;
    new_stable_to_viewport.reserve(ordered.size());

    for (const auto& viewport_id : ordered) {
        StableID stable{};
        if (const auto it = viewport_to_stable_.find(viewport_id); it != viewport_to_stable_.end()) {
            stable = it->second;
        } else {
            stable = AllocateStableID(viewport_id.kind, viewport_id.id);
        }

        new_viewport_to_stable.emplace(viewport_id, stable);
        new_stable_to_viewport.emplace(stable, viewport_id);
    }

    std::unordered_set<uint64_t> active_stable_ids;
    active_stable_ids.reserve(new_stable_to_viewport.size());
    for (const auto& [stable, viewport] : new_stable_to_viewport) {
        (void)viewport;
        active_stable_ids.insert(stable.id);
    }
    for (auto it = aliases_.begin(); it != aliases_.end();) {
        if (it->first == it->second || active_stable_ids.find(it->second) == active_stable_ids.end()) {
            it = aliases_.erase(it);
        } else {
            ++it;
        }
    }

    viewport_to_stable_ = std::move(new_viewport_to_stable);
    stable_to_viewport_ = std::move(new_stable_to_viewport);
}

std::string StableIDRegistry::Serialize() const {
    std::ostringstream out;
    out << "next=" << next_stable_id_ << '\n';

    std::vector<ViewportEntityID> keys;
    keys.reserve(viewport_to_stable_.size());
    for (const auto& [key, stable] : viewport_to_stable_) {
        (void)stable;
        keys.push_back(key);
    }
    std::sort(keys.begin(), keys.end(), [](const ViewportEntityID& a, const ViewportEntityID& b) {
        if (a.kind != b.kind) {
            return static_cast<uint8_t>(a.kind) < static_cast<uint8_t>(b.kind);
        }
        return a.id < b.id;
    });

    for (const auto& key : keys) {
        const auto it = viewport_to_stable_.find(key);
        if (it == viewport_to_stable_.end()) {
            continue;
        }
        out << "map="
            << static_cast<uint32_t>(static_cast<uint8_t>(key.kind)) << ','
            << key.id << ','
            << it->second.id << ','
            << it->second.version << '\n';
    }

    for (const auto& [legacy_id, canonical_id] : aliases_) {
        out << "alias=" << legacy_id << ',' << canonical_id << '\n';
    }

    return out.str();
}

bool StableIDRegistry::Deserialize(std::string_view serialized) {
    Clear();

    uint64_t parsed_next_id = 1u;
    std::unordered_map<ViewportEntityID, StableID, ViewportEntityIDHasher> parsed_viewport_to_stable;
    std::unordered_map<StableID, ViewportEntityID, StableIDHasher> parsed_stable_to_viewport;
    std::unordered_map<uint64_t, uint64_t> parsed_aliases;

    std::istringstream in{ std::string(serialized) };
    std::string line;
    bool ok = true;

    while (std::getline(in, line)) {
        line = Trim(std::move(line));
        if (line.empty() || line[0] == '#') {
            continue;
        }

        const auto eq = line.find('=');
        if (eq == std::string::npos) {
            ok = false;
            break;
        }

        const std::string key = Trim(line.substr(0, eq));
        const std::string value = Trim(line.substr(eq + 1));
        if (key == "next") {
            uint64_t parsed_next = 0;
            if (!ParseU64(value, parsed_next) || parsed_next == 0u) {
                ok = false;
                break;
            }
            parsed_next_id = parsed_next;
            continue;
        }

        if (key == "map") {
            std::istringstream value_stream(value);
            std::string token;
            std::vector<std::string> parts;
            while (std::getline(value_stream, token, ',')) {
                parts.push_back(Trim(token));
            }
            if (parts.size() != 4) {
                ok = false;
                break;
            }

            uint8_t kind_u8 = 0;
            uint32_t viewport_id = 0;
            uint64_t stable_id = 0;
            uint32_t version = 0;
            if (!ParseU8(parts[0], kind_u8) ||
                !ParseU32(parts[1], viewport_id) ||
                !ParseU64(parts[2], stable_id) ||
                !ParseU32(parts[3], version)) {
                ok = false;
                break;
            }
            if (!IsKnownKind(kind_u8) || viewport_id == 0u || stable_id == 0u) {
                ok = false;
                break;
            }

            const ViewportEntityID viewport{
                static_cast<VpEntityKind>(kind_u8),
                viewport_id
            };
            if (!IsMappableKind(viewport.kind)) {
                ok = false;
                break;
            }
            const StableID stable{ stable_id, version == 0u ? 1u : version };

            const auto [vp_it, vp_inserted] = parsed_viewport_to_stable.emplace(viewport, stable);
            if (!vp_inserted && vp_it->second != stable) {
                ok = false;
                break;
            }
            const auto [stable_it, stable_inserted] = parsed_stable_to_viewport.emplace(stable, viewport);
            if (!stable_inserted && !(stable_it->second == viewport)) {
                ok = false;
                break;
            }
            parsed_next_id = std::max(parsed_next_id, stable_id + 1u);
            continue;
        }

        if (key == "alias") {
            std::istringstream value_stream(value);
            std::string left;
            std::string right;
            if (!std::getline(value_stream, left, ',') || !std::getline(value_stream, right)) {
                ok = false;
                break;
            }
            uint64_t legacy_id = 0;
            uint64_t canonical_id = 0;
            if (!ParseU64(Trim(left), legacy_id) ||
                !ParseU64(Trim(right), canonical_id) ||
                legacy_id == 0u ||
                canonical_id == 0u) {
                ok = false;
                break;
            }
            parsed_aliases[legacy_id] = canonical_id;
            continue;
        }

        ok = false;
        break;
    }

    if (!ok) {
        Clear();
        return false;
    }

    next_stable_id_ = std::max<uint64_t>(1u, parsed_next_id);
    viewport_to_stable_ = std::move(parsed_viewport_to_stable);
    stable_to_viewport_ = std::move(parsed_stable_to_viewport);
    aliases_ = std::move(parsed_aliases);
    return true;
}

void StableIDRegistry::Clear() {
    next_stable_id_ = 1u;
    viewport_to_stable_.clear();
    stable_to_viewport_.clear();
    aliases_.clear();
}

StableIDRegistry& GetStableIDRegistry() {
    static StableIDRegistry registry{};
    return registry;
}

} // namespace RogueCity::Core::Editor
