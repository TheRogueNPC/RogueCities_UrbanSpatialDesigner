# CityGenerator Validation Ranges

This document mirrors `CityGenerator::ValidateAndClampConfig()` and `CityGenerator::ValidateAxioms()`.

## Config Ranges
- `width`: 500 to 8192 meters
- `height`: 500 to 8192 meters
- `cell_size`: 1.0 to 50.0 meters
- `num_seeds`: 5 to 200
- `max_texture_resolution`: minimum 64 (default practical cap: 2048)
- `max_iterations_per_axiom`: minimum 1
- `min_trace_step_size`: 0.5 to 50.0
- `max_trace_step_size`: 0.5 to 100.0
- `trace_curvature_gain`: 0.1 to 8.0

If world dimensions and cell size imply a raster above `max_texture_resolution`, `cell_size` is auto-adjusted.

## Derived Capacity Formulas
Given `area_km2 = (width * height) / 1,000,000`:
- `max_districts`: clamped to `round(area_km2 / 0.25)` within [16, 4096]
- `max_lots`: clamped to `round(area_km2 * 2500)` within [500, 500000]
- `max_buildings`: clamped to `(2 * max_lots)` within [1000, 1000000]

## Axiom Validation Rules
- Axiom list must be non-empty.
- Each axiom position must be inside world bounds.
- `radius` must be finite and > 0.
- `decay` must be finite and > 1.0.
- Type-specific checks:
  - `Radial`: `radial_spokes` in [3, 24]
  - `Organic`: `organic_curviness` in [0, 1]
  - `LooseGrid`: `loose_grid_jitter` in [0, 1]
  - `Suburban`: `suburban_loop_strength` in [0, 1]
  - `Stem`: `stem_branch_angle` finite and > 0
  - `Superblock`: `superblock_block_size` finite and > 0

## Validation Error Message Format
Errors are emitted with index context:
- `Axiom[3]: position out of world bounds`
- `Axiom[1]: radial_spokes must be in [3,24]`
