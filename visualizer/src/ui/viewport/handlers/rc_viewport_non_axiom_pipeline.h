#pragma once

#include "ui/viewport/rc_viewport_interaction.h"
#include "ui/tools/rc_tool_interaction_metrics.h"

#include <optional>
#include <vector>

namespace RC_UI::Viewport {

[[nodiscard]] NonAxiomInteractionResult ProcessNonAxiomViewportInteractionPipeline(
    const NonAxiomInteractionParams& params,
    NonAxiomInteractionState* interaction_state);

[[nodiscard]] std::optional<RogueCity::Core::Editor::SelectionItem> PickFromViewportIndexTestHook(
    const RogueCity::Core::Editor::GlobalState& gs,
    const RogueCity::Core::Vec2& world_pos,
    const RC_UI::Tools::ToolInteractionMetrics& interaction_metrics);

[[nodiscard]] std::vector<RogueCity::Core::Editor::SelectionItem> QueryRegionFromViewportIndexTestHook(
    const RogueCity::Core::Editor::GlobalState& gs,
    const RogueCity::Core::Vec2& world_min,
    const RogueCity::Core::Vec2& world_max,
    bool include_hidden = false);

} // namespace RC_UI::Viewport
