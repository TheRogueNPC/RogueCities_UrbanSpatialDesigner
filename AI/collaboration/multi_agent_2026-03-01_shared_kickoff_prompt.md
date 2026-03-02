# Multi-Agent Shared Kickoff Prompt (2026-03-01)

## Mission
Deliver a 3D-ready, non-stubbed urban design stack across `core`, `generators`, `app`, `AI`, and `visualizer` without breaking current editing workflows.

## Baseline From Audit
1. Core/generator contracts are still primarily 2D (`Vec2`) and do not preserve full 3D-ready metadata end-to-end.
2. Visualizer has the highest concentration of placeholders/stub UI paths and performance-risk fallback selection paths.
3. App primary viewport is labeled as 3D/2D hybrid but still operates with 2D orthographic interaction/render assumptions.
4. AI transport has a non-Windows HTTP gap and partial command-path behavior.

## Lane Ownership
1. Claude: `core + generators` 3D contracts and pipeline continuity.
2. Gemini Pro: `visualizer` rendering modernization and metadata presentation.
3. Codex: `visualizer` interaction/tooling wiring and selection acceleration.
4. AI/App Agent: `AI + app` runtime integration and command-path hardening.
5. Project Lead (you): architecture decisions, dependency sequencing, merge gate.

## Global Rules
1. Do not revert unrelated local changes in this dirty repository.
2. Ship code, not docs-only analysis.
3. Preserve current behavior unless explicitly replaced by lane contract.
4. Add tests or deterministic validation for each shipped behavior change.
5. Write a dated lane brief to `AI/collaboration/`.
6. Update `CHANGELOG.md` for shipped runtime behavior changes.
7. If blocked by another lane, leave compile-safe TODO with explicit dependency and continue unblocked work.

## Integration Order
1. Claude lands core/generator contracts first.
2. Gemini and Codex implement visualizer lanes on top of those contracts in parallel.
3. AI/App agent finalizes runtime/tool integration once interfaces stabilize.
4. Project lead performs integration QA, conflict resolution, and merge approval.

## Required Final Report Format (per lane)
1. Implemented behavior summary.
2. Files changed.
3. Tests/validation run.
4. Risks and follow-up items.
5. Confirmation of collaboration brief + changelog update status.

