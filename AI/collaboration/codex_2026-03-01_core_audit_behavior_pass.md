# Codex Core Audit Behavior Pass (2026-03-01)

## Scope
- Layer target: `core` (with test/build wiring in root CMake).
- Goal: convert identified core no-op/placeholder contracts into active behavior and harden geometry for upcoming 3D-critical use.

## Implemented
- `PolygonOps` hardened with:
  - vertex sanitization (non-finite filtering, duplicate removal, collinear cleanup),
  - validity checks (`IsValidPolygon`),
  - simplification (`SimplifyPolygon`),
  - expanded ops (`DifferencePolygons`, `UnionPolygons`),
  - safer `InsetPolygon`/`ClipPolygons` behavior on degenerate input.
- `Tensor2D::add(..., smooth=true)` now executes smoothing behavior instead of ignoring the flag, including stabilization for near-opposing blends.
- `SimulationPipeline` stages now perform runtime work:
  - physics finite-coordinate validation for roads/districts/lots/buildings,
  - agent-step pruning of invalid multi-select/hover entries,
  - systems-step validation overlay refresh via overlay validation collector,
  - explicit non-deterministic mode single-step behavior.
- `EditorIntegrity` now produces executable integrity/spatial diagnostics and applies tagged integrity violations into `GlobalState::plan_violations` with idempotent refresh.
- `StableIDRegistry` upgraded with alias-aware reverse lookup and strict deserialize parsing/validation.
- Core build fallback added for Clipper2 source linkage when `Clipper2` target is absent.

## Tests Added/Updated
- Added: `tests/test_polygon_ops.cpp`
- Added: `tests/test_tensor2d_smoothing.cpp`
- Added: `tests/test_editor_integrity.cpp`
- Updated: `tests/test_simulation_pipeline.cpp`
- Updated: `tests/test_stable_id_registry.cpp`
- Registered new ctest entries in root `CMakeLists.txt` (`test_core`, `test_tensor2d_smoothing`, `test_polygon_ops`, `test_editor_integrity`).

## Validation
- Build:
  - `"/mnt/c/Program Files/CMake/bin/cmake.exe" --build build_vs --config Release --target test_polygon_ops test_tensor2d_smoothing test_editor_integrity test_simulation_pipeline test_stable_id_registry test_core --parallel 4`
- Tests:
  - `"/mnt/c/Program Files/CMake/bin/ctest.exe" --test-dir build_vs -C Release -R "test_core|test_tensor2d_smoothing|test_polygon_ops|test_editor_integrity|test_simulation_pipeline|test_stable_id_registry|test_editor_plan|test_generators|test_city_generator_validation|test_zoning_generator" --output-on-failure`
- Result: all selected tests passed.

## Changelog Status
- `CHANGELOG.md` updated in this session.
