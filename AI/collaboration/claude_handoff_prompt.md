# RogueCities AI Collaboration Handoff: The Stable Vanguard

**To:** Claude (and any other active AI Agents)
**From:** Gemini Base Command
**Topic:** Architectural Stabilization, Telemetry Gates, and Next Steps

## 1. The New Reality: Zero Tolerance for Silent Failures
You are entering a fully stabilized workspace. I have spent the last session fortifying the RogueCities engine with the user.

1.  **AI Telemetry Sink**: We have embedded a C++ telemetry singleton (`RogueCity::Core::Validation::AiTelemetry`). If the app crashes, it serializes a `runtime_crash.json`. You are mandated to read this file upon any user report of a crash. Do not ask for logs.
2.  **UI Introspection**: The ImGui application is now fully deterministic. We have enabled headless background execution (`RogueCityVisualizerHeadless.exe`) that can dump screenshots and widget trees (`ui_introspection_headless_latest.json`).
3.  **Strict Commit Gates**: The PowerShell Dev Shell now features `rc-full-smoke` and `rc-perceive-ui`. If you modify a UI panel, you **must** run these commands and verify your changes programmatically. We do not guess screen bounds anymore.

## 2. The Gemma-First Local Stack
The user has initialized a lightweight, multimodal, lightning-fast local AI stack powered by `Ollama` and the `Gemma` family:
- **`gemma3:4b`**: Fast synthesis and UI layout generation.
- **`codegemma:2b`**: Surgical code triage.
- **`granite3.2-vision` & `glm-ocr`**: Multimodal UI perception validating our headless screenshots.

This entire bridge is orchestrated natively in the `dev-shell.ps1`, spinning up Python servers in the background. If you need to write generation logic, assume it connects to the local Ollama via port `7222`.

## 3. The Unified Mindspace
We are no longer isolated assistants; we are a dedicated engineering organization. 

**MANDATE:** Every time you complete a major feature, draft a plan, or discover a framework gotcha, you must write a Markdown brief and save it inside the `AI/collaboration/` directory.
**MANDATE:** For every tangible change, update `CHANGELOG.md` in the same session and follow `AI/collaboration/CHANGELOG_MANDATE.md`.

## 4. Immediate Context: UI Layout Phase 4 Complete
Gemini has just completed the **Phase 4 UI Layout Refactor** for `RogueCities_UrbanSpatialDesigner`.
- **Global Structure:** The `ImGui` dockspace was constrained to 3 strictly proportioned columns (Master Panel, Viewport, Inspector) residing perfectly between a 34px top `Titlebar` and a 28px bottom `StatusBar`.
- **Inspector Panel:** We integrated a complex `Inspector` panel populated tightly with Tool Runtime telemetry, Gizmos, Layer toggles, and Property Editors matching the Y2K CSS Mockups.
- **HUD Geometry:** Shifted the `CompassGimbal` to `right: 90px`, and moved the scale ruler + coordinate bindings to exactly bottom-center to finalize the viewport aesthetic.
You can review the full breakdown in `AI/collaboration/gemini_handoff_ui_phase4_complete_2026-02-28.md` whenever referencing UI layout queries.

## 5. Implement Phase 5
1. Please proceed with implementing Phase 5 of the UI setup.
2. Review the previous sections and use the original `RC_UI_Mockup.html` you generated for your refactor in full at your discretion. The HTML is your master reference.

The environment is yours. Execute.

## 6. HFSM as the Game Loop — Frame Order Contract (2026-03-06)

The EditorHFSM is the application state driver. Frame order is non-negotiable:

```cpp
glfwPollEvents();          // Poll events
hfsm.update(gs, dt);       // State update — MUST come first
RC_UI::DrawRoot(dt);       // All panels read current-frame HFSM state
ImGui::Render();           // Bake
RenderDrawData(...);       // GPU
glfwSwapBuffers(window);   // Present
```

`hfsm.update` must run before any draw call. Events dispatched from panel buttons
(`hfsm.handle_event(...)`) take effect at the next frame's update.

**Bug fixed (2026-03-06):** `main.cpp` (headless binary) had `DrawRoot` before
`hfsm.update` — one-frame HFSM state lag in smoke tests. Corrected to match
`main_gui.cpp`.

---

## 7. ImGui Coding Standard (updated 2026-03-06)

**MANDATE:** All UI code must comply with:
`AI/collaboration/imgui_coding_standard.md`

### 6a. Zero-OOP Architecture (3 rules)

1. **Pass-by-Reference State** — No UI classes/structs holding widget state.
   Panels are freestanding functions taking engine data by `&`/`*`.
2. **Immediate Conditional** — No callbacks/listeners. User actions handled
   with `if (ImGui::Button(...))` in the same frame. That is the event system.
3. **Static Scoping** — UI-only transients use function-local `static`.
   Never a member variable.

Reject: UI structs with `Draw()`, stored `std::function`, `UIManager`,
`OnChanged`/`SetEnabled()`, engine→UI state copies.

### 6b. ID Stack — THE Most Common Bug

**Same label + same scope = ID collision → widget never responds to clicks.**

- Loops: wrap every iteration with `PushID(i)` / `PopID()`
- Duplicate labels: add `##suffix` — `"Delete##road"` vs `"Delete##zone"`
- Animated label, stable ID: use `###id` — `"FPS: 60###MyWindow"`
- When `PushID(idx)` is active: label `"##row"` is already unique,
  do NOT append `std::to_string(idx)` (redundant heap alloc)
- Debug collisions at runtime: `ImGui::ShowIDStackToolWindow()`

### 6c. Input Dispatch

Always feed mouse/keyboard to ImGui **before** gating on `WantCaptureMouse`.
Never manually check "is mouse over a window" — use `io.WantCaptureMouse`.

**RogueCities exception:** the viewport canvas is itself an ImGui widget so
`WantCaptureMouse` is always true inside it. `rc_ui_input_gate.cpp` handles
this correctly with canvas-hover state. Do not modify this logic.

### 6d. DPI (GLFW — auto-handled)

Project uses GLFW + `ConfigDpiScaleFonts/Viewports` in `main_gui.cpp`.
Never hardcode pixel sizes; express as multiples of `GetFontSize()` or
`GetFrameHeight()`. Use `PushFont(NULL, size)` to change font size per-scope.

### 6e. ImDrawList

After `AddLine`/`AddCircle` etc., call `ImGui::Dummy(ImVec2(w,h))` to
advance the cursor — otherwise the window collapses. Use `GetColorU32()`
when color must respect `style.Alpha`; use `IM_COL32()` for compile-time
constants that bypass global alpha.

### 6f. std::string Performance

Never construct `std::string` per-row per-frame for a widget label. Use:
- `snprintf(buf, sizeof(buf), ...)` with a stack buffer, OR
- plain `"##label"` when `PushID` already scopes the row

---

## 7. ImGui Error Recovery Agent (added 2026-03-05)

A dedicated diagnostic panel `RC_UI::Panels::ImGuiError` now manages all ImGui recoverable-error
handling. Every agent working on UI panels must be aware of it.

**Files:**
- `visualizer/src/ui/panels/rc_panel_imgui_error.h` / `.cpp`
- Initialized in `main.cpp` and `main_gui.cpp` after `ImGui::CreateContext()`
- Drawn as a floating window from `rc_ui_root.cpp::DrawRoot()` (Terminal → ImGui Error Agent)

**API:**
```cpp
// Startup — call after CreateContext
RC_UI::Panels::ImGuiError::Init();

// Frame — draws floating panel when open
RC_UI::Panels::ImGuiError::Draw(dt);

// Wrapping risky / scripted ImGui code (Scenarios 4 & 5)
RC_UI::Panels::ImGuiError::BeginProtectedSection("label");
// ... dynamic ImGui calls ...
RC_UI::Panels::ImGuiError::EndProtectedSection();

// Badge query for status bar
int n = RC_UI::Panels::ImGuiError::GetErrorCount();
```

**MANDATE:** Any panel, tool, or system that drives ImGui calls through a scripting language,
interpreted loop, or exception-capable code block **MUST** wrap those calls with
`BeginProtectedSection` / `EndProtectedSection`. Violation leads to silent stack corruption on
non-programmer seats.

**io.ConfigErrorRecovery* defaults (programmer seat — Scenario 1):**
- `ConfigErrorRecovery = true` (master switch)
- `ConfigErrorRecoveryEnableAssert = true` (asserts fire — this is the correct default)
- `ConfigErrorRecoveryEnableDebugLog = true` (errors surfaced in panel Error Log table)
- `ConfigErrorRecoveryEnableTooltip = true` (tooltip with "re-enable asserts" button)

Use the panel's Config section to switch to Scenario 2 (tooltip-only) at runtime without recompiling.
