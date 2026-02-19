#include "ui/viewport/handlers/rc_viewport_domain_handlers.h"

#include "ui/viewport/handlers/rc_viewport_domain_handlers_internal.h"

namespace RC_UI::Viewport::Handlers {

bool HandleDomainPlacementStage(
    ViewportInteractionContext& context,
    NonAxiomInteractionState& interaction_state) {
    if (HandleWaterPlacement(context, interaction_state)) {
        return true;
    }
    if (HandleRoadPlacement(context, interaction_state)) {
        return true;
    }
    if (HandleDistrictPlacement(context, interaction_state)) {
        return true;
    }
    if (HandleLotPlacement(context, interaction_state)) {
        return true;
    }
    if (HandleBuildingPlacement(context, interaction_state)) {
        return true;
    }
    return false;
}

} // namespace RC_UI::Viewport::Handlers
