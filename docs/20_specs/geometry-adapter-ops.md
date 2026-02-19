# GeometryAdapter Operations

`GeometryAdapter` provides backend-agnostic geometric utilities.

## Supported Ops
- `distance(Vec2, Vec2)`
- `intersects(Bounds, Bounds)`
- `intersects(Ring, Ring)`
- `convexHull(Ring)`
- `simplify(Ring, tolerance)`

## Determinism Notes
- Legacy backend uses deterministic monotonic-chain hull and Douglas-Peucker simplify.
- Spatial and point iteration order is deterministic for the same input order.

## Performance Characteristics
- `convexHull`: `O(n log n)` due to sorting.
- `simplify` (Douglas-Peucker): average `O(n log n)`, worst `O(n^2)` for adversarial input.
- `intersects(Ring, Ring)` (legacy path): edge-pair test is `O(n*m)`.

## Tolerance Guidance
- Use `simplify` tolerance relative to world units and grid resolution.
- Keep tolerance below smallest intended lot/road feature width to avoid topological loss.
