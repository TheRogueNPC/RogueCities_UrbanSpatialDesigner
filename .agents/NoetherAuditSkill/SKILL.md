---
name: NoetherAuditSkill
description: |
  A skill that automates the Noether‑theorem based symmetry audit for the RogueCities codebase. It scans for violations of continuous symmetries (time‑translation, global‑state, spatial‑translation, resource‑budget, configuration, deterministic output) and produces a concise report that can be appended to the audit markdown.
---

# Overview
The skill enforces the **OptimizationContract_Noether.md** contract by detecting common anti‑patterns that break the symmetries required for conserved quantities.

# Steps
1. **Initialize** – Load the contract file `docs/OptimizationContract_Noether.md` to obtain the list of required symmetries.
2. **Search for Time‑Translation Violations** – Grep for `std::chrono::` usage in the `app` directory.
3. **Detect Direct GlobalState Mutations** – Grep for `GetGlobalState()` and any non‑command mutation patterns.
4. **Check Spatial‑Translation/Rotation** – Verify that geometry code uses the `WorldTransform` abstraction (search for `WorldTransform` usage; flag files that compute coordinates without it).
5. **Resource‑Budget Leaks** – Scan for stub creations (`RoadSubtool::Stub`, `RoadStub`) that allocate without RAII pools.
6. **Configuration Symmetry** – Identify parameters passed to `CityGenerator` on every UI change; recommend caching.
7. **Deterministic Output** – Ensure UI‑initiated actions are routed through the HFSM command queue.
8. **Generate Report** – Compile findings into markdown sections and write to `2026-02-26_Audit.md` under a "## Noether Symmetry Audit (App Layer)" heading.
9. **Optional Auto‑Fix** – Provide a script stub that can replace `std::chrono` calls with `SimulationClock::Now()`.

# Usage
```
# In the agent console
> run_skill NoetherAuditSkill
```
The skill will output a short summary and the path to the updated audit report.

# Dependencies
- Requires `grep_search`, `write_to_file` tools.
- Assumes the project follows the directory layout described in the GEMINI mandates.

# Extensibility
Add new symmetry checks by extending the **Steps** list and updating the corresponding grep patterns.
