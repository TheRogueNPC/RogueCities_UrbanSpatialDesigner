# Clang + Builder Enforcement Contract

Last Updated: February 17, 2026

## Purpose

Prevent regressions where code compiles in one toolchain but fails or cascades
in IDE diagnostics (clang/clangd), and prevent accidental removal of required
builder-facing include invariants.

## Enforcement Script

Run:

```powershell
python tools/check_clang_builder_contract.py
```

The check is also wired into:

- `tools/check_ui_compliance.py`
- `.github/workflows/ci-minimap-lod.yml`

## Contract Rules

1. **No default `Config{}` reference args in public headers**
   - Forbidden pattern:
     - `const Config& config = Config{}`
   - Required replacement:
     - overload without config + explicit config-taking overload.

2. **Direct include required for `fva::` usage**
   - If a header uses `fva::`, it must include:
     - `RogueCity/Core/Util/FastVectorArray.hpp` (or `fast_array.hpp`)

3. **`GlobalState.hpp` include invariants are mandatory**
   - Must retain:
     - `FastVectorArray.hpp` with `IWYU pragma: keep`
     - `IndexVector.hpp` with `IWYU pragma: keep`
     - `StableIndexVector.hpp` with `IWYU pragma: keep`

## Rationale

- Avoid `default_member_initializer_not_yet_parsed` cascades in clang.
- Keep builder-visible container types stable (`fva::`, `siv::`, `civ::`).
- Ensure "clean Problems pane" remains enforceable, not best-effort.
