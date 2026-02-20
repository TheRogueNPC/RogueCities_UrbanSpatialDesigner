# Tools Masterpass Plan (Repo-Aligned, 1–4 ASAP Passes)

## Implementation Status (2026-02-20)

### Pass Completion
- Pass 1: complete (non-axiom ingress delegated to handler pipeline, signature unchanged).
- Pass 2: complete (road + district handlers split into dedicated translation units).
- Pass 3: complete (lot + building + water handlers split into dedicated translation units).
- Pass 4: complete (systems-map runtime toggles/query plumbing + minimap bounds hardening + deterministic tests).

### Implemented File Anchors
- Non-axiom pipeline orchestration:
  - `visualizer/src/ui/viewport/handlers/rc_viewport_non_axiom_pipeline.cpp`
- Domain handler split:
  - `visualizer/src/ui/viewport/handlers/rc_viewport_road_handler.cpp`
  - `visualizer/src/ui/viewport/handlers/rc_viewport_district_handler.cpp`
  - `visualizer/src/ui/viewport/handlers/rc_viewport_lot_handler.cpp`
  - `visualizer/src/ui/viewport/handlers/rc_viewport_building_handler.cpp`
  - `visualizer/src/ui/viewport/handlers/rc_viewport_water_handler.cpp`
- Shared handler utilities:
  - `visualizer/src/ui/viewport/handlers/rc_viewport_handler_common.cpp`
  - `visualizer/src/ui/viewport/handlers/rc_viewport_domain_handlers.cpp`
- Systems map query/toggles:
  - `visualizer/src/ui/panels/rc_panel_system_map.cpp`
  - `visualizer/src/ui/panels/rc_system_map_query.cpp`
  - `core/include/RogueCity/Core/Editor/GlobalState.hpp`
- Minimap interaction hardening:
  - `visualizer/src/ui/panels/rc_panel_axiom_editor.cpp`
  - `visualizer/src/ui/viewport/rc_minimap_interaction_math.cpp`

### Latest Delta (2026-02-20)
- Fixed generation-depth propagation race in preview completion callback (AxiomBounds no longer applied as stale FullPipeline):
  - `app/include/RogueCity/App/Integration/RealTimePreview.hpp`
  - `app/src/Integration/RealTimePreview.cpp`
  - `visualizer/src/ui/panels/rc_panel_axiom_editor.cpp`
- Added road clipping for axiom-bounds preview generation so skeleton output stays inside active axiom influence volumes:
  - `generators/include/RogueCity/Generators/Pipeline/CityGenerator.hpp`
  - `generators/src/Generators/Pipeline/CityGenerator.cpp`
- Tuned AxiomBounds preview seed density to a bounded per-axiom range (reduces single-axiom full-map spread during live preview):
  - `app/src/Integration/RealTimePreview.cpp`
- Fixed ImGui conflicting-ID path in tools panel (mode buttons + layer visibility labels no longer collide):
  - `visualizer/src/ui/panels/rc_panel_tools.cpp`
- Refined district polygon authoring behavior:
  - `District -> Zone` now supports vertex-based polygon shaping (click/drag append, `Ctrl+DoubleClick` close)
  - `District -> Select/Inspect` now explicitly picks/selects district targets under cursor
  - file:
    - `visualizer/src/ui/viewport/handlers/rc_viewport_district_handler.cpp`
- Axiom preview generation is now road-skeleton-only and axiom-bounded:
  - `app/src/Integration/RealTimePreview.cpp`
  - `generators/include/RogueCity/Generators/Pipeline/CityGenerator.hpp`
  - `generators/src/Generators/Pipeline/CityGenerator.cpp`
  - `visualizer/src/ui/panels/rc_panel_axiom_editor.cpp`
- Water library primaries now map to distinct interaction intent (no forced pen default for Flow/Contour/Erode):
  - `visualizer/src/ui/tools/rc_tool_dispatcher.cpp`
- Shared spline semantics hardened for road/water:
  - whole-spline selection highlight
  - direct-select + tangent-handle drag behavior
  - freehand pen with Shift point mode
  - Ctrl+double-click close-spline
  - files:
    - `visualizer/src/ui/viewport/handlers/rc_viewport_road_handler.cpp`
    - `visualizer/src/ui/viewport/handlers/rc_viewport_water_handler.cpp`
    - `visualizer/src/ui/panels/rc_panel_axiom_editor.cpp`
- Polygon-domain usability updates:
  - district split safety guards + expanded vertex pick falloff
  - lot merge/slice/align now auto-acquire lot target under cursor if none is selected
  - viewport inspect tooltip now includes nearest-vertex index stubs for spline/polygon entities
  - non-axiom viewport now applies shared command-history hotkeys (`Ctrl+Z`, `Ctrl+Shift+Z`, `Ctrl+Y`)
  - water `Flow`/`Contour`/`Erode` subtools now use differentiated deformation behavior (tangent, axis, radial)
  - files:
    - `visualizer/src/ui/viewport/handlers/rc_viewport_district_handler.cpp`
    - `visualizer/src/ui/viewport/handlers/rc_viewport_lot_handler.cpp`
    - `visualizer/src/ui/viewport/handlers/rc_viewport_non_axiom_pipeline.cpp`
    - `visualizer/src/ui/viewport/handlers/rc_viewport_water_handler.cpp`

### Deterministic Verification Additions
- `tests/test_viewport_interaction_helpers.cpp`
- `tests/test_viewport_interaction_selection.cpp`
- `tests/test_editor_plan.cpp` (viewport invariants extension)
- `tests/test_systems_map_query.cpp`
- `tests/test_minimap_interaction_math.cpp`

### Worktree Clarification (Masterpass Scope)
- CMake wiring:
  - `CMakeLists.txt`
  - `visualizer/CMakeLists.txt`
- Viewport handlers:
  - `visualizer/src/ui/viewport/handlers/`
- Minimap math hardening:
  - `visualizer/src/ui/viewport/rc_minimap_interaction_math.h`
  - `visualizer/src/ui/viewport/rc_minimap_interaction_math.cpp`
  - `visualizer/src/ui/panels/rc_panel_axiom_editor.cpp`
- Systems map query/toggles:
  - `visualizer/src/ui/panels/rc_panel_system_map.cpp`
  - `visualizer/src/ui/panels/rc_system_map_query.cpp`
- Tests:
  - `tests/test_viewport_interaction_helpers.cpp`
  - `tests/test_viewport_interaction_selection.cpp`
  - `tests/test_editor_plan.cpp`
  - `tests/test_systems_map_query.cpp`
  - `tests/test_minimap_interaction_math.cpp`
- Plan/documentation:
  - `docs/41_plans_todos/Tools_Masterpass_Plan.md`

## Current Architecture Map (Repo Truth)

### Runtime entrypoints and control flow
- Tool activation and HFSM routing are handled through:
  - `visualizer/src/ui/tools/rc_tool_contract.cpp`
  - `visualizer/src/ui/tools/rc_tool_dispatcher.cpp`
- Viewport interaction ingress is split by mode:
  - `ProcessAxiomViewportInteraction(...)` in `visualizer/src/ui/viewport/rc_viewport_interaction.cpp`
  - `ProcessNonAxiomViewportInteraction(...)` in `visualizer/src/ui/viewport/rc_viewport_interaction.cpp` (delegates to pipeline in handlers)
- Scene update loop and preview synchronization:
  - `visualizer/src/ui/viewport/rc_viewport_scene_controller.cpp`

### Non-axiom interaction seam (authoritative)
- The non-axiom interaction pipeline is implemented as an internal module:
  - `visualizer/src/ui/viewport/handlers/rc_viewport_non_axiom_pipeline.cpp`
  - Public ingress signature remains unchanged in `visualizer/src/ui/viewport/rc_viewport_interaction.h`
- Existing behavior includes:
  - selection + multi-select + hover
  - gizmo translate/rotate/scale
  - road/district/water vertex editing flows
  - lot/building/water domain placement mutations
  - explicit-generation signaling (`tool_runtime.explicit_generation_pending` path)

### Map/minimap and indexing reality
- Minimap click-to-jump, drag-pan, zoom, and selection highlights exist in:
  - `visualizer/src/ui/panels/rc_panel_axiom_editor.cpp`
- Viewport index rebuild path exists and is used by output application:
  - `app/src/Editor/ViewportIndexBuilder.cpp`
  - `app/src/Integration/CityOutputApplier.cpp`
- Dirty layers and generation policy semantics are defined in:
  - `core/include/RogueCity/Core/Editor/GlobalState.hpp`

### Systems map baseline
- Systems map panel currently lives at:
  - `visualizer/src/ui/panels/rc_panel_system_map.cpp`
- Runtime state is additive and stored in:
  - `core/include/RogueCity/Core/Editor/GlobalState.hpp` (`SystemsMapRuntimeState systems_map`)

---

## Implementation Patterns to Reuse

1. Dispatcher-first activation
- Keep `DispatchToolAction(...)` as the only library click ingress.
- Preserve: `action -> HFSM event -> tool_runtime -> viewport behavior`.

2. Single non-axiom ingress
- Preserve `ProcessNonAxiomViewportInteraction(...)` signature.
- Refactor internals only via `visualizer/src/ui/viewport/handlers/`.

3. Shared interaction metrics/policy
- Reuse:
  - `BuildToolInteractionMetrics(...)` from `visualizer/src/ui/tools/rc_tool_interaction_metrics.cpp`
  - `ResolveToolGeometryPolicy(...)` from `visualizer/src/ui/tools/rc_tool_geometry_policy.cpp`

4. Selection and synchronization
- Reuse `SelectionManager` and `SelectionSync`:
  - `core/include/RogueCity/Core/Editor/SelectionManager.hpp`
  - `RogueCity::Core::Editor::SyncPrimarySelectionFromManager(...)`

5. Manipulation primitives
- Reuse app-layer operations:
  - `ApplyTranslate`, `ApplyRotate`, `ApplyScale`
  - `MoveRoadVertex`, `InsertDistrictVertex`, `RemoveDistrictVertex`
  - source: `app/src/Editor/EditorManipulation.cpp`

6. Dirty layer and explicit-generation semantics
- Preserve existing dirty propagation and policy behavior:
  - `DirtyLayerState`, `GenerationPolicyState`, `ToolRuntimeState`
  - source: `core/include/RogueCity/Core/Editor/GlobalState.hpp`

7. Contract/compliance gates
- Keep wiring checks valid:
  - `tools/check_tool_wiring_contract.py`
  - `tools/check_ui_compliance.py`

---

## Execution Roadmap (Pass 1–4)

### Pass 1 — Foundation + No-Behavior-Change Refactor

#### Goals
- Move non-axiom interaction internals into handler modules without changing public API or runtime semantics.

#### Implementation
1. Introduce handler module path:
- `visualizer/src/ui/viewport/handlers/`
- Add pipeline implementation file:
  - `rc_viewport_non_axiom_pipeline.cpp`
  - `rc_viewport_non_axiom_pipeline.h`

2. Keep signature stable
- `ProcessNonAxiomViewportInteraction(...)` in `rc_viewport_interaction.cpp` delegates to pipeline.
- No changes to `rc_viewport_interaction.h` external function signatures.

3. Preserve phase ordering
- Input gate
- key toggles
- domain placement
- gizmo
- domain vertex edits
- box/lasso
- hover/select
- status/outcome resolution

4. CMake wiring
- Add handler source to both visualizer targets in:
  - `visualizer/CMakeLists.txt`

#### Acceptance
- Builds clean for headless + GUI targets.
- Interaction behavior/status strings remain compatible.

---

### Pass 2 — Road + District Vertical Slice

#### Goals
- Make road/district behavior modular and improve viewport road hierarchy readability.

#### Implementation
1. Keep road + district domain logic in pipeline modules under `handlers/`.
2. Keep shared mutation helpers centralized in pipeline internals (selection anchor, dirty propagation, region query, pick logic).
3. Add hierarchy-aware road render style mapping in:
- `visualizer/src/ui/panels/rc_panel_axiom_editor.cpp`

#### Acceptance
- Road pen/select/anchor/split/simplify/snap-align behavior preserved.
- District paint/zone/split/merge/vertex edit behavior preserved.
- Roads render with distinguishable hierarchy style.

---

### Pass 3 — Lot + Building + Water Vertical Slice

#### Goals
- Preserve lot/building/water interaction behavior with normalized metrics/policy usage.

#### Implementation
1. Keep lot/building/water mutation paths in the handler pipeline.
2. Preserve generation policy behavior and explicit-generation pending signaling.
3. Avoid introducing command-stack migration for these domains in this pass.

#### Acceptance
- Lot plot/slice/align/merge stable.
- Building place/assign stable.
- Water spline edits/falloff stable.

---

### Pass 4 — Systems Map + Minimap Hardening + Panel Appendix Items

#### Goals
- Add structured systems-map toggles and query plumbing with additive runtime state.

#### Implementation
1. Add runtime state field:
- `SystemsMapRuntimeState systems_map` in `GlobalState`.

2. Update systems map panel:
- `visualizer/src/ui/panels/rc_panel_system_map.cpp`
- Add:
  - layer toggles (roads/districts/lots/buildings/water/labels)
  - hover query metadata update to `gs.systems_map.*`
  - optional click-select integration with `SelectionManager` + `SelectionSync`

3. Minimap hardening tasks (non-breaking)
- Keep existing behavior in `rc_panel_axiom_editor.cpp`.
- Add deterministic coverage around coordinate/pick helper behavior where feasible.

4. Panel appendix tasks (documented/deferred)
- Path from local `PropertyHistory()` in `rc_property_editor.cpp` to shared history.
- Optional split strategy for `visualizer/src/ui/panels/RcPanelDrawers.cpp`.

#### Acceptance
- Systems map supports explicit toggles and query payload updates.
- Minimap interactions remain stable.
- Panel appendix remains scoped and non-blocking for viewport pipeline.

---

## Verification Matrix

### Clean Sweep Commands (Executed 2026-02-19)
- Configure:
  - `cmake -S . -B build -DROGUECITY_BUILD_VISUALIZER=ON -DBUILD_TESTING=ON`
- Clean build:
  - `cmake --build build --config Debug --clean-first --target ALL_BUILD`
- Contract gates:
  - `py -3 tools/check_tool_wiring_contract.py`
  - `py -3 tools/check_ui_compliance.py`
  - `py -3 tools/check_clang_builder_contract.py`
- Full test sweep:
  - `ctest --test-dir build -C Debug --output-on-failure`

### Automated

1. Add helper determinism test
- `tests/test_viewport_interaction_helpers.cpp`
- Validate geometry/selection helper behavior (segment distance, polygon inclusion, anchor/selection math).

2. Add selection query test
- `tests/test_viewport_interaction_selection.cpp`
- Validate pick priority, region query behavior, hidden-layer handling, selection sync.

3. Extend plan coverage test
- Extend `tests/test_editor_plan.cpp` with viewport-pipeline invariants.

4. Add systems-map query determinism test
- `tests/test_systems_map_query.cpp`
- Validate bounds derivation + toggle-driven entity query behavior.

5. Add minimap interaction math determinism test
- `tests/test_minimap_interaction_math.cpp`
- Validate zoom clamp, world-per-pixel math, pixel-to-world conversion, world-constraint clamping.

6. Register tests in root CMake
- `CMakeLists.txt`: `add_executable(...)` and `add_test(...)` entries.

7. Baseline tests to keep running
- `test_editor_hfsm`
- `test_viewport`
- `test_viewport_id_stability`
- `test_editor_plan`
- `test_simulation_pipeline`

### Manual

1. Input gate blocked click -> blocked status, no mutation.
2. Road subtools -> pen/add-remove/join-split/simplify/snap-align.
3. District subtools -> paint/zone/split/merge/insert/delete/drag.
4. Lot subtools -> plot/slice/align/merge with dirty cascades.
5. Building subtools -> place/assign + gizmo operations.
6. Water subtools -> pen/anchor/falloff/snap-align.
7. Box/lasso multi-select with Shift/Ctrl modifiers.
8. Minimap drag pan/click jump/zoom/selection highlights.
9. Systems map toggles + hover query payload.
10. Explicit-generation pending behavior by domain policy.

---

## Panel Appendix (Secondary Track)

### A. Property editor undo history convergence
- Current local history lives in `visualizer/src/ui/panels/rc_property_editor.cpp` (`PropertyHistory()`).
- Migration target: shared/global command history path via context/global state.
- This remains explicitly deferred from viewport pipeline critical path.

### B. Panel drawer file split (optional)
- Current registry wrappers are in `visualizer/src/ui/panels/RcPanelDrawers.cpp`.
- Optional future split by category/domain for maintainability.
- Non-blocking for viewport interaction rollout.

---

## Deferred End-State (Y2K Map Polish + 40xx 3D Path)

### Y2K map polish (deferred)
- Continue additive visual tokenization and layering improvements.
- Keep deterministic behavior and tool semantics unchanged.

### 40xx 3D adapter direction (deferred)
- Introduce projection-agnostic pick/transform adapters.
- Preserve existing 2D contracts until 3D adapter layer is proven.

---

## Assumptions and Defaults

1. ASAP is executed as four shippable passes, with Pass 1 first and independent mergeability.
2. Contract compatibility is additive-only: no action ID or dispatcher semantic breaks.
3. Non-axiom command-stack migration is deferred until pipeline modularization is stable.
4. Plan is viewport-primary; panel architecture remains a scoped appendix.
5. Existing build/test topology remains unchanged unless explicitly listed above.
