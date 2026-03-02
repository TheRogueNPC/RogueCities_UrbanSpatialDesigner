# Claude Phase 5 — Inspector Sidebar + Inspector Enhancements

**Date:** 2026-02-28
**Phase:** 5 (UI Refactor — 3-column layout match to RC_UI_Mockup.html)
**Author:** Claude (claude-sonnet-4-6)

---

## What Was Done

### New Files Created

| File | Purpose |
|------|---------|
| `visualizer/src/ui/panels/rc_panel_inspector_sidebar.h` | `RcInspectorSidebar` class declaration |
| `visualizer/src/ui/panels/rc_panel_inspector_sidebar.cpp` | Unified right-column window: "Inspector" window with tab bar switching between Inspector and System Map content |

### Files Modified

#### `visualizer/src/ui/panels/rc_panel_inspector.cpp`
- Added **Viewport Status** row (from `gs.tool_runtime.last_viewport_status`)
- Added **Gen Policy** row (via `gs.generation_policy.ForDomain(active_domain)`)
- Added **domain-specific subtool rows** (Road: subtool + spline sub; Water/Flow: subtool; District: subtool)
- Added **Gizmo Operation combo** (`"Translate"/"Rotate"/"Scale"` → writes `gs.gizmo.operation`)
- Added **"Assign Selection → Active Layer"** button in LAYER MANAGER section
- Added **VALIDATION OVERLAY** section (enabled/show_warnings/show_labels checkboxes + error count)
- Fixed pre-existing `UITokens::OrangeWarning` (undefined) → replaced with `UITokens::AmberGlow`

#### `visualizer/src/ui/panels/RcPanelDrawers.cpp`
- Added `#include` for `rc_panel_validation.h` and `rc_panel_workspace.h`
- Added `namespace Validation { class Drawer ... }` (category: System, type: PanelType::Validation)
- Added `namespace WorkspaceSettings { class Drawer ... }` (category: System, type: PanelType::UISettings)

#### `visualizer/src/ui/panels/PanelRegistry.cpp`
- Added forward declarations for `Validation::CreateDrawer()` and `WorkspaceSettings::CreateDrawer()`
- Registered both in `InitializePanelRegistry()` under "Register System" block

#### `visualizer/src/ui/rc_ui_root.cpp`
- Added `#include "ui/panels/rc_panel_inspector_sidebar.h"`
- Added `static std::unique_ptr<RC_UI::Panels::RcInspectorSidebar> s_inspector_sidebar` in anonymous namespace
- Initialized `s_inspector_sidebar` alongside `s_master_panel` in the lazy-init block
- **Replaced** `Panels::Inspector::Draw(dt)`, `Panels::SystemMap::Draw(dt)`, `Panels::Workspace::Draw(dt)` with single `s_inspector_sidebar->Draw(dt)` call

#### `visualizer/CMakeLists.txt`
- Added `src/ui/panels/rc_panel_inspector_sidebar.cpp` to both `RogueCityVisualizerHeadless` and `RogueCityVisualizerGui` target_sources

---

## Architecture Decisions

1. **Window name stays `"Inspector"`** — `BeginTokenPanel("Inspector", ...)` preserves saved dock layout node IDs. The tab bar is internal (ID `##InspectorSidebarTabs`).

2. **`Workspace::Draw(dt)` removed as standalone** — Its content is now accessible via `WorkspaceSettings::Drawer` registered in the System tab of RcMasterPanel. No content is lost; only the outer window wrapper is gone.

3. **`Validation::DrawContent(dt)` confirmed** — `rc_panel_validation.h` already declares both `Draw(float dt)` and `DrawContent(float dt)`. The drawer uses `DrawContent`.

4. **`OrangeWarning` token fix** — Pre-existing reference to undefined `UITokens::OrangeWarning` replaced with `UITokens::AmberGlow` (255, 180, 0) which is the closest existing warm-orange token.

---

## Verification Checklist

Run after build:
```
rc-perceive-ui -Mode quick -Screenshot
```

Expected in `AI/docs/ui/ui_introspection_headless_latest.json`:
- `"id": "Inspector"` with `"dock_area": "Right"` containing widgets from both Inspector tab and System Map tab
- `"id": "WorkspaceSelector"` still present in left dock (unchanged in MasterPanel)
- No orphaned `"id": "System Map"` standalone panel
- Validation section widgets (`Show Overlay`, `Show Warnings`, `Show Labels`, `Errors: N`) present inside Inspector

---

## Handoff Notes for Next Agent

- The `Workspace::Draw(dt)` function still exists in `rc_panel_workspace.h/.cpp` — it's no longer called from `rc_ui_root.cpp`. If a standalone window is ever needed again, it's still there.
- `Inspector::Draw(dt)` also still exists but is no longer called; `DrawContent(dt)` is called by the sidebar and by the `Inspector::Drawer` in `RcPanelDrawers.cpp`.
- The `PanelType::UISettings` enum slot is reused for `WorkspaceSettings::Drawer`. If a separate UISettings panel is needed in future, a new enum value should be added.
