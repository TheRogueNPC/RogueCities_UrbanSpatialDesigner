#pragma once

#include "ui/viewport/handlers/rc_viewport_handler_common.h"
#include "ui/viewport/rc_viewport_interaction.h"

namespace RC_UI::Viewport::Handlers {

[[nodiscard]] bool HandleDomainPlacementStage(
    ViewportInteractionContext& context,
    NonAxiomInteractionState& interaction_state);

[[nodiscard]] bool HandleRoadVertexEdits(
    ViewportInteractionContext& context,
    NonAxiomInteractionState& interaction_state);

[[nodiscard]] bool HandleDistrictVertexEdits(
    ViewportInteractionContext& context,
    NonAxiomInteractionState& interaction_state);

} // namespace RC_UI::Viewport::Handlers
