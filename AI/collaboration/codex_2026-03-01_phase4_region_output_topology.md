# Codex Phase 4: Region Output Topology (2026-03-01)

## Scope
- Layer targets: `core` geometry API contract + generator integration (`LotGenerator`).
- Goal: preserve explicit region topology (`outer + holes`) in operation outputs for 3D-ready downstream tooling.

## Implemented
- Extended `PolygonOps` API with topology-preserving region output operations:
  - `ClipRegions(const PolygonRegion&, const PolygonRegion&)`
  - `DifferenceRegions(const PolygonRegion&, const PolygonRegion&)`
  - `UnionRegions(const std::vector<PolygonRegion>&)`
- Implemented PolyTree-based extraction to recover region hierarchy (`outer` with immediate `holes`) from Clipper results.
- Added legacy bridge behavior:
  - existing polygon-return region overloads (`ClipPolygons(..., PolygonRegion)`, `DifferencePolygons(..., PolygonRegion)`) now flatten from region outputs to preserve backward compatibility.
- Updated `LotGenerator` intersection selection to consume `ClipRegions(...)` and emit only simple outers (regions without holes) for `LotToken` boundaries.

## Tooling Honor Outcome
- Generator/tool flows now honor region topology in runtime clipping contracts while preserving existing legacy callsites.
- Holed outputs are no longer silently collapsed into ambiguous flat polygon assumptions for lot emission.

## Tests
- `tests/test_polygon_ops.cpp` expanded with Phase 4 region-output invariants:
  - clip region returns a single holed region (`holes.size() == 1`),
  - region difference returns expected simple region,
  - union regions fills the hole as expected and returns hole-free result with correct net area.
- Existing `tests/test_zoning_generator.cpp` holed block runtime coverage remains green.

## Validation
- Build:
  - `"/mnt/c/Program Files/CMake/bin/cmake.exe" --build build_vs --config Release --target test_polygon_ops test_zoning_generator test_generators --parallel 4`
- Tests:
  - `"/mnt/c/Program Files/CMake/bin/ctest.exe" --test-dir build_vs -C Release -R "test_core|test_tensor2d_smoothing|test_polygon_ops|test_editor_integrity|test_simulation_pipeline|test_stable_id_registry|test_generators|test_zoning_generator|test_city_generator_validation" --output-on-failure`
- Result: selected suite passed.
