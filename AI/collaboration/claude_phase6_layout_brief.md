# Claude Phase 6 Layout Refactor — Handoff Brief

**Date:** 2026-02-28
**Agent:** Claude (claude-sonnet-4-6)
**Phase:** 6 — Clean 3-Column Dock + Inline Tool Deck

---

## What Was Done

### Problem
The UI had three sources of layout chaos:
1. Six standalone tool library windows (`Road Library`, `Water Library`, etc.) rendered every frame, all trying to dock to `s_dock_nodes.library = 0` (unassigned) → floating windows
2. `BuildDockLayout` assigned dock slots to `"Tool Deck"` and `"Axiom Library"` (non-existent windows) and `"System Map"` (now a tab in Inspector sidebar)
3. Dead variables `dock_tool_deck`, `dock_library`, `tool_deck_ratio` caused C4189 warnings

### Changes Made

#### `visualizer/src/ui/rc_ui_root.cpp`
- **Removed** the 6-window render loop (`for (const auto tool : kToolLibraryOrder) { RenderToolLibraryWindow(...) }`)
  - `RenderToolLibraryWindow` function body kept (used by popout paths)
- **Cleaned up `BuildDockLayout`**:
  - Removed `dock_tool_deck`, `dock_library` (unused `ImGuiID`)
  - Removed `tool_deck_ratio` (unused `float`)
  - Reduced `DockBuilderDockWindow` calls from 6 to 3: `Master Panel` → left, `[/ / / RC_VISUALIZER / / /]` → center, `Inspector` → right
  - Updated layout comment to describe the actual 3-column viewport-centric design

#### `visualizer/src/ui/panels/rc_panel_tools.cpp`
- **Added includes**: `ui/rc_ui_components.h` and `<magic_enum/magic_enum.hpp>`
- **Added `DrawToolDeckSection(GlobalState& gs, float avail_width)`** static helper:
  - Computes column count from available width and 72px button width
  - `render_btn` lambda applies `CyanAccent` style when subtool is active
  - Switches on `gs.tool_runtime.active_domain` to show the right enum set:
    - `Road` → `RoadSubtool`
    - `Water`/`Flow` → `WaterSubtool`
    - `District`/`Zone` → `DistrictSubtool`
    - `Lot` → `LotSubtool`
    - `Building`/`FloorPlan`/`Furnature` → `BuildingSubtool`
    - `default` → `TextDisabled("No subtools for active domain.")`
  - Uses `magic_enum::enum_values<T>()` for iteration, `magic_enum::enum_name(sub).data()` for button label
- **Called from `DrawContent`** immediately after the tool mode buttons loop:
  ```cpp
  if (RC_UI::Components::DrawSectionHeader("TOOL DECK", UITokens::AmberGlow, /*default_open=*/true)) {
      ImGui::Indent();
      DrawToolDeckSection(gs, ImGui::GetContentRegionAvail().x);
      ImGui::Unindent();
      ImGui::Spacing();
  }
  ```

---

## Expected Outcome

- **No floating tool library windows** — the 6 standalone library windows are gone
- **Clean 3-column dock**: Master Panel (left) | Viewport (center) | Inspector (right)
- **Inline TOOL DECK** inside the Tools tab of Master Panel — subtool grid visible without separate windows
- **No C4189 warnings** for unused dock variables

---

## Verification Steps

```powershell
rc-bld-core
rc-perceive-ui -Mode quick -Screenshot
```

Check `AI/docs/ui/ui_introspection_headless_latest.json`:
- No `"Road Library"`, `"Water Library"`, `"Axiom Library"` panel entries
- `"Master Panel"` in left dock; `"Inspector"` in right dock
- `"TOOL DECK"` section widgets inside Tools drawer

Check `AI/docs/ui/ui_screenshot_latest.png`:
- Viewport dominates center (unobstructed)
- Single master panel on left with subtool grid

---

## Files Modified
- `visualizer/src/ui/rc_ui_root.cpp`
- `visualizer/src/ui/panels/rc_panel_tools.cpp`

## No CMake Changes Needed
- `magic_enum` already used in the same library (`IPanelDrawer.cpp`, `rc_panel_inspector.cpp`, `rc_viewport_overlays.cpp`, `rc_panel_road_editor.cpp`)
- `rc_ui_components.h` already in include path
