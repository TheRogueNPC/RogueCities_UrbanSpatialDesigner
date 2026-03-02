# Codex Phase 3: Polygon Region Ops + Tool Honor Pass (2026-03-01)

## Scope
- Layer targets: `core` geometry APIs + `generators` subdivision runtime.
- Goal: move from implicit hole winding behavior to explicit region semantics (`outer + holes`) and ensure generator/tool paths consume the new contract.

## Implemented
- Added `PolygonRegion` to core geometry API (`PolygonOps.hpp`) and implemented:
  - `ClipPolygons(const Polygon&, const PolygonRegion&)`
  - `DifferencePolygons(const Polygon&, const PolygonRegion&)`
  - `SimplifyRegion(...)`
  - `IsValidRegion(...)`
- Region clipping now executes as a single oriented clip path set (`outer` winding + opposite hole winding), preserving hole semantics robustly in Clipper execution.
- Updated `LotGenerator` to build and validate a `PolygonRegion` from each `BlockPolygon` (`outer + holes`) and clip candidate lots directly against the region API.
- Removed duplicated ad-hoc hole overlap checks from lot subdivision logic; region API is now the authoritative behavior.

## Tool/Runtime Honor Updates
- Zoning/generator tool flows that rely on `LotGenerator` now inherit region semantics automatically.
- Holed blocks are now honored by construction in runtime lot emission; tool-side behavior is consistent with no-build courtyard/void expectations.

## Tests
- Extended `tests/test_polygon_ops.cpp` with region-specific invariants:
  - region clip area against holed regions,
  - region difference area,
  - invalid region rejection.
- Existing `tests/test_zoning_generator.cpp` holed-block scenario continues to validate hole exclusion under runtime generation.

## Validation
- Build:
  - `"/mnt/c/Program Files/CMake/bin/cmake.exe" --build build_vs --config Release --target test_polygon_ops test_zoning_generator test_generators --parallel 4`
- Tests:
  - `"/mnt/c/Program Files/CMake/bin/ctest.exe" --test-dir build_vs -C Release -R "test_core|test_tensor2d_smoothing|test_polygon_ops|test_editor_integrity|test_simulation_pipeline|test_stable_id_registry|test_generators|test_zoning_generator|test_city_generator_validation" --output-on-failure`
- Result: selected suite passed.
