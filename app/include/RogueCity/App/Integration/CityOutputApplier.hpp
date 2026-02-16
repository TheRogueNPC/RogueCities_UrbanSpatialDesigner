#pragma once

#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"

namespace RogueCity::App {

struct CityOutputApplyOptions {
    bool rebuild_viewport_index{ true };
    bool mark_dirty_layers_clean{ true };
};

void ApplyCityOutputToGlobalState(
    const Generators::CityGenerator::CityOutput& output,
    Core::Editor::GlobalState& global_state,
    const CityOutputApplyOptions& options = {});

} // namespace RogueCity::App
