# Codex Follow-up Brief - 2026-03-01 (2D Cursor Precision Overlay)

## Objective
- Add a precision overlay cursor that tracks the real mouse location in the viewport while hovered.
- Name and expose this behavior as `2D_Cursor`.

## Layer Ownership
- `visualizer` UI overlay layer only.

## Files Changed
- `visualizer/src/ui/viewport/rc_viewport_overlays.h`
- `visualizer/src/ui/viewport/rc_viewport_overlays.cpp`

## Implementation Summary
- Replaced center-only overlay hook with `Render2DCursorHUD(...)`.
- Cursor anchor now follows `ImGui::GetMousePos()` when the mouse is inside viewport bounds, and falls back to the viewport center when not hovered.
- Added `2D_Cursor` label near the cursor while hovered to make precision mode explicit.
- Kept existing visual language (pulse/brackets/glow) but tightened radius/alpha when hover-tracking for finer targeting feedback.

## Validation
- Built GUI target successfully:
  - `"/mnt/c/Program Files/CMake/bin/cmake.exe" --build build_vs --config Release --target RogueCityVisualizerGui --parallel 4`
- Build completed with existing unrelated warnings in `rc_panel_axiom_editor.cpp` (no new errors).

## Handoff
- First unblocked next step: tune size/opacity in live UX if you want a sharper or subtler reticle profile.
- `CHANGELOG updated: yes`.
