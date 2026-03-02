# Codex Follow-up Brief - 2026-03-01 (2D Cursor + Selection Controls)

## Objective
- Keep only the 2D cursor visual in viewport hover state (hide system cursor in viewport and remove reticle label text).
- Align selection interaction to viewport cursor intent:
  - `click` selects,
  - `click-and-hold` performs box selection,
  - modifier behavior is preserved/extended for expected workflows.

## Layer Ownership
- `visualizer` UI and viewport interaction layers only.

## Files Changed
- `visualizer/src/ui/viewport/rc_viewport_overlays.cpp`
- `visualizer/src/ui/viewport/handlers/rc_viewport_non_axiom_pipeline.cpp`
- `visualizer/src/ui/panels/rc_panel_axiom_editor.cpp`
- `visualizer/src/ui/viewport/rc_viewport_interaction.h`
- `CHANGELOG.md`

## Implementation Summary
- In `Render2DCursorHUD(...)`, set ImGui mouse cursor to `None` while the pointer is inside viewport bounds and removed the `2D_Cursor` label draw block.
- Added default drag-to-box-select start in the non-axiom pipeline using mouse drag threshold and drag-origin reconstruction from screen-space delta.
- Moved point selection resolution to left-button release for click behavior consistency and kept click modifiers (`Shift` add, `Ctrl` toggle, plain click replace/clear).
- Added region-selection merge modes for box/lasso finalize:
  - plain drag -> replace selection,
  - `Shift` drag -> add to selection,
  - `Ctrl` drag -> toggle membership.
- Added visual drag preview cues in the viewport overlay draw pass:
  - cyan = replace,
  - green = add,
  - amber = toggle.

## Validation
- Built GUI target successfully:
  - `"/mnt/c/Program Files/CMake/bin/cmake.exe" --build build_vs --config Release --target RogueCityVisualizerGui --parallel 4`
- Build completed without new errors; existing warnings in `rc_panel_axiom_editor.cpp` remain pre-existing and unrelated to this feature change.

## Handoff
- First unblocked next step: if desired, mirror 2D cursor mode color (replace/add/toggle) on the reticle itself for direct hover-state feedback before drag begins.
- `CHANGELOG updated: yes`.
