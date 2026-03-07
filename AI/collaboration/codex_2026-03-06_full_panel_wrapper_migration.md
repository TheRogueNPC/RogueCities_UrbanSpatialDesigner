# Codex Handoff - Full Panel Wrapper Migration (2026-03-06)

## Objective
User mandate: all legacy panels must utilize the new canonical RC ImGui API wrapper and stop regressing visually.

## What Changed

### 1) Canonical API surface expanded
- File: `visualizer/src/ui/api/rc_imgui_api.h/.cpp`
- Added wrapper actions for missing panel interactions/layout controls:
  - `SmallButton`, `InvisibleButton`, `Checkbox`, `RadioButton`, `Selectable` (both overloads)
  - `InputText`, `InputTextMultiline`, `InputInt`, `InputFloat`, `InputFloat2`, `InputScalar`
  - `SliderFloat`, `SliderInt`, `SliderScalar`, `SliderAngle`, `DragFloat`, `DragInt`
  - `Combo`, `BeginCombo`, `EndCombo`, `CollapsingHeader`, `MenuItem` (both overloads)
  - `OpenPopup`, `BeginPopup`, `BeginPopupModal`, `EndPopup`, `CloseCurrentPopup`
  - `Spacing`, `Separator`, `SameLine`, `BeginDisabled`, `EndDisabled`
  - `TextDisabled` upgraded to variadic format form
- Added global migration alias at end of header:
  - `namespace API = RC_UI::API;`

### 2) Legacy panel migration (bulk)
- Replaced legacy panel-level widget calls from `ImGui::...` to `API::...` across panel sources in `visualizer/src/ui/panels/*.cpp`.
- Auto-inserted `#include "ui/api/rc_imgui_api.h"` in migrated panel files.

### 3) Strict contract enforcement
- File: `tools/check_imgui_contracts.py`
- Added strict ban on raw panel widget calls (direct `ImGui::Button/Checkbox/Input*/Drag*/Slider*/Combo*/MenuItem/...`) in panel sources.
- Added include contract: if a panel uses `API::`, it must include `ui/api/rc_imgui_api.h`.
- Existing DrawContent window-ownership and input-gate checks remain active.

### 4) Docs/changelog updates
- `docs/20_specs/ui-faq-compliance-matrix.md`: updated to strict wrapper-only panel widget policy.
- `AI/collaboration/imgui_coding_standard.md`: strengthened wrapper mandate to full-panel scope.
- `CHANGELOG.md`: added entry for full panel wrapper migration + strict no-raw-widget gate.

## Validation
- `python tools/check_imgui_contracts.py` -> pass
- `python tools/check_ui_compliance.py` -> pass
- `cmake --build build_vs --target RogueCityVisualizerHeadless --config Debug -j 8` -> pass
- `cmake --build build_vs --target RogueCityVisualizerGui --config Debug -j 8` -> pass

## Notes
- `API::Mutate` remains the explicit escape hatch for specialized raw ImGui behavior.
- This migration targets panel-level interaction/widget calls; non-widget low-level rendering (`ImDrawList`, metrics windows, etc.) remains intentionally explicit where needed.
