# Codex Phase 2: PolygonOps Generator Wiring (2026-03-01)

## Scope
- Layer targets: `generators` + `tests`.
- Goal: make `PolygonOps` mission-critical in runtime zoning/lot/building behavior for upcoming 3D implementation.

## Implemented
- `LotGenerator` now uses `PolygonOps` in the subdivision path:
  - block outer rings are sanitized/validated before use,
  - each candidate lot rectangle is clipped against the block outer polygon via `PolygonOps::ClipPolygons`,
  - emitted lot `boundary`, `area`, and `centroid` derive from clipped geometry instead of fixed rectangle assumptions.
- Block hole handling hardened for lot generation:
  - hole rings are sanitized/validated,
  - lot candidates that geometrically overlap holes are rejected (prevents lots crossing no-build courtyards/voids).
- `ZoningGenerator` setback placement upgraded to polygon-aware clamping:
  - keeps existing deterministic AABB setback intent,
  - computes setback regions by clipping/insetting lot polygons,
  - clamps final building positions to the nearest valid point in the setback region.
- `SiteGenerator` behavior fix:
  - `randomize_sites=false` now truly keeps centroid placement,
  - when randomization is enabled, jitter attempts are bounded to polygon interior.

## Tests
- `tests/test_zoning_generator.cpp`:
  - added a holed-block integration scenario,
  - validates lot polygons are valid,
  - validates lot overlap with hole geometry is zero,
  - validates lots remain clipped to outer shell.
- `tests/test_polygon_ops.cpp`:
  - converted checks to always-on `RC_EXPECT` (active in Release),
  - added holed-difference area invariant,
  - added deterministic randomized robustness sweep for clip/diff/union area relationships and finite/valid polygon output.

## Validation
- Build:
  - `"/mnt/c/Program Files/CMake/bin/cmake.exe" --build build_vs --config Release --target test_polygon_ops test_zoning_generator test_generators test_core test_tensor2d_smoothing test_editor_integrity test_simulation_pipeline test_stable_id_registry --parallel 4`
- Tests:
  - `"/mnt/c/Program Files/CMake/bin/ctest.exe" --test-dir build_vs -C Release -R "test_core|test_tensor2d_smoothing|test_polygon_ops|test_editor_integrity|test_simulation_pipeline|test_stable_id_registry|test_generators|test_zoning_generator|test_city_generator_validation" --output-on-failure`
- Result: all selected tests passed.

## Notes
- `PolygonOps` currently returns flat path sets; for holed results this uses winding/orientation semantics. The new tests and lot-generation integration are written to preserve correctness under that contract.
