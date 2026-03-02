# Codex Build Fix Brief - ToolDomain HUD Badge

## Summary
Resolved a Debug build failure in `RogueCityVisualizerGui` caused by calling `.c_str()` on `gs.tool_runtime.active_domain` (type `ToolDomain` enum) in `rc_viewport_overlays.cpp`.

## Root Cause
- `RenderToolBadgeHUD(...)` treated `ToolDomain` as a string object.
- MSVC error: `C2228: left of '.c_str' must have class/struct/union`.

## Fix Applied
- File: `visualizer/src/ui/viewport/rc_viewport_overlays.cpp`
- Replaced invalid enum `.c_str()` usage with safe `magic_enum::enum_name(...)` formatting and fallback label `VIEWPORT`.

## Validation
- Rebuilt `RogueCityVisualizerGui` in Debug through dev shell environment.
- Confirmed fresh outputs/tlogs at `2026-02-28 09:50`:
  - `build_vs/visualizer/RogueCityVisualizerGui.dir/Debug/rc_viewport_overlays.obj`
  - `bin/RogueCityVisualizerGui.exe`

## Layer Ownership
- Layer worked: `visualizer/` only.
- No changes to `core/`, `generators/`, or generation policy.

## Changelog
- `CHANGELOG.md` updated in the same session: **yes**.
