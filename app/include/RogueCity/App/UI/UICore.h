#pragma once

#include "RogueCity/App/UI/DesignSystem.h"

// Optional strict mode for UI translation units.
// Enable only in files that have been fully migrated to wrapper-based APIs.

//todo ensure that all Ui based systems are wrapper based.
#if defined(ROGUEUI_ENFORCE_DESIGN_SYSTEM_STRICT)
    #define Begin ROGUEUI_ERROR_USE_WRAPPER_BEGIN_PANEL
    #define IM_COL32 ROGUEUI_ERROR_USE_TOKEN_COLOR_INSTEAD_OF_IM_COL32
#endif

