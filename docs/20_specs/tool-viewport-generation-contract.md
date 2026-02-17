# Tool-Viewport-Generation Contract

Purpose: define deterministic click behavior from tool selection to viewport interaction and generation policy.

## Contract

1. Every enabled tool action must produce one of:
   - viewport mutation (`InteractionOutcome::Mutation`),
   - selection/gizmo interaction (`InteractionOutcome::Selection`, `InteractionOutcome::GizmoInteraction`),
   - activation only (`InteractionOutcome::ActivateOnly`),
   - blocked by input gate (`InteractionOutcome::BlockedByInputGate`),
   - no eligible target (`InteractionOutcome::NoEligibleTarget`),
   - explicit non-mutating status.
2. Viewport interaction gating is canvas-local (`##ViewportCanvas`) and mediated by `UiInputGateState`.
3. Generator output application is single-path via `ApplyCityOutputToGlobalState(...)`.
4. Docking behavior remains unchanged and cannot be regressed by this contract.

## Interaction Outcomes

The `InteractionOutcome` enum provides deterministic classification of every viewport click:

- `None`: No action taken
- `Mutation`: State mutated (layer marked dirty, explicit generation required if domain policy is `ExplicitOnly`)
- `Selection`: Selection changed
- `GizmoInteraction`: Gizmo manipulation active
- `ActivateOnly`: Tool/subtool activated without mutation
- `BlockedByInputGate`: Input gate prevented interaction (check `InputBlockReason` diagnostics)
- `NoEligibleTarget`: Click location had no valid interaction target

## Domain Generation Policy

1. `Axiom`: `LiveDebounced`
2. `Water`: `ExplicitOnly`
3. `Road`: `ExplicitOnly`
4. `District`: `ExplicitOnly`
5. `Zone`: `ExplicitOnly`
6. `Lot`: `ExplicitOnly`
7. `Building`: `ExplicitOnly`

## Runtime Signals

1. `tool_runtime.last_action_*`: last catalog/dispatch action.
2. `tool_runtime.last_viewport_status`: last viewport interaction outcome.
3. `tool_runtime.explicit_generation_pending`: true when a mutation occurred in an explicit-only domain and generation has not yet been run.

## Enforcement

1. `python3 tools/check_tool_wiring_contract.py`
2. `python3 tools/check_generator_viewport_contract.py`
3. `python3 tools/check_imgui_contracts.py`
4. `python3 tools/check_ui_compliance.py`
