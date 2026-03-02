# Incident Note: ToolDomain `.c_str()` Build Break (2026-02-28)

## Attribution
- Introduced by: **Gemini** (HUD changes in `rc_viewport_overlays.cpp`)
- Fixed by: **Codex**

## Symptom
- Build target: `RogueCityVisualizerGui` (Debug)
- Error: `C2228: left of '.c_str' must have class/struct/union`
- Failing expression: `gs.tool_runtime.active_domain.c_str()`

## What Gemini Changed (Relevant Scope)
- Added and wired mockup HUD passes in `ViewportOverlays::Render(...)`:
  - `RenderCenterCursorHUD(gs)`
  - `RenderToolBadgeHUD(gs)`
- In `RenderToolBadgeHUD(...)`, `active_domain` was treated as a string.

## Root Cause
- `tool_runtime.active_domain` is `RogueCity::Core::Editor::ToolDomain` (enum), declared in:
  - `core/include/RogueCity/Core/Editor/GlobalState.hpp`
- Enums do not expose `.c_str()`.

## Codex Fix
- File: `visualizer/src/ui/viewport/rc_viewport_overlays.cpp`
- Replaced invalid enum `.c_str()` usage with:
  - `magic_enum::enum_name(gs.tool_runtime.active_domain)`
  - Fallback text when name is unavailable: `VIEWPORT`
  - Bounded formatting via `%.*s` into fixed buffer.

## Edge Cases Around This Issue
1. Unknown/invalid enum values: `magic_enum::enum_name(...)` can be empty, so fallback text is required.
2. Buffer safety: fixed-size HUD label buffers must use bounded formatting (`%.*s`) to avoid overruns.
3. UX naming consistency: if product wants friendly labels, prefer `RC_UI::Tools::ToolDomainName(...)` over raw enum token names.
4. Similar risk surface: any future UI code that reads `ToolDomain` or subtool enums must not call `.c_str()` directly.

## Preventive Self-Check for Gemini
1. Before string ops, verify symbol type via declaration (`Go to definition` or header lookup).
2. For enums, use either:
   - `magic_enum::enum_name(...)` with empty fallback, or
   - canonical name helper (`ToolDomainName(...)`) when available.
3. Run targeted dev-shell build after HUD/panel edits before handoff.

## Validation
- Debug rebuild artifacts refreshed after fix (`rc_viewport_overlays.obj`, `bin/RogueCityVisualizerGui.exe`).

## Changelog
- `CHANGELOG.md` updated in same session: **yes**.
