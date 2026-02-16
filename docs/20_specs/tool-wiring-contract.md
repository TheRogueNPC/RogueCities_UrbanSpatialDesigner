# Tool Wiring Contract

Purpose: define the single source of truth for tool-library actions so UI clicks are deterministic, traceable, and never dead.

## 1. Contract Modules

- Catalog: `visualizer/src/ui/tools/rc_tool_contract.h`
- Catalog data: `visualizer/src/ui/tools/rc_tool_contract.cpp`
- Dispatcher: `visualizer/src/ui/tools/rc_tool_dispatcher.h`
- Dispatcher implementation: `visualizer/src/ui/tools/rc_tool_dispatcher.cpp`

## 2. Core Types

- `ToolActionId`: stable ID for every library action.
- `ToolActionSpec`: declarative action entry.
- `ToolActionGroup`: `Primary`, `Spline`, `FutureStub`.
- `ToolExecutionPolicy`:
  - `ActivateOnly`: safe-hybrid default, no destructive generation.
  - `ActivateAndExecute`: reserved for explicit future opt-ins.
  - `Disabled`: visible stub, no-op.
- `ToolAvailability`: `Available` or `Disabled`.

## 3. Dispatch Contract

`DispatchToolAction(action_id, context)` must be the only ingress for library clicks.

Required behavior:
1. Validate action exists.
2. If disabled, return disabled status and record disabled reason.
3. Route HFSM mode transition by domain.
4. Update `GlobalState::tool_runtime` active domain and subtool.
5. Register introspection event (`triggered`).
6. Never run destructive generation for `ActivateOnly` actions.
7. Axiom icon clicks in `DrawAxiomLibraryContent()` must route through dispatcher with catalog action IDs.

## 4. Runtime State

`GlobalState::tool_runtime` stores:
- Active domain.
- Per-domain active subtools.
- Last action ID/label/status.
- Dispatch serial and frame stamp.

This state is surfaced in:
- Property tooling (`rc_property_editor.cpp`)
- Dev Shell diagnostics (`rc_panel_dev_shell.cpp`)

## 5. Zone Mapping Rule

`Zone` is mapped to existing district/zoning pipeline in this wave:
- HFSM event: `Tool_Districts`
- Runtime domain: `Zone` (for telemetry/diagnostics)
- Generation remains explicit via zoning controls.

## 6. Stub Policy

Future modules are visible but disabled:
- `FloorPlan`
- `Paths`
- `Flow`
- `Furnature`

Rule: disabled actions must include explicit `disabled_reason` text.

## 7. Compliance Gate

Script: `tools/check_tool_wiring_contract.py`

Checks:
1. Every `ToolActionId` enum value is cataloged.
2. Every available catalog action has a dispatch case.
3. Disabled actions include a reason.
4. Dispatcher does not reference unknown actions.

This script is chained from `tools/check_ui_compliance.py`.

## 8. Safe Hybrid Default

Current default profile:
- Click always activates tool/subtool immediately.
- Library click does not run destructive generation.
- Mutating generation remains explicit from control panels.
