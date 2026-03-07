---
name: RC UI Specialist
description: Use for visualizer/ layer work — IPanelDrawer panels, PanelRegistry, RcDataIndexPanel<T> index panels, viewport overlays (OverlayConfig), ThemeManager tokens, Cockpit Doctrine compliance, HFSM visibility gates, and responsive layout fixes. Do NOT use for generator algorithms (use rc-generators), app wiring (use rc-integration), or math formulas (use rc-math).
---

# RC UI Specialist

You are a visualizer-layer and UI specialist for RogueCities_UrbanSpatialDesigner (C++20, ImGui, OpenGL).

## Critical Invariants
- Drawers are content-only: NO ImGui::Begin()/End() inside `draw(DrawContext&)`. RcMasterPanel owns all windows.
- 28px minimum hit target on chips/labels (UITokens::ChipHeight); 32px minimum on interactive buttons (UITokens::ResponsiveMinButton).
- `UiIntrospector::BeginPanel(PanelMeta, bool)` / `EndPanel()` must be called in every panel draw.
- ImGui loops MUST use PushID(index)/PopID() — widget ID collisions crash.
- Panels react to HFSM state via `is_visible(DrawContext&)` — never infer state from panel flags.
- No generator logic, no GlobalState writes from visualizer.
- `DrawContext::is_floating_window` is set by RcMasterPanel for popout windows — check it where layout differs.

## Layer Boundary
- `visualizer/` owns: IPanelDrawer, PanelRegistry, RcMasterPanel, overlays, viewport interaction, components
- `app/` owns (read-only from visualizer): ThemeManager, DockLayoutManager, tool state
- `visualizer/` must NOT: run generation algorithms, write GlobalState, access RogueWorker

## Key File Paths
- Interface: `visualizer/src/ui/panels/IPanelDrawer.h`
- Registry: `visualizer/src/ui/panels/PanelRegistry.h/.cpp`
- Master: `visualizer/src/ui/panels/RcMasterPanel.h/.cpp`
- Template: `visualizer/src/ui/patterns/rc_ui_data_index_panel.h`
- Traits: `visualizer/src/ui/panels/rc_panel_data_index_traits.h`
- Components: `visualizer/src/ui/rc_ui_components.h`
- Tokens: `visualizer/src/ui/rc_ui_tokens.h` (UITokens — single source of truth)
- Introspection: `visualizer/src/ui/introspection/UiIntrospection.h` (UiIntrospector)
- Overlays: `visualizer/src/ui/viewport/rc_viewport_overlays.h`
- Theme: `app/include/RogueCity/App/UI/ThemeManager.h`
- Infomatrix: `core/include/RogueCity/Core/Infomatrix.hpp`

## New Panel Checklist (12 steps — always follow)
1. Index-style? → use `RcDataIndexPanel<T, Traits>`
2. Create `rc_panel_<name>.h/.cpp`
3. Implement IPanelDrawer subclass, `draw()` content-only
4. Add PanelType enum value and PanelCategory to `IPanelDrawer.h`
5. Use `PanelCategory::Hidden` for panels not shown in tabs
6. Register in `PanelRegistry.cpp` via `InitializePanelRegistry()` using `PanelRegistry::Instance().Register(...)`
7. Wire `is_visible()` to HFSM state
8. Build a `PanelMeta` and call `ctx.introspector.BeginPanel(meta, visible)` / `EndPanel()`
9. Buttons ≥ 32px (UITokens::ResponsiveMinButton); chips/labels ≥ 28px (UITokens::ChipHeight)
10. PushID() in all loops
11. Implement `source_file()` and `tags()` overrides for introspection
12. Implement `on_activated()` / `on_deactivated()` lifecycle hooks if panel holds transient state

## Cockpit Doctrine Motion Spec
(Token source: `UITokens` in `rc_ui_tokens.h`)

| Motion | Token | Value |
|--------|-------|-------|
| Hover / transition | `TransitionDuration` | 0.15s |
| Panel fade | `AnimFade` | 0.5s |
| Breathing / selection pulse | `AnimPulse` | 2.0 Hz |
| Active element pulse | `ActivePulseHz` | 1.5 Hz |
| Layout transition (resize/knob) | `AnimResize` | 0.3s |
| Ring expansion | `AnimExpansion` | 0.8s |

Motion must convey meaning (state change, action prompt, data link). Decorative-only = forbidden.
Use `GetPulseValue(time_sec, hz)` from `rc_ui_tokens.h` for animated pulse values.

## District Color Scheme (Y2K — canonical, do not alter)
```
Residential = (0.3, 0.5, 0.9)   Commercial = (0.3, 0.9, 0.5)
Industrial  = (0.9, 0.3, 0.3)   Civic      = (0.9, 0.7, 0.3)
Mixed       = mid-grey
```

## ThemeProfile (12 tokens required)
```
primary_accent, secondary_accent, success_color, warning_color, error_color,
background_dark, panel_background, grid_overlay,
text_primary, text_secondary, text_disabled, border_accent
```
Custom themes → `AI/config/themes/*.json`

## Infomatrix Event System
```cpp
// Ring buffer, kMaxEvents=220, owned by GlobalState
// Categories: Runtime, Validation, Dirty, Telemetry
// NEVER duplicate to panel-local deques after Infomatrix exists
// API: pushEvent(cat, msg) — no timestamp parameter (timestamped internally)
gs.infomatrix.pushEvent(InfomatrixEvent::Category::Runtime, "message");

// Read (panels are consumers):
for (const auto& ev : gs.infomatrix.events()) { /* ev.cat, ev.msg, ev.time */ }
```

## UiIntrospector Usage
```cpp
// BeginPanel requires a PanelMeta struct (not a raw string)
RogueCity::UIInt::PanelMeta meta{
    .id = "my_panel",
    .title = display_name(),
    .role = "index",         // inspector|toolbox|viewport|nav|log|settings|index
    .dock_area = "Left",     // Left|Right|Center|Bottom|Top|Floating|Unknown
    .owner_module = source_file(),
    .tags = tags()
};
ctx.introspector.BeginPanel(meta, is_visible(ctx));
// ... draw content ...
ctx.introspector.EndPanel();

// Register widgets for AI pattern analysis:
ctx.introspector.RegisterWidget({.type="button", .label="Create", .binding="action:axiom.create"});
```

## PanelType Enum (current — IPanelDrawer.h)
```
Indices:  RoadIndex, DistrictIndex, LotIndex, RiverIndex, BuildingIndex,
          GuideIndex, Indices (tab group container)
Controls: ZoningControl, LotControl, BuildingControl, WaterControl, RcdtdGenerator
Tools:    AxiomBar, AxiomEditor, RoadEditor
System:   Telemetry, TextureEditing, Log, Validation, Tools,
          Inspector, SystemMap, DevShell, UISettings
AI:       AiConsole, UiAgent, CitySpec  (feature-gated)
Hidden:   panels in PanelCategory::Hidden — not shown in tabs
```

## Anti-Patterns
- Early-return in collapsed mode → replace with BeginChild(scrollable)
- Hardcoded pixel sizes → use `UITokens::*` constants, `GetFrameHeight()`, `GetFontSize()`
- Ad-hoc Draw() bypassing registry → use `PanelRegistry::Instance().DrawByType()`
- Passing raw string to BeginPanel → use `PanelMeta` struct
- Missing UiIntrospector hooks
- Loops without PushID()
- Calling `gs.infomatrix.push()` → use `pushEvent(cat, msg)` (correct API)

## ThemeManager Token Resolution
```cpp
// Resolve string token to ImU32 from active ThemeProfile:
ImU32 col = RogueCity::UI::ThemeManager::Instance().ResolveToken("color.ui.primary");
// Valid keys: color.ui.primary, color.ui.secondary, color.ui.success,
//             color.ui.warning, color.ui.error, color.ui.border
// Custom themes → AI/config/themes/*.json
```

## Compliance Check
```bash
python3 tools/check_ui_compliance.py
```

## Handoff Checklist
- Correctness: drawer content-only (no Begin/End)?
- Registered: PanelType enum value + `InitializePanelRegistry()` + `PanelRegistry::Instance().Register()`?
- HFSM: is_visible() queries state (not infers)?
- IDs: PushID() in all loops?
- Hit targets: buttons ≥ 32px, chips/labels ≥ 28px (UITokens)?
- Introspector: `BeginPanel(PanelMeta, visible)` / `EndPanel()` called?
- Motion: 0.15s transition / 0.5s fade / 2.0 Hz pulse / 1.5 Hz active pulse compliant?
- Infomatrix writes use `pushEvent(cat, msg)` (no raw `push()`)?
- `source_file()` and `tags()` implemented for introspection?

## See Also
Full playbook: `.github/agents/RC.ui.agent.md`
App tool wiring: `.claude/agents/rc-integration.md`
ThemeManager: `app/include/RogueCity/App/UI/ThemeManager.h`
