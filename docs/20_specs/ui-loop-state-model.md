# UI Loop and State Model

Purpose: define runtime ownership and interaction contracts that prevent layout and input regressions.

## 1. Runtime Layers

### Layer 0: OS Window State (GLFW)

- Source: `visualizer/src/main_gui.cpp`
- Core states:
  - `glfwWindowShouldClose(window)`
  - `glfwGetWindowAttrib(window, GLFW_ICONIFIED)`
- Rules:
  - If iconified, skip frame update/render.
  - Enforce minimum size with `glfwSetWindowSizeLimits`.
  - Restoring from iconified must not force dock topology reset.

### Layer 1: App Main Loop

- Source: `visualizer/src/main_gui.cpp`
- Sequence per active frame:
  1. `glfwPollEvents()`
  2. Optional `LInput` update
  3. `ImGui_ImplOpenGL3_NewFrame()`
  4. `ImGui_ImplGlfw_NewFrame()`
  5. `ImGui::NewFrame()`
  6. `hfsm.update(gs, dt)`
  7. Hotkeys (`Ctrl+Shift+R` hard reset, `Ctrl+R` soft reset)
  8. `RC_UI::DrawRoot(dt)`
  9. `ImGui::Render()` and submit

### Layer 2: Root UI Orchestrator

- Source: `visualizer/src/ui/rc_ui_root.cpp`
- Responsibilities:
  - Host dockspace (`RogueDockHost`, `RogueDockSpace`)
  - Maintain dock topology state
  - Route viewport, tool deck, libraries, and master panel

### Layer 3: Dock Layout State Machine

- Source: `visualizer/src/ui/rc_ui_root.cpp`
- State flags:
  - `s_dock_built`
  - `s_dock_layout_dirty`
  - `s_deferred_layout_on_small_viewport`
  - `s_stable_valid_viewport_frames`
  - `s_force_uncollapse_frames`
- Supporting state:
  - `s_dock_nodes`
  - `s_pending_dock_requests`
- Transition policy:
  - Tiny/invalid viewport defers layout build
  - Rebuild only after stable-frame debounce
  - Uncollapse protection runs for bounded frames after rebuild

### Layer 4: Input Arbitration Gate

- Sources:
  - `visualizer/src/ui/rc_ui_input_gate.cpp`
  - `visualizer/src/ui/panels/rc_panel_axiom_editor.cpp`
  - `visualizer/src/ui/rc_ui_root.cpp`
- Gate outputs:
  - `allow_viewport_mouse_actions`
  - `allow_viewport_key_actions`
  - `mouse_block_reason`
  - `key_block_reason`
  - `any_modal_open`
- Rules:
  - Viewport actions consume `UiInputGateState`.
  - Mouse gating is canvas-local (not global `WantCaptureMouse`).
  - Gate state is published every frame for diagnostics.

### Layer 5: Master Panel Router

- Source: `visualizer/src/ui/panels/RcMasterPanel.cpp`
- State:
  - `m_active_category`
  - `m_active_panel`
  - `m_last_active_panel`
  - `m_popout_states`
  - search state
- Rendering contract:
  - Row 1: categories
  - Row 2: sub-tabs
  - Row 3+: drawer content child
- Activation lifecycle:
  - call `on_deactivated()` before panel switch
  - call `on_activated()` after panel switch

#### Category Routing Snapshot

- `Indices`: road, district, lot, river, building index drawers
- `Controls`: zoning, lot, building, water controls
- `Tools`: tool deck and workflow/tools drawer
- `System`: telemetry, log, inspector, system map, dev shell, UI settings
- `AI`: AI Console, UI Agent, City Spec (hidden when `dev_mode_enabled == false`)

### Layer 5.5: Tool Action Contract and Dispatcher

- Sources:
  - `visualizer/src/ui/tools/rc_tool_contract.cpp`
  - `visualizer/src/ui/tools/rc_tool_dispatcher.cpp`
  - `visualizer/src/ui/tools/rc_tool_interaction_metrics.cpp`
  - `visualizer/src/ui/tools/rc_tool_geometry_policy.cpp`
- Responsibilities:
  - Catalog all non-axiom actions and policies
  - Route library clicks through a single dispatcher path
  - Keep activation safe-hybrid by default
  - Publish runtime state into `GlobalState::tool_runtime`

### Layer 5.6: Viewport Command Registry and Surfaces

- Sources:
  - `visualizer/src/ui/commands/rc_context_command_registry.cpp`
  - `visualizer/src/ui/commands/rc_context_menu_smart.cpp`
  - `visualizer/src/ui/commands/rc_context_menu_pie.cpp`
  - `visualizer/src/ui/commands/rc_command_palette.cpp`
  - `visualizer/src/ui/viewport/rc_viewport_interaction.cpp`
- Responsibilities:
  - Maintain one command registry from tool catalog IDs
  - Execute commands via `ExecuteCommand` → `DispatchToolAction`
  - Handle command trigger routing and viewport interaction outcomes

### Layer 5.7: Generation Coordination and Output Application

- Sources:
  - `app/src/Integration/GenerationCoordinator.cpp`
  - `app/src/Integration/CityOutputApplier.cpp`
  - `visualizer/src/ui/viewport/rc_viewport_scene_controller.cpp`
- Responsibilities:
  - Coordinate request serials and reasons
  - Apply output through canonical applier only
  - Keep scene frame/minimap sync centralized
- Domain policy:
  - `Axiom`: live/debounced
  - `Water`, `Road`, `District`, `Zone`, `Lot`, `Building`: explicit generation

### Layer 6: Drawer and Panel Content

- Sources:
  - `visualizer/src/ui/panels/RcPanelDrawers.cpp`
  - panel files under `visualizer/src/ui/panels/`
- Contract:
  - `DrawContent()` is content-only
  - Wrapper `Draw()` owns standalone window begin/end

## 2. Ownership Rules

### Window Wrapper Ownership

- Wrapper-owner path must balance begin/end in same scope.
- Embedded master-panel content must not open standalone wrappers.

### Tool Library Ownership

- Top-level tool-library windows are root-managed in `rc_ui_root.cpp`.
- Axiom library content is content-only (`DrawAxiomLibraryContent`).
- Library clicks use dispatcher and must honor policy.

## 3. AI Feature Gating

### Compile-Time Gate

- Macro: `ROGUE_AI_DLC_ENABLED`

### Runtime Gate

- Flag: `gs.config.dev_mode_enabled`
- AI tabs are hidden when dev mode is off.

### Service Gate

- `AiBridgeRuntime::IsOnline()` is checked in AI panels.
- Offline behavior must be non-crashing and informative.

## 4. Docking Invariants

- Docking remains enabled for GUI builds.
- Rebuild logic must be stable and non-churning during resize.
- Center workspace keeps minimum width share.
- No helper popups may steal focus during resize flow.

## 5. Recovery Controls

- `Ctrl+R`: soft dock reset
- `Ctrl+Shift+R`: hard reset (ini + dock)
- Minimize/restore may trigger one bounded dock reset path

## 6. Regression Checklist

When changing loop, docking, or input routing:

1. Launch minimized and restore.
2. Verify tabs and sub-tabs remain clickable.
3. Verify dock/undock/popout/redock is stable.
4. Verify AI gating and offline behavior.
5. Verify begin/end ownership integrity.
6. Verify tool dispatch and runtime status updates.

## 7. File Map

- Main loop: `visualizer/src/main_gui.cpp`
- Root dock/layout: `visualizer/src/ui/rc_ui_root.cpp`
- Input gate: `visualizer/src/ui/rc_ui_input_gate.cpp`
- Master router: `visualizer/src/ui/panels/RcMasterPanel.cpp`
- Drawer registry: `visualizer/src/ui/panels/PanelRegistry.cpp`
- Drawer implementations: `visualizer/src/ui/panels/RcPanelDrawers.cpp`
- Tool contract/dispatcher: `visualizer/src/ui/tools/`
- Command surfaces: `visualizer/src/ui/commands/`
- Viewport interaction: `visualizer/src/ui/viewport/rc_viewport_interaction.cpp`
- Generation coordinator/applier: `app/src/Integration/`
