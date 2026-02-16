# Generator-Viewport Contract

Purpose: define the single authoritative path from generation output into runtime editor/view data.

## Ownership

1. Generation scheduling is owned by `RogueCity::App::GenerationCoordinator`.
2. Generation output application is owned by `RogueCity::App::ApplyCityOutputToGlobalState`.
3. Viewport panels (`rc_panel_axiom_editor.cpp`) may request generation, but do not own output-application logic.

## Authoritative Flow

1. UI interaction updates axioms/config.
2. Panel requests generation through `GenerationCoordinator::RequestRegeneration()` or `ForceRegeneration()`.
3. `RealTimePreview` completes and invokes coordinator callback on main thread.
4. Callback applies output through `ApplyCityOutputToGlobalState(...)`.
5. Viewport/minimap consume the current output pointer for render overlays.

## Invariants

1. No panel-local `SyncGlobalStateFromPreview` implementation is allowed.
2. Any new generator completion path must call `ApplyCityOutputToGlobalState`.
3. Live preview and explicit generation must use coordinator APIs only.
4. Viewport index rebuild and dirty-layer cleanup must be controlled by applier options, not ad-hoc panel logic.

## Enforcement

1. `python3 tools/check_generator_viewport_contract.py`
2. `python3 tools/check_ui_compliance.py`
