# Master Panel Architecture - Panel Conversion Guide

## Status: Framework Complete, Panel Conversion In Progress

**Date**: February 14, 2026  
**Version**: RC-0.10 Master Panel Architecture Refactor

## What's Completed ✅

### Core Infrastructure
1. **IPanelDrawer Interface** - Base interface for all panel drawers (`visualizer/src/ui/panels/IPanelDrawer.h`)
2. **PanelRegistry System** - Singleton registry for drawer lookup and routing (`visualizer/src/ui/panels/PanelRegistry.cpp`)
3. **RcMasterPanel** - Main container with hybrid tabs+search navigation (`visualizer/src/ui/panels/RcMasterPanel.cpp`)
4. **Drawer Wrappers** - All 19 panel drawer classes defined (`visualizer/src/ui/panels/RcPanelDrawers.cpp`)
5. **Root Integration** - `rc_ui_root.cpp` updated to use Master Panel system
6. **CMake Configuration** - `ROGUE_AI_DLC_ENABLED` option added, new files integrated
7. **UI Pattern Catalog** - Master Panel pattern documented in `AI/docs/ui/ui_patterns.json`

### Features Implemented
- **Hybrid Navigation**: Tab bar for categories (Indices, Controls, Tools, System, AI) + Ctrl+P search overlay
- **Popout Support**: Each drawer can float independently (button in panel header)
- **State-Reactive Visibility**: Panels show/hide based on HFSM editor state
- **Feature Gating**: AI panels wrapped in `#if ROGUE_AI_DLC_ENABLED` for modular builds
- **Lifecycle Hooks**: `on_activated()` / `on_deactivated()` for panel transitions
- **Introspection**: Full integration with UiIntrospector for AI code-shape awareness

## What's Remaining 🚧

Each existing panel needs a `DrawContent()` method added that strips out window creation. The drawer wrappers in `RcPanelDrawers.cpp` call these methods.

### Panel Conversion Pattern

**Current Structure** (window-owning):
```cpp
void PanelName::Draw(float dt) {
    ImGui::Begin("Panel Name", &open);
    // ... content ...
    ImGui::End();
}
```

**New Structure** (drawer-compatible):
```cpp
// Add new method - CONTENT ONLY, no window creation
void PanelName::DrawContent(float dt) {
    // ... content (same as before, minus Begin/End) ...
}

// Keep old Draw() for backward compatibility during migration
void PanelName::Draw(float dt) {
    ImGui::Begin("Panel Name", &open);
    DrawContent(dt);
    ImGui::End();
}
```

---

## Panel-by-Panel Conversion Checklist

### 1. Index Panels (Template-Based) ✅ PARTIALLY DONE

**Template Updated**: `visualizer/src/ui/patterns/rc_ui_data_index_panel.h` now has `DrawContent()` method that strips window creation.

**Panels Using Template**:
- ✅ RoadIndex - Uses `GetPanel().DrawContent()`
- ✅ DistrictIndex - Uses `GetPanel().DrawContent()`
- ✅ LotIndex - Uses `GetPanel().DrawContent()`
- ✅ RiverIndex - Uses `GetPanel().DrawContent()`
- ✅ BuildingIndex - Uses `GetPanel().DrawContent()`

**Status**: Drawer wrappers call `GetPanel().DrawContent()` directly. ✅ **NO FURTHER ACTION NEEDED**.

---

### 2. Control Panels (HFSM State-Reactive)

#### 2.1 ZoningControl ⚠️ NEEDS WORK
**File**: `visualizer/src/ui/panels/rc_panel_zoning_control.cpp`

**Current**: 
- Line 67: `Components::BeginTokenPanel("Zoning Control", ...)`
- Line 93: `Components::EndTokenPanel()`

**Required Changes**:
```cpp
// Add new function
void DrawContent(float dt) {
    // Copy lines 72-92 (all the content between Begin/EndTokenPanel)
    // Remove introspection Begin/EndPanel calls (drawer handles this)
}

// Drawer calls: ZoningControl::DrawContent(ctx.dt)
```

**Drawer Notes**: Already implements `is_visible()` check for HFSM states (Editing_Districts, Editing_Lots, Editing_Buildings).

#### 2.2 LotControl ⚠️ NEEDS WORK
**File**: `visualizer/src/ui/panels/rc_panel_lot_control.cpp`

**Pattern**: Same as ZoningControl - add `DrawContent()` method, strip window creation.

#### 2.3 BuildingControl ⚠️ NEEDS WORK
**File**: `visualizer/src/ui/panels/rc_panel_building_control.cpp`

**Pattern**: Same as ZoningControl - add `DrawContent()` method, strip window creation.

#### 2.4 WaterControl ⚠️ NEEDS WORK
**File**: `visualizer/src/ui/panels/rc_panel_water_control.cpp`

**Pattern**: Same as ZoningControl - add `DrawContent()` method, strip window creation.

---

### 3. Tools Panels

#### 3.1 AxiomBar ⚠️ NEEDS WORK
**File**: `visualizer/src/ui/panels/rc_panel_axiom_bar.cpp`

**Current**: `AxiomBar::Draw(float dt)` likely has window creation.

**Required**: Add `AxiomBar::DrawContent(float dt)` method.

**Drawer Notes**: `can_popout() = false` (should stay docked as toolbar).

#### 3.2 AxiomEditor ⚠️ NEEDS WORK
**File**: `visualizer/src/ui/panels/rc_panel_axiom_editor.cpp`

**Pattern**: Add `AxiomEditor::DrawContent(float dt)` method.

---

### 4. System Panels

#### 4.1 Telemetry ⚠️ NEEDS WORK
**File**: `visualizer/src/ui/panels/rc_panel_telemetry.cpp`

**Pattern**: Add `Telemetry::DrawContent(float dt)` method.

#### 4.2 Log ⚠️ NEEDS WORK
**File**: `visualizer/src/ui/panels/rc_panel_log.cpp`

**Pattern**: Add `Log::DrawContent(float dt)` method.

#### 4.3 Tools ⚠️ NEEDS WORK
**File**: `visualizer/src/ui/panels/rc_panel_tools.cpp`

**Pattern**: Add `Tools::DrawContent(float dt)` method.

#### 4.4 Inspector ⚠️ NEEDS WORK
**File**: `visualizer/src/ui/panels/rc_panel_inspector.cpp`

**Pattern**: Add `Inspector::DrawContent(float dt)` method.

#### 4.5 SystemMap ⚠️ NEEDS WORK
**File**: `visualizer/src/ui/panels/rc_panel_system_map.cpp`

**Pattern**: Add `SystemMap::DrawContent(float dt)` method.

#### 4.6 DevShell ⚠️ NEEDS WORK
**File**: `visualizer/src/ui/panels/rc_panel_dev_shell.cpp`

**Pattern**: Add `DevShell::DrawContent(float dt)` method.

---

### 5. AI Panels (Feature-Gated) 🔒 SPECIAL HANDLING

These panels currently use class instances with `Render()` methods. Conversion is different:

#### 5.1 AiConsole ⚠️ NEEDS WORK
**File**: `visualizer/src/ui/panels/rc_panel_ai_console.cpp`
**Class**: `RogueCity::UI::AiConsolePanel`

**Current**: 
- Line 16: `const bool open = RC_UI::Components::BeginTokenPanel(...)`
- Line 111: `RC_UI::Components::EndTokenPanel()`

**Required Changes**:
```cpp
// Add new method to AiConsolePanel class
void RenderContent() {
    // Copy lines 39-107 (all content between Begin/EndTokenPanel)
}

// Drawer calls: instance.RenderContent()
```

**Header Update**: Add `void RenderContent();` to `rc_panel_ai_console.h` public methods.

#### 5.2 UiAgent ⚠️ NEEDS WORK
**File**: `visualizer/src/ui/panels/rc_panel_ui_agent.cpp`
**Class**: `RogueCity::UI::UiAgentPanel`

**Pattern**: Same as AiConsole - add `RenderContent()` method.

#### 5.3 CitySpec ⚠️ NEEDS WORK
**File**: `visualizer/src/ui/panels/rc_panel_city_spec.cpp`
**Class**: `RogueCity::UI::CitySpecPanel`

**Pattern**: Same as AiConsole - add `RenderContent()` method.

**Feature Gate**: All three wrapped in `#if defined(ROGUE_AI_DLC_ENABLED)` blocks in `RcPanelDrawers.cpp`.

---

## Build & Test Workflow

### 1. Build Without AI DLC (Core Panels Only)
```bash
cmake -B build -S . -DROGUE_AI_DLC_ENABLED=OFF
cmake --build build --config Release
```

**Expected**: 16 panels (no AI tab in Master Panel)

### 2. Build With AI DLC (All Panels)
```bash
cmake -B build -S . -DROGUE_AI_DLC_ENABLED=ON
cmake --build build --config Release
```

**Expected**: 19 panels (AI tab appears with 3 drawers)

### 3. Runtime Verification
- **Tab Navigation**: Click Indices/Controls/Tools/System/(AI) tabs
- **Search**: Press Ctrl+P, type "Road", verify Road Index activates
- **Popout**: Click "Popout" button on any panel, verify floating window works
- **State-Reactive**: Change HFSM to `Editing_Districts`, verify ZoningControl appears in Controls tab
- **Introspection**: Check UiDesignAssistant can query panel metadata

### 4. Error Checking
```bash
# Check for compile errors
cmake --build build --config Release 2>&1 | grep error

# Check for missing DrawContent calls (runtime)
# Master Panel will show "Panel not available" if drawer CreateDrawer() fails
```

---

## Migration Strategy

### Phase 1: Control Panels (Highest Priority)
Convert ZoningControl, LotControl, BuildingControl, WaterControl first. These have state-reactive visibility and are most visible to users.

### Phase 2: System Panels
Convert Telemetry, Log, Tools, Inspector, SystemMap, DevShell. Less critical but frequently used for debugging.

### Phase 3: Tools Panels
Convert AxiomBar, AxiomEditor. These have complex interactions with HFSM.

### Phase 4: AI Panels
Convert AiConsole, UiAgent, CitySpec last. Feature-gated, so can be done after core panels work.

### Phase 5: Cleanup
Remove deprecated static AI panel instances from `rc_ui_root.cpp` (lines 48-50, commented out).

---

## Code Hygiene Notes

### Introspection
- Drawers register panels via `DrawContext::introspector`
- Remove `Begin/EndPanel()` calls from panel content - drawer handles this
- Keep `RegisterWidget()` calls for individual UI elements

### Static State
- Panels with static state (e.g., `ZoningControl::GetPanelState()`) are safe to keep
- All panels run on main thread (no threading concerns)

### HFSM Visibility
- State-reactive panels override `is_visible(DrawContext& ctx)` in drawer
- Check `ctx.hfsm.state()` for editor mode
- Master Panel skips drawing if `is_visible() == false`

### Y2K Geometry Animations
- Keep glow/pulse animations in panel content
- Drawers preserve existing visual design

---

## Next Steps

1. **Start with Control Panels**: Add `DrawContent()` methods to ZoningControl, LotControl, BuildingControl, WaterControl
2. **Test Build**: Verify compile with `ROGUE_AI_DLC_ENABLED=ON`
3. **Runtime Test**: Launch visualizer, verify Master Panel appears, tabs switch, search works
4. **Iterate**: Convert System panels, then Tools, then AI
5. **Final Cleanup**: Remove deprecated code, update documentation

---

## Support Files Created

- `visualizer/src/ui/panels/IPanelDrawer.h` - Base interface
- `visualizer/src/ui/panels/IPanelDrawer.cpp` - Helper functions
- `visualizer/src/ui/panels/PanelRegistry.h` - Registry interface
- `visualizer/src/ui/panels/PanelRegistry.cpp` - Registry implementation + initialization
- `visualizer/src/ui/panels/RcMasterPanel.h` - Master panel interface
- `visualizer/src/ui/panels/RcMasterPanel.cpp` - Master panel implementation
- `visualizer/src/ui/panels/RcPanelDrawers.cpp` - All 19 drawer wrapper classes

**Total**: ~1500 lines of new framework code. Each panel conversion: ~20-50 lines per panel.

---

## Questions / Clarifications

If any panel has unique requirements not covered by this guide, refer to:
- **Template Pattern**: `rc_ui_data_index_panel.h` (RoadIndex conversion)
- **Control Pattern**: `rc_panel_zoning_control.cpp` (line 67-93 for extraction example)
- **AI Pattern**: `rc_panel_ai_console.cpp` (line 16-111 for RenderContent extraction)

**Contact**: Architect Agent / UI/UX Master for pattern questions.
