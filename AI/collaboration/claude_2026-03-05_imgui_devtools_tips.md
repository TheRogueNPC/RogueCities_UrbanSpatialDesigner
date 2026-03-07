# ImGui Developer Tools — Debug Productivity + PVS-Studio

**Date:** 2026-03-05
**Author:** Claude (claude-sonnet-4-6)
**CHANGELOG updated:** yes

---

## What Was Done

Extended the Dev Shell panel (`RC_UI::Panels::DevShell`) with three new collapsing sections
implementing the canonical Dear ImGui developer productivity tips. Added `rc-pvs-studio` to
`tools/dev-shell.ps1` as the project's static analysis entry point.

---

## New Sections in Dev Shell `DrawContent()`

### 1. ImGui Debug Productivity
- **Alt-key breakpoint filter** — live indicator of `io.KeyAlt` state with explanatory text
  on the pattern `if (ImGui::GetIO().KeyAlt) printf("");`. Devs see instantly whether the
  ALT condition is currently satisfied.
- **Natvis file status** — checks `3rdparty/imgui/misc/debuggers/imgui.natvis` exists and
  provides a "Copy Path" button. Tooltip explains how to add it to a VS project for
  `ImVector<>` type expansion in Watch/Locals.

### 2. Test Suite (CTest)
- Four async launch buttons: **Core**, **Gen**, **App**, **All** — each runs the
  corresponding `rc-tst-*` / `rc-tst` dev-shell command via `RunDevShellCmdAsync`.
- Scrollable 120px output child window; busy spinner while running.
- Purpose: catch regressions after any code change; add a test case when fixing a bug.

### 3. PVS-Studio Static Analysis
- Input fields: PVS-Studio Dir, Output Dir, Project Name.
- Checkbox: "Open HTML report after analysis".
- Run button calls `rc-pvs-studio` via `RunDevShellCmdAsync`; button disabled when PVS not
  installed (detected by checking `PVS-Studio_Cmd.exe` exists at configured dir).
- Scrollable 100px output child window; busy spinner during analysis.

---

## New Helper: `RunDevShellCmdAsync`

Added to the anonymous namespace in `rc_panel_dev_shell.cpp`:

```cpp
static void RunDevShellCmdAsync(
    const std::string& cmd_suffix,
    std::string* out_status, std::mutex* out_mutex, std::atomic<bool>* out_busy);
```

Spawns a detached `std::thread` that runs:
```
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command
"& { . .\tools\dev-shell.ps1 > $null; <cmd_suffix> }" 2>&1
```
Captures stdout+stderr; writes result under `*out_mutex`; clears `*out_busy`.

---

## New Dev-Shell Command: `rc-pvs-studio`

**File:** `tools/dev-shell.ps1`

PowerShell adaptation of the `run_pvs_studio.bat` recipe from Dear ImGui developer tips.

```powershell
rc-pvs-studio [-PvsDir <path>] [-ProjectName RogueCityVisualizer]
              [-OutputDir tools/.run/pvs_studio] [-Severity GA:1,2;OP:1] [-OpenReport]
```

Workflow:
1. Locate `<ProjectName>.vcxproj` recursively under `RC_BUILD_DIR`.
2. Run `PVS-Studio_Cmd.exe -r -t <proj> -o <proj>.plog`.
3. Convert via `PlogConverter.exe -a GA:1,2;OP:1 -t Html,FullHtml,Txt,Totals`.
4. Delete `.plog`; print Totals; optionally open `fullhtml/index.html`.

Graceful degradation: warns if PVS-Studio is not installed or `RC_BUILD_DIR` is unset.

---

## Files Modified

| File | Change |
|------|--------|
| `visualizer/src/ui/panels/rc_panel_dev_shell.cpp` | Static state for test/pvs; `RunDevShellCmdAsync` helper; three new collapsing headers in `DrawContent()` |
| `tools/dev-shell.ps1` | `rc-pvs-studio` function + `rc-help` entry under Advanced Tooling |
| `AI/collaboration/claude_handoff_prompt.md` | — (no update needed; handoff already covers dev shell) |
| `CHANGELOG.md` | Entry in [Unreleased] |

---

## Architecture Notes

- `RunDevShellCmdAsync` is intentionally **not** thread-pool'd — each dev action is user-triggered
  and infrequent. Detached threads are acceptable here (no cleanup needed on panel close).
- PVS-Studio detection is filesystem-based (`std::filesystem::exists`), not registry-based.
  This means it works even in CI containers where the installer is copied manually.
- The severity filter `GA:1,2;OP:1` matches the Dear ImGui reference bat. Project can tune
  this via `-Severity` param once false-positive baseline is established.
- Natvis path `3rdparty/imgui/misc/debuggers/imgui.natvis` is verified present in repo tree.

---

## Verification

```
rc-bld-headless
rc-perceive-ui -Mode quick -Screenshot
```

Open "Dev Shell" (Ctrl+`) and confirm:
- [ ] "ImGui Debug Productivity" collapses/expands; ALT indicator updates live
- [ ] Natvis: found (green)
- [ ] "Test Suite" — click "Core" — output appears after ctest completes
- [ ] "PVS-Studio" — if not installed, shows amber warning; button disabled

---

## Handoff Notes

- `rc-pvs-studio` is also callable from the REPL in `main.cpp` as:
  `shell rc-pvs-studio` (via the `! <cmd>` passthrough). No REPL command registration needed.
- The test suite buttons complement `rc-full-smoke` (which runs smoke + perception).
  They are a lighter, faster check during active development.
- Consider adding `rc-pvs-studio` to the full smoke gate once a false-positive baseline
  is established.
