# Changelog

## [Unreleased] - 2026-02-25
- **UI Restoration (Regression Fix)**: Restored the primary viewport drawing call and re-enabled the Tool Library action icons by restoring their orphaned rendering loop. Corrected the Master Panel dock layout to include `Library` and `ToolDeck` nodes and categorized the `AxiomEditor` drawer as hidden to prevent UI tab redundancy.

### Added
- **Architectural Refactor**: Migrated `GeometryPolicy` from the visualizer layer to the app layer (`app/src/Tools/GeometryPolicy.cpp`) to enforce strict layer boundaries. Updated `WaterTool` and `RoadTool` to utilize the dynamic `GeometryPolicy`, removing hardcoded geometric constants.
- **Core Generator Logic**: 
  - Implemented `FrontageProfiler.cpp` to procedurally trace building facades along edge splines using a discrete `TransitionMatrix` (Markov Chain), including adaptive scaling and truncation handling for terminal modules.
  - Implemented `Policies.cpp` enforcing the Road MDP logic (`GridPolicy`, `OrganicPolicy`) based on distance fields and local network density constraints.
- **Geometry Foundation (`PolygonOps.cpp`)**: Fully implemented polygon clipping and insetting utilizing the newly integrated `Clipper2` library. Fixed integer coordinate scaling (`kClipperScale = 1000.0`) is enforced per architectural specifications.
- **Dependency Cleanliness**: Added `Clipper2` directly to the `3rdparty/` directory for full debug visibility and stripped unneeded noise directories (`.git`, `test`) to ensure clean audits.
- **UI Framework & Panels (RC-0.12)**:
  - **Unified Design System**: Centralized all UI tokens (colors, spacings, rounding) into `rc_ui_theme.h` and refactored the theme implementation to enforce Cockpit Doctrine compliance.
  - **Multi-Column Data Panels**: Refactored `RcDataIndexPanel` to support generic multi-column traits. Updated all entity index panels (Roads, Districts, Lots, Buildings) and the River index to use this unified template.
  - **Building Search Overlay**: Implemented a viewport-contextual search overlay for buildings, activated by `Ctrl+F`. Supports real-time filtering and automatic selection sync.
  - **Viewport Integration**: Integrated search overlay rendering and hotkeys directly into the `AxiomEditorPanel` viewport chrome.
- **Workspace & Theme System**:
  - **WorkspaceProfile**: Implemented `WorkspacePersona` enum and `WorkspaceProfile` struct (`visualizer/include/RogueCity/Visualizer/WorkspaceProfile.hpp`).
  - **Themes**: Added `Enterprise`, `Planner`, `Tron`, and `RedlightDistrict` themes. Registered 8 built-in themes in `ThemeManager`.
  - **Presets**: Upgraded `WorkspacePresetStore` to schema 2 to support theme persistence alongside layout INI.
  - **UI**: Added `rc_panel_workspace` for persona switching and theme management.
- **AI Integration**:
  - **Feature Flag**: Added `RC_FEATURE_AI_BRIDGE` to `AI/CMakeLists.txt` to gate AI features.
  - **Gating**: Wrapped `UiAgentClient` and `UiDesignAssistant` calls with feature flag checks.
- **UI/UX Redesign (In Progress)**:
  - **Mockup**: Adopted `RC_UI_Mockup.html` as the design specification.
  - **Strategy**: Defined mapping from HTML/CSS tokens to ImGui/C++:
    - CSS Custom Properties -> `DesignTokens` namespace (`rc_ui_tokens.h`)
    - `div.panel` -> `UI::BeginPanel()` (`DesignSystem.cpp`)
    - Flexbox/Grid -> `ImGui::SameLine()`, `ImGui::BeginTable()`
    - CSS Classes -> ImGui Style Pushes / Helper functions

### Fixed
- **Gemma-First Pipeline V2 Staging (2026-02-28)**:
  - Added Toolserver pipeline endpoints: `POST /pipeline/query`, `POST /pipeline/eval`, and `POST /pipeline/index/build` with deterministic triage/retrieval/synthesis/verification flow.
  - Added pipeline schemas (`TriagedQueryPlan`, `ToolCall`, `VisualEvidence`, `EvidenceBundle`, `PipelineAnswer`) and audit-mode strict verification behavior (hard-fail in audit mode only).
  - Added feature-flag gating via `RC_AI_PIPELINE_V2` / `RC_AI_AUDIT_STRICT` with config fallback from `AI/ai_config.json`.
  - Added Gemma-first model role routing defaults for pipeline v2 (`functiongemma`, `codegemma`, `gemma3:4b`, `gemma3:12b`, `embeddinggemma`) while keeping legacy endpoints unchanged.
  - Added JSONL-based semantic index scaffolding under `tools/.ai-index/` (chunking, embed build, flat cosine retrieval).
  - Updated `rc-ai-query`/`rc-ai-eval` to call Toolserver pipeline endpoints when `RC_AI_PIPELINE_V2=on`, with fallback to legacy direct Ollama flow when off.
  - Added new AI config keys (`controller_model`, `triage_model`, `synth_fast_model`, `synth_escalation_model`, `embedding_model`, `vision_model`, `ocr_model`, `pipeline_v2_enabled`, `audit_strict_enabled`, `embedding_dimensions`) and C++ parser support.
  - Validation status: Python and PowerShell syntax checks pass for updated files; live HTTP endpoint smoke is pending in a Windows shell with `fastapi`/`uvicorn` installed.
  - Performance profile update: set `synth_escalation_model` to `gemma3:4b` in `AI/ai_config.json` to avoid automatic 12B latency on current hardware.
  - Bridge hardening update: `Start_Ai_Bridge_Fixed.ps1` now verifies pipeline-v2 endpoint availability when reusing an existing listener and warns/fails fast on legacy bridge processes that lack `/pipeline/query`.
  - Dev-shell fix: `rc-ai-query`/`rc-ai-eval` now only apply `-Model` overrides when explicitly passed (v2 no longer silently forces `deepseek-coder-v2:16b`).
  - Dev-shell UX hardening: improved pipeline error messaging for 404/legacy bridge scenarios with explicit restart command guidance.
  - Visualizer AI Console enhancement: added a safe terminal-like command launcher (allowlisted dev-shell commands) for runtime/build control from inside the app panel.
  - Dev-shell model defaults migrated to Gemma-first stack (`gemma3:4b` default for smoke/query/eval); `rc-ai-setup` now pulls Gemma-first models instead of DeepSeek.
  - Applied pre-tuned Gemma generation defaults for pipeline/dev-shell (`temperature=0.10`, `top_p=0.85`, `num_predict=256`) based on quick latency/quality sweep.
  - Full 6-case tuning sweep completed: near-fast profiles tied at `2/6` pass with stable required-path recall failures; `-IncludeRepoIndex` currently degrades to `0/6` and remains non-default.
  - Implemented deterministic required-path enforcement in pipeline post-processing; full 6-case eval now passes `6/6` with balanced profile (`temperature=0.10`, `top_p=0.85`, `num_predict=256`).
  - Added Program Perception endpoints in toolserver: `POST /perception/observe` and `POST /perception/audit` with capture, mockup contract checks, code-candidate mapping, and hybrid vision/OCR hooks (degrading safely when screenshots/models are unavailable).
  - Added perception shell commands: `rc-perceive-ui` and `rc-perception-audit`.
  - Added custom Python MCP server scaffold at `tools/mcp-server/roguecity-mcp/` with allowlisted tools for bridge/build/snapshot/perception/pipeline/report orchestration.
  - Extended AI Console allowlisted command launcher with `rc-perceive-ui` and `rc-perception-audit`.
  - Bridge startup endpoint listing now advertises perception endpoints when pipeline v2 is available.
  - Runtime compatibility hardening: migrated remaining legacy model defaults from DeepSeek to Gemma-first (`gemma3:4b` / `codegemma:2b`) across `AI/ai_config.json`, C++ AI config fallbacks, bridge startup mode probe, and toolserver legacy request defaults.
  - Bridge startup now initializes compatibility env defaults (`RC_AI_PIPELINE_V2=on`, model-role envs, bridge base URL) when unset for deterministic shell/runtime behavior.
  - Dev shell “hypercharge” commands added: `rc-ai-harden`, `rc-mcp-setup`, and deterministic `rc-mcp-smoke` readiness JSON for bridge + endpoint + MCP dependency checks.
  - Env doctor expanded with AI stack checks (ai_config alignment, bridge script presence, MCP server presence, Python AI module inventory, and Ollama model stack completeness).
  - RogueCity MCP server upgraded with: `--self-test`, strict build target/preset allowlists, repo-path sanitization for path-bearing tools, `rc_env_validate`, and composite `rc_observe_and_map`.
  - Added dual-runtime bridge strategy for broad compatibility:
    - Cross-host bind mode in `Start_Ai_Bridge_Fixed.ps1` via `-BindAll` / `-BindHost`, with WSL/Linux access hints in startup output.
    - Same-environment WSL bridge scripts: `tools/start_ai_bridge_wsl.sh` and `tools/stop_ai_bridge_wsl.sh`.
    - Dev-shell wrappers: `rc-ai-start-wsl` and `rc-ai-stop-wsl`.
    - MCP tools for same-runtime control: `rc_bridge_start_local` and `rc_bridge_stop_local`.
  - Applied Ollama tuning defaults in hardened env setup and bridge startup (`OLLAMA_FLASH_ATTENTION=1`, `OLLAMA_KV_CACHE_TYPE=f16`) when unset.
  - Cross-runtime Ollama compatibility hardening:
    - Toolserver Ollama calls now resolve candidate base URLs from `OLLAMA_BASE_URL`/`OLLAMA_HOST` with WSL-friendly fallbacks (`host.docker.internal`, `/etc/resolv.conf` nameserver) instead of hardcoded localhost.
    - `start_ai_bridge_wsl.sh` now supports `--ollama-base-url` and auto-selects a reachable Ollama base when unset.
    - Dev-shell Ollama-aware commands (`rc-ai-harden`, `rc-ai-smoke`, `rc-ai-query`) now use resolved Ollama base URLs.
    - MCP self-test/health probing now checks candidate Ollama endpoints and reports tried URLs for faster diagnostics.
    - Added short per-candidate connect timeouts for Toolserver Ollama fallback calls to prevent long stalls on blackhole hosts.
  - **PowerShell-Primary Stabilization Pass (2026-02-28)**:
    - Pipeline verifier now enforces answer quality (`answer_quality_ok`, `answer_quality_reason`) with retry/escalation; audit mode hard-fails on low-quality/empty answers and normal mode returns structured failure instead of silent empties.
    - Dev-shell strict query/eval paths now treat `answer_quality_ok=false` as a failure signal and expose answer-quality regression counts in eval summaries.
    - Added explicit runtime `Titlebar` and `Status Bar` introspection surfaces in `rc_ui_root.cpp` with live status counters (validation/log/dirty), closing missing-section contract gaps.
    - Added headless runtime screenshot export support via `--export-ui-screenshot <path>` in `RogueCityVisualizerHeadless` with hidden OpenGL render path and PNG framebuffer capture.
    - Updated headless CMake wiring to enable screenshot runtime when GLFW/OpenGL are available (including ImGui OpenGL backend and gl3w linkage).
    - Perception full mode now requires runtime screenshot artifacts for OCR/vision gate runs and no longer relies on seeded fixture screenshots in smoke flow.
    - Perception multimodal lane now runs vision/OCR concurrently with trimmed prompt/`num_predict` payloads for lower full-mode latency.
    - MCP self-test semantics now report `ok_core`, `ok_full`, and `runtime_recommendation`; top-level readiness maps to `ok_core` for PowerShell-primary operation.
    - Dev-shell `rc-mcp-smoke` now surfaces runtime recommendation and fallback commands; `rc-ai-start-wsl` warns and prints explicit PowerShell fallback commands on degraded WSL/Ollama routing.
    - Added commit-gate full smoke command path (`rc-full-smoke`) and upgraded `tools/.run/rc_full_smoke.ps1` to enforce gate checks (pipeline quality, required UI sections, p95 latency, runtime screenshot + OCR/vision evidence).
- **Dev Shell Headless + Local AI Command Surface (2026-02-28)**:
  - Added headless helpers in `tools/dev-shell.ps1`: `rc-bld-headless`, `rc-run-headless`, and `rc-smoke-headless`.
  - Added `rc-ai-query` to query local Ollama with workspace context from `.gemini/GEMINI.md` and `.agents/Agents.md`.
  - Updated `rc-help` command listing to document new headless and AI query workflows.
  - Added `--export-ui-snapshot` CLI support in `RogueCityVisualizerHeadless` to emit runtime UI introspection JSON for layout auditing.
  - Added `rc-ai-stop-admin [-Port]` to invoke elevated bridge stop when normal shell privileges cannot terminate inherited listener processes.
  - Hardened `rc-ai-query` with strict JSON contract, path existence validation, retry-on-invalid output, and deterministic sampling controls (`temperature`/`top_p`).
  - Added `rc-ai-eval` plus benchmark cases (`tools/ai_eval_cases.json`) to measure local-model grounding/compliance instead of ad-hoc prompts.
  - Added context file manifest injection to improve path-grounded answers from local model.
  - Added required-path enforcement (`RequiredPaths`) and correction retries so local AI can be pushed to include mandatory repo file references.
  - Added optional repo/search context injection (`-IncludeRepoIndex`, `-SearchPattern`) so local AI can ground answers using live file index/search evidence from the workspace.
- **AI Bridge Script Non-Interactive Hardening (2026-02-28)**:
  - Added `-NonInteractive` support to `tools/Start_Ai_Bridge_Fixed.ps1` and `tools/Stop_Ai_Bridge_Fixed.ps1` so bridge lifecycle can be automated from shell tooling and CI-like smoke flows.
  - Fixed PowerShell automatic variable collision in stop script by replacing local `$pid` usage with explicit target PID variables.
  - Added port-check fallback logic for environments where `netstat` is unavailable, using `Get-NetTCPConnection`.
  - Hardened stop behavior with explicit Windows `taskkill` fallback invocation path and clearer diagnostics when listener PID metadata is inaccessible.
  - Standardized restart semantics: `rc-ai-start` now force-recycles listener state; if termination is blocked and a healthy process remains, live-mode start now fails explicitly instead of silently reusing an unknown-mode bridge.
  - Added bridge health metadata (`mock`, `pid`) and local `/admin/shutdown` endpoint for graceful teardown flows.
  - Removed default `uvicorn --reload` from bridge startup to reduce orphan watcher edge cases during automation.
  - Added `-Port` support to `rc-ai-start`/`rc-ai-stop`/`rc-ai-restart` and bridge scripts for deterministic fallback when default listener ownership blocks termination.
  - Hardened `rc-ai-query` request serialization to UTF-8 JSON bytes and sanitized control characters from embedded context files.
- **AI Config DeepSeek Defaults + Fixed Script Paths (2026-02-28)**:
  - Updated AI config fallbacks (`AI/config/AiConfig.h/.cpp`) to use fixed bridge script names and standardized default models for code assistant/naming to `deepseek-coder-v2:16b`.
- **Build Warning Cleanup (2026-02-27)**:
  - Removed an unused `GlobalState` local in `visualizer/src/ui/panels/rc_panel_road_editor.cpp`.
  - Removed an unreferenced static helper `ToolLibraryPopoutWindowName(...)` in `visualizer/src/ui/rc_ui_root.cpp`.
  - Removed unused HUD color locals in `visualizer/src/ui/viewport/rc_viewport_overlays.cpp`.
  - Result: prior `C4189`/`C4505` warnings in the GUI target no longer reproduce.
- **Dev Shell Python Tooling Hardening (2026-02-28)**:
  - Refactored `Invoke-RCPythonTool` in `tools/dev-shell.ps1` to stop depending on `cmd` and instead resolve/execute `py`, `python`, or `python3` directly with safe argument forwarding.
  - Added launcher/path fallbacks and improved terminal error messaging so `rc-doctor`, `rc-problems`, and related `rc-*` Python helpers fail with actionable diagnostics instead of command-resolution crashes.
  - Normalized launcher success handling for environments where native process calls can leave `$LASTEXITCODE` unset despite successful execution.
  - Added de-duplicated launcher attempts, enforced execution from `$env:RC_ROOT`, and included per-attempt exit diagnostics to make cross-shell failures deterministic and debuggable.
- **Env Doctor + Compile Commands Workflow (2026-02-28)**:
  - Updated `tools/env_doctor.py` to treat Visual Studio generator workflows as valid when `build_vs/compile_commands.json` is absent, while still preferring and validating Ninja-style compile databases when available.
  - Generated `build_ninja/compile_commands.json` for editor tooling and verified `env_doctor` now reports `PASS` for compile commands in this workspace.
- **Toolchain + AI + Tooling Validation Sweep (2026-02-28)**:
  - Fixed `env_doctor` toolchain probing for `VSCMD_VER` in WSL-hosted cmd bootstrap flow and verified full doctor status reached `PASS=6 WARN=0 FAIL=0`.
  - Completed a full visualizer build + AI bridge mock connectivity smoke (`/health`, `/ui_agent`, `/city_spec`) and a practical `rc-*` tool suite run with all tested commands passing.
- **AI Base Model Standardization (2026-02-28)**:
  - Updated base defaults from `qwen2.5:latest` to `deepseek-coder-v2:16b` across shell smoke tooling, Python toolserver request models, and C++ AI config fallbacks.
  - Verified non-mock live AI smoke executes on `deepseek-coder-v2:16b` with healthy `/health`, valid `city_spec` response, and no transport/API errors.

### Done
- Implemented viewport render spatial indexing in `GlobalState` with CSR layers (`roads`, `districts`, `lots`, `water`, `buildings`) and per-entity ID-to-handle maps.
- Extended `fva::Container` API (`const operator[]`, `isValidIndex`, `indexCount`) to support safe render-time dereference from `const GlobalState`.
- Updated `ViewportIndexBuilder::Build` to rebuild both legacy `viewport_index` and new render spatial index/ID maps in one deterministic pass.
- Wired derived-index lifecycle through existing flows (`CityOutputApplier` and axiom editor viewport guard) without introducing manager classes.
- Added viewport LOD policy and active-domain visibility override in overlay/panel rendering paths.
- Refactored major overlay loops to visible-cell traversal with dedupe passes for roads/districts/lots/water/buildings.
- Centralized base road rendering into `rc_viewport_overlays.cpp` (`RenderRoadNetwork`) so core geometry and overlay rendering share one viewport-local pipeline.
- Added reusable scratch buffers in overlays to reduce per-frame allocations on static geometry draw paths.

### Need To Do
- Add missing tests `tests/test_viewport_spatial_grid.cpp` and `tests/test_viewport_lod_policy.cpp`, then register both in `CMakeLists.txt` with `add_test`.
- Run and document telemetry/perf acceptance at metro scale, including `< 2 ms` zoomed-out target on baseline machine profile.
- Decide and document whether minimap LOD policy remains intentionally separate or is unified with viewport LOD policy.
- Complete pass-3 research items (grid occupancy tuning, pathological geometry behavior, and GPU-forward data-layout notes).

### Verified
- CMake reconfiguration succeeded with `-DCMAKE_TOOLCHAIN_FILE=C:/Users/teamc/vcpkg/scripts/buildsystems/vcpkg.cmake`, `-DVCPKG_TARGET_TRIPLET=x64-windows`, and `-DROGUECITY_BUILD_VISUALIZER=ON`.
- Build succeeded for visualizer runtime target `RogueCityVisualizerHeadless`.
- Smoke run succeeded (exit code `0`) for `./bin/RogueCityVisualizerHeadless.exe --help` (under timeout).
- Full project build completed after integration changes with no new blocking compile/link errors in modified viewport and editor files.
- Visualizer build passed on 2026-02-27 via:
  - `"/mnt/c/Program Files/CMake/bin/cmake.exe" --build build_vs --config Release --target RogueCityVisualizerGui --parallel 4`

## [0.11.0] - 2026-02-24

### Added
- **Foundational Mandates (`.gemini/GEMINI.md`)**: Established core architectural and engineering standards, including layer ownership and mathematical reference tables for AESP and Grid Quality.
- **Urban Analytics Core (`GridMetrics.hpp`)**: Introduced `GridQualityReport` and `RoadStroke` data structures to the core layer for semantic road network analysis.
- **Grid Quality Engine (`GridAnalytics.cpp`)**: Developed a high-performance module to calculate Straightness ($\varsigma$), Orientation Order ($\Phi$), and 4-Way Proportion ($I$) through stroke-based extraction.
- **"Urban Hell" Diagnostics**: Implemented hardening checks for disconnected network islands, dead-end sprawl (Gamma Index), and micro-segment geometric health.
- **Spline Tool Architecture (`SplineManipulator.cpp`)**: Centralized interactive spline logic into a reusable manipulator in the `app` layer for consistent road and river editing.
- **Hydrated Viewport Tools**: Replaced `RoadTool` and `WaterTool` stubs with functional implementations that handle vertex dragging, pen-tool addition, and proportional editing.
- **Telemetry UI Integration**: Enhanced the `Analytics` panel with real-time visual feedback for grid quality metrics and structural integrity warnings.

### Build & UI Architecture Hardening
- **Centralized Event System (`Infomatrix`)**: Created a decoupled event logging and telemetry backbone in the `core` layer to separate generator data from UI sinks.
- **Type-Driven UI Layout**: Implemented a declarative panel schema in `rc_ui_root.cpp` that drives docking and visibility via `PanelType` routing, replacing scattered imperative calls.
- **Button-to-Dock Popouts**: Introduced `ButtonDockedPanel` functionality, allowing UI components to toggle between docked states and floating windows with Shift-click overrides.
- **Live Validation Panel**: Added a dedicated panel for real-time generator rule violations and constraint monitoring, powered by the new `Infomatrix` stream.
- **Build System Hardening**: Fixed `vcpkg` toolchain integration, resolved cross-layer namespace ambiguities (`Urban` vs `Roads`), and corrected multiple header/source linkage issues.
- **Core Utility Consolidation**: Centralized entity lookup helpers into `EditorUtils` to resolve unresolved external symbols and maintain strict layer boundaries.

### Changed
- **Pipeline Integration**: Updated `CityGenerator` to compute and cache grid analytics at the end of the road tracing stage for zero-latency UI updates.
- **Global State Wiring**: Refactored `CityOutputApplier` and `GlobalState` to persist and broadcast urban analytics across the application.
- **Build System**: Registered the new Scoring and Analytics modules in the `RogueCityGenerators` library via `CMakeLists.txt`.

## [0.10.0] - 2026-02-19

### Added
- Determinism hashing and baseline validation APIs:
  - `RogueCity/Core/Validation/DeterminismHash.hpp`
  - `ComputeDeterminismHash`, `SaveBaselineHash`, `ValidateAgainstBaseline`
- Fixed-step simulation pipeline API:
  - `RogueCity/Core/Simulation/SimulationPipeline.hpp`
- Stable viewport identity API:
  - `RogueCity/Core/Editor/StableIDRegistry.hpp`
- Incremental/cancelable generation APIs:
  - `RogueCity/Generators/Pipeline/GenerationStage.hpp`
  - `RogueCity/Generators/Pipeline/GenerationContext.hpp`
- Profile-driven AESP scoring API:
  - `RogueCity/Generators/Scoring/ScoringProfile.hpp`
- CMake package export for external consumers:
  - `RogueCitiesConfig.cmake`
  - `RogueCitiesTargets.cmake`

### Changed
- Generator pipeline now supports staged incremental regeneration and cancellation context propagation.
- Performance checks for generation latency and stage behavior are integrated into the test suite.
- Install rules now include generator headers and libraries for package consumers.

### Fixed
- Removed legacy, unreferenced compatibility include headers under:
  - `generators/include/Pipeline/`
  - `generators/include/Roads/`
  - `generators/include/Tensors/`
  - `generators/include/Districts/`
- Addressed test warning regressions in:
  - `tests/test_core.cpp`
  - `tests/test_viewport.cpp`

### Verification Highlights
- Clean build and full test suite pass (`26/26`).
- Determinism baseline hash regenerated from a clean state and validated.
- External package consumption validated via `find_package(RogueCities CONFIG REQUIRED)`.
