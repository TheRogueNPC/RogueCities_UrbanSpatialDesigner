# Codex Handoff - UI Compliance Contract Refresh (2026-03-06)

## Scope
Updated contract enforcement and docs so current ImGui standards include the canonical RC wrapper API (`RC_UI::API`).

## Enforcement Changes
- `tools/check_imgui_contracts.py`
  - `DrawContent()` top-level window guard now also blocks `API::BeginPanel`.
  - Loop ID / duplicate-label heuristics now scan both `ImGui::...` and `API::...` interactive calls.
  - Added canonical migration guardrails:
    - `CANONICAL_API_PANELS = { "rc_panel_texture_editing.cpp" }`
    - Requires `#include "ui/api/rc_imgui_api.h"`
    - Rejects regressions to raw calls: `ImGui::Button`, `DragFloat`, `DragInt`, `SliderScalar`, `TextDisabled`, `SetNextItemWidth`, `Indent`, `Unindent`.
- `tools/check_ui_compliance.py`
  - Updated guidance text for raw `ImGui::Begin(...)` violations to include `RC_UI::API::BeginPanel()` as valid wrapper path.

## Documentation Alignment
- `docs/20_specs/ui-faq-compliance-matrix.md`
  - Added canonical wrapper row and enforcement ownership for `visualizer/src/ui/api/rc_imgui_api.*`.
- `docs/knowledge-base/notes/ui-migration-compliance-and-automation.md`
  - Added wrapper-first + `API::Mutate` policy and corrected source references.
- `AI/collaboration/imgui_coding_standard.md`
  - Added "Part 1.5: Canonical RC Wrapper API" rules.
- `CHANGELOG.md`
  - Added 2026-03-06 entry for contract refresh.

## Validation
- `python tools/check_imgui_contracts.py` -> pass
- `python tools/check_ui_compliance.py` -> pass
- `cmake --build build_vs --target check_ui_compliance --config Debug` -> pass
- `cmake --build build_vs --target check_imgui_contracts --config Debug` -> pass
