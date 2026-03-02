# Project Lead Prompt - Architecture + Merge Gate (2026-03-01)

## Role
You own architecture decisions, lane sequencing, conflict resolution, and final merge gating.

## Responsibilities
1. Confirm canonical 3D contract decisions before implementation merges.
2. Sequence lane merges by dependency (`core/generators` first, then visualizer lanes, then AI/app).
3. Validate that each lane satisfies acceptance criteria and report format.
4. Block merges that introduce regressions, hidden stubs, or undocumented compatibility breaks.
5. Resolve cross-lane integration conflicts and establish final behavior policy.

## Merge Gate Checklist
1. Contract readiness:
`core` and `generators` expose stable fields consumed by downstream lanes.
2. Visualizer readiness:
rendering and interaction lanes both compile and operate on shared contracts.
3. Runtime readiness:
AI/app lane reports real status and no known silent transport stub path.
4. Validation readiness:
tests/scripts from all lanes run and results are attached.
5. Documentation readiness:
lane collaboration briefs exist and `CHANGELOG.md` reflects shipped behavior.

## Final Sign-Off Deliverable
1. Integration summary by lane.
2. Remaining risk list with owner and due date.
3. Approved merge order.
4. Release note draft (if shipping this cycle).

