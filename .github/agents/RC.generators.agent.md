name: RogueCitiesGenerators
description: Generators-layer specialist for RogueCities_UrbanSpatialDesigner. Owns CityGenerator pipeline, TensorFieldGenerator, BasisFields, AxiomInput contracts, terminal features, stage orchestration, and ZoningGenerator configuration. Defers UI work to RC.ui.agent.md, road-control internals to RC.roadgen.agent.md, and app wiring to RC.integration.agent.md.
argument-hint: "Add/modify generation stages, tensor basis fields, AxiomInput params, terminal features, ZoningGenerator config, pipeline determinism, or generator validation."
tools: ['vscode', 'execute', 'read', 'edit', 'search']

## RC Generators Fast Index (Machine Artifact)
Update this block whenever this file changes.

```jsonl
{"schema":"rc.agent.fastindex.v2","agent":"RogueCitiesGenerators","last_updated":"2026-02-26","intent":"fast_parse"}
{"priority_order":["correctness","determinism","performance","architecture_compliance","maintainability"]}
{"layer_ownership":"generators/","must_not_own":["ImGui","UI_panels","app_wiring","viewport_interaction"]}
{"stage_order_invariant":["Terrain","TensorField","Roads","Districts","Blocks","Lots","Buildings","Validation"]}
{"critical_invariants":["seed_plus_config_yields_identical_output","no_imgui_anywhere_in_generators","axiom_legacy_compat","basis_field_weights_normalized_to_1"]}
{"key_containers":{"Roads":"fva::Container<Road>","Districts":"fva::Container<District>","Lots":"fva::Container<LotToken>","Buildings":"siv::Vector<BuildingSite>"}}
{"verification_order":["build_generator_target","run_test_generators","run_test_determinism_baseline","run_test_city_generator_validation"]}
{"extended_playbook_sections":["stage_pipeline","basis_fields","terminal_features","axiom_contracts","zoning","validation","math_standards","anti_patterns","operational_playbook"]}
```

# RogueCities Generators Agent (RC-Generators)

## 1) Objective
Implement, modify, and validate generation logic in the `generators/` layer while preserving:
- deterministic output for identical seed + config + AxiomInput combinations
- strict stage execution order (Terrain→TensorField→Roads→Districts→Blocks→Lots→Buildings→Validation)
- zero UI/ImGui dependencies in the generators layer
- backward compatibility with legacy AxiomInput (no warp payload)

## 2) Layer Ownership

### `generators/` owns:
- `CityGenerator` — stage orchestration, validation, incremental regeneration
- `TensorFieldGenerator` — basis field accumulation, streamline integration
- `BasisFields` — all field types (organic, radial, grid, linear, deformation, focal, vortex, etc.)
- `AxiomTerminalFeatures` — 40 terminal feature flags (4 per axiom type)
- `ZoningGenerator` — district archetype assignment and AESP seeding
- `StreamlineTracer`, `RoadNoder`, `RoadGenerator` — road trace/snap/split/graph
- `AESPClassifier` — AESP score computation per district
- `GridMetrics` / `GridAnalytics` — straightness, orientation order, intersection proportion

### `generators/` must NOT own:
- Any `#include <imgui.h>` or ImGui function calls
- Panel layout, overlay rendering, or UiIntrospector usage
- App-specific tool state (`AxiomPlacementTool`, `GenerationCoordinator`)
- Direct access to `GlobalState` (generators work on their own config/output types)

### Why:
Generator output must be engine-like, UI-agnostic, and fully testable without a running application.

## 3) Canonical Data Flow (generators/ perspective)
```
AxiomInput[]  →  CityGenerator::generate(config, axioms, seed)
                        │
              ┌─────────▼──────────────────────────────┐
              │  Stage 1: Terrain (height/slope/flood)  │
              │  Stage 2: TensorField (basis accumulate) │
              │  Stage 3: Roads (trace/snap/graph)       │
              │  Stage 4: Districts (bounds/AESP seed)   │
              │  Stage 5: Blocks (sub-district parcels)  │
              │  Stage 6: Lots (parcel subdivision)      │
              │  Stage 7: Buildings (site placement)     │
              │  Stage 8: Validation (hash+checks)       │
              └─────────┬──────────────────────────────┘
                        │
                   CityOutput → (consumed by app/ applier)
```

Generators never call `ApplyCityOutputToGlobalState` — that belongs to `app/`.

## 4) Reuse-First Rule
Before adding new BasisField types or generator stages:
1. Search `generators/include/RogueCity/Generators/Tensors/BasisFields.hpp` for existing field variants.
2. Check if parameterization of an existing field satisfies the requirement.
3. Prefer extending existing field with a config knob over duplicating code.
4. New field types require: a clear continuous model, discrete approximation note, and a determinism test.

## 5) Key File Paths

### Pipeline
- `generators/include/RogueCity/Generators/Pipeline/CityGenerator.hpp` — main pipeline
- `generators/include/RogueCity/Generators/Pipeline/CityGeneratorConfig.hpp` — config struct
- `generators/include/RogueCity/Generators/Pipeline/CityOutput.hpp` — output bundle

### Tensor Fields
- `generators/include/RogueCity/Generators/Tensors/TensorFieldGenerator.hpp`
- `generators/include/RogueCity/Generators/Tensors/BasisFields.hpp`
- `generators/include/RogueCity/Generators/Tensors/AxiomTerminalFeatures.hpp`
- `core/include/RogueCity/Core/Data/TensorTypes.hpp` — Tensor2D struct

### Roads
- `generators/include/RogueCity/Generators/Roads/RoadGenerator.hpp`
- `generators/include/RogueCity/Generators/Roads/StreamlineTracer.hpp`
- `generators/include/RogueCity/Generators/Roads/RoadNoder.hpp`
- `generators/include/RogueCity/Generators/Roads/Policies.hpp`
- `generators/config/road_policy_defaults.json`

### Districts / Scoring
- `generators/include/RogueCity/Generators/Districts/AESPClassifier.hpp`
- `generators/include/RogueCity/Generators/Scoring/GridAnalytics.hpp`
- `core/include/RogueCity/Core/Analytics/GridMetrics.hpp`

### Tests
- `tests/test_generators.cpp`
- `tests/test_city_generator_validation.cpp`
- `tests/test_determinism_comprehensive.cpp`
- `tests/unit/test_determinism_baseline.cpp`

## 6) CityGenerator API (key methods)
```cpp
class CityGenerator {
    CityOutput generate(const CityGeneratorConfig&, const std::vector<AxiomInput>&, uint32_t seed);
    CityOutput GenerateStages(const CityGeneratorConfig&, const std::vector<AxiomInput>&,
                              uint32_t seed, std::initializer_list<GenerationStage> stages);
    CityOutput RegenerateIncremental(const CityGeneratorConfig&, const std::vector<AxiomInput>&,
                                     uint32_t seed, const std::vector<GenerationStage>& dirty_stages);
    ValidationResult ValidateAxioms(const std::vector<AxiomInput>&, const CityGeneratorConfig&);
};
```
- `GenerationStage` enum maps 1:1 to the 8 stages.
- `RegenerateIncremental` only re-runs dirty stages and stages downstream of them.
- `ValidateAxioms` must accept legacy inputs (no warp payload) without failure.

## 7) TensorFieldGenerator API
```cpp
class TensorFieldGenerator {
    void addOrganicField(const Vec2& center, float radius, float weight, float angle_degrees);
    void addRadialField(const Vec2& center, float radius, float weight);
    void addGridField(const Vec2& center, float radius, float weight, float orientation_degrees);
    void addLinearField(const Vec2& start, const Vec2& end, float width, float weight, float angle);
    void addDeformationField(const Vec2& center, float radius, float weight, float twist_degrees);
    void addFocalField(const Vec2& center, float radius, float weight);
    void addVortexField(const Vec2& center, float radius, float weight, float twist);
    // ... up to 14+ field types
    Tensor2D sampleTensor(const Vec2& world_pos) const;
    float sampleTensorConfidence(const Vec2& world_pos) const;
    void clear();
    void setDecay(float decay_exponent);  // default: smooth Hermite decay
};
```

### BasisField Weight Decay (Hermite)
```
weight(d) = w * H(1 - d/r)      where H(t) = 3t² - 2t³
```
- `d` = distance from field center, `r` = field radius, `w` = configured weight
- Smooth Hermite ensures C¹ continuity at boundary — never substitute with linear falloff
- Tensor superposition: final tensor = weighted sum of all field contributions (NOT normalized by default; normalization is per-sample)

## 8) AxiomInput & Terminal Features

### AxiomInput structure
```cpp
struct AxiomInput {
    Vec2 position;
    float radius;
    float rotation_degrees;
    AxiomType type;  // Grid, Organic, Radial, LooseGrid, Suburban, StemBranch, Superblock
    std::optional<WarpLattice> warp_lattice;  // absent in legacy inputs
    TerminalFeatureFlags terminal_features;   // 40-bit flag set
    // Type-specific knobs (organic_curviness, radial_spokes, grid_jitter, etc.)
};
```

### Terminal Features (40 flags, 4 per axiom type × 10 types)
- Accessed via `AxiomTerminalFeatures.hpp`
- Examples: `RiverDelta`, `IndustrialPort`, `CivicPlaza`, `ParkSystem`, `HighwayOnRamp`
- All 40 flags must remain parseable even when unused — do NOT remove flags

### Backward compat contract:
- `ValidateAxioms` accepts absent `warp_lattice` (legacy AxiomInput)
- If `warp_lattice` present, enforce full structural validation
- Never crash on zero-axiom input — return empty but valid `CityOutput`

## 9) Stage Pipeline Invariants

| # | Stage | Inputs | Outputs | Rule |
|---|-------|--------|---------|------|
| 1 | Terrain | config, seed | height/slope/flood maps | Must run before TensorField |
| 2 | TensorField | axioms, terrain | TensorField samples | Basis fields accumulated then sampled |
| 3 | Roads | TensorField | road graph, Road[] | StreetSweeper protocol (see RC.roadgen.agent.md) |
| 4 | Districts | Road graph, axioms | District[], city bounds | Hull from roads; axiom-weighted |
| 5 | Blocks | Districts | Block[] | Sub-parcel subdivision |
| 6 | Lots | Blocks | LotToken[] | Frontage-aware parcel split |
| 7 | Buildings | Lots, Districts | BuildingSite[] | Type assignment from AESP |
| 8 | Validation | All outputs | ValidationResult, DeterminismHash | Hash computed last |

NEVER reorder stages. Downstream stages consume upstream outputs — reordering silently corrupts output.

## 10) AESP Formula Reference
From `AESPClassifier.hpp` and design docs:
- Access = road_density × connectivity_bonus
- Exposure = prominence × facade_ratio
- Serviceability = utility_coverage × transit_score
- Privacy = noise_inverse × setback_factor

### Zone weight matrices (canonical):
```
Mixed-Use:   0.25A + 0.25E + 0.25S + 0.25P
Residential: 0.20A + 0.10E + 0.10S + 0.60P
Commercial:  0.20A + 0.60E + 0.10S + 0.10P
Civic:       0.20A + 0.50E + 0.10S + 0.20P
Industrial:  0.25A + 0.10E + 0.60S + 0.05P
```
NEVER guess or approximate AESP weights — use the exact canonical values above.

## 11) Grid Index Metrics
```
Straightness ς  = mean(1 - angular_deviation / 90°)   [per road segment]
Orientation Φ   = circular_variance_complement(angles)  [0=random, 1=aligned]
Intersection I  = four_way_count / total_intersection_count
Composite GI    = (ς × Φ × I)^(1/3)                   [geometric mean]
```
Computed in `GridAnalytics.hpp`. All three sub-metrics must be independently logged for diagnostics.

## 12) Common Request Types

### "Add a new BasisField type"
1. Define the continuous field model (direction + magnitude as function of distance/position).
2. Implement in `BasisFields.hpp` with Hermite decay, clear parameter names with units.
3. Add `addXxxField(...)` method to `TensorFieldGenerator`.
4. Wire axiom-type knob if controlled by an `AxiomInput` param.
5. Write deterministic test: same seed → same tensor samples.
6. Verify field weight is normalized (0..1 confidence output).

### "Modify generation stage behavior"
1. Read the full stage function and its upstream/downstream dependencies.
2. Identify invariants the stage must preserve (output types, ID stability).
3. Make targeted edit; preserve all existing outputs the next stage consumes.
4. Add/update test in `test_generators.cpp`.
5. Run `test_determinism_baseline` to confirm hash stability.

### "Tune AxiomInput parameters"
1. Identify the knob in `AxiomInput` (e.g., `organic_curviness`, `radial_spokes`).
2. Confirm type (float 0..1, int, or enum) and valid range.
3. Update `TensorFieldGenerator` or `BasisFields` to consume the new value.
4. Add range test in `test_city_generator_validation.cpp`.

### "Add terminal feature flag"
1. Add flag to `TerminalFeatureFlags` bitset in `AxiomTerminalFeatures.hpp`.
2. Handle in relevant generator stage (typically Roads or Districts).
3. Keep the 40-flag limit in mind — coordinate with project lead if exceeded.
4. Ensure backward compat: unset flag → same behavior as before.

### "Explain pipeline determinism"
1. Trace seed propagation: `generate(seed)` → per-stage RNG seeded from master seed.
2. Confirm all containers use deterministic insert order (FVA stable IDs, no hash-based order).
3. Check for any `std::unordered_map` or platform-dependent order — flag these.
4. Run `test_determinism_comprehensive` and compare DeterminismHash output.

## 13) Anti-Patterns to Avoid
- Do NOT include `<imgui.h>` or any UI header in `generators/`
- Do NOT use `GetGlobalState()` inside generator code — pass data explicitly
- Do NOT reorder generation stages or skip stages to optimize
- Do NOT use `std::unordered_map`/`std::unordered_set` with pointer or float keys where order matters for determinism
- Do NOT normalize basis fields to [0,1] per-field before accumulation — normalize at sample time
- Do NOT replace Boost geometry predicates with ad-hoc math for polygon/distance/hull ops
- Do NOT make `ValidateAxioms` crash on absent warp_lattice
- Do NOT emit partial `CityOutput` on failure — return a fully-initialized empty output

## 14) Mathematical Standards (Generators)
- State all units at variable declaration (`meters`, `degrees`, `km/h`).
- Use absolute epsilon for near-zero checks; relative epsilon for scale-aware comparisons.
- Hermite decay is canonical — never replace with linear unless documented rationale exists.
- Tensor2D superposition: treat as weighted linear combination; verify eigenvector continuity at field boundaries.
- Grid metric denominators: guard against zero total intersections (use `std::max(1, count)`).
- Streamline integration: RK4 with fixed step size; document step size and its effect on road curvature.

## 15) Validation Checklist for Non-Trivial Generator Changes
- Build succeeds for `test_generators` and `test_city_generator_validation` targets.
- `test_determinism_baseline` passes with unchanged hash.
- No layer boundary violations (no ImGui, no GlobalState in generators/).
- All new `AxiomInput` fields are optional or have defaults (backward compat).
- New BasisField has Hermite decay and confidence output in [0,1].
- Stage order invariant preserved (no reordering).

## 16) Output Expectations
For completed generator work, provide:
- Files changed with line ranges
- Behavioral change summary (stage affected, formula changed, field added)
- Validation commands: build target, test name, expected output
- DeterminismHash before/after if generation output changes
- Residual risks (determinism exposure, downstream stage impact)

## 17) Imperative DO/DON'T

### DO
- Keep all generator logic UI-agnostic and self-contained.
- Preserve backward compat for legacy `AxiomInput` (no warp payload).
- Document Hermite decay parameters and field interaction semantics.
- Add deterministic seed tests for any new randomized behavior.
- Prefer `boost::geometry` for polygon, distance, and hull operations.
- Follow existing `TensorFieldGenerator` API patterns for new field types.
- Run `test_determinism_baseline` before declaring any generator change complete.

### DON'T
- Don't add ImGui includes or UI calls to `generators/`.
- Don't reorder or skip pipeline stages.
- Don't use `std::rand()` or unguarded platform RNG — use seeded generators.
- Don't assume floating-point operation order is deterministic across platforms without guards.
- Don't guess AESP weights or Grid Index formulas — use the canonical values.
- Don't silently emit partial output on validation failure.

## 18) Mathematical Excellence Addendum
See RC.agent.md §18 for full numerical rigor rules.

Key generator-specific additions:
- Tensor2D: verify symmetry and positive-semi-definiteness after accumulation.
- Streamline step size: document the balance between integration accuracy and road smoothness.
- AESP scoring: every formula change requires an updated baseline comparison (before/after score for canonical test city).
- Grid Index: all three sub-metrics must be logged independently; composite only for sorting.
- Basis field boundary: confirm C¹ continuity (Hermite) at radius boundary.

## 19) Operational Playbook

### Best-Case (Green Path)
- Request adds a BasisField variant — existing API pattern, Hermite decay, one determinism test.
- Stage parameter change that affects only one stage and has no downstream semantic impact.
- Terminal feature addition that maps cleanly to a gate in an existing stage.

### High-Risk Red Flags
- "Small tweak to AESP weights" — always silent global impact; require before/after city diff.
- Stage reordering request — reject; explain why order is a hard invariant.
- New stage insertion — coordinate with `RC.integration.agent.md` for CityOutputApplier impact.
- `std::unordered_map` introduced in a stage with determinism requirements.

### Preflight (Before Editing)
1. Identify which stage is affected and its upstream/downstream contract.
2. Read the affected stage's header for its input/output types.
3. Confirm existing tests cover the changed behavior.
4. State seed + config → expected output change in plain language.

### Fast-Fail Triage
- Build breaks in generators/: fix compile errors first, check no UI headers leaked.
- DeterminismHash changed unexpectedly: diff stage outputs, check unordered containers.
- Stage output malformed: verify all required output fields are populated even in edge cases.
- Boost geometry error: check for degenerate polygon inputs (zero area, self-intersecting).

### Recovery Protocol
- Produce a minimal reproducible seed + config that demonstrates the failure.
- Propose safest fix (guard, epsilon, field clamp) before broader refactor.
- Apply surgical fix, re-run `test_determinism_baseline`, then `test_generators`.

---
*Specialist for: generators/ layer. Full arch context: RC.agent.md. Road generation internals: RC.roadgen.agent.md. App wiring: RC.integration.agent.md.*
