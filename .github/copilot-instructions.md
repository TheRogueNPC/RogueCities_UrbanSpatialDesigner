# Copilot Instructions for RogueCities_UrbanSpatialDesigner

Last updated: 2026-02-21

## 1) Primary Rule
Prioritize correctness and architecture compliance over speed.  
If a change conflicts with module boundaries, stop and refactor the approach.

## 1.1) Reuse-First Rule
- Use existing APIs, data contracts, and subsystem patterns before introducing new abstractions.
- Add new types/helpers only when existing code cannot satisfy correctness/performance requirements.
- Keep edits surgical; avoid parallel systems that duplicate existing behavior.

## 2) Architecture Map (Source of Truth)
### `core/`
- Owns: domain data, math types, editor state containers.
- Never add: ImGui/UI logic, generation algorithms.

### `generators/`
- Owns: procedural generation algorithms and pipeline orchestration (`CityGenerator`).
- Never add: ImGui/UI code or panel behavior.

### `app/`
- Owns: tool logic, bridge logic, output application, preview coordination.
- Bridges user edits to generator contracts.

### `visualizer/`
- Owns: ImGui panels, viewport interaction, overlays, HUD/chrome.
- Renders state; does not implement generation algorithms.

## 2.1) Container and Library Standards
### FVA/CIV/SIV usage
- `fva::Container<T>` (FVA): editor-facing entities needing stable external handles under high mutation.
- `civ::IndexVector<T>` (CIV): internal high-performance/scratch sets; avoid exposing CIV internals directly as UI-facing handles.
- `siv::Vector<T>` (SIV): long-lived references requiring validity-checked handles.

### HFSM ownership
- HFSM (occasionally written HSFM) state semantics belong to core/app orchestration.
- Visualizer reacts to HFSM state; it does not own HFSM transition policy.

### Boost guidance
- Prefer `boost::geometry` for polygon, distance, point-in-polygon, hull, and correction operations.
- Prefer `boost::polygon::voronoi` where graph extraction already uses that family of algorithms.
- Do not replace robust Boost-based paths with ad-hoc geometry without clear justification.

## 3) Current Contracts You Must Preserve
### Axiom lattice
- Axioms are represented with `ControlLattice` in `AxiomVisual`.
- `AxiomVisual::to_axiom_input()` serializes lattice into `AxiomInput::warp_lattice`.

### Backward compatibility
- Legacy generator inputs may omit lattice payload.
- `CityGenerator::ValidateAxioms` must only apply strict lattice checks when warp payload is present.

### Foundation boundary
- Use generator-emitted `city_boundary` whenever provided.
- Fallback to derived bounds only when no boundary exists.

### Compass/HUD behavior
- Compass is parentable to Scene Stats.
- Compass drag can emit requested yaw; panel applies that yaw to camera.

## 4) Where to Put Changes
- Data model changes: `core/include/...` and corresponding serializers/hash paths.
- Generation logic changes: `generators/src/...`.
- Input mapping and output merging: `app/src/Integration/...`.
- Viewport/UI behavior: `visualizer/src/ui/...`.

Do not implement policy logic in the wrong layer.

## 5) Commenting Standard
Write comments for:
- why this logic exists
- invariants/ranges/units
- compatibility and determinism constraints

Do not:
- restate obvious code
- leave vague TODOs
- describe UI behavior inside generator internals unless it affects contracts

Preferred format:
- short rationale + invariant
- explicit units (`meters`, `cell_size`, `seconds`)
- explicit compatibility note when preserving old behavior

## 6) Knowledge Gathering Before Editing
1. Read target header(s) first to understand contracts.
2. Trace call path using search from entrypoint to consumers.
3. Inspect nearest tests before changing behavior.
4. Prefer existing patterns over introducing new ones.
5. Verify container/library conventions already used in the touched subsystem.

## 7) Safe Assumptions
- Determinism is expected for same seed/config.
- Core and generator layers stay UI-free.
- Existing lock semantics (`generation_locked`, source-based preservation) must be preserved.
- Feature flags are usually default ON unless explicitly requested otherwise.
- Existing Boost geometry + FVA/CIV/SIV patterns are the default baseline.

## 8) Ask Questions Before Proceeding If
- behavior change is ambiguous (bug vs intended)
- contract changes affect multiple layers
- change can break save/load compatibility
- change can alter determinism or stage ordering
- requirements conflict (e.g., strict validation vs legacy input support)
- a change would replace existing FVA/CIV/SIV strategy in a stable subsystem
- a change would replace a Boost geometry path with custom math

## 9) Build/Test Expectations
For non-trivial changes:
1. Build affected targets.
2. Run targeted tests first.
3. Run wider integration tests when touching contracts or pipeline behavior.

Typical commands (Windows toolchain):
- `cmake.exe --build build_vs --config Debug --target <target>`
- `./bin/test_generators.exe`
- `./bin/test_full_pipeline.exe`
- `./bin/test_city_generator_validation.exe`

## 10) Output Quality
When proposing or completing changes, include:
- exact files touched
- why each change belongs in that layer
- risk and compatibility notes
- what was validated and result
clear