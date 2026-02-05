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
- Follow the agent roles and mandates in [AGENTS.md](AGENTS.md) when delegating tasks (Architect + helper agents).

## Useful references
- Design/architecture narrative: [ReadMe.md](ReadMe.md) and [docs/TheRogueCityDesignerSoft.md](docs/TheRogueCityDesignerSoft.md).
