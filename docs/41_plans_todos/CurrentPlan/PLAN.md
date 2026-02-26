## Viewport O(visible) Rendering Refactor (GlobalState + Applier + Visualizer)

### Summary
Implement a deterministic, precomputed **uniform-grid CSR spatial index** in `GlobalState`, rebuild it through existing state pipelines, and make both viewport render paths consume only visible cells plus LOD-filtered entities.  
Scope includes both overlay rendering and base road rendering, with active-domain visibility override during editing.

### Status Snapshot (2026-02-25)
Completed in code:
1. Public API and type changes in `GlobalState`, `DirtyLayerState`, `fva::Container`, and `ViewportIndexBuilder` entrypoint behavior.
2. Spatial CSR build + `*_handle_by_id` map population in `ViewportIndexBuilder::Build`.
3. Rebuild lifecycle wiring in applier and viewport panel (`EnsureViewportDerivedIndices` + dirty-layer clean).
4. LOD policy and active-domain override behavior in both panel and overlays.
5. Visible-cell traversal for major overlay loops plus base road loop in axiom editor panel.
6. Overlay scratch buffers for polygon/line/dedupe paths to reduce per-frame allocations.

Not done yet:
1. Full build/test verification is blocked by pre-existing compilation failures outside this refactor scope (`core/src/Core/Data/CityTypes.cpp`, `core/src/Core/Data/TensorTypes.cpp`, and existing `app/include/RogueCity/App/Tools/*.hpp` include/typing breakages).
2. Manual acceptance run/telemetry validation has not been executed yet.
3. Performance acceptance target (`< 2ms` fully zoomed out on baseline machine) is still pending measurement.

### Pass 2 Start Criteria and Scope
Start condition:
1. Full build succeeds and app smoke-run succeeds.

Pass 2 implementation scope:
1. Centralize major geometry rendering loops into the viewport overlay pipeline so road + overlay geometry are debugged in one render locality.
2. Move base road rendering traversal from `rc_panel_axiom_editor.cpp` into `rc_viewport_overlays.cpp` (single path for visible-cell culling + LOD filtering).
3. Keep one shared LOD/road-detail policy source used by both base road draw and overlay labels to avoid drift.
4. Preserve current selection correctness while keeping selection optimization out of scope.

### Pass 2 Progress (2026-02-25)
Completed:
1. Build + smoke run gate passed with `ROGUECITY_BUILD_VISUALIZER=ON` and `RogueCityVisualizerHeadless.exe`.
2. Base road geometry rendering moved into `rc_viewport_overlays.cpp` via `RenderRoadNetwork`.
3. `rc_panel_axiom_editor.cpp` no longer owns base road draw traversal.
4. Duplicate viewport-road LOD/style code removed from panel; base roads + road labels now share overlay-local LOD/detail policy.

Still pending in Pass 2:
1. Optional further unification of minimap LOD and viewport LOD policy into a single shared module (currently intentionally separate).
2. Validation/perf telemetry pass to confirm no regressions in frame time after centralization.

### Pass 3 Needs and Research (2026-02-25)
Objective:
1. Convert the current implementation from "structurally optimized" to "measured and hardened" by proving O(visible) behavior with repeatable evidence.

Needs (implementation):
1. Add and wire pending tests:
- `tests/test_viewport_spatial_grid.cpp`
- `tests/test_viewport_lod_policy.cpp`
- `CMakeLists.txt` registration (`add_executable` + `add_test`).
2. Add viewport telemetry counters for render cost attribution:
- visible cell count
- per-layer deduped handle count
- fallback full-scan usage count
- frame-time correlation labels in telemetry panel.
3. Remove remaining per-frame transient allocations in viewport panel overlays/highlights where still using local vectors.
4. Decide whether to keep minimap LOD policy separate or formally unify with viewport LOD policy through one shared policy helper.
5. Add selection/picking readiness hooks for future optimization without changing current correctness-first behavior.

Research tasks:
1. Grid sensitivity study:
- Compare occupancy targets (`10`, `20`, `30` entries/cell) on metropolis-scale scenes.
- Measure memory overhead and visible-query cost.
2. Pathological geometry study:
- Evaluate long-roads-crossing-many-cells behavior.
- Determine whether a hybrid strategy (uniform grid + segment bins) is needed.
3. Data-layout forward compatibility study:
- Validate that CSR layer buffers and handle maps remain contiguous/stable for future GPU upload.
- Document SoA/packed-buffer requirements for future terrain/3D pipeline integration.
4. Selection correctness regression study:
- Verify no regressions in inspect/select workflows while spatial index is active.

Pass 3 verification targets:
1. Stable frame time at fixed viewport resolution while total city size increases.
2. Zoomed-out frame time target under ~2 ms on baseline machine profile.
3. New spatial and LOD tests passing in CI/local test run.
4. Telemetry evidence captured and recorded in changelog/release notes.

### Pass 3 Progress (2026-02-25)
Completed in code:
1. Added pass-3 tests and build wiring:
- `tests/test_viewport_spatial_grid.cpp`
- `tests/test_viewport_lod_policy.cpp`
- `CMakeLists.txt` `add_executable` + `add_test` entries.
2. Added viewport render telemetry state in `GlobalState` and wired per-frame instrumentation:
- visible cell count
- per-layer deduped-handle counts
- fallback full-scan usage count.
3. Exposed frame-time correlation labels and render-attribution counters in telemetry panel (`rc_panel_telemetry.cpp`).
4. Removed remaining per-frame transient `std::vector<ImVec2>` allocations in viewport highlights/lasso paths in `rc_panel_axiom_editor.cpp` by reusing persistent scratch storage.
5. Added selection optimization readiness hooks in `rc_viewport_handler_common.cpp` (`TryPickFromSpatialGrid`, `TryQueryRegionFromSpatialGrid`) that preserve current correctness-first behavior by falling back to legacy viewport index scans.
6. Added shared viewport LOD policy helper (`rc_viewport_lod_policy.h/.cpp`) and switched overlays to use it, while keeping minimap LOD policy separate in this pass.

Pending / blocked:
1. Full compile + test execution remains blocked by unrelated existing compile errors in core/app files (outside pass-3 changes).
2. Performance acceptance measurements and manual telemetry capture are pending once baseline build health is restored.

### Public API and Type Changes
1. Add render spatial index types to [GlobalState.hpp](/mnt/d/Projects/RogueCities/RogueCities_UrbanSpatialDesigner/core/include/RogueCity/Core/Editor/GlobalState.hpp).
`RenderSpatialGrid`, `RenderSpatialLayer`, world bounds, grid dimensions, cell size, validity/version fields.
2. Add O(1) ID lookup tables in [GlobalState.hpp](/mnt/d/Projects/RogueCities/RogueCities_UrbanSpatialDesigner/core/include/RogueCity/Core/Editor/GlobalState.hpp).
`road_handle_by_id`, `district_handle_by_id`, `lot_handle_by_id`, `water_handle_by_id`, `building_handle_by_id`.
3. Add single-layer clean helper to dirty state in [GlobalState.hpp](/mnt/d/Projects/RogueCities/RogueCities_UrbanSpatialDesigner/core/include/RogueCity/Core/Editor/GlobalState.hpp).
`DirtyLayerState::MarkClean(DirtyLayer layer)`.
4. Extend `fva::Container` const/random access support in [fast_array.hpp](/mnt/d/Projects/RogueCities/RogueCities_UrbanSpatialDesigner/core/include/RogueCity/Core/Util/fast_array.hpp).
Add `const operator[]`, `isValidIndex`, and `indexCount` so render code can safely dereference stored handles from `const GlobalState`.
5. Extend viewport builder API in [ViewportIndexBuilder.hpp](/mnt/d/Projects/RogueCities/RogueCities_UrbanSpatialDesigner/app/include/RogueCity/App/Editor/ViewportIndexBuilder.hpp).
Keep `Build(gs)` as entrypoint, but make it build both legacy `viewport_index` and new spatial render index + ID maps.

### Implementation Plan
1. Build spatial index and ID maps in [ViewportIndexBuilder.cpp](/mnt/d/Projects/RogueCities/RogueCities_UrbanSpatialDesigner/app/src/Editor/ViewportIndexBuilder.cpp).
Use current entity data to:
- Compute world bounds priority: `city_boundary` AABB, else `texture_space_bounds`, else entity-derived bounds.
- Compute adaptive grid size with fixed defaults: target occupancy `20 entries/cell`, clamped axis range `[64, 384]`.
- Build CSR layers (`offsets`, `handles`) per kind.
- Insertion policy:
  - Roads: segment-based cell coverage with per-road dedupe stamp during build.
  - District/Lot/Water: AABB cell coverage.
  - Buildings: point cell.
- Populate all `*_handle_by_id` maps in same pass.
2. Ensure rebuild lifecycle is always correct.
- [CityOutputApplier.cpp](/mnt/d/Projects/RogueCities/RogueCities_UrbanSpatialDesigner/app/src/Integration/CityOutputApplier.cpp): keep existing rebuild call path but now `Build(gs)` also creates spatial index.
- [rc_panel_axiom_editor.cpp](/mnt/d/Projects/RogueCities/RogueCities_UrbanSpatialDesigner/visualizer/src/ui/panels/rc_panel_axiom_editor.cpp): add `EnsureViewportDerivedIndices(gs)` before viewport interaction/render. Rebuild when:
  - `DirtyLayer::ViewportIndex` is dirty, or
  - spatial index invalid while entities exist, or
  - `viewport_index` missing while entities exist.
  Then mark `DirtyLayer::ViewportIndex` clean.
3. Add screen-space LOD policy and active-domain override.
- In [rc_viewport_overlays.cpp](/mnt/d/Projects/RogueCities/RogueCities_UrbanSpatialDesigner/visualizer/src/ui/viewport/rc_viewport_overlays.cpp) and [rc_panel_axiom_editor.cpp](/mnt/d/Projects/RogueCities/RogueCities_UrbanSpatialDesigner/visualizer/src/ui/panels/rc_panel_axiom_editor.cpp):
  - `ViewportLOD::Coarse|Medium|Fine` based on px-per-meter (`zoom`) thresholds:
    - `Coarse < 0.20`
    - `Medium < 0.80`
    - `Fine >= 0.80`
  - Road detail filtering:
    - Coarse: arterial only
    - Medium: arterial + collector
    - Fine: all
  - LOD visibility:
    - Coarse: hide lots/buildings
    - Medium: hide buildings
    - Fine: all layers per config
  - Active-domain override: always show currently edited domain even when LOD would hide it.
4. Refactor base road render loop to visible-cell iteration.
- Replace full road scan in [rc_panel_axiom_editor.cpp](/mnt/d/Projects/RogueCities/RogueCities_UrbanSpatialDesigner/visualizer/src/ui/panels/rc_panel_axiom_editor.cpp) with:
  - viewport world AABB from `PrimaryViewport::screen_to_world` corners
  - visible cell range query
  - deduped road-handle iteration
  - LOD road-class gate
5. Refactor overlay loops to visible-cell iteration.
- In [rc_viewport_overlays.cpp](/mnt/d/Projects/RogueCities/RogueCities_UrbanSpatialDesigner/visualizer/src/ui/viewport/rc_viewport_overlays.cpp), update:
`RenderZoneColors`, `RenderAESPHeatmap`, `RenderRoadLabels`, `RenderWaterBodies`, `RenderBuildingSites`, `RenderLotBoundaries`, and selection-anchor lookups to use ID maps or visible-cell traversal instead of full container scans.
6. Remove per-frame allocations for static geometry draw paths.
- In [rc_viewport_overlays.h](/mnt/d/Projects/RogueCities/RogueCities_UrbanSpatialDesigner/visualizer/src/ui/viewport/rc_viewport_overlays.h) and [rc_viewport_overlays.cpp](/mnt/d/Projects/RogueCities/RogueCities_UrbanSpatialDesigner/visualizer/src/ui/viewport/rc_viewport_overlays.cpp):
  - Add reusable scratch buffers (`screen_points`, triangle indices, dedupe stamp arrays, epoch counters).
  - Replace local per-call vectors in static-geometry paths (`DrawPolygon`, outline polyline conversion, visible-entity dedupe).

### Tests and Validation
1. Add deterministic spatial-index correctness test.
New file: [tests/test_viewport_spatial_grid.cpp](/mnt/d/Projects/RogueCities/RogueCities_UrbanSpatialDesigner/tests/test_viewport_spatial_grid.cpp).  
Validate:
- expected entities returned for known camera/world windows,
- no misses on edge-crossing roads/polygons,
- rebuild after edits updates cell membership.
2. Add LOD policy behavior test.
New file: [tests/test_viewport_lod_policy.cpp](/mnt/d/Projects/RogueCities/RogueCities_UrbanSpatialDesigner/tests/test_viewport_lod_policy.cpp).  
Validate threshold transitions and active-domain override behavior.
3. Extend build config in [CMakeLists.txt](/mnt/d/Projects/RogueCities/RogueCities_UrbanSpatialDesigner/CMakeLists.txt) with both tests and `add_test` entries.
4. Manual acceptance run using telemetry panel.
- Generate metro-scale city.
- At fixed viewport pixel size, compare frame ms while scaling total city area/entity count.
- Confirm near-stable frame time at same zoom level and similar local density.
- Confirm coarse/medium/fine visual behavior and editing-domain visibility override.
- Target: zoomed-out frame time under ~2 ms on the project’s baseline machine profile.

### Non-goals in this phase
1. Selection/picking algorithmic complexity (`PickFromViewportIndex`) remains outside this contract unless needed by regression fixes. - however we need to ensure that veiwport selection is still functional and correct with the new spatial index, even if it is not optimized to O(visible) yet. comment and stub out any necessary API changes to support future selection optimization.

2. GPU batching/instancing redesign is out of scope; this phase is CPU-side culling/LOD/data-layout only. while we have no current need for gpu acceleration yet as renering remains in a simple test and node based phase we need to consider the future for when we can finally implament our terrain features which will be part of the 3D rendering pipeline and will require gpu acceleration. we should ensure that the new spatial index and data layout are designed with future gpu-friendly access patterns in mind, even if we do not implement gpu rendering yet. this includes considerations for memory layout, cache locality, and potential compute shader access patterns.

### Assumptions and Defaults
1. Canonical doc path is [the-rogue-city-designer-soft.md](/mnt/d/Projects/RogueCities/RogueCities_UrbanSpatialDesigner/docs/20_specs/design-and-research/the-rogue-city-designer-soft.md); it does not define concrete Quadtree-vs-Grid or numeric LOD thresholds, so defaults above are adopted.
2. “O(1) relative to city size N” is interpreted as cost driven by visible cells + visible primitives under roughly constant spatial density, not pathological all-geometry-overlapping-one-cell cases.
3. No new manager classes will be introduced; functionality is added through `GlobalState`, `ViewportIndexBuilder`, and existing viewport render files.
4. assumptions can and contract reasoning can be derived from docs\41_plans_todos\CurrentPlan\RefactorContract.md and the referenced principles in docs\41_plans_todos\CurrentPlan\PlanDocs\Archetecture_Hardening_and_optimization_planning.md, which are not repeated here for brevity but should be consulted for deeper understanding of the design rationale.
