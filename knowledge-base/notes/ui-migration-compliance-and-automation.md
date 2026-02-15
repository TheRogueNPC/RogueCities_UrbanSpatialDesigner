---
tags: [roguecity, ui-migration, compliance, design-system]
type: how-to
created: 2026-02-15
---

# UI Migration Compliance and Automation (Design System Enforcement)

UI migration policy prohibits raw panel styling shortcuts and enforces token/wrapper architecture through `tools/check_ui_compliance.py`, with `ROGUEUI_ENFORCE_DESIGN_SYSTEM` enabled by default in the root build.

## Rules Snapshot
- Avoid raw `ImGui::Begin(...)` panel shells
- Avoid direct color literals in panel sources
- Route styling through shared UI token system

## Enforcement Integration
- Script: `tools/check_ui_compliance.py`
- Build switch: `ROGUEUI_ENFORCE_DESIGN_SYSTEM`
- CMake adds `check_ui_compliance` dependency for visualizer targets

## Source Files
- `docs/UI_MIGRATION.md`
- `CMakeLists.txt`

## Related
- [[topics/ui-system-and-panel-patterns]]
- [[notes/ui-token-panel-and-dock-wrapper-patterns]]
- [[notes/phase-4-code-shape-design-assistant]]
