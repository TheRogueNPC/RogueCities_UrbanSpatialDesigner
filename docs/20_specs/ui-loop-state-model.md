# UI Loop and State Model

Purpose: define the exact runtime layers, state flags, and ownership rules for the GUI so layout/input regressions are prevented.

## 1. Runtime Layers

### Layer 0: OS Window State (GLFW)
- Source: `visualizer/src/main_gui.cpp`
- Core states:
  - `glfwWindowShouldClose(window)`
  - `glfwGetWindowAttrib(window, GLFW_ICONIFIED)`
- Rules:
  - If iconified, do **not** run frame update/render.
  - Enforce minimum window shrink via `glfwSetWindowSizeLimits()` at 25% of detected monitor work area (floor: `320x240`).
  - Restore from iconified does not force a dock reset; existing dock topology is preserved.

### Layer 1: App Main Loop
- Source: `visualizer/src/main_gui.cpp`
- Sequence per frame (when not iconified):
  1. `glfwPollEvents()`
  2. Optional low-level input bridge update (`LInput`)
  3. `ImGui_ImplOpenGL3_NewFrame()`
  4. `ImGui_ImplGlfw_NewFrame()`
  5. `ImGui::NewFrame()`
  6. HFSM update: `hfsm.update(gs, dt)`
  7. Hotkeys:
     - `Ctrl+Shift+R`: hard reset ImGui settings + dock layout reset
     - `Ctrl+R`: dock layout reset
  8. `RC_UI::DrawRoot(dt)`
  9. `ImGui::Render()` + OpenGL submit + optional platform windows

### Layer 2: Root UI Orchestrator
- Source: `visualizer/src/ui/rc_ui_root.cpp`
- Responsibilities:
  - Build/host dockspace (`RogueDockHost`, `RogueDockSpace`)
  - Maintain dock topology state
  - Route high-level windows: viewport, tool deck, libraries, master panel

### Layer 3: Dock Layout State Machine
- Source: `visualizer/src/ui/rc_ui_root.cpp`
- Control flags:
  - `s_dock_built`
  - `s_dock_layout_dirty`
  - `s_deferred_layout_on_small_viewport`
  - `s_stable_valid_viewport_frames`
  - `s_force_uncollapse_frames`
- Supporting state:
  - `s_dock_nodes` (resolved node IDs)
  - `s_pending_dock_requests` (queued redocks)
- Transitions:
  - Invalid/tiny viewport -> defer layout build
  - Restored valid viewport -> wait for stable valid-frame debounce, then rebuild layout
  - Rebuild complete -> finish dock builder + force uncollapse windows for several frames

### Layer 4: Input Arbitration Gate
- Sources:
  - `visualizer/src/ui/rc_ui_input_gate.cpp`
  - `visualizer/src/ui/panels/rc_panel_axiom_editor.cpp`
  - `visualizer/src/ui/rc_ui_root.cpp`
- Gate outputs:
  - `allow_viewport_mouse_actions`
  - `allow_viewport_key_actions`
- Rule:
  - Viewport actions must consume `UiInputGateState`, not manual hover-only checks.
  - Gate state is published each frame and visible in Dev Shell diagnostics.

### Layer 5: Master Panel Router
- Source: `visualizer/src/ui/panels/RcMasterPanel.cpp`
- State:
  - `m_active_category`
  - `m_active_panel`
  - `m_last_active_panel`
  - `m_popout_states`
  - search state (`m_search_open`, `m_search_filter`, etc.)
- Rendering contract:
  - Row 1: category tabs
  - Row 2: sub-tabs for active category
  - Row 3+: drawer content child
- Activation lifecycle:
  - `on_deactivated()` previous drawer
  - `on_activated()` new drawer

#### Category Routing Snapshot (Current)
- `Indices`: Road, District, Lot, River, Building index drawers.
- `Controls`: Zoning, Lot, Building, Water control drawers.
- `Tools`: Tool Deck content drawer (`AxiomBar`) and workflow/tools drawer (`Tools`).
- `System`: Telemetry, Log, Inspector, System Map, Dev Shell, UI Settings.
- `AI`: AI Console, UI Agent, City Spec (runtime hidden when `dev_mode_enabled == false`).

### Layer 5.5: Tool Action Contract and Dispatcher
- Sources:
  - `visualizer/src/ui/tools/rc_tool_contract.cpp`
  - `visualizer/src/ui/tools/rc_tool_dispatcher.cpp`
  - `visualizer/src/ui/rc_ui_root.cpp`
- Responsibilities:
  - Catalog all non-axiom library actions (`ToolActionId`, policy, availability, disabled reason).
  - Enforce safe-hybrid click behavior:
    - Immediate mode/subtool activation
    - No destructive generation on library click
  - Route all library clicks through a single dispatcher path.
- Runtime state sink:
  - `GlobalState::tool_runtime` records active domain, active subtool, and last dispatch metadata.

### Layer 5.6: Viewport Command Registry and Surfaces
- Sources:
  - `visualizer/src/ui/commands/rc_context_command_registry.cpp`
  - `visualizer/src/ui/commands/rc_context_menu_smart.cpp`
  - `visualizer/src/ui/commands/rc_context_menu_pie.cpp`
  - `visualizer/src/ui/commands/rc_command_palette.cpp`
  - `visualizer/src/ui/viewport/rc_viewport_interaction.cpp`
- Responsibilities:
  - Expose one command registry sourced from `ToolActionCatalog`.
  - Expose global viewport commands (minimap toggle, force generate, dock reset).
  - Render three command UIs (Smart List, Pie, Global Palette) over the same command IDs.
  - Route all command execution through `Commands::ExecuteCommand` -> `DispatchToolAction`.
- Input rules:
  - Respect `UiInputGateState` and `io.WantTextInput`.
  - Preserve `Ctrl+P` Master Panel search while keeping `P` viewport palette when viewport-focused.

### Layer 5.7: Generation Coordination and Output Application
- Sources:
  - `app/src/Integration/GenerationCoordinator.cpp`
  - `app/src/Integration/CityOutputApplier.cpp`
  - `visualizer/src/ui/viewport/rc_viewport_scene_controller.cpp`
  - `visualizer/src/ui/panels/rc_panel_axiom_editor.cpp`
- Responsibilities:
  - Coordinate generation request serials and reason codes.
  - Apply `CityOutput` into `GlobalState` via a single canonical applier.
  - Maintain shared `SceneFrame` updates + minimap synchronization from one controller path.
  - Keep panel rendering code free of direct output-application ownership.

### Layer 6: Drawer/Panel Content
- Sources:
  - `visualizer/src/ui/panels/RcPanelDrawers.cpp`
  - individual panel files
- Contract:
  - Drawer `draw()` is content-only (no standalone window begin/end)
  - `DrawContent()` functions are content-only
  - `Draw()` functions may own standalone windows, but are not used inside Master Panel routing

## 2. Ownership Rules (Critical)

### Window Wrapper Ownership
Only wrapper owners may call matching End functions.

- Wrapper-owner path (`Draw()` style):
  - `BeginTokenPanel` / `BeginDockableWindow`
  - `uiint.BeginPanel`
  - content
  - `uiint.EndPanel`
  - `EndTokenPanel` / `EndDockableWindow`

- Embedded path (`DrawContent()` in Master Panel):
  - **No** `BeginTokenPanel`
  - **No** `EndTokenPanel`
  - **No** `uiint.EndPanel` unless a matching `BeginPanel` exists in same function scope

Violation of this rule causes stack mismatch and can crash.

### Tool Library Ownership
- Top-level tool-library windows are root-managed in `visualizer/src/ui/rc_ui_root.cpp`.
- Axiom library content is rendered through `Panels::AxiomEditor::DrawAxiomLibraryContent()` (content-only, no window begin/end).
- Cross-layer creation (viewport content creating top-level tool windows) is disallowed.
- Non-axiom library actions must be catalog-driven (`rc_tool_contract`) and dispatched via `DispatchToolAction`.
- No library click handler may directly mutate generation data unless policy is explicitly `ActivateAndExecute`.

## 3. AI Feature Gating Model

### Compile-Time Gate
- Macro: `ROGUE_AI_DLC_ENABLED`
- If not defined, AI drawers are not registered.

### Runtime Gate
- Flag: `gs.config.dev_mode_enabled`
- Behavior:
  - AI category tabs are hidden when `dev_mode_enabled == false`.
  - AI drawers return not visible when `dev_mode_enabled == false`.

### Service Gate
- `AiBridgeRuntime::IsOnline()` checked inside AI panel content.
- Offline state shows non-crashing status guidance and returns safely.

## 4. Docking Invariants

- Docking must remain enabled in ImGui config for GUI builds.
- Dock layout rebuild must never churn continuously from iconified/tiny viewport geometry.
- Center workspace must retain a minimum width share.
- Legacy standalone panel windows must not be resurrected from stale ini settings.

## 5. Recovery and Reset Controls

- Soft reset: `Ctrl+R`
  - Rebuild dock tree, keep saved window settings.
- Hard reset: `Ctrl+Shift+R`
  - Clear ImGui ini settings and rebuild dock tree.
- Restore-from-minimize:
  - Main loop detects transition and triggers one dock reset.

## 6. Regression Checklist

When touching UI loop/docking/panel code, verify:

1. Launch minimized, then restore: layout is valid and uncollapsed.
2. Top category tabs are clickable.
3. Sub-tabs (indices/controls/etc.) are clickable and not overlapped.
4. Dock/undock works for primary windows.
5. AI category behavior:
   - hidden/blocked when dev mode off
   - no crash when AI bridge offline
6. No unmatched Begin/End calls in content-only paths.
7. No dead tool-library clicks; each action must dispatch or show explicit disabled reason.
8. `GlobalState::tool_runtime` updates on each dispatched action.

## 7. File Map

- Main frame loop: `visualizer/src/main_gui.cpp`
- Root dock/layout: `visualizer/src/ui/rc_ui_root.cpp`
- Input gate: `visualizer/src/ui/rc_ui_input_gate.cpp`
- Master router: `visualizer/src/ui/panels/RcMasterPanel.cpp`
- Drawer registry: `visualizer/src/ui/panels/PanelRegistry.cpp`
- Drawer implementations: `visualizer/src/ui/panels/RcPanelDrawers.cpp`
- Tool contract: `visualizer/src/ui/tools/rc_tool_contract.h`
- Tool dispatcher: `visualizer/src/ui/tools/rc_tool_dispatcher.h`
- Viewport command registry: `visualizer/src/ui/commands/rc_context_command_registry.h`
- Viewport command smart menu: `visualizer/src/ui/commands/rc_context_menu_smart.h`
- Viewport command pie menu: `visualizer/src/ui/commands/rc_context_menu_pie.h`
- Viewport command palette: `visualizer/src/ui/commands/rc_command_palette.h`
- Viewport command trigger arbitration: `visualizer/src/ui/viewport/rc_viewport_interaction.h`
- Generation coordinator: `app/include/RogueCity/App/Integration/GenerationCoordinator.hpp`
- City output applier: `app/include/RogueCity/App/Integration/CityOutputApplier.hpp`
- Viewport scene controller: `visualizer/src/ui/viewport/rc_viewport_scene_controller.h`
- RogueProfiler: `generators/include/RogueCity/Generators/Scoring/RogueProfiler.hpp`
- Compliance matrix: `docs/20_specs/ui-faq-compliance-matrix.md`
- Tool contract spec: `docs/20_specs/tool-wiring-contract.md`
- Tool action matrix: `docs/20_specs/tool-action-matrix.md`
- Generator/viewport spec: `docs/20_specs/generator-viewport-contract.md`
- Context command spec: `docs/20_specs/context-command-system.md`
- RogueProfiler spec: `docs/20_specs/rogue-profiler-spec.md`
- Tool wiring runbook: `docs/30_runbooks/tool-wiring-regression-checklist.md`
- AI panels:
  - `visualizer/src/ui/panels/rc_panel_ai_console.cpp`
  - `visualizer/src/ui/panels/rc_panel_ui_agent.cpp`
  - `visualizer/src/ui/panels/rc_panel_city_spec.cpp`
