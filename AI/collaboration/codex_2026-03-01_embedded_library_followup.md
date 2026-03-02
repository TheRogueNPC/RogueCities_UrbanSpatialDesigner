# Codex Follow-up Brief - 2026-03-01 (Embedded Tool Libraries Default)

## Objective
- Move tool-library interaction into the Tools drawer by default to avoid duplicated tool/subtool surfaces.
- Keep separate library windows as opt-in popouts only.

## Layer Ownership
- `visualizer` UI layer only.

## Files Changed
- `visualizer/src/ui/panels/rc_panel_tools.cpp`
- `visualizer/src/ui/rc_ui_root.cpp`

## Implementation Summary
- Reworked the `TOOL DECK` area in `rc_panel_tools.cpp` into an embedded library host:
  - Added per-library tabs (`Axiom`, `Water`, `Road`, `District`, `Lot`, `Building`).
  - Added `Popout Active Library` control for explicit detached windows.
  - Routed `Axiom` tab to `AxiomEditor::DrawAxiomLibraryContent()` for full per-type tooling.
  - Routed non-axiom tabs through tool-action dispatch grids in-panel.
- Updated root window routing so shared library windows are no longer drawn by default when embedded mode is active; popout windows still render.

## Validation
- Built GUI target successfully:
  - `"/mnt/c/Program Files/CMake/bin/cmake.exe" --build build_vs --config Release --target RogueCityVisualizerGui --parallel 4`
- Contract checks reported pass during build (tool wiring/context command/ImGui/UI compliance).

## Open Notes
- Active library selection falls back to current domain when no library-open state is set, so embedded tabs remain functional even after layout resets.
- If a future preference toggle is needed, the embedded-vs-windowed policy can be exposed from `DrawToolLibraryWindows()` as runtime config.

## Handoff
- First unblocked next step: interactive UX check in app for tab-switching and popout behavior across all six libraries.
- `CHANGELOG updated: yes`.
