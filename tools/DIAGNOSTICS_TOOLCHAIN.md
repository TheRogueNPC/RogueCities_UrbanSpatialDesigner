# RogueCities Diagnostics Toolchain

This workspace includes four environment + diagnostics utilities that work with
the VS Code Problems Bridge export file.

## Inputs

- Problems export: `.vscode/problems.export.json`
- Snapshot history: `.vscode/problems-history/`

## Tools

1. `tools/env_doctor.py`
   - Validates toolchain, compile DB, VS Code settings, and Problems export wiring.

2. `tools/check_clang_builder_contract.py`
   - Enforces clang/builder safety rules (header API + include invariants).

3. `tools/problems_triage.py`
   - Groups diagnostics by root cause and priority.
   - Supports focused sweeps via `--source`, `--severity`, `--include-path`, `--exclude-path`.

4. `tools/problems_diff.py`
   - Diffs two diagnostics snapshots and tracks added/removed issues.

5. `tools/dev_refresh.py`
   - One-click: configure -> compile DB generation -> build -> diff -> triage.

## Typical Workflow

```powershell
python tools/env_doctor.py
python tools/check_clang_builder_contract.py
python tools/problems_triage.py --input .vscode/problems.export.json
python tools/problems_triage.py --input .vscode/problems.export.json --source clang --source clangd --exclude-path /3rdparty/ --exclude-path /_Temp/
python tools/problems_diff.py --current .vscode/problems.export.json --snapshot-current
python tools/dev_refresh.py --configure-preset dev --build-preset gui-release
```

