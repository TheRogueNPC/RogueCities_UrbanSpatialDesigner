# RogueCities Urban Spatial Designer — Claude Memory

## Project Identity
- **Name**: RogueCities_UrbanSpatialDesigner (RC-USD)
- **Stack**: C++20, ImGui (immediate mode), OpenGL, CMake/vcpkg, Windows/MSVC
- **Version**: RC-0.09-Test (Feb 2026) | Branch: main
- **Goal**: Procedural city generator with interactive ImGui editor + AI augmentation (Ollama toolserver)
- **Design philosophy**: "Goldilocks Complexity" — AESP zoning, Grid Index metrics, Organized Complexity

## Layer Architecture (STRICT — no exceptions)
| Layer | Dir | Owns | Must NOT own |
|---|---|---|---|
| 0 Core | `core/` | data types, math, GlobalState, HFSM | ImGui, algorithms |
| 1 Generators | `generators/` | CityGenerator, tensor/road/district/lot/building | ImGui, UI |
| 2 App | `app/` | GeneratorBridge, tools, AxiomVisual | raw ImGui panels |
| 3 Visualizer | `visualizer/` | ImGui panels, overlays, viewport | generator logic |
| AI | `AI/` | toolserver clients, protocol, pattern catalog | — |

Data flow: `AxiomVisual → GeneratorBridge → CityGenerator → CityOutputApplier → GlobalState → Visualizer`

## Container Policy
- `fva::Container<T>` (FVA): editor-facing, stable external handles (Roads, Districts, Lots, Axioms, WaterBodies)
- `civ::IndexVector<T>` (CIV): internal/scratch, performance-critical
- `siv::Vector<T>` (SIV): long-lived refs, validity-checked (BuildingSites)

## Key Patterns (see patterns.md for details)
- Index panels: `RcDataIndexPanel<T, Traits>` template in `visualizer/src/ui/patterns/rc_ui_data_index_panel.h`
- Trait specializations: `visualizer/src/ui/panels/rc_panel_data_index_traits.h`
- Panel interface: `IPanelDrawer` in `visualizer/src/ui/panels/IPanelDrawer.h`
- Panel registration: `PanelRegistry` + `InitializePanelRegistry()` in `visualizer/src/ui/panels/`
- All drawers: content only (no ImGui::Begin/End) — RcMasterPanel owns the window
- HFSM: `core/include/RogueCity/Core/Editor/EditorState.hpp`
- Heavy work: `RogueWorker` (never in UI callbacks or HFSM transitions)
- Boost geometry: default for all polygon/distance/hull ops — do NOT replace with ad-hoc math
- Surgical edits only; no rewrites of unrelated code

## Important File Paths
- Agent rules: `.github/agents/RC.agent.md`
- GlobalState: `core/include/RogueCity/Core/Editor/GlobalState.hpp`
- EditorState/HFSM: `core/include/RogueCity/Core/Editor/EditorState.hpp`
- CityTypes enums: `core/include/RogueCity/Core/Data/CityTypes.hpp`
- CityGenerator: `generators/include/RogueCity/Generators/Pipeline/CityGenerator.hpp`
- TensorFieldGenerator: `generators/include/RogueCity/Generators/Tensors/TensorFieldGenerator.hpp`
- Road policy config: `generators/config/road_policy_defaults.json`
- GeneratorBridge: `app/include/RogueCity/App/Integration/GeneratorBridge.hpp`
- AxiomVisual: `app/include/RogueCity/App/Tools/AxiomVisual.hpp`
- AxiomPlacementTool: `app/include/RogueCity/App/Tools/AxiomPlacementTool.hpp`
- ThemeManager: `app/include/RogueCity/App/UI/ThemeManager.h`
- GeometryPolicy: `app/include/RogueCity/App/Tools/GeometryPolicy.hpp`
- RcMasterPanel: `visualizer/src/ui/panels/RcMasterPanel.h/.cpp`
- PanelRegistry (viz): `visualizer/src/ui/panels/PanelRegistry.h/.cpp`
- UI components: `visualizer/src/ui/rc_ui_components.h`
- AI docs: `AI/docs/` | AI clients: `AI/client/`

## Currently Modified Files (git status)
- `app/include/RogueCity/App/UI/ThemeManager.h`
- `visualizer/src/ui/panels/RcMasterPanel.cpp`
- `visualizer/src/ui/panels/rc_panel_ui_settings.cpp`
- `visualizer/src/ui/rc_ui_components.h`

## Current Status (Feb 2026)
- Phase 2 refactor complete: RcDataIndexPanel<T> template, selection/undo/redo, dirty-layer propagation
- Road editor panel added; GeometryPolicy, road policies + validation UI done
- In progress: StreetSweeper full pipeline, Lua/serialization compat layer
- Next: preset/template system, interchange templates, code-shape AI refactoring assistant

## Build Commands
```bash
cmake.exe --build build_vs --config Debug --target <target>
./bin/test_generators.exe | ./bin/test_full_pipeline.exe | ./bin/test_city_generator_validation.exe
```

## Key Runtime Types (quick lookup)
- GenerationScope: RoadsOnly | RoadsAndBounds | FullCity (CityOutputApplier)
- GenerationDepth: AxiomBounds | FullPipeline (RealTimePreview)
- GenerationPhase: Idle | InitStreetSweeper | Sweeping | Cancelled | StreetsSwept
- GenerationRequestReason: Unknown | LivePreview | ForceGenerate | ExternalRequest
- InteractionOutcome: None|Mutation|Selection|GizmoInteraction|ActivateOnly|BlockedByInputGate|NoEligibleTarget
- VpEntityKind: Unknown|Axiom|Road|District|Lot|Building|Water|Block
- GenerationMutationPolicy: Axiom=LiveDebounced, all others=Explicit (WP4 contract)

## Active Contracts (RC-0.09)
- WP1: Input gate must be viewport-local (NOT global WantCaptureMouse). Blocker at rc_ui_input_gate.cpp:20
- WP3: Every tool action → InteractionOutcome (never silent). rc_viewport_interaction.cpp:1255 is known exit gap
- WP4: ApplyCityOutputToGlobalState is the ONLY output-application path
- WP5: Min window = max(25% display, 1100x700)
- Infomatrix: ring buffer (kMaxEvents=220) in GlobalState; Categories: Runtime/Validation/Dirty/Telemetry
- NO C++ reflection — always manual property grids in inspector/property editor

## Agent Collaboration
Working alongside: Codex, Copilot (GitHub), Gemini, Perplexity
- See `patterns.md` for full API details (panels, interaction, overlays, generation pipeline)
- See `domain.md` for generation pipeline, enums, GlobalState
- See `ui.md` for visualizer file map and panel system
- See `containers_and_threading.md` for fva/civ/siv internals and RogueWorker
- See `ai_integration.md` for toolserver, AI clients, phase roadmap
- See `ci_tools_contracts.md` for CI workflows, tools scripts, active WP contracts

## User/Workflow Preferences
- Surgical edits; prefer extending existing contracts
- Explain WHY (architecture intent), not just WHAT
- Always provide handoff checklist for non-trivial tasks (Correctness/Numerics/Complexity/Performance/Determinism/Tests/Risks)
- See RC.agent.md §17-19 for full imperative DO/DON'T and math excellence standards
