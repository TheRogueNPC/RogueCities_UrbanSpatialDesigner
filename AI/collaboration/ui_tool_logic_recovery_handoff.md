# UI Sub-Tool Logic Recovery & Reintegration Handoff

## Context
The recent "TOOL DECK" removal in the Master Panel orphaned several sub-tools (Spline, Slice, Merge, Select, etc.). Investigation revealed the logic wasn't deleted; it was registered in the dispatcher (`RC_UI::Tools::ToolActionId`) and App layer but lacked a UI surface to trigger it.

## Approach Taken
**Cockpit-Doctrine Sub-Tool Reintegration**
Instead of reviving a unified "TOOL DECK", we migrated the sub-tool action grids directly into their respective domain panels (`rc_panel_road_editor.cpp`, `rc_panel_lot_control.cpp`, `rc_panel_building_control.cpp`, `rc_panel_water_control.cpp`, `rc_panel_zoning_control.cpp`).

1. **`DrawToolActionGrid`**: Created a reusable UI component (`RC_UI::Components::DrawToolActionGrid`) in `rc_ui_components.h` that:
   - Queries `RC_UI::Tools::GetToolActionsForLibrary()`
   - Dispatches via `RC_UI::Tools::DispatchToolAction()`
   - Enforces ID-collision safety with `ImGui::PushID()`
   - Adheres to Y2K geometry and AnimatedActionButton styling.
   - Highlights the active action by probing `gs.tool_runtime.last_action_id`.

2. **Axiom Editor**: Left the `AxiomLibrary` intact, as `rc_panel_axiom_editor.cpp` already uses a highly specialized custom geometry component (`DrawAxiomLibraryContent`) for its axioms.

## Fixes & Polish
- Resolved missing STL includes (`<iterator>`, `<forward_list>`) impacting the VisualStudio MSVC compiler by re-ordering `#include "ui/introspection/UiIntrospection.h"`.
- Fixed namespace pollution on the `RC_UI::ToolLibrary` enum.
- Re-enabled `[[nodiscard]]` casting on `DispatchToolAction` to suppress compiler warnings in `rc_ui_components.h`.

## Checkpoints
- `CHANGELOG.md` updated with "Domain Sub-Tool Reintegration".
- CMake GUI compilation successful.
- `rc-full-smoke` verified ImGui invariants.

This codebase is now fully Cockpit-Doctrine compliant regarding tool switching, with all sub-tools correctly re-exposed within their active domain sub-panels.
