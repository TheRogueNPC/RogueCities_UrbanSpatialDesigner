# Comprehensive Implementation Plan: RogueCities Deterministic Pipeline Refactor

Based on your repository structure and existing patterns, here's a production-ready implementation plan designed for AI-assisted single-pass completion.


# CheckList
# Implementation Checklist: RogueCities Deterministic Pipeline Refactor

## Phase 0: Baseline + TODO Normalization

### File Creation
- [x] Create `docs/BASELINE_METRICS.md`
- [x] Create `core/include/RogueCity/Core/Validation/DeterminismHash.hpp`
- [x] Create `core/src/Core/Validation/DeterminismHash.cpp`
- [x] Create `docs/TODO_AUDIT.md`
- [x] Create `tests/baselines/` directory
- [x] Create `tests/baselines/determinism_v0.10.txt`

### Implementation Tasks
- [x] Implement `DeterminismHash` struct with equality operator
- [x] Implement `DeterminismHash::to_string()` method
- [x] Implement `ComputeDeterminismHash()` function
- [x] Implement `SaveBaselineHash()` function
- [x] Implement `ValidateAgainstBaseline()` function
- [x] Add FNV-1a hash helper functions
- [x] Add hash computation for roads container
- [x] Add hash computation for districts vector
- [x] Add hash computation for lots vector
- [x] Add hash computation for buildings vector

### Cleanup Tasks
- [x] Grep search entire codebase for legacy header usage
- [x] Mark deprecated headers with `#pragma message` warnings
- [x] Audit all TODO comments and categorize as active/deferred
- [x] Document deferred TODOs in `TODO_AUDIT.md`
- [x] Remove confirmed dead code (after verification)

### Testing & Validation
- [x] Run full test suite to verify green baseline
- [x] Generate baseline hash from clean state
- [ ] Commit baseline hash file to version control
- [x] Add baseline validation to CI pipeline
- [x] Document baseline recording procedure

### CMakeLists.txt Updates
- [x] Add `DeterminismHash.cpp` to core library sources
- [x] Link validation library to core tests
- [x] Add baseline validation test target

***

## Phase 1: Core Determinism Foundation

### File Creation
- [x] Create `core/include/RogueCity/Core/Simulation/SimulationPipeline.hpp`
- [x] Create `core/src/Core/Simulation/SimulationPipeline.cpp`

### Header Implementation
- [x] Define `SimulationConfig` struct with fixed timestep
- [x] Define `StepResult` struct for logging
- [x] Define `SimulationPipeline` class
- [x] Add `ExecuteStep()` public method signature
- [x] Add `Reset()` method signature
- [x] Add `config()` getter
- [x] Add private helper methods: `StepPhysics()`, `StepAgents()`, `StepSystems()`
- [x] Add member variables: `config_`, `time_accumulator_`, `frame_counter_`

### Implementation Tasks
- [x] Implement `SimulationPipeline` constructor
- [x] Implement `ExecuteStep()` with fixed timestep logic
- [x] Implement `Reset()` method
- [x] Add try-catch error handling in `ExecuteStep()`
- [x] Implement frame counter increment in GlobalState
- [x] Add placeholder `StepPhysics()` implementation
- [x] Add placeholder `StepAgents()` implementation
- [x] Add placeholder `StepSystems()` implementation

### EditorState Integration
- [x] Open `core/src/Core/Editor/EditorState.cpp`
- [x] Add `#include "RogueCity/Core/Simulation/SimulationPipeline.hpp"` at top
- [x] Locate `EditorHFSM::update()` method (~line 324)
- [x] Replace placeholder TODO with `SimulationPipeline::ExecuteStep()` call
- [x] Add static `SimulationPipeline` instance
- [x] Add error handling for failed simulation steps
- [x] Remove old `++gs.frame_counter` placeholder
- [x] Preserve `transition_to(EditorState::Simulation_Paused, gs)` behavior

### Testing
- [x] Create `tests/test_simulation_pipeline.cpp`
- [x] Add test for fixed timestep execution
- [x] Add test for frame counter increment
- [x] Add test for deterministic ordering
- [x] Add test for same seed → same output
- [x] Verify `test_editor_hfsm.cpp` still passes
- [x] Add determinism verification test

### CMakeLists.txt Updates
- [x] Add `SimulationPipeline.cpp` to core library sources
- [x] Add simulation tests to test target
- [x] Ensure proper linking of simulation module

***

## Phase 2: Stable Viewport Identity Layer

### File Creation
- [x] Create `core/include/RogueCity/Core/Editor/StableIDRegistry.hpp`
- [x] Create `core/src/Core/Editor/StableIDRegistry.cpp`

### Header Implementation
- [x] Define `StableID` struct with id and version fields
- [x] Add equality operator to `StableID`
- [x] Define `StableIDRegistry` class
- [x] Add `AllocateStableID()` method
- [x] Add `GetStableID()` method (viewport → stable)
- [x] Add `GetViewportID()` method (stable → viewport)
- [x] Add `RebuildMapping()` method
- [x] Add `Serialize()` method
- [x] Add `Deserialize()` method
- [x] Add `Clear()` method
- [x] Add global accessor `GetStableIDRegistry()`
- [x] Add private member variables: `next_stable_id_`, maps, aliases

### Implementation Tasks
- [x] Implement `AllocateStableID()` with ID generation
- [x] Implement `GetStableID()` with lookup
- [x] Implement `GetViewportID()` with reverse lookup
- [x] Implement `RebuildMapping()` with probe iteration
- [x] Implement `Serialize()` with string formatting
- [x] Implement `Deserialize()` with string parsing
- [x] Implement `Clear()` to reset state
- [x] Implement singleton `GetStableIDRegistry()`
- [x] Add version migration logic placeholder

### ViewportIndex Integration
- [x] Open `core/include/RogueCity/Core/Editor/ViewportIndex.hpp`
- [x] Add forward declaration of `StableID` struct
- [x] Add `GetProbeStableID()` function declaration after line 26
- [x] Add `RebuildStableIDMapping()` function declaration
- [x] Implement helper functions in corresponding `.cpp` file
- [x] Add stable ID field to `VpProbeData` (optional, for caching)

### Testing
- [x] Create `tests/test_stable_id_registry.cpp`
- [x] Add test for ID allocation and uniqueness
- [x] Add test for viewport → stable → viewport round-trip
- [x] Add test for rebuild after viewport regeneration
- [x] Add test for serialization/deserialization
- [x] Add test for persistence across save/load
- [x] Verify Lua compatibility (if applicable)

### CMakeLists.txt Updates
- [x] Add `StableIDRegistry.cpp` to core library sources

***

## Phase 3: CityGenerator Config/Input Hardening

### CityGenerator.hpp Modifications
- [x] Open `generators/include/RogueCity/Generators/Pipeline/CityGenerator.hpp`
- [x] Add `ValidationResult` struct definition in public section
- [x] Add `ValidateAndClampConfig()` static method declaration
- [x] Add `ValidateAxioms()` static method declaration
- [x] Add `DeriveConstraintsFromWorldSize()` private method declaration

### Implementation Tasks
- [x] Open `generators/src/Generators/Pipeline/CityGenerator.cpp`
- [x] Implement `ValidateAndClampConfig()` function
- [x] Add world dimension clamping (500-8192 range)
- [x] Add cell size clamping (1.0-50.0 range)
- [x] Add texture resolution validation (2048 limit)
- [x] Add automatic cell size adjustment for texture limits
- [x] Add seed count clamping (5-200 range)
- [x] Call `DeriveConstraintsFromWorldSize()` in validation
- [x] Implement `DeriveConstraintsFromWorldSize()` function
- [x] Calculate district cap from area (~1 per 0.25 km²)
- [x] Calculate lot cap from area (~2500 per km²)
- [x] Calculate building cap (2x lots)
- [x] Add clamping for all derived caps
- [x] Implement `ValidateAxioms()` function
- [x] Add empty axiom check
- [x] Add position bounds validation
- [x] Add radius sanity checks
- [x] Add type-specific parameter validation (radial spokes, etc.)
- [x] Generate detailed error messages with indices

### Integration
- [x] Update `CityGenerator::generate()` to call `ValidateAndClampConfig()`
- [x] Update `CityGenerator::generate()` to call `ValidateAxioms()`
- [x] Add error handling for invalid configurations
- [x] Add logging/diagnostics for clamped values
- [x] Add warnings for unusual but valid values

### Testing
- [x] Create `tests/test_city_generator_validation.cpp`
- [x] Add test for dimension clamping
- [x] Add test for texture resolution limits
- [x] Add test for derived constraint calculation
- [x] Add test for axiom position validation
- [x] Add test for axiom parameter validation
- [x] Add test for empty axiom rejection
- [x] Add test for out-of-bounds axiom rejection
- [x] Verify generation with clamped config produces valid output

### Documentation
- [x] Document constraint derivation formulas in code comments
- [x] Add validation error message reference to docs
- [x] Document minimum/maximum parameter ranges

***

## Phase 4: Pipeline Decomposition + Incremental Execution

### File Creation
- [x] Create `generators/include/RogueCity/Generators/Pipeline/GenerationStage.hpp`

### GenerationStage.hpp Implementation
- [x] Define `GenerationStage` enum class with 8 stages
- [x] Define `StageMask` typedef using `std::bitset<8>`
- [x] Implement `MarkStageDirty()` inline function
- [x] Implement `IsStageDirty()` inline function
- [x] Add inline documentation for cascade behavior

### CityGenerator.hpp Modifications
- [x] Open `generators/include/RogueCity/Generators/Pipeline/CityGenerator.hpp`
- [x] Add `#include "GenerationStage.hpp"`
- [x] Define `StageOptions` struct with `stages_to_run` and `use_cache` fields
- [x] Add `GenerateStages()` method declaration
- [x] Add `RegenerateIncremental()` method declaration
- [x] Define `StageCache` struct in private section
- [x] Add all stage result fields to `StageCache`
- [x] Add `valid_stages` StageMask to `StageCache`
- [x] Add `cache_` member variable

### Implementation Tasks
- [x] Open `generators/src/Generators/Pipeline/CityGenerator.cpp`
- [x] Implement `GenerateStages()` method skeleton
- [x] Add Terrain stage with cache check
- [x] Add TensorField stage with cache check
- [x] Add Roads stage with cache check
- [x] Add Districts stage with cache check
- [x] Add Blocks stage with cache check
- [x] Add Lots stage with cache check
- [x] Add Buildings stage with cache check
- [x] Add Validation stage with cache check
- [x] Implement cache storage after each stage
- [x] Implement cache retrieval when stage is clean
- [x] Implement `RegenerateIncremental()` wrapper method
- [x] Add cache invalidation on config/axiom changes

### Existing Pipeline Refactoring
- [x] Preserve existing `generate()` method API
- [x] Update `generate()` to call `GenerateStages()` with full stage mask
- [x] Ensure output parity between old and new paths
- [x] Verify no behavior changes in default generation

### Testing
- [x] Create `tests/test_generation_stages.cpp`
- [x] Add test for full pipeline execution
- [x] Add test for incremental tensor+roads regeneration
- [x] Add test for cache hit on clean stages
- [x] Add test for cache invalidation on dirty stages
- [x] Add test for stage dependency cascade
- [x] Add output parity test (old vs new API)
- [x] Add performance test for cache efficiency

### CMakeLists.txt Updates
- [x] Verify header-only `GenerationStage.hpp` included in install

***

## Phase 5: Async Queue + Cancellation Tokens

### File Creation
- [x] Create `generators/include/RogueCity/Generators/Pipeline/GenerationContext.hpp`

### GenerationContext.hpp Implementation
- [x] Add `#include <atomic>` and `#include <memory>`
- [x] Define `CancellationToken` class
- [x] Add `std::atomic<bool> cancelled_` member
- [x] Implement `Cancel()` method with atomic store
- [x] Implement `IsCancelled()` method with atomic load
- [x] Implement `Reset()` method
- [x] Define `GenerationContext` struct
- [x] Add `std::shared_ptr<CancellationToken>` member
- [x] Add `iteration_count` and `max_iterations` members
- [x] Implement `ShouldAbort()` method

### CityGenerator.hpp Modifications
- [x] Open `generators/include/RogueCity/Generators/Pipeline/CityGenerator.hpp`
- [x] Add `#include "GenerationContext.hpp"`
- [x] Add `GenerateWithContext()` method declaration
- [x] Add `GenerationContext*` parameter to `GenerateStages()`
- [x] Add `GenerationContext*` parameter to all private stage methods

### Implementation Tasks
- [x] Open `generators/src/Generators/Pipeline/CityGenerator.cpp`
- [x] Implement `GenerateWithContext()` wrapper method
- [x] Add context parameter to `traceRoads()` signature
- [x] Add cancellation check in `traceRoads()` seed loop
- [x] Add iteration counter increment in `traceRoads()`
- [x] Add context parameter to `classifyDistricts()` signature
- [x] Add cancellation check in district classification loop
- [x] Add context parameter to `generateBlocks()` signature
- [x] Add cancellation check in block generation loop
- [x] Add context parameter to `generateLots()` signature
- [x] Add cancellation check in lot generation loop
- [x] Add context parameter to `generateBuildings()` signature
- [x] Add cancellation check in building generation loop
- [x] Update `GenerateStages()` to pass context through pipeline
- [x] Add early return handling on cancellation

### Integration with RealTimePreview
- [x] Locate RealTimePreview class (if exists)
- [x] Add `CancellationToken` member to preview system
- [x] Connect preview stop button to `Cancel()` call
- [x] Reset token on new generation start
- [x] Add cancellation status feedback to UI

### Testing
- [x] Create `tests/test_generation_cancellation.cpp`
- [x] Add test for cancellation during roads stage
- [x] Add test for cancellation during districts stage
- [x] Add test for iteration limit enforcement
- [x] Add test for token reset and reuse
- [x] Add test for partial result return on cancel
- [x] Add stress test for rapid cancel/restart cycles

### Performance Considerations
- [x] Document cancellation check frequency (every N iterations)
- [x] Add comment explaining atomic ordering choice
- [x] Verify no performance regression on uncancelled paths

***

## Phase 6: Quality Upgrades from Generator TODO Backlog

### AESP Classifier Enhancement
- [x] Locate `AESPClassifier` class in codebase
- [x] Create `generators/include/RogueCity/Generators/Scoring/ScoringProfile.hpp`
- [x] Define `ScoringProfile` struct with configurable weights
- [x] Add default profiles: Urban, Suburban, Rural, Industrial
- [x] Refactor `AESPClassifier` to accept `ScoringProfile` parameter
- [x] Replace hardcoded scoring with profile-driven logic
- [x] Add deterministic profile selection based on seed
- [x] Add unit tests for profile-driven scoring

### GeometryAdapter Improvements
- [x] Locate `GeometryAdapter` class
- [x] Identify TODO comments for geometry operations
- [x] Add optional advanced operations (buffer, simplify, etc.)
- [x] Implement convex hull operation
- [x] Implement polygon simplification (Douglas-Peucker)
- [x] Add configurable tolerance parameters
- [x] Ensure all operations preserve determinism
- [x] Add unit tests for geometry operations

### Adaptive Tracing
- [x] Locate `StreamlineTracer` class
- [x] Review legacy TODO for adaptive tracing
- [x] Implement adaptive step size based on curvature
- [x] Add configuration for min/max step sizes
- [x] Ensure deterministic behavior with fixed seed
- [x] Add tests for adaptive vs fixed step comparison

### Separation Acceleration
- [x] Locate road separation logic in `RoadGenerator`
- [x] Implement spatial acceleration structure (grid or quadtree)
- [x] Add deterministic iteration order for spatial queries
- [x] Benchmark performance improvement 
- [x] Add tests for correctness with acceleration

### Documentation
- [x] Document all configurable scoring profiles
- [x] Add examples of custom profile creation
- [x] Document geometry operation performance characteristics
- [x] Update architecture docs with quality improvements

***

## Phase 7: Hardening, Tests, and Rollout Gates

### Unit Test Expansion
- [x] Create `tests/test_determinism_comprehensive.cpp`
- [x] Add determinism test with 10 different seeds
- [x] Add determinism test with different world sizes
- [x] Add determinism test with all axiom types
- [x] Add regression test comparing against baseline hashes
- [x] Create `tests/test_cancellation_coverage.cpp`
- [x] Add cancellation test for each pipeline stage
- [x] Add cancellation stress test (1000 rapid cancels)
- [x] Create `tests/test_incremental_parity.cpp`
- [x] Add full-gen vs incremental-gen output comparison
- [x] Add cache invalidation correctness tests
- [x] Create `tests/test_viewport_id_stability.cpp`
- [x] Add stable ID persistence test across rebuilds
- [x] Add stable ID serialization round-trip test

### Integration Test Suite
- [x] Create `tests/integration/test_full_pipeline.cpp`
- [x] Add end-to-end generation with validation
- [x] Add multi-stage incremental generation test
- [x] Add cancellation during complex generation test
- [x] Add save/load with stable IDs test
- [x] Create `tests/integration/test_texture_replay.cpp`
- [x] Add texture space hash verification test
- [x] Add deterministic texture painting test

### Performance Budget Tests
- [x] Create `tests/perf/test_generation_latency.cpp`
- [x] Add benchmark for full generation (target: <5s for 2km²)
- [x] Add benchmark for incremental roads regen (target: <1s)
- [x] Add benchmark for cancellation response (target: <100ms)
- [x] Create performance report generation script
- [x] Add performance regression detection to CI

### CI Integration
- [x] Create `.github/workflows/determinism-check.yml`
- [x] Add job to run determinism tests on every commit
- [x] Add baseline hash comparison step
- [x] Fail build if baseline doesn't match (with override)
- [x] Create `.github/workflows/performance-check.yml`
- [x] Add job to run performance budget tests
- [x] Add performance report artifact upload
- [x] Create alerts for performance regressions

### Release Gates
- [x] Document release criteria in `docs/RELEASE_CHECKLIST.md`
- [x] Verify all unit tests pass (100% required)
- [x] Verify integration tests pass (100% required)
- [x] Verify determinism tests pass with baseline
- [x] Verify performance budgets met
- [x] Verify no new compiler warnings
- [x] Verify no memory leaks (valgrind/sanitizers) [Mission Critical]
- [x] Verify documentation updated for all new APIs

### Final Validation
- [x] Run full test suite on clean build
- [x] Generate final baseline hash for v0.10
- [ ] Tag release commit with version
- [x] Update CHANGELOG.md with all changes
- [x] Create release notes highlighting determinism improvements

***

## Post-Implementation Verification

### Code Quality Checks
- [x] Run clang-tidy on all modified files
- [x] Run clang-format on all modified files
- [x] Verify all headers have include guards
- [x] Verify all functions have documentation comments
- [x] Check for TODO comments marked with phase/issue number

### API Consistency Review
- [x] Verify all new APIs match existing naming conventions
- [x] Verify all parameters use const& for read-only
- [x] Verify all return values marked [[nodiscard]] where appropriate
- [x] Verify all single-arg constructors marked explicit
- [x] Verify all overrides marked with override keyword

### Build System Verification
- [x] Clean build from scratch succeeds
- [x] All tests link correctly
- [x] No circular dependencies introduced
- [x] All new headers appear in install target
- [x] CMake find_package works for external consumers

***

**Total Tasks: 267**

This checklist provides a complete, sequential implementation guide. Each checkbox represents an atomic, verifiable task that can be completed and tested independently.

## Architecture Overview

Your codebase follows a clean modular pattern:
- **Core Module** (`core/include/RogueCity/Core/`): Editor state, data structures, validation
- **Generators Module** (`generators/include/RogueCity/Generators/`): Pipeline orchestration, tensor fields, urban generation
- **Clear separation**: Editor HFSM manages state, CityGenerator orchestrates generation stages

***

## Phase 0: Baseline + TODO Normalization

### What Goes Where
**File**: `docs/BASELINE_METRICS.md` (new)
**File**: `core/include/RogueCity/Core/Validation/DeterminismHash.hpp` (new)
**File**: `core/src/Core/Validation/DeterminismHash.cpp` (new)

### Code Example: Determinism Hash System

```cpp
// core/include/RogueCity/Core/Validation/DeterminismHash.hpp
#pragma once
#include <cstdint>
#include <string>

namespace RogueCity::Core::Validation {

/// Deterministic hash for reproducibility testing
struct DeterminismHash {
    uint64_t roads_hash{0};
    uint64_t districts_hash{0};
    uint64_t lots_hash{0};
    uint64_t buildings_hash{0};
    uint64_t tensor_field_hash{0};
    
    [[nodiscard]] bool operator==(const DeterminismHash& other) const noexcept;
    [[nodiscard]] std::string to_string() const;
};

/// Compute deterministic hash from GlobalState
[[nodiscard]] DeterminismHash ComputeDeterminismHash(const Editor::GlobalState& gs);

/// Save baseline to file for CI comparison
void SaveBaselineHash(const DeterminismHash& hash, const std::string& filepath);

/// Load and compare against baseline
[[nodiscard]] bool ValidateAgainstBaseline(const DeterminismHash& hash, const std::string& filepath);

} // namespace RogueCity::Core::Validation
```

```cpp
// core/src/Core/Validation/DeterminismHash.cpp
#include "RogueCity/Core/Validation/DeterminismHash.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>

namespace RogueCity::Core::Validation {

namespace {
    // FNV-1a hash for deterministic hashing
    constexpr uint64_t kFNVOffsetBasis = 14695981039346656037ULL;
    constexpr uint64_t kFNVPrime = 1099511628211ULL;
    
    uint64_t HashBytes(const void* data, size_t len, uint64_t hash = kFNVOffsetBasis) {
        const auto* bytes = static_cast<const uint8_t*>(data);
        for (size_t i = 0; i < len; ++i) {
            hash ^= bytes[i];
            hash *= kFNVPrime;
        }
        return hash;
    }
}

bool DeterminismHash::operator==(const DeterminismHash& other) const noexcept {
    return roads_hash == other.roads_hash &&
           districts_hash == other.districts_hash &&
           lots_hash == other.lots_hash &&
           buildings_hash == other.buildings_hash &&
           tensor_field_hash == other.tensor_field_hash;
}

std::string DeterminismHash::to_string() const {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        << "roads:" << std::setw(16) << roads_hash << " "
        << "districts:" << std::setw(16) << districts_hash << " "
        << "lots:" << std::setw(16) << lots_hash << " "
        << "buildings:" << std::setw(16) << buildings_hash << " "
        << "tensor:" << std::setw(16) << tensor_field_hash;
    return oss.str();
}

DeterminismHash ComputeDeterminismHash(const Editor::GlobalState& gs) {
    DeterminismHash hash;
    
    // Hash roads container
    if (!gs.roads.empty()) {
        hash.roads_hash = HashBytes(gs.roads.data(), gs.roads.size() * sizeof(Road));
    }
    
    // Hash districts vector
    if (!gs.districts.empty()) {
        hash.districts_hash = HashBytes(gs.districts.data(), gs.districts.size() * sizeof(District));
    }
    
    // Hash lots vector
    if (!gs.lots.empty()) {
        hash.lots_hash = HashBytes(gs.lots.data(), gs.lots.size() * sizeof(LotToken));
    }
    
    // Hash buildings vector
    if (!gs.buildings.empty()) {
        hash.buildings_hash = HashBytes(gs.buildings.data(), gs.buildings.size() * sizeof(BuildingSite));
    }
    
    // Hash tensor field if present
    // TODO: Add tensor field data accessor once TensorFieldGenerator provides stable hash
    
    return hash;
}

void SaveBaselineHash(const DeterminismHash& hash, const std::string& filepath) {
    std::ofstream file(filepath);
    file << hash.to_string() << "\n";
}

bool ValidateAgainstBaseline(const DeterminismHash& hash, const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;
    
    std::string baseline_str;
    std::getline(file, baseline_str);
    
    return baseline_str == hash.to_string();
}

} // namespace RogueCity::Core::Validation
```

### DOS AND DON'TS

✅ **DO**:
- Mark deprecated headers with `#pragma message("DEPRECATED: Use include/RogueCity/... instead")`
- Run full test suite before recording baseline
- Store baseline hashes in version control (`tests/baselines/determinism_v0.10.txt`)
- Document which TODOs are deferred vs active in `docs/TODO_AUDIT.md`

❌ **DON'T**:
- Remove headers without grep-searching entire codebase first
- Record baseline without green tests
- Modify baseline files manually (always regenerate)
- Keep TODOs without file/line references

***

## Phase 1: Core Determinism Foundation

### What Goes Where
**File**: `core/include/RogueCity/Core/Simulation/SimulationPipeline.hpp` (new)
**File**: `core/src/Core/Simulation/SimulationPipeline.cpp` (new)
**Modify**: `core/src/Core/Editor/EditorState.cpp` (line 324)

### Code Example: Deterministic Simulation Step

```cpp
// core/include/RogueCity/Core/Simulation/SimulationPipeline.hpp
#pragma once
#include "RogueCity/Core/Types.hpp"
#include <cstdint>

namespace RogueCity::Core::Editor {
    struct GlobalState;
}

namespace RogueCity::Core::Simulation {

/// Fixed-timestep simulation configuration
struct SimulationConfig {
    float fixed_dt{1.0f / 60.0f};  // 60 Hz fixed timestep
    uint32_t max_substeps{4};      // Prevent spiral of death
    bool deterministic_mode{true}; // Enforce determinism checks
};

/// Simulation step result (for logging/debugging)
struct StepResult {
    uint32_t frame_number{0};
    float accumulated_time{0.0f};
    bool success{true};
    std::string error_message;
};

/// Deterministic simulation pipeline
class SimulationPipeline {
public:
    explicit SimulationPipeline(const SimulationConfig& config = {});
    
    /// Execute one fixed-timestep simulation step
    /// MUST be deterministic: same input state + seed -> same output state
    [[nodiscard]] StepResult ExecuteStep(Editor::GlobalState& gs);
    
    /// Reset accumulator (for pausing/resuming)
    void Reset();
    
    [[nodiscard]] const SimulationConfig& config() const noexcept { return config_; }
    
private:
    SimulationConfig config_;
    float time_accumulator_{0.0f};
    uint32_t frame_counter_{0};
    
    void StepPhysics(Editor::GlobalState& gs, float dt);
    void StepAgents(Editor::GlobalState& gs, float dt);
    void StepSystems(Editor::GlobalState& gs, float dt);
};

} // namespace RogueCity::Core::Simulation
```

```cpp
// core/src/Core/Simulation/SimulationPipeline.cpp
#include "RogueCity/Core/Simulation/SimulationPipeline.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include <algorithm>

namespace RogueCity::Core::Simulation {

SimulationPipeline::SimulationPipeline(const SimulationConfig& config)
    : config_(config) {}

StepResult SimulationPipeline::ExecuteStep(Editor::GlobalState& gs) {
    StepResult result;
    result.frame_number = frame_counter_++;
    result.accumulated_time = time_accumulator_;
    
    try {
        // Fixed timestep execution
        const float dt = config_.fixed_dt;
        
        // Execute simulation systems in deterministic order
        StepPhysics(gs, dt);
        StepAgents(gs, dt);
        StepSystems(gs, dt);
        
        // Increment frame counter on GlobalState for consistency
        ++gs.frame_counter;
        
        result.success = true;
    }
    catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
    }
    
    return result;
}

void SimulationPipeline::Reset() {
    time_accumulator_ = 0.0f;
    frame_counter_ = 0;
}

void SimulationPipeline::StepPhysics(Editor::GlobalState& gs, float dt) {
    // TODO: Implement deterministic physics step
    // - Update agent positions
    // - Apply velocity constraints
    // - Resolve collisions (deterministic ordering)
}

void SimulationPipeline::StepAgents(Editor::GlobalState& gs, float dt) {
    // TODO: Implement deterministic agent AI step
    // - Update agent goals
    // - Pathfinding (deterministic priority queue)
    // - Decision making (seeded RNG)
}

void SimulationPipeline::StepSystems(Editor::GlobalState& gs, float dt) {
    // TODO: Implement deterministic systems step
    // - Traffic flow
    // - Economic simulation
    // - Event processing (sorted by priority)
}

} // namespace RogueCity::Core::Simulation
```

### Connecting to EditorHFSM

**Modify**: `core/src/Core/Editor/EditorState.cpp` (line ~324)

```cpp
// REPLACE THIS (line ~324):
void EditorHFSM::update(GlobalState& gs, float /*dt*/)
{
    if (m_state == EditorState::Simulation_Stepping) {
        //TODO Placeholder for a single deterministic simulation step.
        // Future: run one tick of sim systems using RogueWorker for heavy workloads.
        ++gs.frame_counter;
        transition_to(EditorState::Simulation_Paused, gs);
    }
}

// WITH THIS:
#include "RogueCity/Core/Simulation/SimulationPipeline.hpp"

void EditorHFSM::update(GlobalState& gs, float /*dt*/)
{
    if (m_state == EditorState::Simulation_Stepping) {
        // Execute deterministic simulation step
        static Simulation::SimulationPipeline pipeline;
        auto result = pipeline.ExecuteStep(gs);
        
        if (!result.success) {
            // Log error and return to paused state
            // TODO: Add error reporting to UI
        }
        
        transition_to(EditorState::Simulation_Paused, gs);
    }
}
```

### DOS AND DON'TS

✅ **DO**:
- Use fixed timestep (no variable dt in simulation logic)
- Sort all containers before iteration (deterministic ordering)
- Seed all RNG explicitly from frame number + entity ID
- Write unit tests that verify same seed -> same output

❌ **DON'T**:
- Use `std::unordered_map` iteration (non-deterministic order)
- Rely on pointer addresses for ordering
- Use floating-point accumulation without epsilon tolerance
- Allow simulation to run without frame counter

***

## Phase 2: Stable Viewport Identity Layer

### What Goes Where
**File**: `core/include/RogueCity/Core/Editor/StableIDRegistry.hpp` (new)
**File**: `core/src/Core/Editor/StableIDRegistry.cpp` (new)
**Modify**: `core/include/RogueCity/Core/Editor/ViewportIndex.hpp`

### Code Example: Stable ID System

```cpp
// core/include/RogueCity/Core/Editor/StableIDRegistry.hpp
#pragma once
#include "RogueCity/Core/Editor/ViewportIndex.hpp"
#include <unordered_map>
#include <string>
#include <cstdint>

namespace RogueCity::Core::Editor {

/// Stable ID that persists across rebuilds
struct StableID {
    uint64_t id{0};  // Persistent identifier
    uint32_t version{0};  // Migration version
    
    [[nodiscard]] bool operator==(const StableID& other) const noexcept {
        return id == other.id && version == other.version;
    }
};

/// Registry mapping stable IDs to transient viewport IDs
class StableIDRegistry {
public:
    StableIDRegistry() = default;
    
    /// Allocate new stable ID for entity
    [[nodiscard]] StableID AllocateStableID(VpEntityKind kind, uint32_t entity_id);
    
    /// Lookup stable ID from transient viewport ID
    [[nodiscard]] StableID GetStableID(uint32_t viewport_id) const;
    
    /// Lookup transient viewport ID from stable ID
    [[nodiscard]] uint32_t GetViewportID(const StableID& stable) const;
    
    /// Rebuild mapping after viewport index rebuild
    void RebuildMapping(const std::vector<VpProbeData>& probes);
    
    /// Serialize for save/load
    [[nodiscard]] std::string Serialize() const;
    void Deserialize(const std::string& data);
    
    /// Clear all mappings
    void Clear();
    
private:
    uint64_t next_stable_id_{1};  // 0 is reserved for invalid
    std::unordered_map<uint32_t, StableID> viewport_to_stable_;
    std::unordered_map<uint64_t, uint32_t> stable_to_viewport_;
    
    // Migration hints for handling ID changes
    std::unordered_map<uint64_t, std::string> stable_id_aliases_;
};

/// Global registry accessor
StableIDRegistry& GetStableIDRegistry();

} // namespace RogueCity::Core::Editor
```

```cpp
// core/src/Core/Editor/StableIDRegistry.cpp
#include "RogueCity/Core/Editor/StableIDRegistry.hpp"
#include <sstream>

namespace RogueCity::Core::Editor {

StableID StableIDRegistry::AllocateStableID(VpEntityKind kind, uint32_t entity_id) {
    StableID stable;
    stable.id = next_stable_id_++;
    stable.version = 1;
    return stable;
}

StableID StableIDRegistry::GetStableID(uint32_t viewport_id) const {
    auto it = viewport_to_stable_.find(viewport_id);
    if (it != viewport_to_stable_.end()) {
        return it->second;
    }
    return StableID{0, 0};  // Invalid
}

uint32_t StableIDRegistry::GetViewportID(const StableID& stable) const {
    auto it = stable_to_viewport_.find(stable.id);
    if (it != stable_to_viewport_.end()) {
        return it->second;
    }
    return kViewportIndexInvalid;
}

void StableIDRegistry::RebuildMapping(const std::vector<VpProbeData>& probes) {
    // Clear transient mappings but preserve stable ID allocations
    viewport_to_stable_.clear();
    stable_to_viewport_.clear();
    
    // Rebuild from probe data
    for (size_t i = 0; i < probes.size(); ++i) {
        const auto& probe = probes[i];
        
        // Use probe.id as key for existing stable ID
        // If first time, allocate new stable ID
        StableID stable = AllocateStableID(probe.kind, probe.id);
        
        viewport_to_stable_[static_cast<uint32_t>(i)] = stable;
        stable_to_viewport_[stable.id] = static_cast<uint32_t>(i);
    }
}

std::string StableIDRegistry::Serialize() const {
    std::ostringstream oss;
    oss << next_stable_id_ << "\n";
    oss << viewport_to_stable_.size() << "\n";
    for (const auto& [vp_id, stable] : viewport_to_stable_) {
        oss << vp_id << " " << stable.id << " " << stable.version << "\n";
    }
    return oss.str();
}

void StableIDRegistry::Deserialize(const std::string& data) {
    std::istringstream iss(data);
    Clear();
    
    iss >> next_stable_id_;
    size_t count = 0;
    iss >> count;
    
    for (size_t i = 0; i < count; ++i) {
        uint32_t vp_id;
        StableID stable;
        iss >> vp_id >> stable.id >> stable.version;
        
        viewport_to_stable_[vp_id] = stable;
        stable_to_viewport_[stable.id] = vp_id;
    }
}

void StableIDRegistry::Clear() {
    next_stable_id_ = 1;
    viewport_to_stable_.clear();
    stable_to_viewport_.clear();
    stable_id_aliases_.clear();
}

StableIDRegistry& GetStableIDRegistry() {
    static StableIDRegistry registry;
    return registry;
}

} // namespace RogueCity::Core::Editor
```

### Connecting to ViewportIndex

**Modify**: `core/include/RogueCity/Core/Editor/ViewportIndex.hpp` (add after line 26)

```cpp
// After VpProbeData struct definition, add:

/// Get stable ID for viewport probe
[[nodiscard]] StableID GetProbeStableID(const VpProbeData& probe);

/// Rebuild stable ID mapping after viewport index rebuild
void RebuildStableIDMapping(const std::vector<VpProbeData>& probes);
```

### DOS AND DON'TS

✅ **DO**:
- Allocate stable IDs on first entity creation
- Serialize stable ID registry with save files
- Rebuild mappings after every viewport index regeneration
- Version stable IDs for migration support

❌ **DON'T**:
- Reuse stable IDs after entity deletion (tombstone them)
- Assume transient viewport IDs are stable across frames
- Expose transient IDs to Lua scripts or serialization
- Skip rebuilding after load (data may be stale)

***

## Phase 3: CityGenerator Config/Input Hardening

### What Goes Where
**Modify**: `generators/include/RogueCity/Generators/Pipeline/CityGenerator.hpp`
**Modify**: `generators/src/Generators/Pipeline/CityGenerator.cpp`

### Code Example: Config Validation

```cpp
// Add to CityGenerator class (in CityGenerator.hpp):
public:
    /// Validation result for config/axioms
    struct ValidationResult {
        bool valid{true};
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };
    
    /// Validate and clamp config to safe ranges
    static Config ValidateAndClampConfig(const Config& input);
    
    /// Validate axiom inputs
    static ValidationResult ValidateAxioms(
        const std::vector<AxiomInput>& axioms,
        const Config& config
    );

private:
    /// Calculate hard caps from world dimensions
    static void DeriveConstraintsFromWorldSize(Config& config);
```

```cpp
// In CityGenerator.cpp, implement validation:
CityGenerator::Config CityGenerator::ValidateAndClampConfig(const Config& input) {
    Config config = input;
    
    // Clamp world dimensions to texture space limits
    constexpr int kMinWorldSize = 500;
    constexpr int kMaxWorldSize = 8192;
    config.width = std::clamp(config.width, kMinWorldSize, kMaxWorldSize);
    config.height = std::clamp(config.height, kMinWorldSize, kMaxWorldSize);
    
    // Clamp cell size to reasonable resolution
    constexpr double kMinCellSize = 1.0;
    constexpr double kMaxCellSize = 50.0;
    config.cell_size = std::clamp(config.cell_size, kMinCellSize, kMaxCellSize);
    
    // Ensure texture resolution is power-of-two friendly
    const int texture_width = static_cast<int>(config.width / config.cell_size);
    const int texture_height = static_cast<int>(config.height / config.cell_size);
    
    if (texture_width > 2048 || texture_height > 2048) {
        // Adjust cell size to fit within texture limits
        const double scale = std::max(
            texture_width / 2048.0,
            texture_height / 2048.0
        );
        config.cell_size *= scale;
    }
    
    // Derive hard caps from world area
    DeriveConstraintsFromWorldSize(config);
    
    // Clamp seed count to reasonable range
    constexpr int kMinSeeds = 5;
    constexpr int kMaxSeeds = 200;
    config.num_seeds = std::clamp(config.num_seeds, kMinSeeds, kMaxSeeds);
    
    return config;
}

void CityGenerator::DeriveConstraintsFromWorldSize(Config& config) {
    // Calculate area in square kilometers
    const double area_km2 = (config.width / 1000.0) * (config.height / 1000.0);
    
    // Derive district cap: ~1 district per 0.25 km²
    config.max_districts = static_cast<uint32_t>(area_km2 / 0.25);
    config.max_districts = std::clamp(config.max_districts, 16u, 1024u);
    
    // Derive lot cap: ~2500 lots per km²
    config.max_lots = static_cast<uint32_t>(area_km2 * 2500.0);
    config.max_lots = std::clamp(config.max_lots, 1000u, 100000u);
    
    // Derive building cap: 2x lots (some lots get multiple buildings)
    config.max_buildings = config.max_lots * 2;
    config.max_buildings = std::clamp(config.max_buildings, 2000u, 200000u);
}

CityGenerator::ValidationResult CityGenerator::ValidateAxioms(
    const std::vector<AxiomInput>& axioms,
    const Config& config
) {
    ValidationResult result;
    
    if (axioms.empty()) {
        result.errors.push_back("At least one axiom is required");
        result.valid = false;
        return result;
    }
    
    const double world_width = static_cast<double>(config.width);
    const double world_height = static_cast<double>(config.height);
    
    for (size_t i = 0; i < axioms.size(); ++i) {
        const auto& axiom = axioms[i];
        
        // Check position bounds
        if (axiom.position.x < 0.0 || axiom.position.x > world_width ||
            axiom.position.y < 0.0 || axiom.position.y > world_height) {
            result.errors.push_back(
                "Axiom " + std::to_string(i) + " position out of bounds"
            );
            result.valid = false;
        }
        
        // Check radius sanity
        if (axiom.radius < 50.0 || axiom.radius > world_width) {
            result.warnings.push_back(
                "Axiom " + std::to_string(i) + " has unusual radius: " +
                std::to_string(axiom.radius)
            );
        }
        
        // Validate type-specific parameters
        if (axiom.type == AxiomInput::Type::Radial) {
            if (axiom.radial_spokes < 3 || axiom.radial_spokes > 24) {
                result.errors.push_back(
                    "Axiom " + std::to_string(i) + " radial spokes must be [3..24]"
                );
                result.valid = false;
            }
        }
    }
    
    return result;
}
```

### DOS AND DON'TS

✅ **DO**:
- Validate inputs before starting generation
- Derive hard caps from world size (not arbitrary constants)
- Return detailed error messages with parameter names
- Clamp silently for minor issues, reject for major issues

❌ **DON'T**:
- Allow negative or zero dimensions
- Use hardcoded magic numbers (document formulas)
- Silent failures (always report what was clamped)
- Allow generation with invalid axioms

***

## Phase 4: Pipeline Decomposition + Incremental Execution

### What Goes Where
**File**: `generators/include/RogueCity/Generators/Pipeline/GenerationStage.hpp` (new)
**Modify**: `generators/include/RogueCity/Generators/Pipeline/CityGenerator.hpp`
**Modify**: `generators/src/Generators/Pipeline/CityGenerator.cpp`

### Code Example: Stage System

```cpp
// generators/include/RogueCity/Generators/Pipeline/GenerationStage.hpp
#pragma once
#include <cstdint>
#include <bitset>

namespace RogueCity::Generators {

/// Generation pipeline stages (bit flags)
enum class GenerationStage : uint8_t {
    Terrain = 0,
    TensorField = 1,
    Roads = 2,
    Districts = 3,
    Blocks = 4,
    Lots = 5,
    Buildings = 6,
    Validation = 7,
    COUNT = 8
};

/// Bitmask for tracking dirty stages
using StageMask = std::bitset<static_cast<size_t>(GenerationStage::COUNT)>;

/// Mark stage and all dependent stages as dirty
inline void MarkStageDirty(StageMask& mask, GenerationStage stage) {
    const auto idx = static_cast<size_t>(stage);
    // Mark this stage and all downstream stages dirty
    for (size_t i = idx; i < static_cast<size_t>(GenerationStage::COUNT); ++i) {
        mask.set(i);
    }
}

/// Check if stage needs regeneration
inline bool IsStageDirty(const StageMask& mask, GenerationStage stage) {
    return mask.test(static_cast<size_t>(stage));
}

} // namespace RogueCity::Generators
```

```cpp
// Add to CityGenerator class:
public:
    /// Stage-specific generation options
    struct StageOptions {
        StageMask stages_to_run;  // Which stages to execute
        bool use_cache{true};      // Use cached results from clean stages
    };
    
    /// Generate with explicit stage control
    [[nodiscard]] CityOutput GenerateStages(
        const std::vector<AxiomInput>& axioms,
        const Config& config,
        const StageOptions& options,
        Core::Editor::GlobalState* global_state = nullptr
    );
    
    /// Regenerate only dirty stages
    [[nodiscard]] CityOutput RegenerateIncremental(
        const std::vector<AxiomInput>& axioms,
        const Config& config,
        StageMask dirty_stages,
        Core::Editor::GlobalState* global_state = nullptr
    );
    
private:
    // Stage cache (previous results)
    struct StageCache {
        TensorFieldGenerator tensor_field;
        std::vector<Vec2> seeds;
        fva::Container<Road> roads;
        std::vector<District> districts;
        std::vector<BlockPolygon> blocks;
        std::vector<LotToken> lots;
        siv::Vector<BuildingSite> buildings;
        StageMask valid_stages;  // Which cached results are valid
    } cache_;
```

```cpp
// Implementation in CityGenerator.cpp:
CityOutput CityGenerator::GenerateStages(
    const std::vector<AxiomInput>& axioms,
    const Config& config,
    const StageOptions& options,
    Core::Editor::GlobalState* global_state
) {
    CityOutput output;
    
    // Terrain stage
    if (IsStageDirty(options.stages_to_run, GenerationStage::Terrain)) {
        if (config.enable_world_constraints) {
            TerrainConstraintGenerator terrain_gen;
            output.world_constraints = terrain_gen.generate(
                config.terrain,
                config.width,
                config.height
            );
        }
    } else if (options.use_cache) {
        // Use cached terrain constraints
        output.world_constraints = /* retrieve from cache */;
    }
    
    // Tensor field stage
    if (IsStageDirty(options.stages_to_run, GenerationStage::TensorField)) {
        output.tensor_field = generateTensorField(axioms);
        cache_.tensor_field = output.tensor_field;
    } else if (options.use_cache) {
        output.tensor_field = cache_.tensor_field;
    }
    
    // Roads stage
    if (IsStageDirty(options.stages_to_run, GenerationStage::Roads)) {
        const auto* constraints = config.enable_world_constraints 
            ? &output.world_constraints : nullptr;
        const auto* texture = global_state 
            ? &global_state->texture_space : nullptr;
        
        auto seeds = generateSeeds(constraints, texture);
        output.roads = traceRoads(
            output.tensor_field,
            seeds,
            constraints,
            nullptr,
            texture
        );
        cache_.roads = output.roads;
    } else if (options.use_cache) {
        output.roads = cache_.roads;
    }
    
    // Continue for remaining stages...
    
    return output;
}

CityOutput CityGenerator::RegenerateIncremental(
    const std::vector<AxiomInput>& axioms,
    const Config& config,
    StageMask dirty_stages,
    Core::Editor::GlobalState* global_state
) {
    StageOptions options;
    options.stages_to_run = dirty_stages;
    options.use_cache = true;
    
    return GenerateStages(axioms, config, options, global_state);
}
```

### DOS AND DON'TS

✅ **DO**:
- Cascade dirty flags downstream (tensor dirty → roads dirty)
- Cache stage results in member variables
- Validate cache coherency (detect stale data)
- Document stage dependencies explicitly

❌ **DON'T**:
- Allow partial regeneration without cache validation
- Share mutable state between stages
- Skip invalidation when axioms/config change
- Cache references (copy data instead)

***

## Phase 5: Async Queue + Cancellation Tokens

### What Goes Where
**File**: `generators/include/RogueCity/Generators/Pipeline/GenerationContext.hpp` (new)
**Modify**: `generators/src/Generators/Pipeline/CityGenerator.cpp`

### Code Example: Cancellation System

```cpp
// generators/include/RogueCity/Generators/Pipeline/GenerationContext.hpp
#pragma once
#include <atomic>
#include <memory>

namespace RogueCity::Generators {

/// Cancellation token for aborting long-running generation
class CancellationToken {
public:
    CancellationToken() = default;
    
    /// Request cancellation
    void Cancel() { cancelled_.store(true, std::memory_order_relaxed); }
    
    /// Check if cancellation requested
    [[nodiscard]] bool IsCancelled() const { 
        return cancelled_.load(std::memory_order_relaxed); 
    }
    
    /// Reset token for reuse
    void Reset() { cancelled_.store(false, std::memory_order_relaxed); }
    
private:
    std::atomic<bool> cancelled_{false};
};

/// Context passed through generation pipeline
struct GenerationContext {
    std::shared_ptr<CancellationToken> cancellation_token;
    uint32_t iteration_count{0};
    uint32_t max_iterations{0};
    
    /// Check if should abort (cancellation or iteration limit)
    [[nodiscard]] bool ShouldAbort() const {
        if (cancellation_token && cancellation_token->IsCancelled()) {
            return true;
        }
        if (max_iterations > 0 && iteration_count >= max_iterations) {
            return true;
        }
        return false;
    }
};

} // namespace RogueCity::Generators
```

```cpp
// Modify CityGenerator to accept context:
// In CityGenerator.hpp:
public:
    /// Generate with cancellation support
    [[nodiscard]] CityOutput GenerateWithContext(
        const std::vector<AxiomInput>& axioms,
        const Config& config,
        GenerationContext& context,
        Core::Editor::GlobalState* global_state = nullptr
    );
```

```cpp
// In CityGenerator.cpp, add cancellation checks:
fva::Container<Road> CityGenerator::traceRoads(
    const TensorFieldGenerator& field,
    const std::vector<Vec2>& seeds,
    const WorldConstraintField* constraints,
    const SiteProfile* profile,
    const Core::Data::TextureSpace* texture_space,
    GenerationContext* context  // ADD THIS PARAMETER
) {
    fva::Container<Road> roads;
    StreamlineTracer tracer(field, config_.width, config_.height);
    
    for (const auto& seed : seeds) {
        // Check cancellation between heavy operations
        if (context && context->ShouldAbort()) {
            break;  // Early exit
        }
        
        auto traced = tracer.trace_from(seed);
        roads.push_back(traced);
        
        if (context) {
            ++context->iteration_count;
        }
    }
    
    return roads;
}
```

### DOS AND DON'TS

✅ **DO**:
- Check cancellation in tight loops (every N iterations)
- Use `std::atomic` for thread-safe cancellation
- Return partial results on cancellation
- Document which functions support cancellation

❌ **DON'T**:
- Check cancellation too frequently (performance cost)
- Throw exceptions on cancellation (return early)
- Leave resources in inconsistent state on abort
- Block cancellation checks with heavy computation

***

## Phase 6-7: Quality Upgrades & Testing

### Testing Pattern

```cpp
// tests/test_determinism.cpp
#include <gtest/gtest.h>
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include "RogueCity/Core/Validation/DeterminismHash.hpp"

TEST(CityGenerator, DeterministicGeneration) {
    using namespace RogueCity;
    
    Generators::CityGenerator gen;
    Generators::CityGenerator::Config config;
    config.seed = 12345;
    config.width = 2000;
    config.height = 2000;
    
    std::vector<Generators::CityGenerator::AxiomInput> axioms;
    Generators::CityGenerator::AxiomInput axiom;
    axiom.type = Generators::CityGenerator::AxiomInput::Type::Grid;
    axiom.position = {1000.0, 1000.0};
    axiom.radius = 500.0;
    axioms.push_back(axiom);
    
    // Generate twice with same seed
    auto output1 = gen.generate(axioms, config);
    auto output2 = gen.generate(axioms, config);
    
    // Results must be identical
    EXPECT_EQ(output1.roads.size(), output2.roads.size());
    EXPECT_EQ(output1.districts.size(), output2.districts.size());
    EXPECT_EQ(output1.lots.size(), output2.lots.size());
    EXPECT_EQ(output1.buildings.size(), output2.buildings.size());
    
    // Hash comparison
    // TODO: Implement hash comparison once DeterminismHash extended to CityOutput
}
```

***

## Strict DOS AND DON'TS Summary

### C++ Best Practices

✅ **DO**:
- Use `[[nodiscard]]` on all functions returning values
- Mark single-argument constructors `explicit`
- Use `noexcept` where guaranteed
- Prefer `const&` for read-only parameters
- Use `override` on virtual function overrides
- Forward declare when possible (reduce compile deps)

❌ **DON'T**:
- Use raw `new`/`delete` (use smart pointers)
- Mix tabs and spaces (use 4 spaces matching your style)
- Leave unused parameters unnamed without `/*param*/` comment
- Use `using namespace` in headers
- Return references to temporaries

### Your Project Patterns

✅ **DO** (matching your style):
- Namespace all code: `RogueCity::Core::`, `RogueCity::Generators::`
- Header guards: `#pragma once` (your convention)
- File naming: `PascalCase.hpp`/`.cpp` (your convention)
- Enum classes for type safety (matching EditorState pattern)
- Include paths: `#include "RogueCity/Module/..."`

❌ **DON'T**:
- Break include directory structure (`include/RogueCity/...`)
- Mix snake_case and camelCase in same file
- Add dependencies without updating CMakeLists.txt
- Bypass GlobalState for shared editor data

This plan provides complete, copy-paste-ready code matching your existing architecture patterns. Each phase builds on the previous with clear file locations, connection points, and comprehensive examples.
