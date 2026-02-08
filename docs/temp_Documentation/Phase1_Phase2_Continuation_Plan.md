# Phase 1/2 Continuation Plan (Critical Path + Agent Tasks)

- Date: 2026-02-08
- Inputs:
  - `_Temp/Old-to-New.md`
  - `.github/Agents.md`
  - `.github/copilot-instructions.md`
- JSON validation:
  - `Master JSON Execution Plan` parsed successfully from `_Temp/Old-to-New.md`.
  - Anchors: `_Temp/Old-to-New.md:694`, `_Temp/Old-to-New.md:718`, `_Temp/Old-to-New.md:743`, `_Temp/Old-to-New.md:755`

## Current Status

### Phase 1 Critical Path (`1.3 -> 1.4 -> 1.6 -> 1.9`)

1. `1.3_spatial_math_integration` - DONE
   - Urban math/geometry helpers transplanted: `generators/include/RogueCity/Generators/Urban/Integrator.hpp`, `generators/src/Generators/Urban/Integrator.cpp`, `generators/include/RogueCity/Generators/Urban/PolygonUtil.hpp`, `generators/src/Generators/Urban/PolygonUtil.cpp`.
2. `1.4_data_model_implementation` - DONE
   - Data contract additions in `core/include/RogueCity/Core/Data/CityTypes.hpp:117`, `core/include/RogueCity/Core/Data/CityTypes.hpp:214`.
   - Global state additions in `core/include/RogueCity/Core/Editor/GlobalState.hpp:56`, `core/include/RogueCity/Core/Editor/GlobalState.hpp:67`.
3. `1.6_generator_integration` - DONE
   - Placeholder pipeline stages replaced in `generators/src/Generators/Pipeline/CityGenerator.cpp:30`, `generators/src/Generators/Pipeline/CityGenerator.cpp:136`, `generators/src/Generators/Pipeline/CityGenerator.cpp:170`.
   - Zoning lot/building pipeline replaced in `generators/src/Generators/Pipeline/ZoningGenerator.cpp:85`, `generators/src/Generators/Pipeline/ZoningGenerator.cpp:172`.
   - Bridge wiring completed in `app/src/Integration/ZoningBridge.cpp:81`, `app/src/Integration/ZoningBridge.cpp:103`.
4. `1.9_cmake_updates` - DONE
   - New Urban modules wired in `generators/CMakeLists.txt:17`, `generators/CMakeLists.txt:40`.

### Phase 2 Critical Path (`2.1 -> 2.2 -> 2.3 -> 2.7 -> 2.8`)

1. `2.1_ui_architecture_planning` - PENDING
2. `2.2_hfsm_extension` - PENDING
3. `2.3_panel_design` - PENDING
4. `2.7_application_merge` - PENDING (must be adapted; see conflict notes)
5. `2.8_integration_testing` - PARTIAL
   - Core/generators/app build and tests pass.
   - `RogueCityVisualizerGui` still fails due pre-existing `ToolDeck` symbol issues in `visualizer/src/ui/panels/rc_panel_axiom_editor.cpp:593`.

## Agent Task List (Ordered)

1. `Coder_Agent` - Harden deterministic output checks for Phase 1 gate
   - Add deterministic assertions for roads/districts/lots/buildings in `tests/test_generators.cpp`.
2. `Debug_Manager` - Expand gate coverage beyond build-only
   - Add seeded parity metrics and AESP distribution checks in `tests/*`.
3. `Resource_Manager` - Add performance gate enforcement
   - Verify RogueWorker activation policy for heavy paths (`N = axiom_count * district_count * lot_density`) and timings.
4. `The_Architect` - Resolve Phase 2.7 merge strategy conflict
   - Approve behavior extraction only (no old app-shell transplant).
5. `Coder_Agent` - Implement HFSM/UI continuation in existing shell
   - Wire panel behavior into `visualizer/src/ui/panels/rc_panel_*`, `visualizer/src/ui/viewport/rc_viewport_overlays.cpp`, `visualizer/src/ui/rc_ui_root.cpp`.
6. `Debug_Manager` - Stabilize visualizer build blocker
   - Fix `ToolDeck` API mismatch in `visualizer/src/ui/panels/rc_panel_axiom_editor.cpp`.

## Conflict Notes

1. `Old-to-New` Task 2.7 conflicts with current architecture direction.
   - Old plan suggests merging old shell files (`Application.cpp`, `ViewportWindow.cpp`, `Windows.cpp`) at `_Temp/Old-to-New.md:560`.
   - Current directive is extraction-only into current `core/generators/app/visualizer` layering (no shell transplant).
2. Validation gate definition is too weak for `_Temp` deletion.
   - JSON gate after `1.9` only checks compile/link/execute at `_Temp/Old-to-New.md:757`.
   - Must include deterministic parity + AESP checks + policy checks.
3. Rogue protocol enforcement gap remains.
   - `.github/Agents.md:41` and `.github/copilot-instructions.md:226` require RogueWorker policy for heavy workloads.
   - Current pipeline computes threshold decision but does not yet guarantee threaded execution for all heavy stages.
4. UI/Core boundary must remain strict.
   - `.github/copilot-instructions.md:79` requires core UI-free; keep all UI ties in `app/visualizer`.
   - Cockpit Doctrine and HFSM-reactive UI behavior remain required (`.github/Agents.md:170`, `.github/copilot-instructions.md:83`).

## Validation Snapshot

- No `_Temp` code-path references found in `app/`, `core/`, `generators/`, `visualizer/`, `tests/`.
- Build/Test status:
  - PASS: `RogueCityCore`, `test_generators`, `test_editor_hfsm`, `RogueCityApp`
  - PASS: `bin/test_generators.exe`, `bin/test_editor_hfsm.exe`
  - FAIL (pre-existing): `RogueCityVisualizerGui` (`ToolDeck` symbols)

