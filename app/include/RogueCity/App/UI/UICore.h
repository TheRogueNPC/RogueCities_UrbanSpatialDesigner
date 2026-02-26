/**
 * @file UICore.h
 * @brief Core UI definitions and strict mode enforcement for RogueCities Urban Spatial Designer.
 *
 * Includes the main DesignSystem header and provides optional strict mode macros for UI translation units.
 * Strict mode should be enabled only in files fully migrated to wrapper-based APIs, enforcing usage of
 * wrapper panels and token-based color definitions instead of legacy ImGui macros.
 *
 * @note To enforce strict mode, define ROGUEUI_ENFORCE_DESIGN_SYSTEM_STRICT before including this header.
 *       This will replace legacy macros with error tokens to prevent their usage.
 */
#pragma once

#include "RogueCity/App/UI/DesignSystem.h"

// Optional strict mode for UI translation units.
// Enable only in files that have been fully migrated to wrapper-based APIs.

//todo ensure that all Ui based systems are wrapper based.
#if defined(ROGUEUI_ENFORCE_DESIGN_SYSTEM_STRICT)
    #define Begin ROGUEUI_ERROR_USE_WRAPPER_BEGIN_PANEL
    #define IM_COL32 ROGUEUI_ERROR_USE_TOKEN_COLOR_INSTEAD_OF_IM_COL32
#endif

