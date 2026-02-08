# Temp-to-New Migration Manifest

- Date: 2026-02-08
- Scope: Phase 0/1/2 (manifest, data contracts, generator transplant)
- Rule: `_Temp` remains unlinked donor source until parity gates pass.

## Baseline (pre-change)

- Configure: `cmake -B build -S .` -> PASS
- Build: `--target test_generators` -> PASS
- Build: `--target test_editor_hfsm` -> PASS
- Run: `./bin/test_generators.exe` -> PASS
- Run: `./bin/test_editor_hfsm.exe` -> PASS
- Build: `--target RogueCityApp` -> PASS
- Build: `--target RogueCityVisualizerGui` -> FAIL (existing `rc_panel_axiom_editor.cpp` ToolDeck symbols not found; pre-existing issue)

## Mapping

| Source |_Class| Target | Action |
|---|---|---|---|
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\AxiomInput.h` | `extract-behavior` | `core/include/RogueCity/Core/Editor/GlobalState.hpp; generators/include/RogueCity/Generators/Pipeline/CityGenerator.hpp` | Map to EditorAxiom + AxiomInput |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\BlockGenerator.cpp` | `adapt` | `generators/include/RogueCity/Generators/Urban/BlockGenerator.hpp; generators/src/Generators/Urban/BlockGenerator.cpp` | GEOS-off default, optional GEOS path |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\BlockGenerator.h` | `adapt` | `generators/include/RogueCity/Generators/Urban/BlockGenerator.hpp; generators/src/Generators/Urban/BlockGenerator.cpp` | GEOS-off default, optional GEOS path |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\BlockGeneratorGEOS.cpp` | `adapt` | `generators/include/RogueCity/Generators/Urban/BlockGenerator.hpp; generators/src/Generators/Urban/BlockGenerator.cpp` | GEOS-off default, optional GEOS path |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\BlockGeneratorGEOS.h` | `adapt` | `generators/include/RogueCity/Generators/Urban/BlockGenerator.hpp; generators/src/Generators/Urban/BlockGenerator.cpp` | GEOS-off default, optional GEOS path |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\CityModel.cpp` | `extract-behavior` | `core/include/RogueCity/Core/Data/CityTypes.hpp` | Normalize into canonical core data contracts |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\CityModel.h` | `extract-behavior` | `core/include/RogueCity/Core/Data/CityTypes.hpp` | Normalize into canonical core data contracts |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\CityParams.cpp` | `extract-behavior` | `core/include/RogueCity/Core/Data/CityTypes.hpp; core/include/RogueCity/Core/Editor/GlobalState.hpp` | Normalize generator/runtime params |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\CityParams.h` | `extract-behavior` | `core/include/RogueCity/Core/Data/CityTypes.hpp; core/include/RogueCity/Core/Editor/GlobalState.hpp` | Normalize generator/runtime params |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\DistrictGenerator.cpp` | `adapt` | `generators/include/RogueCity/Generators/Urban/DistrictGenerator.hpp; generators/src/Generators/Urban/DistrictGenerator.cpp` | Port district assignment and boundaries |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\DistrictGenerator.h` | `adapt` | `generators/include/RogueCity/Generators/Urban/DistrictGenerator.hpp; generators/src/Generators/Urban/DistrictGenerator.cpp` | Port district assignment and boundaries |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\ExportSchema.cpp` | `extract-behavior` | `tools/export/* (future)` | Deferred; not part of Phase 1/2 critical path |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\ExportSchema.h` | `extract-behavior` | `tools/export/* (future)` | Deferred; not part of Phase 1/2 critical path |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\FrontageProfiles.cpp` | `drop-in` | `generators/include/RogueCity/Generators/Urban/FrontageProfiles.hpp; generators/src/Generators/Urban/FrontageProfiles.cpp` | Direct semantic port |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\FrontageProfiles.h` | `drop-in` | `generators/include/RogueCity/Generators/Urban/FrontageProfiles.hpp; generators/src/Generators/Urban/FrontageProfiles.cpp` | Direct semantic port |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\Graph.cpp` | `adapt` | `generators/include/RogueCity/Generators/Urban/Graph.hpp; generators/src/Generators/Urban/Graph.cpp` | Port connectivity helper |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\Graph.h` | `adapt` | `generators/include/RogueCity/Generators/Urban/Graph.hpp; generators/src/Generators/Urban/Graph.cpp` | Port connectivity helper |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\GridStorage.cpp` | `adapt` | `generators/include/RogueCity/Generators/Urban/GridStorage.hpp; generators/src/Generators/Urban/GridStorage.cpp` | Port spatial index helper |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\GridStorage.h` | `adapt` | `generators/include/RogueCity/Generators/Urban/GridStorage.hpp; generators/src/Generators/Urban/GridStorage.cpp` | Port spatial index helper |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\Integrator.cpp` | `adapt` | `generators/include/RogueCity/Generators/Urban/Integrator.hpp; generators/src/Generators/Urban/Integrator.cpp` | Port RK4 helper surface |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\Integrator.h` | `adapt` | `generators/include/RogueCity/Generators/Urban/Integrator.hpp; generators/src/Generators/Urban/Integrator.cpp` | Port RK4 helper surface |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\LotGenerator.cpp` | `adapt` | `generators/include/RogueCity/Generators/Urban/LotGenerator.hpp; generators/src/Generators/Urban/LotGenerator.cpp` | Port lot emission/classification |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\LotGenerator.h` | `adapt` | `generators/include/RogueCity/Generators/Urban/LotGenerator.hpp; generators/src/Generators/Urban/LotGenerator.cpp` | Port lot emission/classification |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\PolygonFinder.cpp` | `adapt` | `generators/include/RogueCity/Generators/Urban/PolygonFinder.hpp; generators/src/Generators/Urban/PolygonFinder.cpp` | Port cycle/block finder helper |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\PolygonFinder.h` | `adapt` | `generators/include/RogueCity/Generators/Urban/PolygonFinder.hpp; generators/src/Generators/Urban/PolygonFinder.cpp` | Port cycle/block finder helper |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\PolygonUtil.cpp` | `adapt` | `generators/include/RogueCity/Generators/Urban/PolygonUtil.hpp; generators/src/Generators/Urban/PolygonUtil.cpp` | Port geometry utility |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\PolygonUtil.h` | `adapt` | `generators/include/RogueCity/Generators/Urban/PolygonUtil.hpp; generators/src/Generators/Urban/PolygonUtil.cpp` | Port geometry utility |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\RoadGenerator.cpp` | `adapt` | `generators/include/RogueCity/Generators/Urban/RoadGenerator.hpp; generators/src/Generators/Urban/RoadGenerator.cpp` | Wrap/bridge with existing tracer |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\RoadGenerator.h` | `adapt` | `generators/include/RogueCity/Generators/Urban/RoadGenerator.hpp; generators/src/Generators/Urban/RoadGenerator.cpp` | Wrap/bridge with existing tracer |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\RoadGraphMetrics.cpp` | `adapt` | `generators/include/RogueCity/Generators/Urban/RoadGraphMetrics.hpp; generators/src/Generators/Urban/RoadGraphMetrics.cpp` | Deferred optional metric module |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\RoadGraphMetrics.h` | `adapt` | `generators/include/RogueCity/Generators/Urban/RoadGraphMetrics.hpp; generators/src/Generators/Urban/RoadGraphMetrics.cpp` | Deferred optional metric module |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\SiteGenerator.cpp` | `adapt` | `generators/include/RogueCity/Generators/Urban/SiteGenerator.hpp; generators/src/Generators/Urban/SiteGenerator.cpp` | Port building placement |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\SiteGenerator.h` | `adapt` | `generators/include/RogueCity/Generators/Urban/SiteGenerator.hpp; generators/src/Generators/Urban/SiteGenerator.cpp` | Port building placement |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\Streamlines.cpp` | `adapt` | `generators/include/RogueCity/Generators/Roads/StreamlineTracer.hpp; generators/src/Generators/Roads/StreamlineTracer.cpp` | Already partially ported; backfill missing behavior |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\Streamlines.h` | `adapt` | `generators/include/RogueCity/Generators/Roads/StreamlineTracer.hpp; generators/src/Generators/Roads/StreamlineTracer.cpp` | Already partially ported; backfill missing behavior |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\TensorField.cpp` | `adapt` | `generators/include/RogueCity/Generators/Tensors/*; generators/src/Generators/Tensors/*` | Already partially ported; backfill missing behavior |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\TensorField.h` | `adapt` | `generators/include/RogueCity/Generators/Tensors/*; generators/src/Generators/Tensors/*` | Already partially ported; backfill missing behavior |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\WaterGenerator.cpp` | `adapt` | `generators/include/RogueCity/Generators/Urban/WaterGenerator.hpp; generators/src/Generators/Urban/WaterGenerator.cpp` | Deferred optional module |
| `_Temp/_TempInclusionFolder_NO_LINK\Generator inclusions\WaterGenerator.h` | `adapt` | `generators/include/RogueCity/Generators/Urban/WaterGenerator.hpp; generators/src/Generators/Urban/WaterGenerator.cpp` | Deferred optional module |
| `_Temp/_TempInclusionFolder_NO_LINK\Readme.md` | `extract-behavior` | `docs/temp_Documentation/*` | Reference-only donor notes |
| `_Temp/_TempInclusionFolder_NO_LINK\TempApp\Application.cpp` | `extract-behavior` | `visualizer/src/ui/* + app/src/*` | Do not transplant shell; extract behavior only |
| `_Temp/_TempInclusionFolder_NO_LINK\TempApp\Application.hpp` | `extract-behavior` | `visualizer/src/ui/* + app/src/*` | Do not transplant shell; extract behavior only |
| `_Temp/_TempInclusionFolder_NO_LINK\TempApp\CityParams.hpp` | `extract-behavior` | `core/include/RogueCity/Core/Data/CityTypes.hpp + UI panel state` | Map data/settings, avoid parallel type systems |
| `_Temp/_TempInclusionFolder_NO_LINK\TempApp\DebugFlags.cpp` | `extract-behavior` | `visualizer/src/ui/*` | Only extract useful UI helper behavior |
| `_Temp/_TempInclusionFolder_NO_LINK\TempApp\DebugFlags.hpp` | `extract-behavior` | `visualizer/src/ui/*` | Only extract useful UI helper behavior |
| `_Temp/_TempInclusionFolder_NO_LINK\TempApp\DebugLog.hpp` | `extract-behavior` | `visualizer/src/ui/*` | Only extract useful UI helper behavior |
| `_Temp/_TempInclusionFolder_NO_LINK\TempApp\DockManager.cpp` | `extract-behavior` | `visualizer/src/ui/* + app/src/*` | Do not transplant shell; extract behavior only |
| `_Temp/_TempInclusionFolder_NO_LINK\TempApp\DockManager.hpp` | `extract-behavior` | `visualizer/src/ui/* + app/src/*` | Do not transplant shell; extract behavior only |
| `_Temp/_TempInclusionFolder_NO_LINK\TempApp\Icons.hpp` | `extract-behavior` | `visualizer/src/ui/*` | Only extract useful UI helper behavior |
| `_Temp/_TempInclusionFolder_NO_LINK\TempApp\ImGuiCompat.hpp` | `extract-behavior` | `visualizer/src/ui/*` | Only extract useful UI helper behavior |
| `_Temp/_TempInclusionFolder_NO_LINK\TempApp\main.cpp` | `extract-behavior` | `visualizer/src/ui/* + app/src/*` | Do not transplant shell; extract behavior only |
| `_Temp/_TempInclusionFolder_NO_LINK\TempApp\Presets.cpp` | `extract-behavior` | `core/include/RogueCity/Core/Data/CityTypes.hpp + UI panel state` | Map data/settings, avoid parallel type systems |
| `_Temp/_TempInclusionFolder_NO_LINK\TempApp\Presets.hpp` | `extract-behavior` | `core/include/RogueCity/Core/Data/CityTypes.hpp + UI panel state` | Map data/settings, avoid parallel type systems |
| `_Temp/_TempInclusionFolder_NO_LINK\TempApp\RoadTypes.hpp` | `extract-behavior` | `core/include/RogueCity/Core/Data/CityTypes.hpp + UI panel state` | Map data/settings, avoid parallel type systems |
| `_Temp/_TempInclusionFolder_NO_LINK\TempApp\UIManager.cpp` | `extract-behavior` | `visualizer/src/ui/* + app/src/*` | Do not transplant shell; extract behavior only |
| `_Temp/_TempInclusionFolder_NO_LINK\TempApp\UIManager.hpp` | `extract-behavior` | `visualizer/src/ui/* + app/src/*` | Do not transplant shell; extract behavior only |
| `_Temp/_TempInclusionFolder_NO_LINK\TempApp\UserPreferences.cpp` | `extract-behavior` | `core/include/RogueCity/Core/Data/CityTypes.hpp + UI panel state` | Map data/settings, avoid parallel type systems |
| `_Temp/_TempInclusionFolder_NO_LINK\TempApp\UserPreferences.hpp` | `extract-behavior` | `core/include/RogueCity/Core/Data/CityTypes.hpp + UI panel state` | Map data/settings, avoid parallel type systems |
| `_Temp/_TempInclusionFolder_NO_LINK\TempApp\ViewportWindow.cpp` | `extract-behavior` | `visualizer/src/ui/viewport/rc_viewport_overlays.* + app/src/Viewports/*` | Extract interaction and overlay behavior |
| `_Temp/_TempInclusionFolder_NO_LINK\TempApp\ViewportWindow.hpp` | `extract-behavior` | `visualizer/src/ui/viewport/rc_viewport_overlays.* + app/src/Viewports/*` | Extract interaction and overlay behavior |
| `_Temp/_TempInclusionFolder_NO_LINK\TempApp\Windows.cpp` | `extract-behavior` | `visualizer/src/ui/panels/rc_panel_*; visualizer/src/ui/patterns/rc_ui_data_index_panel.h; visualizer/src/ui/panels/rc_panel_data_index_traits.h` | Extract panel behaviors/actions |
| `_Temp/_TempInclusionFolder_NO_LINK\TempApp\Windows.hpp` | `extract-behavior` | `visualizer/src/ui/panels/rc_panel_*; visualizer/src/ui/patterns/rc_ui_data_index_panel.h; visualizer/src/ui/panels/rc_panel_data_index_traits.h` | Extract panel behaviors/actions |

## Phase 1/2 Commit Scope

- Phase 1: data contracts in `CityTypes.hpp` + `GlobalState.hpp`.
- Phase 2: replace generator placeholders in `CityGenerator.cpp`, `ZoningGenerator.cpp`, `ZoningBridge.cpp` and add Urban donor modules.
- GEOS path remains compile-guarded and OFF by default.

The existing build directory is pinned to generator Visual Studio 18 2026, so rerun with that exact CMake toolchain to avoid cache mismatch and get a valid verification result.

Phase 3: UI ties transplant (behavior extraction, not monolith copy)
Do not transplant old app shell:
Application.cpp, UIManager.*, DockManager.*, main.cpp
Extract behavior into existing panel system:
visualizer/src/ui/panels/rc_panel_*
rc_ui_data_index_panel.h
rc_panel_data_index_traits.h (wire TODO actions)
rc_viewport_overlays.cpp (lot boundaries, budget overlays)
Keep HFSM-reactive visibility and current docking flow in rc_ui_root.cpp.
Phase 4: CitySpec + AI pipeline completion
Wire CitySpec through full generation path (roads → districts → lots → buildings).
Ensure CitySpecPanel and ZoningControl trigger the same pipeline and update GlobalState deterministically.
Phase 5: Hardening + _Temp deletion gate
Deterministic tests for seed-based outputs.
Performance gates for RogueWorker thresholds.
Remove duplicate/legacy generator include trees after final switch.
move_Temp only after all gates pass. to dir _Del

## Phase 3/4 Execution Status (2026-02-08)

- Phase 3 implemented in current architecture (`visualizer + app`), no old shell transplant.
- Wired index panel context actions in `visualizer/src/ui/panels/rc_panel_data_index_traits.h`:
  - Inspect/focus/delete for roads/districts/lots/buildings.
  - Selection now routes to inspector handles and viewport highlights.
- Completed overlay extraction in `visualizer/src/ui/viewport/rc_viewport_overlays.cpp`:
  - Lot boundaries now render in AESP heatmap.
  - District budget bars now use real lot/district budget data.
- Overlay renderer is now actively called from the viewport panel (`rc_panel_axiom_editor.cpp`) with live camera transform.

- Phase 4 implemented with deterministic shared pipeline:
  - Added `ZoningBridge::GenerateFromCitySpec` in `app/include/RogueCity/App/Integration/ZoningBridge.hpp` and `app/src/Integration/ZoningBridge.cpp`.
  - Pipeline path is now `CitySpecAdapter -> CityGenerator -> ZoningGenerator` with shared `GlobalState` updates.
  - `CitySpecPanel` and `ZoningControl` now invoke the same bridge-backed path when CitySpec is active.
  - `GlobalState` stats and district budget/population metadata are updated after generation.
  - CitySpec overrides are now applied in zoning generation (`generators/src/Generators/Pipeline/ZoningGenerator.cpp`).

## Debug Agent Notes (Next Pass)

- Build warning remains in `RogueCityVisualizerGui` link stage:
  - `LNK4098`: defaultlib `MSVCRT` conflicts with other libs.
- Existing float narrowing warnings remain in several app files (`GeneratorBridge.cpp`, tool/viewports files).
- HFSM tool-mode switching UX is still fragmented (tool transitions mostly via UI Agent panel); consider adding explicit tool-switch controls in main cockpit panels.
