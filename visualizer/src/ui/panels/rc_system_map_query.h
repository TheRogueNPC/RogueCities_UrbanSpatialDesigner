#pragma once

#include "RogueCity/Core/Editor/GlobalState.hpp"

namespace RC_UI::Panels::SystemMap {

struct SystemsMapQueryHit {
    bool valid{ false };
    RogueCity::Core::Editor::VpEntityKind kind{ RogueCity::Core::Editor::VpEntityKind::Unknown };
    uint32_t id{ 0 };
    RogueCity::Core::Vec2 anchor{};
    float distance{ 0.0f };
    const char* label{ "" };
};

[[nodiscard]] const char* SystemsMapKindLabel(RogueCity::Core::Editor::VpEntityKind kind);

[[nodiscard]] bool BuildSystemsMapBounds(
    const RogueCity::Core::Editor::GlobalState& gs,
    RogueCity::Core::Bounds& out_bounds);

[[nodiscard]] SystemsMapQueryHit QuerySystemsMapEntity(
    const RogueCity::Core::Editor::GlobalState& gs,
    const RogueCity::Core::Editor::SystemsMapRuntimeState& runtime,
    const RogueCity::Core::Vec2& world_pos,
    float pick_radius);

} // namespace RC_UI::Panels::SystemMap
