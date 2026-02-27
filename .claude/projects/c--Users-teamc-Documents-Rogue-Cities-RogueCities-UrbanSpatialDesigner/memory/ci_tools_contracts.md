# RogueCities — CI, Tools & Active Contracts

## GitHub Workflows (.github/workflows/)

| Workflow | Trigger | Platform | Tests |
|---|---|---|---|
| determinism-check.yml | push/PR/dispatch | ubuntu | test_determinism_baseline, test_determinism_comprehensive, test_incremental_parity |
| performance-check.yml | push/PR/dispatch | ubuntu | test_generation_stage_perf, test_generation_latency |
| ci-minimap-lod.yml | push/PR | windows | test_minimap_lod_determinism |
| auto-commit.yml | dispatch | ubuntu | stefanzweifel git-auto-commit-action |
| manual-commit.yml | dispatch | ubuntu | manual commit to main |
| main.yml | dispatch | ubuntu | GPT agent file create/update |

Performance check uploads artifacts (performance_report.json, ctest_perf.log).
Budget file: `tests/baselines/performance_budgets_v0.10.json`
CMake configure flags: BUILD_TESTING=ON, ROGUEUI_ENFORCE_DESIGN_SYSTEM=OFF (determinism CI)

---

## Tools Directory (tools/)

### Validation & Compliance Scripts
- `check_imgui_contracts.py` — flags WantCaptureMouse hard-blocks and ImGui anti-patterns
- `check_generator_viewport_contract.py` — validates generator→viewport API contracts + domain gen policy
- `check_clang_builder_contract.py` — clang/CMake compatibility enforcement
- `check_context_command_contract.py` — command interface compliance
- `check_tool_wiring_contract.py` — tool integration wiring checks
- `check_ui_compliance.py` — UI consistency checks
- `check_licenses.ps1 / .sh` — license header verification
- `check_perf_regression.py` — performance baseline detection

### Build & Dev Tools
- `env_doctor.py` — validates toolchain, compile DB, VS Code settings (run first when diagnosing)
- `dev_refresh.py` — one-click: configure → compile DB → build → diff → triage
- `problems_triage.py` — groups diagnostics by root cause
- `problems_diff.py` — diffs two diagnostic snapshots
- `generate_perf_report.py` — performance report generation
- `generate_compile_commands.py` — clang compile DB for IDE
- `dev-shell.ps1` — PowerShell dev env setup
  - Sets RC_ROOT, RC_BUILD_DIR
  - CMAKE_GENERATOR = "Visual Studio 18 2026"
  - CMAKE_BUILD_PARALLEL_LEVEL = 8
  - Enforces absolute paths in MSVC diagnostics

### AI Bridge Scripts
- `tools/.run/toolserver.py` — active FastAPI bridge server (port 7077)
- `Debug/quickFix.bat` / `runLive.bat` / `runMock.bat`
- `Start_Ai_Bridge_Fixed.ps1` / `Stop_Ai_Bridge_Fixed.ps1`
- `External_Connect/Start-Stack.ps1 / .bat` / `Stop-Stack.ps1 / .bat`
- `preflight_startup.ps1` — pre-launch environment checks

### Preflight Startup (Windows Recovery Order)
1. `tools/preflight_startup.ps1 -VerboseChecks`
2. `StartupBuild.bat`
3. `build_and_run.bat`
4. `build_and_run_gui.ps1` (UI workflow only)

---

## Active Work Contracts (from .github/prompts/PLAN.md)

### Current Phase: RC-0.09 Stability & Contracts

#### 8 Work Packages (in execution order)
1. **WP1 Input Gate Rewrite (P0)** — Refactor BuildUiInputGateState; gate by viewport canvas hover + no popup/modal/text-edit (not `!io.WantCaptureMouse`). File: `visualizer/src/ui/rc_ui_input_gate.cpp:20`
2. **WP2 Canonical Viewport Surface** — Define InvisibleButton canvas in rc_panel_axiom_editor.cpp; route hover/click eligibility through it
3. **WP3 Deterministic Mutation Contract** — One explicit left-click path per domain/subtool in rc_viewport_interaction.cpp. Every action → Mutation/Selection/Gizmo/ActivateOnly (never silent)
4. **WP4 Domain Generation Policy** — Axiom=LiveDebounced, all others=Explicit. Central policy map in GlobalState. `ApplyCityOutputToGlobalState` is the ONLY output-application path
5. **WP5 Smooth Resize** — glfwSetWindowSizeLimits; debounced dock rebuild; no churn during drag; min: max(25% display, 1100×700)
6. **WP6 No Clipping** — metric-driven sizing (GetFrameHeight/GetFontSize); no collapsed-mode early-returns; scrollable at min size
7. **WP7 Guardrail Checks** — extend tools/check_imgui_contracts.py and check_generator_viewport_contract.py for new contracts
8. **WP8 Docs** — docs/20_specs/tool-viewport-generation-contract.md, docs/30_runbooks/resize-regression-checklist.md

#### Locked Decisions
- Input gate: viewport-local (not global ImGui WantCaptureMouse)
- Resize policy: debounced rebuild
- Min window: max(25% display, 1100×700)
- Domain generation: Axiom=live, others=explicit

#### Key Problematic Locations (from WP1-2 research)
- Click blocker: `visualizer/src/ui/rc_ui_input_gate.cpp:20` (WantCaptureMouse hard block)
- Non-axiom exit: `visualizer/src/ui/rc_viewport_interaction.cpp:1255`
- Resize auto-dirty: `visualizer/src/ui/rc_ui_root.cpp:1000`
- Main loop clamp: `visualizer/src/main_gui.cpp:289`
- Tool dispatch: `visualizer/src/ui/tools/rc_tool_dispatcher.cpp` (working)

#### API Changes Required by Contracts
- `rc_ui_input_gate.h` — UiInputGateState gains explicit block-reason semantics
- `GlobalState.hpp` — Add GenerationMutationPolicy domain→method mapping
- `rc_viewport_interaction.h` — InteractionOutcome enum (already added ✓)
- `rc_panel_axiom_editor.cpp` — Canonical viewport canvas InvisibleButton

---

## EditorPlanMap — MVP Requirements (from .github/EditorPlanMap.json)

### Must implement before MVP
1. ViewportIndexBuilder (Build() from GlobalState) ✓
2. SelectionManager (multi-select, Shift/Ctrl/Alt+Drag) ✓
3. CommandHistory undo/redo ✓
4. Context-sensitive Property Editor (NO C++ reflection — manual grids)
5. Validation system with real-time overlays ✓

### Can defer post-MVP
- Timeline Scrubbing UI, Branch/Fork support, Lua Bindings, Asset Browser, Tutorial System

### Architecture Mandates from Roundtable
- Do NOT attempt C++ reflection — manual property grids only
- Viewport Index = causal compiler output; "If UI needs data not in index, index is wrong"
- Undo/Redo is BLOCKING priority (not nice-to-have)
- District Boundary overrides bumped to HIGH priority

### VpProbeData Performance Targets
- Village (310 entities): 27 KB index
- Large City (8,080 entities): 711 KB index
- Metropolis (68,200 entities): 6 MB index

### Motion Design Specs
- 0.15s hover fade-in, 0.25s fade-out
- 2.0 Hz selection pulse
- 0.3s ease-in-out for layout transitions

---

## 2-Pass UI/UX Enforcement Plan (from .github/prompts/plan-uiUxEnforcementTwoPass.md)

### Pass 1 (Core Infrastructure)
- PanelType enum + DrawPanelByType dispatcher (replaces 15+ scattered Draw calls)
- Infomatrix event system in GlobalState (ring buffer, max 220 events)
  - File: `core/include/RogueCity/Core/Infomatrix.hpp`
  - Categories: Runtime, Validation, Dirty, Telemetry
  - Log panel consumes via gs.infomatrix.events()
- Layout schema loop replaces scattered calls in rc_ui_root.cpp lines 813-837
- ImGui FAQ compliance (PushID/##suffix in loops, ClipRect, input routing)

### Pass 2 (Advanced)
- ButtonDockedPanel (Shift-click dock/float toggle)
- Indices tab group extracted to rc_ui_indices_views.h
- AI panels emit to Infomatrix telemetry
- Telemetry + Validation panels filter by Infomatrix category
- (Optional) UIUserSchema engine behind feature flag

### Key Never-Dos (2-Pass)
- Never let any panel push directly to local deque after Infomatrix exists
- Never implement layout-level behavior inside individual panel Draw calls
- Never ship ImGui widget ID collisions (always PushID in loops)
- Infomatrix capacity: kMaxEvents = 220 (fixed, ring buffer)
