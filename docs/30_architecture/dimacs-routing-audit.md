# DIMACS Routing Audit

## Scope
This audit treats "Dimanics" as DIMACS-style routing algorithms (challenge-family shortest path and hierarchy methods), not ODE dynamics.

## Current Implementation Location
- `generators/include/RogueCity/Generators/Urban/RoutingLibrary.hpp`
- `generators/src/Generators/Urban/RoutingLibrary.cpp`
- `tests/test_routing_library.cpp`

## Current Role In Pipeline
- `RoutingLibrary` is additive and tested but not a mandatory city generation stage.
- `CityGenerator` does not currently require precompute-backed DIMACS routines for baseline generation.

## Algorithm Status
- Implemented: Yen K-shortest, hierarchy extraction, delta-stepping, edge-flag Dijkstra, constrained shortest path variants, transit node fallback routing, arc-flags wrapper, REAL A* wrapper.
- Placeholder: Pascoal loopless candidate generation remains non-production.
- Fallback safety: precompute-dependent routes degrade to valid shortest-path behavior when auxiliaries are absent.

## Failure Modes
- Missing precompute data reduces performance claims (not correctness).
- Placeholder loopless candidates leave route-diversification potential underutilized.
- No mandatory precompute stage means DIMACS acceleration is not yet end-to-end realized.

## Upgrade Path
1. Add deterministic precompute builder artifacts and versioned cache keys.
2. Wire precompute-backed queries into bounded policy/auditor passes.
3. Add stress and tie-break determinism tests for large/disconnected graphs.
