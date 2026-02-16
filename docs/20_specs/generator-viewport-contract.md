# Generator-Viewport Contract

Purpose: define the single authoritative path from generation output into runtime editor/view data.

## Ownership

1. Generation scheduling is owned by `RogueCity::App::GenerationCoordinator`.
2. Generation output application is owned by `RogueCity::App::ApplyCityOutputToGlobalState`.
3. Viewport panels (`rc_panel_axiom_editor.cpp`) may request generation, but do not own output-application logic.

## Authoritative Flow

1. UI interaction updates axioms/config.
2. `UpdateSceneController(...)` performs per-frame sync update, generation update, scene-frame build, and minimap sync.
3. Panel requests generation through `GenerationCoordinator::RequestRegeneration()` or `ForceRegeneration()`.
4. `RealTimePreview` completes and invokes coordinator callback on main thread.
5. Callback applies output through `ApplyCityOutputToGlobalState(...)`.
6. Viewport/minimap consume `SceneFrame` for render overlays.

## Invariants

1. No panel-local `SyncGlobalStateFromPreview` implementation is allowed.
2. Any new generator completion path must call `ApplyCityOutputToGlobalState`.
3. Live preview and explicit generation must use coordinator APIs only.
4. Viewport index rebuild and dirty-layer cleanup must be controlled by applier options, not ad-hoc panel logic.
5. Scene-frame ownership/update path must stay in viewport scene controller utilities, not duplicated in panel code.
6. Axiom camera/nav/tool mouse input path must stay in viewport interaction utilities, not duplicated in panel code.
7. Non-axiom viewport interaction (selection, gizmo, road/district/water vertex editing, and domain placement clicks) must stay in `rc_viewport_interaction.cpp`.

## Enforcement

1. `python3 tools/check_generator_viewport_contract.py`
2. `python3 tools/check_ui_compliance.py`
