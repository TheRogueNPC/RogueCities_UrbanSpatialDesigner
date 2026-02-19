#pragma once

#include "ui/viewport/handlers/rc_viewport_handler_common.h"
#include "ui/viewport/rc_viewport_interaction.h"

namespace RC_UI::Viewport::Handlers {

[[nodiscard]] bool HandleRoadPlacement(
    ViewportInteractionContext& context,
    NonAxiomInteractionState& interaction_state);

[[nodiscard]] bool HandleDistrictPlacement(
    ViewportInteractionContext& context,
    NonAxiomInteractionState& interaction_state);

[[nodiscard]] bool HandleLotPlacement(
    ViewportInteractionContext& context,
    NonAxiomInteractionState& interaction_state);

[[nodiscard]] bool HandleBuildingPlacement(
    ViewportInteractionContext& context,
    NonAxiomInteractionState& interaction_state);

[[nodiscard]] bool HandleWaterPlacement(
    ViewportInteractionContext& context,
    NonAxiomInteractionState& interaction_state);

[[nodiscard]] bool HandleRoadVertexEdits(
    ViewportInteractionContext& context,
    NonAxiomInteractionState& interaction_state);

[[nodiscard]] bool HandleDistrictVertexEdits(
    ViewportInteractionContext& context,
    NonAxiomInteractionState& interaction_state);

} // namespace RC_UI::Viewport::Handlers
