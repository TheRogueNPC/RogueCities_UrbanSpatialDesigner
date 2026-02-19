# Determinism Baseline Metrics (v0.10)

This file records the deterministic baseline used by tests and CI.

## Active Baseline
- File: `tests/baselines/determinism_v0.10.txt`
- Scope: empty `GlobalState` hash signature (roads/districts/lots/buildings/tensor)
- Hash method: FNV-1a field-wise deterministic hashing

## Recording Procedure
1. Run deterministic tests from a clean build.
2. Verify no non-deterministic data paths were added.
3. If an intentional schema or pipeline change modifies deterministic output, regenerate and review baseline.
4. Commit updated baseline file and document rationale in PR notes.

## Validation
- Test target: `test_determinism_baseline`
- Helper API: `RogueCity::Core::Validation::ValidateAgainstBaseline(...)`

## Notes
- Baseline changes are treated as compatibility-impacting changes.
- Baseline updates should be rare and tied to explicit release notes.
