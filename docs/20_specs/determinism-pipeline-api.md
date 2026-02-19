# Determinism Pipeline API (RC 0.10)

## Core APIs

### `RogueCity/Core/Validation/DeterminismHash.hpp`
- `struct DeterminismHash`
  - field-wise deterministic signatures for roads, districts, lots, buildings, tensor field.
- `ComputeDeterminismHash(const Editor::GlobalState&)`
  - computes deterministic snapshot hash.
- `SaveBaselineHash(const DeterminismHash&, const std::string&)`
  - writes baseline file.
- `ValidateAgainstBaseline(const DeterminismHash&, const std::string&)`
  - compares current hash against persisted baseline.

### `RogueCity/Core/Simulation/SimulationPipeline.hpp`
- `struct SimulationConfig`
  - fixed timestep + max substeps.
- `struct StepResult`
  - execution success, substep count, post-step frame counter, error message.
- `class SimulationPipeline`
  - `ExecuteStep(Editor::GlobalState&, float dt_seconds)`
  - `Reset()`
  - `config() const`

### `RogueCity/Core/Editor/StableIDRegistry.hpp`
- `struct StableID`
- `struct ViewportEntityID`
- `class StableIDRegistry`
  - `AllocateStableID(...)`
  - `GetStableID(...) const`
  - `GetViewportID(...) const`
  - `RebuildMapping(...)`
  - `Serialize() const`
  - `Deserialize(...)`
  - `Clear()`
- `GetStableIDRegistry()`

## Generator APIs

### `RogueCity/Generators/Pipeline/GenerationStage.hpp`
- `enum class GenerationStage`
- `using StageMask`
- helpers:
  - `StageIndex(...)`
  - `MarkStageDirty(...)`
  - `IsStageDirty(...)`
  - `FullStageMask()`
  - `CascadeDirty(...)`

### `RogueCity/Generators/Pipeline/GenerationContext.hpp`
- `class CancellationToken`
  - `Cancel()`, `IsCancelled() const`, `Reset()`
- `struct GenerationContext`
  - `cancellation`, `iteration_count`, `max_iterations`
  - `ShouldAbort() const`, `BumpIterations(...)`

### `RogueCity/Generators/Scoring/ScoringProfile.hpp`
- `struct DistrictScoreWeights`
- `struct ScoringProfile`
  - built-in profiles:
    - `Urban()`, `Suburban()`, `Rural()`, `Industrial()`
  - deterministic selection:
    - `FromSeed(uint32_t)`

## Packaging
- Installed package now exports:
  - `RogueCity::RogueCityCore`
  - `RogueCity::RogueCityGenerators`
- Consumer usage:
  - `find_package(RogueCities CONFIG REQUIRED)`
  - link exported targets above.
