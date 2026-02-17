# RogueCity Agent Onboarding Guide

Last Updated: February 17, 2026

For: AI agents working on RogueCities_UrbanSpatialDesigner

---

## Quick Start Checklist

Before working on any task, verify all items:

- [ ] Understand the 3-layer architecture: Core → Generators → App.
- [ ] Understand the Rogue Protocol performance mandates.
- [ ] Understand Cockpit Doctrine UI principles.
- [ ] Select the correct specialist agent lane.
- [ ] Verify active contracts before editing.

---

## Architecture Foundation

### Three-Layer Law

```text
Layer 2: App (Editor + UI + HFSM)
  Depends on: Core + Generators
  Paths: app/*, visualizer/*

Layer 1: Generators (Algorithms)
  Depends on: Core only
  Paths: generators/*

Layer 0: Core (Data + Math)
  Depends on: math libs only
  Paths: core/*
  Forbidden: ImGui, OpenGL, GLFW
```

Critical rule: Core must not depend on UI libraries.

### Rogue Protocol Container Matrix

- `FVA`
  - use case: roads, districts, UI entities
  - when: stable IDs required in editor/UI
  - aliases: `FVA<T>`
- `SIV`
  - use case: buildings, props, churn-heavy entities
  - when: validity checks under churn are required
  - aliases: `SIV<T>`, `SIVHandle<T>`
- `CIV`
  - use case: internal/scratch data
  - when: non-UI calculations and temporary geometry
  - aliases: `CIV<T>`, `CIVRef<T>`
- `std::vector`
  - use case: simple local data
  - when: not an editor-exposed entity store
  - aliases: none

### Rogue Protocol Threading

- `RogueWorker`
  - use case: heavy parallel work
  - trigger: grid/tensor/trace operations
  - threshold: `N = axioms × districts × density > 100`
- `Main thread`
  - use case: UI state and drawing
  - trigger: all ImGui and state transitions
  - threshold: always

Rule: if work loops over the grid, use worker policy.
If work touches UI/GPU state, keep it on main thread.

---

## Agent Roles and Responsibilities

### Architect

Use this lane for cross-module architecture, contracts, and policy decisions.

Authority:

- Final decision on architecture and contract enforcement.
- Final review on performance and design compliance.

### Specialist Lanes

#### Coder Agent

When to use:

- C++ implementation and refactor.
- Layer separation and CMake hygiene.
- Test updates and integration wiring.

#### Math Genius Agent

When to use:

- Formulas, tensor fields, AESP math, and numeric stability.

Must read first:

- `docs/20_specs/design-and-research/the-rogue-city-designer-soft.md`

#### City Planner Agent

When to use:

- District archetypes, zoning semantics, and urban logic.

#### Resource Manager Agent

When to use:

- Capacity caps, memory budgets, and growth guardrails.

#### Debug Manager Agent

When to use:

- Determinism, transition tests, profiling, and regressions.

#### Documentation Keeper Agent

When to use:

- Specs, build docs, architecture docs, and runbooks.

#### UI/UX/ImGui/ImVue Master

When to use:

- State-reactive UI behavior and Cockpit Doctrine compliance.

#### AI Integration Agent

When to use:

- Toolserver, protocol compatibility, AI clients, pattern catalog.

#### API Alias Keeper

When to use:

- API stability, migration aliases, and Lua-facing signature continuity.

---

## Canonical Generator Workflow

Use this 8-step pattern for any new generator track:

1. Define generator pipeline structs/methods in Generators.
2. Extend `GlobalState` with appropriate containers.
3. Implement App bridge wiring (UI → generator → state).
4. Add HFSM state/events and transition tests.
5. Add index panels and context menus.
6. Add viewport overlays.
7. Add control panel inputs.
8. Update AI pattern catalog metadata.

---

## Build Workflows

### Standard Build

```powershell
cmake -B build -S .
cmake --build build --target RogueCityCore --config Release
cmake --build build --target RogueCityGenerators --config Release
cmake --build build --target RogueCityVisualizerGui --config Release
```

### Fast Core Iteration

```powershell
cmake -B build_core -S . -DBUILD_CORE_ONLY=ON
cmake --build build_core --target RogueCityCore --config Release
```

### Visual Studio

```powershell
start build_vs\RogueCities.sln
```

### Tests

```powershell
cmake --build build --target test_generators --config Release
ctest --test-dir build --output-on-failure
```

### Diagnostics Toolchain (Agent-RC Standard)

Use this sequence when IDE Problems diverge from build output:

```powershell
# 1) Environment and toolchain sanity
python tools/env_doctor.py

# 2) Triage current Problems export
python tools/problems_triage.py `
  --input .vscode/problems.export.json

# 3) Compare with previous snapshot and save current
python tools/problems_diff.py `
  --current .vscode/problems.export.json `
  --snapshot-current

# 4) One-click configure/build/diagnostics refresh
python tools/dev_refresh.py `
  --configure-preset dev `
  --build-preset gui-release
```

Paths:

- Problems export: `.vscode/problems.export.json`
- Snapshot history: `.vscode/problems-history/`

---

## Contracts and Compliance

### Generator-Viewport Contract

Single output application path:

- `RogueCity::App::ApplyCityOutputToGlobalState`

Checks:

```powershell
python3 tools/check_generator_viewport_contract.py
```

### Tool-Wiring Contract

Single action ingress:

- `DispatchToolAction(action_id, context)`

Checks:

```powershell
python3 tools/check_tool_wiring_contract.py
```

### UI Compliance Contract

Use approved panel/shell patterns and draw ownership boundaries.

Checks:

```powershell
python3 tools/check_ui_compliance.py
```

---

## Cockpit Doctrine

Core principles:

1. Viewport is sacred.
2. Tools are tactile.
3. Properties are contextual.
4. Critical data remains visible.

Required:

- State-reactive UI behavior tied to HFSM.
- Motion conveys meaning.
- No decorative-only animation.

Forbidden:

- Viewport-obstructing chrome.
- Static-only instructional UX for interactive flows.
- UI that ignores active state context.

---

## Common Pitfalls

Avoid:

1. Using `std::vector` for editor-identity entities.
2. Threading tiny tasks below threshold.
3. Pulling UI deps into Core.
4. Guessing formulas without source spec.
5. Bypassing coordinator/applier contracts.

Do instead:

1. Pick FVA/SIV/CIV by contract.
2. Compute complexity threshold before threading.
3. Keep Core UI-free.
4. Cite design spec section for formula changes.
5. Route all generation mutations through contract paths.

---

## Onboarding Exit Criteria

Agent is ready when it can answer:

1. Layer dependency rules.
2. FVA vs SIV vs CIV choice rationale.
3. RogueWorker threshold policy.
4. Cockpit Doctrine core principles.
5. Canonical generation output application path.

---

## Help and Escalation

When blocked:

1. Read the relevant spec first.
2. Verify contract assumptions.
3. Run compliance scripts.
4. Escalate architectural conflicts to Architect lane.

Useful commands:

```powershell
Get-ChildItem -Path docs -Recurse -Filter *.md
python3 tools/check_ui_compliance.py
python3 tools/check_generator_viewport_contract.py
python3 tools/check_tool_wiring_contract.py
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```
