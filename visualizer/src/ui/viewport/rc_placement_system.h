#pragma once

#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Generators/Pipeline/GenerationStage.hpp"

namespace RC_UI::Viewport {

struct PlacementGenerationPlan {
    bool use_incremental{ false };
    RogueCity::Generators::StageMask dirty_stages{};
};

[[nodiscard]] PlacementGenerationPlan BuildPlacementGenerationPlan(
    const RogueCity::Core::Editor::DirtyLayerState& dirty_layers);

} // namespace RC_UI::Viewport

