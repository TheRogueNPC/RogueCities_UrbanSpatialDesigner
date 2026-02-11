#pragma once

#include "RogueCity/Core/Editor/GlobalState.hpp"

#include <vector>

namespace RogueCity::Core::Validation {

std::vector<Editor::ValidationError> CollectOverlayValidationErrors(
    const Editor::GlobalState& gs,
    float min_lot_area = 120.0f);

} // namespace RogueCity::Core::Validation

