---
tags: [roguecity, core, data-models, editor-state]
type: reference
created: 2026-02-15
---

# Core Library Data and Editor Types (RogueCityCore)

`RogueCityCore` contains the foundational data and math types, editor state models, texture-processing utilities, and validation modules, and it explicitly blocks accidental UI dependencies in its CMake checks.

## Notable Core Areas
- Math/data: `Vec2`, `CityTypes`, `TensorTypes`, `CitySpec`
- Editor state: `EditorState`, `GlobalState`, terrain/texture editing
- Utilities: CIV/SIV containers and `RogueWorker`
- Validation: editor integrity and overlay validation

## Source Files
- `core/CMakeLists.txt`
- `core/include/RogueCity/Core/Types.hpp`

## Related
- [[topics/project-overview-and-architecture]]
- [[notes/testing-suite-coverage-and-execution]]
- [[notes/layered-module-structure-core-generators-app-ai-visualizer]]
