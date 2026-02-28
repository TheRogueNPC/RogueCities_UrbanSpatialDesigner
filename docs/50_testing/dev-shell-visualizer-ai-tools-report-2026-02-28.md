# Dev Shell + Visualizer + AI Tools Report (2026-02-28)

## Scope
- Resolve `env_doctor` false warning for `VSCMD_VER` in WSL-hosted checks.
- Build visualizer using project toolchain path.
- Validate AI bridge connectivity in mock mode.
- Run full practical `rc-*` tool smoke suite and summarize findings.

## Environment
- Host: WSL2 (Linux) invoking Windows toolchain via `.vscode/vsdev-init.cmd`.
- Visual Studio toolchain: VS 2026 (`MSBuild 18.0.5`).
- CMake generator for primary build: `Visual Studio 18 2026`.

## Changes Applied
1. `tools/env_doctor.py`
- `check_toolchain` now probes `VSCMD_VER` via `set VSCMD_VER` after `vsdev-init` rather than `echo VSCMD_VER=%VSCMD_VER%`, eliminating false negatives caused by cmd expansion timing.
- `check_compile_commands` was previously updated to support VS-mode validity and both MSVC (`arguments`) and Ninja/GCC-style (`command`) compile database formats.

2. Compile commands workflow
- `build_ninja/compile_commands.json` is present and used by `env_doctor` as the preferred compile database source in this workspace.

## Verification Results

### 1. Env Doctor
- Command: `python3 tools/env_doctor.py`
- Result: `PASS=6 WARN=0 FAIL=0`
- Key checks:
  - `toolchain`: PASS (no `VSCMD_VER` warning)
  - `compile_commands`: PASS (`build_ninja/compile_commands.json`, 369 entries)

### 2. Visualizer Build
- Command:
  - `cmd.exe /c ".vscode\vsdev-init.cmd && cmake --build build_vs --config Release --target RogueCityVisualizerGui --parallel 8"`
- Result: PASS
- Output target confirmed:
  - `bin/RogueCityVisualizerGui.exe`

### 3. AI Bridge Connectivity (Mock Mode)
- Method:
  - Start toolserver with `ROGUECITY_TOOLSERVER_MOCK=1` via `uvicorn tools.toolserver:app --host 127.0.0.1 --port 7077`
  - Probe endpoints:
    - `GET /health`
    - `POST /ui_agent`
    - `POST /city_spec`
- Result: PASS
- Observed:
  - `health={"status":"ok","service":"RogueCity AI Bridge"}`
  - `ui_mock=True; ui_commands=2`
  - `spec_mock=True; districts=1`

### 3b. AI Bridge Connectivity (Live / Non-Mock Mode)
- Method:
  - Added `rc-ai-smoke` helper to dev shell and executed `rc-ai-smoke -Live`.
  - Probed the same endpoints in live mode.
- Result: PASS
- Observed:
  - Health endpoint OK.
  - `city_spec` returned a valid spec (`district_count=1`).
  - `ui_agent` completed without transport/server error but returned `command_count=0` (valid response shape, no generated commands for this prompt/model combination).

### 4. `rc-*` Tool Suite Smoke
- Executed commands:
  - `rc-env`, `rc-context`, `rc-doctor`, `rc-contract`, `rc-problems -MaxItems 10`, `rc-pdiff`, `rc-index`, `rc-cfg -Preset dev`, `rc-bld-vis`
- Result: all PASS
- Notes:
  - `rc-pdiff` created new snapshot files under `.vscode/problems-history/`.
  - `rc-problems` continues to report markdown/cSpell diagnostics concentrated in `CHANGELOG.md` (non-build-blocking).

## Findings
- No blocking build, toolchain, or AI bridge issues remain in tested paths.
- Diagnostic noise remains from markdown/cSpell in `CHANGELOG.md`; this is quality lint debt, not runtime/build failure.

## Recommended Follow-ups
1. Optional: add a lightweight `rc-ai-smoke` command in `tools/dev-shell.ps1` to automate the mock `/health`, `/ui_agent`, and `/city_spec` probe performed manually here.
2. Optional: split operational changelog notes into a narrower file to reduce markdownlint concentration in `CHANGELOG.md`.
