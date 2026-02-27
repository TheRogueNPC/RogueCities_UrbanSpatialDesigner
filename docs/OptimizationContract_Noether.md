# Optimization Contract – Noether Symmetry Audit

**Purpose**: Provide a contract that codifies symmetry‑based optimization principles for the RogueCities codebase.  The contract is derived from Noether’s theorem, stating that every continuous symmetry of a physical system’s action yields a conserved quantity.  Translating this to software, we treat **symmetries** as invariances (time, space, API contracts, resource budgets) and the **conserved quantities** as deterministic outputs, invariant geometry, stable state transitions, and bounded resources.

## Contract Items

1. **Deterministic Time Symmetry**
   - All simulation‑step logic must be driven by a single **SimulationClock** service.
   - UI animations and timers must query this clock, never call `std::chrono::now()` directly.
   - Tests must be able to set the clock to a fixed seed.

2. **Spatial Translation & Rotation Symmetry**
   - Global geometry must be expressed through a **WorldTransform** object.
   - All coordinate calculations (generation, UI display) must use this transform so that a global shift or rotation does not alter relative relationships.

3. **API Contract Symmetry**
   - All entry points to the generation pipeline must go through **GeneratorBridge**.
   - Direct calls to `CityGenerator` from UI are prohibited.
   - UI‑initiated state changes must be expressed as **HFSM commands** (command pattern) to guarantee a single source of truth for state transitions.

4. **Resource‑Budget Conservation**
   - Temporary objects (e.g., stub roads) must be allocated from **RAII pools** (`civ::IndexVector<T>` or similar) and automatically reclaimed.
   - Memory and handle usage must be bounded; leaks are a violation of the conservation law.

5. **Performance Budget Symmetry**
   - Re‑generation should only occur when a **symmetry‑breaking** parameter changes.
   - Cache intermediate stages and annotate parameters with a `[[symmetry: …]]` tag to indicate invariance.

6. **Verification**
   - Unit tests must assert that given a fixed clock and identical inputs, the output is identical (determinism).
   - Geometry invariance tests must verify that applying a global translation/rotation yields unchanged adjacency relationships.
   - Resource‑budget tests must ensure no net increase in live handles after UI actions.

---

*Generated automatically by Antigravity based on Noether‑theory audit.*

## Skill Support Documentation

- **NoetherAuditSkill** – Scans the App layer for symmetry‑related anti‑patterns (time‑translation, global‑state, spatial‑translation, resource‑budget, configuration, deterministic output) and appends a concise audit section to the markdown report.
- **SimulationClockSkill** – Finds all `std::chrono` usages, proposes a diff that replaces them with `SimulationClock::Now()`, can apply the changes (with approval) and updates the audit report.

These skills are defined in the `.agents` directory and can be invoked via:
```
run_skill NoetherAuditSkill
run_skill SimulationClockSkill
```
