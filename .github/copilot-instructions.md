# Copilot instructions for RogueCities_UrbanSatialDesigner

## Recent Updates (RC-0.09-Test)

**Date**: February 7, 2026  
**Integration Status**: ✅ Phase 2 Refactor + AI Bridge Startup Fixes Complete

### AI Assistant Integration
Four-phase AI assistant integration is now complete and production-ready:

1. **Phase 1**: AI Bridge Runtime Control
   - PowerShell bridge management with pwsh/powershell fallback
   - Health check polling via WinHTTP
   - AI Console panel for start/stop control
   - Configuration: `AI/ai_config.json`, `AI/config/AiConfig.*`

2. **Phase 2**: UI Agent Protocol
   - Real WinHTTP client (replaced stub)
   - UiAgentClient for layout optimization
   - Natural language UI commands
   - Protocol types: `AI/protocol/UiAgentProtocol.*`

3. **Phase 3**: CitySpec MVP
   - AI-driven city design from natural language
   - CitySpec types in core: `core/include/RogueCity/Core/Data/CitySpec.hpp`
   - CitySpecClient and generator panel
   - JSON serialization for city specifications

4. **Phase 4**: Code-Shape Awareness
   - Enhanced UiSnapshot with code metadata (role, owner_module, data_bindings)
   - UI Pattern Catalog: `AI/docs/ui/ui_patterns.json`
   - Design Assistant for refactoring suggestions
   - Dual-mode UI Agent (layout + design/refactor planning)

### Phase 2 (RC-0.09-Test) Additions
- **Template Refactor**: `RcDataIndexPanel<T>` in `visualizer/src/ui/patterns/rc_ui_data_index_panel.h`
- **Traits**: `visualizer/src/ui/panels/rc_panel_data_index_traits.h`
- **Viewport Margins**: `visualizer/src/ui/rc_ui_viewport_config.h` for padding/margins
- **Context Menus**: Right-click menus in index panels with HFSM hooks
- **Lua Linking Fix**: Visualizer links `sol2::sol2` when available
- **Startup Fixes**: New bridge scripts + 3-tier fallback

### Key Components
- **Runtime**: `AI/runtime/AiBridgeRuntime.*` - Bridge lifecycle management
- **Clients**: `AI/client/*` - UiAgentClient, CitySpecClient, UiDesignAssistant
- **Toolserver**: `tools/toolserver.py` - FastAPI endpoints for AI services
- **UI Panels**: `visualizer/src/ui/panels/rc_panel_ai_*.cpp` - ImGui interfaces
- **Startup Scripts**: `tools/Start_Ai_Bridge_Fixed.ps1`, `tools/Stop_Ai_Bridge_Fixed.ps1`, `tools/Start_Ai_Bridge_Fallback.bat`
- **Utilities**: `tools/Quick_Fix.ps1`, `tools/create_shortcut.ps1`, `tools/move_object_files.ps1`, `tools/build_verify_RC-0.09.bat`

### Documentation
- Summary: `docs/AI_Integration_Summary.md`
- Phase 1: `docs/AI_Integration_Phase1_Complete.md`
- Phases 2-3: `docs/AI_Integration_Phase2_3_Complete.md`
- Phase 4: `docs/AI_Integration_Phase4_Complete.md`
- Quick ref: `AI/docs/Phase4_QuickReference.md`

---

## Big picture architecture
- Three-layer layout: core (data types + math), generators (procedural algorithms), app (ImGui UI). See [ReadMe.md](ReadMe.md) and [core/CMakeLists.txt](core/CMakeLists.txt).
- `RogueCityCore` is pure data/utility with zero UI deps; it hard-fails if imgui/glfw/glad are pulled in. See [core/CMakeLists.txt](core/CMakeLists.txt).
- `RogueCityGenerators` builds on core for tensor fields, streamline tracing, road/district logic. Key pipeline orchestrator: `CityGenerator` in [generators/include/RogueCity/Generators/Pipeline/CityGenerator.hpp](generators/include/RogueCity/Generators/Pipeline/CityGenerator.hpp).
- AESP district logic lives in `AESPClassifier` with fixed lookup tables that match the design doc. See [generators/include/RogueCity/Generators/Districts/AESPClassifier.hpp](generators/include/RogueCity/Generators/Districts/AESPClassifier.hpp).

## Build & test workflows (CMake)
- Standard build from repo root (core + generators + test executable):
  - Configure: `cmake -B build -S .`
  - Build core: `cmake --build build --target RogueCityCore --config Release`
  - Build generators: `cmake --build build --target RogueCityGenerators --config Release`
- Fast core-only iteration: `cmake -B build_core -S . -DBUILD_CORE_ONLY=ON` then `cmake --build build_core --target RogueCityCore --config Release` (documented in [ReadMe.md](ReadMe.md)).
- Tests are currently a single executable: update CMakeLists to reference `tests/test_generators.cpp`. Build target `test_generators`, run directly or via `ctest --test-dir build --output-on-failure`.

## Dependencies & integration points
- GLM is required; it is found via `find_package(glm)` or from `3rdparty/glm` fallback. See [CMakeLists.txt](CMakeLists.txt).
- `magic_enum` is vendored and required at `3rdparty/magic_enum/include`. The configure step errors if missing. See [CMakeLists.txt](CMakeLists.txt).

## Project-specific coding patterns
- Core types are under `RogueCity::Core` (e.g., `Vec2`, `Tensor2D`) and are intended to stay UI-free. See [core/include/RogueCity/Core](core/include/RogueCity/Core).
- Generation pipeline stages are explicit methods in `CityGenerator` (tensor field → seed generation → road tracing → future districts/lots). Keep new stages in this orchestrator to preserve the pipeline flow. See [generators/src/Generators/Pipeline/CityGenerator.cpp](generators/src/Generators/Pipeline/CityGenerator.cpp).
- Road classification and AESP values are table-driven; if you add road types or adjust AESP weights, update the lookup arrays in `AESPClassifier` to keep tests and docs consistent. See [generators/include/RogueCity/Generators/Districts/AESPClassifier.hpp](generators/include/RogueCity/Generators/Districts/AESPClassifier.hpp).

## State Machine (HFSM) Guidelines
- **Purpose:** The codebase now uses a deterministic hierarchical finite state machine (HFSM) for editor UI flows and discrete generator orchestration.
- **Where to review:** See `tests/test_editor_hfsm.cpp` and `ImDesignManager/design_manager.cpp` for concrete examples and tests.
- **Design rules:**
  - **Separation:** HFSM types and state implementations belong to the `app/` or `generators/` layers. Do not introduce HFSM types into `core/`.
  - **Performance:** Avoid heavy work inside state `enter`/`exit` handlers; delegate work >10ms to `RogueWorker` per the Rogue Protocol.
  - **Threading & Safety:** State transitions should run on the main thread by default. Offloaded transitions must be explicitly documented and made thread-safe.
  - **Instrumentation:** Add deterministic unit tests and transition logging when adding or modifying states; prefer adding tests in `tests/test_editor_hfsm.cpp`.
- **Agent responsibilities:**
  - **Coder Agent:** Implement new states in `app/` or `generators/`, expose only data-safe hooks, and follow HFSM rules above.
  - **Debug Manager Agent:** Add transition tests, timing assertions, and deterministic replay fixtures when changing HFSM behaviour.

## Agent usage
- Follow the agent roles and mandates in [.github/AGENTS.md](.github/AGENTS.md) when delegating tasks (Architect + helper agents).

## Helper agent decision trees (when to consult + tools)
- If the request changes C++ code paths → consult Coder Agent.
  - Use: file_search (locate files), read_file (confirm context), apply_patch (edit), get_errors (validate).
- If the request involves formulas, metrics, tensor fields, AESP weights, or grid index math → consult Math Genius Agent.
  - Use: read_file on [docs/TheRogueCityDesignerSoft.md](docs/TheRogueCityDesignerSoft.md), then read_file on relevant headers/impls; avoid guessing formulas.
- If the request is about district archetypes, zoning behavior, or player flow → consult City Planner Agent.
  - Use: read_file on [docs/TheRogueCityDesignerSoft.md](docs/TheRogueCityDesignerSoft.md) and AESP classifier/road types; verify semantics before edits.
- If the request affects counts, memory, or data growth → consult Resource Manager Agent.
  - Use: read_file on generator configs, grep_search for caps/limits, then suggest bounds.
- If the request is debugging, profiling, or reproducibility → consult Debug Manager Agent.
  - Use: grep_search for assertions/logging, run_in_terminal for build/test, get_errors for compile issues.
- If the request is docs or build instructions → consult Documentation Keeper Agent.
  - Use: read_file on [ReadMe.md](ReadMe.md) and docs, apply_patch for updates.
- If the request touches Lua-facing APIs or signature stability → consult Commenter/API Alias Keeper or Lua Overseer Agent.
  - Use: read_file on exposed headers, list_code_usages for impacted symbols, update comments/aliases.
- If the request touches UI/UX, ImGui panels, motion design, state-reactive surfaces, or editor experience → consult UI/UX/ImGui/ImVue Master.
  - Use: read_file under app/, grep_search for ImGui/HFSM integration, enforce Cockpit Doctrine (Vignelli structure, Y2K geometry, guided affordance).
  - Critical: Ensure Core stays UI-free, validate state-reactive design, check motion has instructional purpose.
- If the request involves AI integration, toolserver endpoints, protocol types, or assistant workflows → consult AI Integration Agent.
  - Use: read_file on [docs/AI_Integration_Summary.md](docs/AI_Integration_Summary.md), verify protocol compatibility, check toolserver.py endpoints.
  - Key files: `AI/protocol/UiAgentProtocol.*`, `AI/client/*`, `tools/toolserver.py`
  - Critical: Maintain backward compatibility, document protocol changes, ensure mock mode works.

## Useful references
- Design/architecture narrative: [ReadMe.md](ReadMe.md) and [docs/TheRogueCityDesignerSoft.md](docs/TheRogueCityDesignerSoft.md).
- Axiom tool integration roadmap: [docs/AxiomToolIntegrationRoadmap.md](docs/AxiomToolIntegrationRoadmap.md) (UI → Generator connection).
