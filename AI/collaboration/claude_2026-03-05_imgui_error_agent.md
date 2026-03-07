# ImGui Error Recovery Agent

**Date:** 2026-03-05
**Author:** Claude (claude-sonnet-4-6)
**CHANGELOG updated:** yes

---

## What Was Done

Implemented `RC_UI::Panels::ImGuiError` — a self-contained ImGui error recovery agent that:

1. **Configures `io.ConfigErrorRecovery*` flags** at startup (programmer-seat defaults: all ON).
2. **Scans the ImGui internal debug log** (`ImGuiContext::DebugLogBuf`) on every frame for lines tagged
   `[imgui-error]` and surfaces them in a live scrollable table.
3. **Exposes `BeginProtectedSection` / `EndProtectedSection`** wrapping
   `ImGui::ErrorRecoveryStoreState` / `ImGui::ErrorRecoveryTryToRecoverState` for
   scripting hosts and exception boundaries.
4. **Provides runtime config toggles** in the panel UI so devs can switch between
   Scenario 1 (asserts), Scenario 2 (tooltips-only), Scenario 3 (non-programmer), etc.

---

## New Files Created

| File | Purpose |
|------|---------|
| `visualizer/src/ui/panels/rc_panel_imgui_error.h` | Public API: `Init`, `Shutdown`, `IsOpen`, `Toggle`, `Draw`, `BeginProtectedSection`, `EndProtectedSection`, `GetErrorCount` |
| `visualizer/src/ui/panels/rc_panel_imgui_error.cpp` | Implementation: debug log scanner, error deque (max 64), config section, error log table, protected section API doc |

## Files Modified

| File | Change |
|------|--------|
| `visualizer/src/main.cpp` | `#include rc_panel_imgui_error.h`; `ImGuiError::Init()` after `CreateContext`; `ImGuiError::Shutdown()` before `DestroyContext` |
| `visualizer/src/main_gui.cpp` | Same Init/Shutdown as `main.cpp` |
| `visualizer/src/ui/rc_ui_root.cpp` | `#include rc_panel_imgui_error.h`; `Panels::ImGuiError::Draw(dt)` in DrawRoot; "ImGui Error Agent" menu item in Terminal menu |
| `visualizer/CMakeLists.txt` | `rc_panel_imgui_error.cpp` added to both `RogueCityVisualizerHeadless` and `RogueCityVisualizerGui` source lists |
| `AI/collaboration/claude_handoff_prompt.md` | New section 6: error agent mandate + API summary |
| `CHANGELOG.md` | Entry in [Unreleased] |

---

## Architecture Decisions

1. **Standalone floating window** (not master panel drawer): The error agent is a developer diagnostic
   tool that should be accessible at any time, independent of the master panel tab state. It has its
   own `Draw(dt)` with `ImGui::Begin/End`. It can be promoted to the drawer registry later if needed.

2. **Debug log scanning, not a callback**: ImGui's `ConfigErrorRecoveryEnableDebugLog=true` writes
   `[imgui-error]` lines to `ImGuiContext::DebugLogBuf`. We scan this buffer incrementally (tracking
   `s_log_consumed` byte offset) each frame. No threads, no callbacks, no ImGui internals beyond
   `imgui_internal.h` which is already in the PCH.

3. **Programmer-seat defaults all ON**: `ConfigErrorRecoveryEnableAssert=true` is the safe default.
   Devs explicitly toggle it off in the panel if they want tooltip-only mode. This follows the
   ImGui doc mandate: "on programmer seats you MUST have at minimum Asserts or Tooltips enabled."

4. **`BeginProtectedSection` is NOT nested-safe**: Only one protection frame is active at a time
   (single static `s_protection_state`). Callers must not nest. This is sufficient for current use.

---

## Error Recovery Scenarios (from ImGui docs)

| Scenario | ConfigErrorRecoveryEnableAssert | ConfigErrorRecoveryEnableTooltip | Notes |
|----------|--------------------------------|----------------------------------|-------|
| 1 — Programmer (default) | **true** | true | Asserts fire + tooltip visible |
| 2 — Programmer (nicer) | false | **true** | Tooltip only, tooltip has "re-enable asserts" button |
| 3 — Non-programmer seat | false | false | Only log output (ensure it's visible!) |
| 4 — Scripting host | false (during script) | true | Use `BeginProtectedSection` per script call |
| 5 — Exception boundary | any | any | `BeginProtectedSection` before `try{}`, `EndProtectedSection` in `catch{}` |

---

## Verification Checklist

```
rc-bld-headless
rc-bld-gui  (if available)
rc-perceive-ui -Mode quick -Screenshot
```

Open "Terminal → ImGui Error Agent" in the running UI. Confirm:
- [ ] Panel opens as floating window
- [ ] All 4 Config checkboxes show `true` (programmer defaults)
- [ ] Error Log table is empty on clean run
- [ ] Toggling checkboxes updates `ImGui::GetIO()` live

---

## Handoff Notes for Next Agent

- **MANDATE**: Any panel or system driving dynamic / scripted ImGui calls MUST wrap that code
  with `RC_UI::Panels::ImGuiError::BeginProtectedSection` / `EndProtectedSection`.
- `GetErrorCount()` returns the deque size — suitable for a status-bar badge. Not wired to the
  status bar yet; that's a future enhancement.
- The panel does **not** persist errors across sessions. `s_log_consumed` resets on `Init()`.
- `imgui_internal.h` is required for `ImGuiErrorRecoveryState` and `ErrorRecoveryStore/TryToRecover`.
  It is already in the PCH for both executables.
