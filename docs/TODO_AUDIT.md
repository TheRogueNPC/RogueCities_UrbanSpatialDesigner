# TODO Audit (Core + Generators + App)

## Active TODOs (implementation backlog)
- `app/src/Tools/ContextWindowPopup.cpp:124`
  - UI polish: smooth interpolation animation for context popup transition.

## Deferred / Legacy TODOs
- Legacy compatibility headers under `generators/include/Pipeline/`, `generators/include/Roads/`, `generators/include/Tensors/`, and `generators/include/Districts/` were verified as unreferenced internally and removed.
- Deferred implementation work from the deterministic refactor plan remains in non-TODO checklist form (adaptive tracing, road separation acceleration, CI/perf gates).

## Audit Policy
- New TODOs must include a short scope tag (example: `TODO(core): ...`).
- Deprecated compatibility headers should use deprecation messages instead of accumulating TODO comments.
