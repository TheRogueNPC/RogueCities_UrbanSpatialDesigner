# RC 0.10 Determinism Release Notes

Date: 2026-02-19

## Summary
This release hardens deterministic generation behavior and adds production-ready APIs for fixed-step simulation, stable viewport identity, incremental staged generation, and cancellation-aware execution.

## Key Improvements
- Determinism:
  - Added deterministic hash computation and baseline validation in Core.
  - Baseline checks are wired into tests and CI.
- Simulation:
  - Added `SimulationPipeline` with fixed timestep accumulation.
- Viewport identity:
  - Added `StableIDRegistry` for stable id allocation, mapping rebuild, and serialization.
- Generator pipeline:
  - Added stage masks and incremental regeneration entry points.
  - Added cancellation token/context propagation through generation stages.
- Scoring:
  - Added profile-driven AESP scoring via `ScoringProfile`.

## Performance
- Current measured metrics from `test_generation_latency`:
  - `full_generation_ms`: 211 ms
  - `incremental_roads_ms`: 139 ms
  - `cancel_response_ms`: 1 ms
- These remain within configured budgets in `tests/baselines/performance_budgets_v0.10.json`.

## Build and Packaging
- Added install/export support for `RogueCityCore` and `RogueCityGenerators`.
- Added `RogueCitiesConfig.cmake` and `RogueCitiesTargets.cmake`.
- Verified external consumer flow with:
  - `find_package(RogueCities CONFIG REQUIRED)`
  - link targets `RogueCity::RogueCityCore`, `RogueCity::RogueCityGenerators`.

## Validation Snapshot
- Clean configure/build from scratch: PASS
- Full test suite: PASS (26/26)
- Sanitizer (ASAN) smoke on representative tests: PASS
- Lua runtime compatibility smoke (Lua 5.4.8 C API): PASS
