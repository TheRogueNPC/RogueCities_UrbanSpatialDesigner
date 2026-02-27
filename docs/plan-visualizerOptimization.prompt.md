Contract: Optimization Physics Implementation
---------------------------------------------
Objective: Achieve O(1) rendering cost relative to total city size (N), dependent only on viewport resolution (k).

Mathematical Foundation (from Architecture Hardening):
1. Principle of Least Action (PLA): $\delta S = 0$
   - "Action" (Energy * Time) corresponds to CPU Cycles * Frame Time.
   - We minimize action by only processing entities that contribute to the final pixel buffer.
   - Implementation: Spatial Culling (QuadTree/Grid) to reject invisible entities early.

2. Noether's Theorem (Scale Symmetry): $x \to \lambda x$
   - Invariance under scale transformation implies conservation of visual density.
   - As zoom ($\lambda$) decreases, geometric detail must decrease to maintain constant vertex density per screen pixel.
   - Implementation: Semantic LOD (Level of Detail) tiers based on camera zoom.

3. Data Inertia: $I \propto m$
   - Massive datasets (City State) have high inertia; moving them (CPU->GPU) or transforming them requires high energy.
   - Implementation: Pre-compute spatial indices during generation (static state) to avoid per-frame O(N) iteration.

Constraints:
- No new "Manager" classes; extend `GlobalState` and `CityOutputApplier`.
- Zero per-frame allocation for static geometry.
---------------------------------------------

Plan: Visualizer Optimization (Integrated Physics-Based Refactor)

TL;DR
Refactor the Visualizer to respect "Optimization Physics" by integrating spatial culling and LOD directly into the existing `GlobalState` and `ViewportOverlays` architecture.

Principles Applied
1. **Principle of Least Action (Spatial Culling)**: Do not iterate 50k entities to draw 50.
2. **Scale Symmetry (LOD)**: Do not draw windows when the city is a pixel.
3. **Data Inertia (Pre-computation)**: Build spatial structures once during generation, not every frame.

Steps

1. Extend GlobalState with Spatial Grid (Data Inertia)
- **File**: `core/include/RogueCity/Core/Editor/GlobalState.hpp`
- **Action**: Add a lightweight `SpatialGrid` struct to `GlobalState`.
    - `struct SpatialCell { std::vector<uint32_t> road_ids; std::vector<uint32_t> building_ids; ... }`
    - `std::vector<SpatialCell> spatial_grid;`
    - `int grid_resolution;`
- **Why**: `GlobalState` is the single source of truth. The index belongs with the data to ensure cache locality.

2. Populate Grid in Applier (Data Inertia)
- **File**: `app/src/Integration/CityOutputApplier.cpp`
- **Action**: In `ApplyCityOutputToGlobalState`, after populating vectors, iterate them once to populate `gs.spatial_grid`.
- **Optimization**: This happens only on generation/load, ensuring zero per-frame cost for index maintenance.

3. Implement Semantic LOD (Scale Symmetry)
- **File**: `visualizer/src/ui/viewport/rc_viewport_overlays.cpp`
- **Action**: Add `LODLevel GetLOD(float zoom)` helper.
    - `Zoom < 0.2`: Draw Districts (Poly) + Arterials (Lines). Hide Lots/Buildings.
    - `Zoom < 0.8`: Draw Roads (Mesh) + Lot Outlines. Hide Buildings.
    - `Zoom >= 0.8`: Draw Everything.

4. Refactor Render Loops (Least Action)
- **File**: `visualizer/src/ui/viewport/rc_viewport_overlays.cpp`
- **Action**: Replace naive `for (auto& entity : gs.entities)` loops.
    - Calculate visible grid cells from `view_transform_`.
    - Iterate only visible cells: `for (cell : visible_cells) { for (id : cell.ids) { Draw(gs.entities[id]); } }`
- **Benefit**: O(1) rendering cost relative to city size (cost depends only on screen pixels).

Verification
- Load "Metropolis" preset.
- Zoom out fully -> Check Frame Time (should be < 2ms).
- Zoom in -> Check Frame Time (should remain stable).

Research & Hardening (Pass 3)
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

Files Referenced
- `core/include/RogueCity/Core/Editor/GlobalState.hpp`
- `app/src/Integration/CityOutputApplier.cpp`
- `visualizer/src/ui/viewport/rc_viewport_overlays.cpp`

AI Reinforcement Prompt
- **Documentation Check**: Verify assumptions against `docs/TheRogueCityDesignerSoft.md` regarding spatial partitioning strategies (QuadTree vs Grid) and LOD thresholds.
- **Constraint Verification**: Ensure `GlobalState` modifications strictly adhere to the "No Manager Classes" rule and use POD types where possible for cache locality.
- **Data Availability**: Confirm `CityOutputApplier` has access to the city's bounding box to initialize the `SpatialGrid` dimensions correctly.
- **Viewport Math**: Verify `view_transform_` in `rc_viewport_overlays.cpp` provides the necessary `ScreenToWorld` methods to calculate the visible grid range.