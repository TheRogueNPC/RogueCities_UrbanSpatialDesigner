# UI Code Reduction Gameplan
**Date:** 2026-03-01
**Agent:** Claude (Sonnet 4.6)
**Layer:** visualizer (Layer 2)
**Audit source:** `AI/docs/code-reduction-audit.md`
**Target:** ≥40% reduction in each targeted zone; ~1,048 LOC overall

---

## Overview

Four implementation changes, ordered by impact × effort ratio. All are purely additive (new helpers/macros) or consolidating (no feature changes). The test suite (all 17 `test_ai` tests) should remain green throughout.

---

## Change 1 — DEFINE_SIMPLE_DRAWER macro in RcPanelDrawers.cpp
**File:** `visualizer/src/ui/panels/RcPanelDrawers.cpp`
**Impact:** 582 → ~120 LOC (79% reduction, **Zone 1b**)
**Effort:** Low — mechanical substitution

### What to add

Insert at top of file (after includes):

```cpp
// ---- DEFINE_SIMPLE_DRAWER --------------------------------------------------
// Collapses the 18-25 line Drawer class + CreateDrawer boilerplate into 2 lines.
// DRAW_EXPR: expression to call inside draw(DrawContext& ctx); may reference ctx.
#define DEFINE_SIMPLE_DRAWER(NS, TYPE, DISPLAY, CAT, FILE, DRAW_EXPR, ...)  \
namespace NS {                                                                \
class Drawer : public IPanelDrawer {                                          \
public:                                                                       \
  PanelType type() const override { return PanelType::TYPE; }                \
  const char* display_name() const override { return DISPLAY; }              \
  PanelCategory category() const override { return PanelCategory::CAT; }     \
  const char* source_file() const override { return FILE; }                  \
  std::vector<std::string> tags() const override { return {__VA_ARGS__}; }   \
  void draw(DrawContext& ctx) override { DRAW_EXPR; }                         \
};                                                                            \
IPanelDrawer* CreateDrawer() { return new Drawer(); }                        \
} /* namespace NS */
// ---------------------------------------------------------------------------
```

### How to apply

Replace each namespace block that follows the pattern. Example — before:

```cpp
namespace RoadIndex {
class Drawer : public IPanelDrawer {
public:
  PanelType type() const override { return PanelType::RoadIndex; }
  const char *display_name() const override { return "Road Index"; }
  PanelCategory category() const override { return PanelCategory::Indices; }
  const char *source_file() const override {
    return "visualizer/src/ui/panels/rc_panel_road_index.cpp";
  }
  std::vector<std::string> tags() const override {
    return {"index", "road", "data"};
  }
  void draw(DrawContext &ctx) override {
    auto &panel = RC_UI::Panels::RoadIndex::GetPanel();
    panel.DrawContent(ctx.global_state, ctx.introspector);
  }
};
IPanelDrawer *CreateDrawer() { return new Drawer(); }
} // namespace RoadIndex
```

After:

```cpp
DEFINE_SIMPLE_DRAWER(RoadIndex, RoadIndex, "Road Index", Indices,
    "visualizer/src/ui/panels/rc_panel_road_index.cpp",
    RC_UI::Panels::RoadIndex::GetPanel().DrawContent(ctx.global_state, ctx.introspector),
    "index", "road", "data")
```

### All 23 drawer invocations to generate

| Namespace | TYPE | DISPLAY | CAT | draw expr snippet |
|---|---|---|---|---|
| RoadIndex | RoadIndex | "Road Index" | Indices | GetPanel().DrawContent(ctx) |
| DistrictIndex | DistrictIndex | "District Index" | Indices | GetPanel().DrawContent(ctx) |
| LotIndex | LotIndex | "Lot Index" | Indices | GetPanel().DrawContent(ctx) |
| RiverIndex | RiverIndex | "River Index" | Indices | RiverIndex::Draw(0.0f) |
| BuildingIndex | BuildingIndex | "Building Index" | Indices | GetPanel().DrawContent(ctx) |
| ZoningControl | ZoningControl | "Zoning Control" | Controls | ZoningControl::DrawContent(0.0f) |
| LotControl | LotControl | "Lot Subdivision Control" | Controls | LotControl::DrawContent(0.0f) |
| BuildingControl | BuildingControl | "Building Placement Control" | Controls | BuildingControl::DrawContent(0.0f) |
| WaterControl | WaterControl | "Water Control" | Controls | WaterControl::DrawContent(0.0f) |
| AxiomBar | AxiomBar | "Tool Deck" | ToolDecks | AxiomBar::DrawContent(0.0f) |
| AxiomEditor | AxiomEditor | "Axiom Editor" | Viewports | AxiomEditor::Draw(0.0f) |
| RoadEditor | RoadEditor | "Road Network Editor" | Editors | RoadEditor::DrawContent(0.0f) |
| Telemetry | Telemetry | "Telemetry" | Diagnostics | Telemetry::DrawContent(0.0f) |
| Log | Log | "Log" | Diagnostics | Log::DrawContent(0.0f) |
| Tools | Tools | "Tools" | Toolboxes | Tools::DrawContent(0.0f) |
| Inspector | Inspector | "Inspector" | Inspectors | Inspector::DrawContent(0.0f) |
| SystemMap | SystemMap | "System Map" | Nav | SystemMap::DrawContent(0.0f) |
| DevShell | DevShell | "Dev Shell" | Diagnostics | DevShell::DrawContent(0.0f) |
| Validation | Validation | "Validation" | System | Validation::DrawContent(0.0f) |
| WorkspaceSettings | WorkspaceSettings | "Workspace Settings" | System | WorkspaceSettings::DrawContent(0.0f) |
| AiConsole | AiConsole | "AI Console" | AI | AiConsole::DrawContent(0.0f) |
| UiAgent | UiAgent | "UI Agent" | AI | UiAgent::DrawContent(0.0f) |
| CitySpec | CitySpec | "City Spec" | AI | CitySpec::DrawContent(0.0f) |

**Note:** `ctx.global_state` and `ctx.introspector` are the DrawContext fields. Verify the exact
field names match the DrawContext struct before applying.

### Verification
- `wc -l RcPanelDrawers.cpp` should drop from 582 to ~120
- Compile both executables; CreateDrawer() symbols must still link

---

## Change 2 — RC_PANEL_DRAW_IMPL macro for canonical panels
**Files:** new `visualizer/src/ui/rc_ui_panel_macros.h` + 8–10 panel `.cpp` files
**Impact:** ~240 → ~30 LOC (87% reduction, **Zone 1a**)
**Effort:** Low per panel; read each Draw() to confirm it matches Variant A

### Macro definition (`rc_ui_panel_macros.h`)

```cpp
#pragma once
// RC_PANEL_DRAW_IMPL — collapses the canonical 24-line Draw(float dt) wrapper.
//
// ONLY use for Variant A panels: BeginTokenPanel + uiint.BeginPanel-always
// (i.e., BeginPanel is called regardless of 'open', with open passed in).
//
// Parameters:
//   DISPLAY  — title string shown in the ImGui window and panel registry
//   LABEL    — component label (often same as DISPLAY, used in PanelMeta field 2)
//   PANEL_ID — kebab-case id string ("inspector", "zoning_control", etc.)
//   DOCK     — default dock area string ("Left", "Right", "Bottom", "Center")
//   COLOR    — UITokens:: border color
//   SRC_FILE — source file path literal for introspection
//   ...      — tag strings (variadic)
//
// Place inside the panel's namespace, after DrawContent is declared.
#define RC_PANEL_DRAW_IMPL(DISPLAY, LABEL, PANEL_ID, DOCK, COLOR, SRC_FILE, ...) \
void Draw(float dt) {                                                              \
    const bool _open = Components::BeginTokenPanel(DISPLAY, COLOR);               \
    auto& _uiint = RogueCity::UIInt::UiIntrospector::Instance();                  \
    _uiint.BeginPanel(                                                             \
        RogueCity::UIInt::PanelMeta{DISPLAY, LABEL, PANEL_ID, DOCK,               \
                                     SRC_FILE, {__VA_ARGS__}},                    \
        _open);                                                                    \
    if (!_open) {                                                                  \
        _uiint.EndPanel();                                                         \
        Components::EndTokenPanel();                                               \
        return;                                                                    \
    }                                                                              \
    DrawContent(dt);                                                               \
    _uiint.EndPanel();                                                             \
    Components::EndTokenPanel();                                                   \
}
```

### Panels confirmed Variant A (apply macro immediately)

| Panel file | DISPLAY | LABEL | PANEL_ID | DOCK | COLOR | Tags |
|---|---|---|---|---|---|---|
| rc_panel_validation.cpp | "Validation" | "Validation" | "validation" | "Bottom" | AmberGlow | "validation","runtime" |
| rc_panel_system_map.cpp | "System Map" | "System Map" | "system_map" | "Right" | MagentaHighlight | "nav","map" |
| rc_panel_river_index.cpp* | "Water Index" | "Water Index" | "index" | "Bottom" | InfoBlue | "water","rivers","index" |

*river_index calls `GetPanel().DrawContent(...)` not `DrawContent(dt)`, so the macro body needs slight adjustment — or skip it and handle manually.

### Panels to verify before applying

Read the Draw() body of these files and confirm they match Variant A:
- rc_panel_telemetry.cpp
- rc_panel_ai_console.cpp
- rc_panel_workspace.cpp
- rc_panel_city_spec.cpp
- rc_panel_ui_settings.cpp
- rc_panel_ui_agent.cpp
- rc_panel_zoning_control.cpp (may have state gate — skip if so)
- rc_panel_water_control.cpp

### Panels that must NOT use this macro

| Panel | Reason |
|---|---|
| rc_panel_axiom_editor.cpp | Uses BeginDockableWindow, not BeginTokenPanel |
| rc_panel_axiom_bar.cpp | Uses BeginDockableWindow |
| rc_panel_dev_shell.cpp | Uses BeginDockableWindow + s_open gate |
| rc_panel_log.cpp | Viewport-positioned overlay window |
| rc_panel_inspector.cpp | Early-return before BeginPanel (Variant B) |
| rc_panel_road_editor.cpp | BeginTokenPanel but no uiint pattern |
| rc_panel_lot_control.cpp | HFSM state gate (Variant C) |
| rc_panel_building_control.cpp | HFSM state gate (Variant C) |

### Verification
- Rebuild; all 17 test_ai tests pass
- UI panels still open/close correctly in both headed and headless modes

---

## Change 3 — Extract DrawEntityFlags helpers in rc_property_editor.cpp
**File:** `visualizer/src/ui/panels/rc_property_editor.cpp`
**Impact:** ~508 → ~352 LOC in DrawSingle* functions (31% reduction, **Zone 3**)
**Effort:** Medium — template helpers require careful type-parametrization

### Helpers to add (before `DrawSingleRoad` at line 442)

```cpp
// ---- Shared entity property helpers ----------------------------------------

// DrawUserPlacedFlag: the 20-line "User Placed + generation_tag sync" block
// that appears identically in DrawSingleRoad, DrawSingleDistrict,
// DrawSingleLot, DrawSingleBuilding.
template<typename T>
static void DrawUserPlacedFlag(PropertyHistory& h, GlobalState& gs,
                                T& entity, VpEntityKind kind) {
    const bool before = entity.is_user_placed;
    if (ImGui::Checkbox("User Placed", &entity.is_user_placed)) {
        const bool after = entity.is_user_placed;
        if (after) {
            entity.generation_tag = RogueCity::Core::GenerationTag::M_user;
            entity.generation_locked = true;
        } else if (entity.generation_tag == RogueCity::Core::GenerationTag::M_user) {
            entity.generation_tag = RogueCity::Core::GenerationTag::Generated;
            entity.generation_locked = false;
        }
        CommitValueChange(h, "User Flag", before, after,
            [&entity](bool v) { entity.is_user_placed = v; });
        MarkDirtyForKind(gs, kind);
    }
}

// DrawGenerationLocked: the 15-line "Generation Locked" checkbox block.
template<typename T>
static void DrawGenerationLocked(PropertyHistory& h, GlobalState& gs,
                                  T& entity, VpEntityKind kind) {
    ImGui::Text("Generation Tag: %s", GenerationTagName(entity.generation_tag));
    const bool before = entity.generation_locked;
    if (ImGui::Checkbox("Generation Locked", &entity.generation_locked)) {
        const bool after = entity.generation_locked;
        CommitValueChange(h, "Generation Lock", before, after,
            [&entity](bool v) { entity.generation_locked = v; });
        MarkDirtyForKind(gs, kind);
    }
}

// DrawLayerAssignment: the 4-line layer input block.
static void DrawLayerAssignment(GlobalState& gs, VpEntityKind kind, uint32_t id) {
    const uint8_t cur = gs.GetEntityLayer(kind, id);
    int idx = static_cast<int>(cur);
    if (ImGui::InputInt("Layer", &idx)) {
        idx = std::clamp(idx, 0, 255);
        gs.SetEntityLayer(kind, id, static_cast<uint8_t>(idx));
        gs.dirty_layers.MarkDirty(DirtyLayer::ViewportIndex);
    }
}

// ---------------------------------------------------------------------------
```

### How to apply in each DrawSingleX function

Replace the 20-line User Placed block with:
```cpp
DrawUserPlacedFlag(history, gs, road, VpEntityKind::Road);
```

Replace the 15-line Generation Locked + tag display with:
```cpp
DrawGenerationLocked(history, gs, road, VpEntityKind::Road);
```

Replace the 4-line layer block with:
```cpp
DrawLayerAssignment(gs, VpEntityKind::Road, road.id);
```

### Verification
- Compile rc_property_editor.cpp — templates instantiate for Road, District, LotToken, BuildingSite
- Verify `wc -l` on DrawSingleRoad (92→~55), DrawSingleDistrict (160→~100), DrawSingleLot (101→~65), DrawSingleBuilding (~155→~95)
- Run test_ai: all 17 tests pass (property editor not tested directly but compile success is sufficient)

---

## Change 4 — Extract GenericEntitySelected/Hovered in rc_panel_data_index_traits.h
**File:** `visualizer/src/ui/panels/rc_panel_data_index_traits.h`
**Impact:** ~200 → ~20 LOC in OnEntitySelected/Hovered blocks (90% reduction, **Zone 2**)
**Effort:** Low — template function, entity kind is the only variation

### Helpers to add (at top of traits namespace)

```cpp
// ---- Generic selection/hover helpers ---------------------------------------
// These replace the 95%-identical OnEntitySelected and OnEntityHovered
// implementations in Road/District/Lot/Building/RiverIndex traits.

// GenericEntitySelected: clear all other selection handles, then select this entity.
template<typename EntityT>
static void GenericEntitySelected(GlobalState& gs, EntityT& entity,
                                   VpEntityKind kind, int /*idx*/) {
    gs.selection.ClearAll();
    gs.selection.SetHandle(kind, entity.id);
    RogueCity::Core::Editor::ApplySelectionModifier(gs, kind, entity.id);
}

// GenericEntityHovered: forward to the viewport overlay's SetHovered dispatch.
template<typename EntityT>
static void GenericEntityHovered(GlobalState& gs, EntityT& entity,
                                  VpEntityKind kind) {
    RC_UI::Viewport::GetViewportOverlays().SetHoveredByKind(kind, entity.id);
}
// Note: if SetHoveredByKind does not exist, each trait may still need its own
// OnEntityHovered — verify the overlay API supports a kind-dispatch overload.
// ---------------------------------------------------------------------------
```

### How to apply in each trait

Replace the 30-line OnEntitySelected body with:
```cpp
static void OnEntitySelected(RogueCity::Core::Road& e, int idx, GlobalState& gs) {
    GenericEntitySelected(gs, e, VpEntityKind::Road, idx);
}
```

Replace the 10-line OnEntityHovered body with:
```cpp
static void OnEntityHovered(RogueCity::Core::Road& e, int /*idx*/, GlobalState& gs) {
    GenericEntityHovered(gs, e, VpEntityKind::Road);
}
```

**Prerequisite check:** Verify `RC_UI::Viewport::GetViewportOverlays().SetHoveredByKind(kind, id)` exists or if each entity type uses a distinct method (SetHoveredLot, SetHoveredBuilding, etc.). If distinct methods are used, GenericEntityHovered cannot be applied and the OnEntityHovered bodies must stay per-trait.

### Verification
- Compile traits header — 5 template instantiations (Road, District, Lot, Building, River)
- Run smoke test to confirm hover/select still works in headed mode

---

## Execution Order

| # | Change | Files | Expected LOC delta | Risk |
|---|---|---|---|---|
| 1 | DEFINE_SIMPLE_DRAWER | RcPanelDrawers.cpp | −462 | Low (purely structural) |
| 2 | DrawEntityFlags helpers | rc_property_editor.cpp | −156 | Low (templates, same logic) |
| 3 | GenericEntitySelected/Hovered | rc_panel_data_index_traits.h | −180 | Medium (verify overlay API) |
| 4 | RC_PANEL_DRAW_IMPL + panels | rc_ui_panel_macros.h + 8–10 panels | −210 | Low (Variant A only) |
| **TOTAL** | | **~35 files** | **~−1,008 LOC** | Low–Medium |

---

## Validation Checklist

After all changes:
- [ ] `rc-bld-core` — all 17 `test_ai` tests pass
- [ ] Both executables compile cleanly (`visualizer_main`, `visualizer_headless`)
- [ ] `wc -l RcPanelDrawers.cpp` shows ≤130 lines
- [ ] `wc -l rc_property_editor.cpp` shows ≤1,136 lines
- [ ] `wc -l rc_panel_data_index_traits.h` shows ≤355 lines
- [ ] `rc-full-smoke -Port 7222 -Runs 1` — perception/audit checks pass
- [ ] Panel IDs in headless introspection JSON still match test_ai.cpp expected schema

---

## Out of Scope (Do Not Touch)

- `rc_panel_axiom_editor.cpp` (3,626 LOC) — BeginDockableWindow pattern, unique architecture
- `rc_ui_root.cpp` (2,287 LOC) — layout root, too risky to refactor without full UI regression
- `rc_panel_tools.cpp` (926 LOC) — complex tool catalog, unique logic
- State-gated panels (lot/building/zoning_control) Draw() functions — HFSM gate is intentional behavior
