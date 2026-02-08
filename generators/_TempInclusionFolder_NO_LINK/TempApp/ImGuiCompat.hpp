#pragma once

#include <imgui.h>

// Feature detection for docking/viewports. Docking builds define one of these macros.
#if defined(IMGUI_HAS_DOCK) || defined(IMGUI_HAS_DOCKING)
#define RCG_IMGUI_HAS_DOCKING 1
#else
#define RCG_IMGUI_HAS_DOCKING 0
#endif

#if defined(IMGUI_HAS_VIEWPORT) || defined(IMGUI_HAS_VIEWPORTS)
#define RCG_IMGUI_HAS_VIEWPORTS 1
#else
#define RCG_IMGUI_HAS_VIEWPORTS 0
#endif
