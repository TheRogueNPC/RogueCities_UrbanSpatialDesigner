---
tags: [roguecity, ui, panel-patterns, docking, tokens]
type: concept
created: 2026-02-15
---

# UI Token Panel and Dock Wrapper Patterns (ImGui)

Panel rendering conventions require wrapper APIs for token styling and docking behavior so panels remain consistent, recoverable, and compatible with saved workspace presets.

## Canonical Wrapper APIs
- `RC_UI::Components::BeginTokenPanel(...)`
- `RC_UI::BeginDockableWindow(...)`
- `RC_UI::Components::TextToken(...)`
- Workspace preset helpers: save/load/list APIs

## Source Files
- `docs/UI_PATTERNS.md`
- `visualizer/src/ui/rc_ui_components.h`

## Related
- [[topics/ui-system-and-panel-patterns]]
- [[notes/ui-migration-compliance-and-automation]]
- [[notes/module-visualizer-executables]]
