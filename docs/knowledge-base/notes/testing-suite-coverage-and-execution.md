---
tags: [roguecity, testing, core-tests, generator-tests, benchmarks]
type: how-to
created: 2026-02-15
---

# Testing Suite Coverage and Execution

The test suite covers core math/types/utilities, generator integration, editor behavior, and specialized unit tests, with documented build and run commands for `test_core`, `test_generators`, and additional focused executables.

## Build and Run Basics
- Configure: `cmake -S . -B build`
- Build: `cmake --build build --config Release --target test_core`
- Run: `build/Release/test_core.exe`

## Coverage Highlights
- Vec2 and Tensor2D behavior
- CIV/SIV utility container semantics
- Editor and viewport plan validation
- Benchmark-style performance checks

## Source Files
- `docs/Tests/TestingQuickStart.md`
- `docs/Tests/TestingSummary.md`
- `tests/`

## Related
- [[topics/testing-and-quality-assurance]]
- [[notes/known-rogueworker-test-gaps]]
- [[notes/core-library-data-and-editor-types]]
