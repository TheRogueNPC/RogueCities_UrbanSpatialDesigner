# Copilot instructions for RogueCities_UrbanSatialDesigner

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
- Tests are currently a single executable: [test_generators.cpp](test_generators.cpp). Build target `test_generators`, run directly or via `ctest --test-dir build --output-on-failure`.

## Dependencies & integration points
- GLM is required; it is found via `find_package(glm)` or from `3rdparty/glm` fallback. See [CMakeLists.txt](CMakeLists.txt).
- `magic_enum` is vendored and required at `3rdparty/magic_enum/include`. The configure step errors if missing. See [CMakeLists.txt](CMakeLists.txt).

## Project-specific coding patterns
- Core types are under `RogueCity::Core` (e.g., `Vec2`, `Tensor2D`) and are intended to stay UI-free. See [core/include/RogueCity/Core](core/include/RogueCity/Core).
- Generation pipeline stages are explicit methods in `CityGenerator` (tensor field → seed generation → road tracing → future districts/lots). Keep new stages in this orchestrator to preserve the pipeline flow. See [generators/src/Generators/Pipeline/CityGenerator.cpp](generators/src/Generators/Pipeline/CityGenerator.cpp).
- Road classification and AESP values are table-driven; if you add road types or adjust AESP weights, update the lookup arrays in `AESPClassifier` to keep tests and docs consistent. See [generators/include/RogueCity/Generators/Districts/AESPClassifier.hpp](generators/include/RogueCity/Generators/Districts/AESPClassifier.hpp).

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
- If the request touches Lua-facing APIs or signature stability → consult Commenter/API Alias Keeper.
  - Use: read_file on exposed headers, list_code_usages for impacted symbols, update comments/aliases.
- If the request touches UI workflows or ImGui panels → consult ImGui Designer Agent.
  - Use: read_file under app/, grep_search for ImGui panels, ensure Core stays UI-free.

## Useful references
- Design/architecture narrative: [ReadMe.md](ReadMe.md) and [docs/TheRogueCityDesignerSoft.md](docs/TheRogueCityDesignerSoft.md).
