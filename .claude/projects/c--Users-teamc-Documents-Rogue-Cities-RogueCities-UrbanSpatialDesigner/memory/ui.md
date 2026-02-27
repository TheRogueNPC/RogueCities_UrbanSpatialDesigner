# RogueCities — UI Architecture & Visualizer Details

## Cockpit Doctrine (Design Language)
- Y2K retro aesthetic: cyan/neon accents, scanline VFX, pulsing borders
- Motion as instruction: animated borders signal active state, scale/glow on hover
- Reactive affordances: every interactive element has visible feedback
- Min 28px hit targets (P0 requirement)
- State-reactive panels: HFSM gates what is visible, no dead content shown

## RcMasterPanel (visualizer/src/ui/panels/RcMasterPanel.h/.cpp)
Single container window with hybrid tabs + search navigation.
- Tab categories: Indices | Controls | Tools | System | AI
- Ctrl+P opens search overlay
- Popout: each panel can float as a separate window
- DrawContext passed to active drawer for all data access
- Breathing animation border when focused
- m_active_panel, m_popout_states, m_search_filter tracked internally

## Panel Lifecycle
```
InitializePanelRegistry()           ← called once at startup
  → PanelRegistry::Register(drawer)  ← ownership transferred
RcMasterPanel::Draw(dt)
  → PanelRegistry::DrawByType(type, ctx)
    → IPanelDrawer::draw(ctx)         ← content only, no window
```

## Key Visualizer Files
```
visualizer/src/ui/panels/
  IPanelDrawer.h            ← interface + DrawContext + PanelType/Category enums
  PanelRegistry.h/.cpp      ← singleton registry + InitializePanelRegistry()
  RcMasterPanel.h/.cpp      ← master window, tab bar, search
  RcPanelDrawers.cpp        ← concrete drawer implementations
  rc_panel_data_index_traits.h  ← RoadIndexTraits, DistrictIndexTraits, etc.
  rc_panel_road_editor.h/.cpp
  rc_panel_axiom_editor.h/.cpp
  rc_panel_axiom_bar.h/.cpp
  rc_panel_inspector.h/.cpp
  rc_panel_validation.h/.cpp
  rc_panel_tools.h/.cpp
  rc_panel_zoning_control.h/.cpp
  rc_panel_building_control.h/.cpp
  rc_panel_lot_control.h/.cpp
  rc_panel_water_control.h/.cpp
  rc_panel_ui_settings.h/.cpp   ← theme editor, layout prefs (MODIFIED)
  rc_panel_telemetry.h/.cpp
  rc_panel_log.h/.cpp
  rc_panel_system_map.h/.cpp
  rc_panel_dev_shell.h/.cpp
  rc_panel_ai_console.h/.cpp    ← feature-gated
  rc_panel_ui_agent.h/.cpp      ← feature-gated
  rc_panel_city_spec.h/.cpp     ← feature-gated
  rc_property_editor.h/.cpp
  RogueWidgets.hpp

visualizer/src/ui/patterns/
  rc_ui_data_index_panel.h  ← RcDataIndexPanel<T, Traits> template

visualizer/src/ui/
  rc_ui_components.h        ← reusable components (MODIFIED)
  rc_ui_tokens.h            ← UITokens namespace (colors, sizes)
  rc_ui_responsive.h        ← ResponsiveLayout queries
  rc_ui_anim.h/.cpp         ← animation helpers
  rc_ui_theme.h/.cpp        ← ImGui style application
  rc_ui_root.h/.cpp         ← root draw loop
  rc_ui_input_gate.h/.cpp   ← input filtering

visualizer/src/ui/viewport/
  rc_viewport_interaction.h/.cpp
  rc_viewport_overlays.h/.cpp
  rc_viewport_lod_policy.h/.cpp
  rc_viewport_scene_controller.h/.cpp
  rc_scene_frame.h/.cpp
  rc_minimap_renderer.h/.cpp
  rc_placement_system.h/.cpp
  handlers/
    rc_viewport_domain_handlers.h/.cpp
    rc_viewport_handler_common.h/.cpp
    rc_viewport_road_handler.cpp
    rc_viewport_district_handler.cpp
    rc_viewport_lot_handler.cpp
    rc_viewport_building_handler.cpp
    rc_viewport_water_handler.cpp

visualizer/src/ui/tools/
  rc_tool_contract.h/.cpp
  rc_tool_dispatcher.h/.cpp
  rc_tool_geometry_policy.h/.cpp
  rc_tool_interaction_metrics.h/.cpp

visualizer/src/ui/commands/
  rc_command_palette.h/.cpp
  rc_context_command_registry.h/.cpp
  rc_context_menu_pie.h/.cpp
  rc_context_menu_smart.h/.cpp

visualizer/src/ui/introspection/
  UiIntrospection.h/.cpp

visualizer/src/ui/
  LayoutPresets.h/.cpp
```

## UiIntrospector
Must be used in all panel work — provides metadata for AI pattern analysis.
Call `uiint.BeginPanel()` and `uiint.EndPanel()` around panel draw content.
File: `visualizer/src/ui/introspection/UiIntrospection.h`

## UITokens Namespace (rc_ui_tokens.h)
Canonical color constants:
- `UITokens::YellowWarning` — panel borders, caution states
- `UITokens::PanelBackground` — default panel bg
- `UITokens::GreenHUD` — scanline tint, active indicators
Use `ThemeManager::ResolveToken()` for runtime theming.

## UI Settings Panel (rc_panel_ui_settings.cpp)
State: s_open, s_selected_theme_index, s_custom_theme_name[64]
Auto-save: 2 second debounce (kSaveDebounceSeconds = 2.0f) on theme edit
Key ops: theme selection combo, color picker for 12 tokens,
         CreateCustomFromActive(), SaveThemeToFile()

## Selection System
- ApplySelectionModifier() helper in trait OnEntitySelected callbacks
- Shift = Add to selection, Ctrl = Toggle, Default = Replace
- Updates GlobalState selection + SelectionManager
- Triggers viewport overlay refresh
- ID stability: FVA stable handles survive regeneration

## Viewport System
- PrimaryViewport: main editing canvas
- MinimapViewport: overview + navigation
- ViewportSyncManager: keeps viewports in sync
- Domain handlers per entity type (road, district, lot, building, water)
- LOD policy: `rc_viewport_lod_policy.h`
- Overlays: `rc_viewport_overlays.h` — validation markers, selection, hover

## Anti-Patterns (Do NOT do these in visualizer)
- Never call Draw() directly — use IPanelDrawer + registry
- Never early-return on layout for collapsed state — use scrollable/adaptive containers
- Never hardcode pixel sizes — use ResponsiveLayout queries
- Never skip UiIntrospector usage in panels
- Never put generation logic in visualizer files
