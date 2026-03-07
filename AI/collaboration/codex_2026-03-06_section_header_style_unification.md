# Codex Handoff - Section Header Style Unification (2026-03-06)

## Request
Normalize fold-node visuals in Dev Shell, UI Settings, and AI panels to match Inspector-style folding nodes.

## Implemented
Converted target panels to shared token section headers (`API::SectionHeader`) and removed mixed/default collapsing styles in those areas.

### Updated files
- `visualizer/src/ui/panels/rc_panel_dev_shell.cpp`
  - `Developer Mode`, `ImGui Debug Productivity`, `Test Suite (CTest)`, `PVS-Studio Static Analysis`
- `visualizer/src/ui/panels/rc_panel_ui_settings.cpp`
  - `Theme Selection`, `Theme Editor Options`, `Display Options`, `Layout Preferences`, `Custom Theme (Advanced)`
- `visualizer/src/ui/panels/rc_panel_ai_console.cpp`
  - `AI Bridge Unavailable`, `Bridge Status`, `Dev Terminal (Safe)`, `Control`, `Model Configuration`, `Connection`
- `visualizer/src/ui/panels/rc_panel_ui_agent.cpp`
  - `Layout Optimization`, `Design & Refactoring`
- `visualizer/src/ui/panels/rc_panel_city_spec.cpp`
  - `City Description`, `Generated Specification`

## Verification
- `python tools/check_imgui_contracts.py` -> pass
- `python tools/check_ui_compliance.py` -> pass
- `cmake --build build_vs --target RogueCityVisualizerHeadless --config Debug -j 8` -> pass
- `cmake --build build_vs --target RogueCityVisualizerGui --config Debug -j 8` -> linker file lock if exe running (`LNK1168`), not code failure.
