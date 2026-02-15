---
tags: [roguecity, visualizer, imgui, glfw, gui, headless]
type: reference
created: 2026-02-15
---

# Visualizer Executables (Headless and GUI Targets)

The visualizer layer defines two executables: a headless app and a full GUI app using GLFW/OpenGL/ImGui, each linking the shared project libraries plus panel implementations.

## Targets
- `RogueCityVisualizerHeadless`: non-GLFW panel runtime and logic
- `RogueCityVisualizerGui`: GLFW + OpenGL + ImGui backend integration

## Feature Gates
- `ROGUE_AI_DLC_ENABLED` toggles AI panel inclusion
- `ROGUE_SHIP_DEMO_MODE` forcibly disables AI DLC features

## Source Files
- `visualizer/CMakeLists.txt`
- `visualizer/src/ui/panels/`

## Related
- [[topics/project-overview-and-architecture]]
- [[topics/ui-system-and-panel-patterns]]
- [[notes/layered-module-structure-core-generators-app-ai-visualizer]]
