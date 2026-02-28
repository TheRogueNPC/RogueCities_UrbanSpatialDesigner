# RogueCities Gemini CLI Mandates

This document defines the foundational mandates for Gemini CLI in the RogueCities_UrbanSpatialDesigner workspace. These instructions take absolute precedence over general workflows and tool defaults.

## 1. Core Objectives
- **Architecture Compliance:** Prioritize strict module boundaries and existing patterns over implementation speed.
- **Determinism:** Every generation pipeline must support fixed seeds and reproducible outputs.
- **Correctness:** Prioritize surgical edits that preserve backward compatibility for legacy inputs and maintain data integrity.
- **Performance:** Maintain efficiency in hot paths; avoid virtual dispatch or heap churn in inner loops.

## 2. Layer Ownership (Strict Boundaries)
- **`core/`**: Owns domain data models (`Vec2`, city entities) and editor runtime state (`GlobalState`). **Strictly NO UI (ImGui/GLFW) or generation algorithms.**
- **`generators/`**: Owns procedural algorithms (`CityGenerator`), tensor/road/building logic, and deterministic stage caching. **Strictly NO UI/ImGui.**
- **`app/`**: Owns tool behavior, integration bridges (`GeneratorBridge`), and runtime preview coordination. Translates user intent to generator contracts.
- **`visualizer/`**: Owns ImGui panels, viewport interaction, and overlay drawing. Renders state but does not implement generation policy.

## 3. Canonical Data Flow
1. `AxiomVisual` edits produce `AxiomInput` data.
2. `GeneratorBridge` validates and maps tool data to generator contracts.
3. `CityGenerator` runs staged generation and emits `CityOutput`.
4. `CityOutputApplier` merges output into `GlobalState` (preserving lock semantics).
5. `Visualizer` consumes `GlobalState` for presentation and interaction.

## 4. Engineering & Math Standards
- **Containers:** 
    - `fva::Container<T>`: Editor-facing collections needing stable handles.
    - `civ::IndexVector<T>`: Performance-critical internal/scratch sets (do not expose to UI).
    - `siv::Vector<T>`: Long-lived references requiring validity-checked handles.
- **Libraries:**
    - **Boost Geometry:** Use for robust geometric predicates (distance, hull, intersection, point-in-polygon). Do not use ad-hoc math if Boost is applicable.
- **HFSM:** State semantics belong to `core/app` orchestration. `visualizer` only reacts to state; it does not own transition policy.
- **Math Excellence:** 
    - Always state assumptions, units (`meters`, `seconds`, `cell_size`), and valid ranges.
    - Use explicit tolerance for floating-point comparisons (absolute epsilon for zero-checks, relative for scale-aware equality).
    - Offload heavy compute from UI transition paths to `RogueWorker`.

## 5. Implementation Contracts
- **Axiom Lattice:** `AxiomVisual` uses `ControlLattice`. `CityGenerator::ValidateAxioms` must accept legacy inputs unless a warp payload is present.
- **Foundation Boundary:** Prefer generator-emitted `city_boundary` for the world envelope; use fallbacks only if empty.
- **Compass/HUD:** Compass is parented to Scene Stats and can drive camera yaw.
- **UI Panels:** Use `PanelRegistry` and `RcDataIndexPanel<T, Traits>` patterns. Do not bypass the registry with ad-hoc `Draw()` calls.

## 6. Operational Protocols
- **Research Phase:** Identify target layers, read headers for contracts, and locate the nearest canonical implementation to mirror.
- **Execution Phase:** Make surgical edits. Reuse existing APIs and container patterns (FVA/CIV/SIV) before introducing new abstractions.
- **Validation Phase:** 
    1. Build impacted targets (`cmake --build build_vs --target <target>`).
    2. Run targeted tests (`test_generators`, `test_editor_hfsm`, etc.).
    3. Run wider integration tests (`test_full_pipeline`) if contracts or generation semantics changed.
- **Handoff Checklist:** For non-trivial tasks, confirm: Correctness, Numerics, Complexity, Performance, Determinism, Tests, Risks/Tradeoffs.

## 7. Prohibited Actions (Anti-Patterns)
- **NO** UI dependencies in `core/` or `generators/`.
- **NO** silent changes to generation semantics (AESP, road classification, tensor behavior).
- **NO** early-returns in UI layout that cause content to disappear in collapsed modes.
- **NO** removal of compatibility paths for legacy editor inputs without explicit approval.
- **NO** bypassing of `UiIntrospector` or state-aware instrumentation in UI work.

## 9. Changelog Management
- **Explicit Updates:** Update `CHANGELOG.md` immediately upon user request, summarizing all work performed since the last entry.
- **Atomic Proactivity:** For every significant code change or feature addition, proactively append a small, atomic descriptor (maximum 2 sentences) to the "Unreleased" or current version section of the `CHANGELOG.md`, even if not explicitly prompted.
- **Spirit of Change:** Ensure descriptors capture the "spirit" and architectural impact of the change rather than just narrating the file diff.

## 8. Mathematical Reference

### Grid Quality Metrics
Used for evaluating street networks and balancing the "organized complexity" of the urban form.

- **Straightness ($\varsigma$):**
  $$\varsigma = \frac{\text{average great-circle distance}}{\text{average segment length}}$$
- **Four-Way Intersection Proportion ($I$):**
  $$I = \frac{\text{four-way intersections}}{\text{total intersections}}$$
- **Composite Grid Index:**
  $$\text{GridIndex} = (\varsigma \times \Phi \times I)^{1/3}$$
  *(Where $\Phi$ is the Orientation Order calculated via street orientation entropy)*

### District Affinity Formulas (AESP)
Determines land-use classification based on road frontage profiles: **Access (A)**, **Exposure (E)**, **Serviceability (S)**, and **Privacy (P)**.

- **Mixed-Use:** $0.25(A + E + S + P)$ (Balanced)
- **Residential:** $0.60P + 0.20A + 0.10S + 0.10E$ (Privacy-dominant)
- **Commercial:** $0.60E + 0.20A + 0.10S + 0.10P$ (Exposure-dominant)
- **Civic:** $0.50E + 0.20A + 0.10S + 0.20P$ (Exposure/Privacy balance)
- **Industrial:** $0.60S + 0.25A + 0.10E + 0.05P$ (Serviceability-dominant)

## 10. AI Agent Shell Environment
- **Mandatory Initialization:** All AI agents executing build, configure, or test operations must utilize or assume the configuration provided by `tools/dev-shell.ps1`.
- **CMake Resolution:** AI agents must not attempt to manually locate or bypass the CMake configuration defined in the dev shell. The shell dynamically injects the correct CMake binaries into the `$env:PATH` to prevent conflicts with standard VS Code terminals.

## 11. AI Telemetry Bridge
- **Run-Time Crash Analysis:** Whenever the user reports a "crash" or "assertion failure," the AI *must* immediately read `AI/diagnostics/runtime_crash.json` to extract the callstack, file, and line number context generated by `RogueCity::Core::Validation::AiTelemetry`.
- **Compile-Time Issue Analysis:** Whenever the user reports a build failure, the AI *must* look at either `.vscode/problems.export.json` via `rc-problems` or `AI/diagnostics/last_compile_error.json` before asking for arbitrary logs.
