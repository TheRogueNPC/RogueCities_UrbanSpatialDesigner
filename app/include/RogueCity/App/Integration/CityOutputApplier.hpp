#pragma once

#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"

#include <cstdint>

namespace RogueCity::App {
// Applies the given city generation output to the editor's global state, modifying roads, districts, and other entities as needed based on the specified options.
enum class GenerationScope : uint8_t {
    RoadsOnly = 0,
    RoadsAndBounds,
    FullCity
};

// Options for how to apply city generation output to the editor's global state.
struct CityOutputApplyOptions { 
    GenerationScope scope{ GenerationScope::FullCity };
    bool rebuild_viewport_index{ true };
    bool mark_dirty_layers_clean{ true };
    bool preserve_locked_user_entities{ true };
};

 
void ApplyCityOutputToGlobalState(
    const Generators::CityGenerator::CityOutput& output,
    Core::Editor::GlobalState& global_state,
    const CityOutputApplyOptions& options = {});

} // namespace RogueCity::App
