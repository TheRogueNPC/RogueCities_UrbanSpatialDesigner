# Changelog

## [0.11.0] - 2026-02-24

### Added
- **Foundational Mandates (`.gemini/GEMINI.md`)**: Established core architectural and engineering standards, including layer ownership and mathematical reference tables for AESP and Grid Quality.
- **Urban Analytics Core (`GridMetrics.hpp`)**: Introduced `GridQualityReport` and `RoadStroke` data structures to the core layer for semantic road network analysis.
- **Grid Quality Engine (`GridAnalytics.cpp`)**: Developed a high-performance module to calculate Straightness ($\varsigma$), Orientation Order ($\Phi$), and 4-Way Proportion ($I$) through stroke-based extraction.
- **"Urban Hell" Diagnostics**: Implemented hardening checks for disconnected network islands, dead-end sprawl (Gamma Index), and micro-segment geometric health.
- **Spline Tool Architecture (`SplineManipulator.cpp`)**: Centralized interactive spline logic into a reusable manipulator in the `app` layer for consistent road and river editing.
- **Hydrated Viewport Tools**: Replaced `RoadTool` and `WaterTool` stubs with functional implementations that handle vertex dragging, pen-tool addition, and proportional editing.
- **Telemetry UI Integration**: Enhanced the `Analytics` panel with real-time visual feedback for grid quality metrics and structural integrity warnings.

### Changed
- **Pipeline Integration**: Updated `CityGenerator` to compute and cache grid analytics at the end of the road tracing stage for zero-latency UI updates.
- **Global State Wiring**: Refactored `CityOutputApplier` and `GlobalState` to persist and broadcast urban analytics across the application.
- **Build System**: Registered the new Scoring and Analytics modules in the `RogueCityGenerators` library via `CMakeLists.txt`.

## [0.10.0] - 2026-02-19

### Added
- Determinism hashing and baseline validation APIs:
  - `RogueCity/Core/Validation/DeterminismHash.hpp`
  - `ComputeDeterminismHash`, `SaveBaselineHash`, `ValidateAgainstBaseline`
- Fixed-step simulation pipeline API:
  - `RogueCity/Core/Simulation/SimulationPipeline.hpp`
- Stable viewport identity API:
  - `RogueCity/Core/Editor/StableIDRegistry.hpp`
- Incremental/cancelable generation APIs:
  - `RogueCity/Generators/Pipeline/GenerationStage.hpp`
  - `RogueCity/Generators/Pipeline/GenerationContext.hpp`
- Profile-driven AESP scoring API:
  - `RogueCity/Generators/Scoring/ScoringProfile.hpp`
- CMake package export for external consumers:
  - `RogueCitiesConfig.cmake`
  - `RogueCitiesTargets.cmake`

### Changed
- Generator pipeline now supports staged incremental regeneration and cancellation context propagation.
- Performance checks for generation latency and stage behavior are integrated into the test suite.
- Install rules now include generator headers and libraries for package consumers.

### Fixed
- Removed legacy, unreferenced compatibility include headers under:
  - `generators/include/Pipeline/`
  - `generators/include/Roads/`
  - `generators/include/Tensors/`
  - `generators/include/Districts/`
- Addressed test warning regressions in:
  - `tests/test_core.cpp`
  - `tests/test_viewport.cpp`

### Verification Highlights
- Clean build and full test suite pass (`26/26`).
- Determinism baseline hash regenerated from a clean state and validated.
- External package consumption validated via `find_package(RogueCities CONFIG REQUIRED)`.
