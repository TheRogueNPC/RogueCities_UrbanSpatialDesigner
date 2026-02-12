# UI Migration Plan

## Goal
Enforce a single token/wrapper architecture for all editor UI so layout/theme changes are centralized and safe.

## Rules
- Panel windows must use `RC_UI::Components::BeginTokenPanel()` or `RC_UI::BeginDockableWindow()`.
- No `IM_COL32(...)` literals in panel source files.
- No raw `ImGui::PushStyleColor(ImGuiCol_WindowBg, ...)` in panel source files.
- Colors, spacing, and borders must come from `UITokens`.

## Enforcement
- Script: `tools/check_ui_compliance.py`
- CMake switch: `ROGUEUI_ENFORCE_DESIGN_SYSTEM`
- Default: `ON` (compliance target runs with normal builds)

## Current Migration State
- [x] Remove raw `ImGui::Begin(...)` from `visualizer/src/ui/panels/*.cpp`
- [x] Add wrapper support for closable token panels (`p_open`)
- [x] Add workspace preset save/load API in `RC_UI`
- [x] Add Dev Shell controls for preset save/load/reset
- [ ] Convert remaining raw style pushes in panels to wrapper helpers
- [ ] Add right-click HFSM context actions into shared context-menu wrapper
- [ ] Add ribbon-mode layout templates backed by saved presets

## AI Integration TODO
- Surface workspace presets and monitor-aware layout metadata to the UI Agent so it can suggest/apply dock/ribbon workspace setups safely.
