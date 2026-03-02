# UI Code Reduction Audit
**Date:** 2026-03-01
**Audit Frame:** `tests/test_ai.cpp` — 17 tests encoding the 12-panel schema, all dock positions, all UiCommand types, and cross-layer state_model keys.
**Scope:** `visualizer/src/ui/panels/` + `visualizer/src/ui/rc_ui_root.cpp`
**Total UI LOC inventoried:** 15,001

---

## LOC Inventory

| File | LOC | Notes |
|---|---|---|
| rc_panel_axiom_editor.cpp | 3,626 | Engine — BeginDockableWindow pattern |
| rc_ui_root.cpp | 2,287 | Root layout |
| rc_property_editor.cpp | 1,292 | **Zone 3 target** |
| rc_panel_tools.cpp | 926 | Engine |
| RcPanelDrawers.cpp | 582 | **Zone 1 target** |
| rc_panel_data_index_traits.h | 535 | **Zone 2 target** |
| RcMasterPanel.cpp | 546 | Infrastructure |
| rc_panel_ui_agent.cpp | 516 | Feature (AI-gated) |
| rc_panel_ui_settings.cpp | 470 | Feature |
| rc_panel_dev_shell.cpp | 414 | BeginDockableWindow |
| rc_panel_axiom_bar.cpp | 304 | BeginDockableWindow |
| rc_panel_workspace.cpp | 287 | Feature |
| rc_panel_inspector.cpp | 278 | Early-return-before-BeginPanel variant |
| rc_panel_system_map.cpp | 274 | Clean BeginTokenPanel ✓ |
| rc_panel_ai_console.cpp | 250 | Feature (AI-gated) |
| rc_panel_city_spec.cpp | 246 | Feature (AI-gated) |
| rc_panel_road_editor.cpp | 232 | BeginTokenPanel no-uiint variant |
| rc_panel_log.cpp | 230 | Viewport-positioned overlay |
| rc_panel_zoning_control.cpp | 226 | HFSM state-gated |
| rc_system_map_query.cpp | 211 | Helper |
| PanelRegistry.cpp | 199 | Infrastructure |
| rc_panel_building_control.cpp | 151 | HFSM state-gated |
| rc_panel_lot_control.cpp | 142 | HFSM state-gated |
| rc_panel_telemetry.cpp | 133 | Feature |
| rc_panel_water_control.cpp | 133 | Feature |
| rc_panel_validation.cpp | 96 | Clean BeginTokenPanel ✓ |
| rc_panel_inspector_sidebar.cpp | 59 | Class-based container |
| rc_panel_river_index.cpp | 45 | Template delegate |
| Index .cpp stubs × 4 | 39 | Already reduced 85% via template |

---

## Zone 1 — Registration Boilerplate (67–94% reducible)

### 1a. Panel Draw() wrappers

Every panel exposes a `void Draw(float dt)` function. The function falls into one of four structural variants:

| Variant | Panels | Boilerplate % |
|---|---|---|
| A: BeginTokenPanel + uiint.BeginPanel-always | validation, system_map, river_index, ~8 others | **90%** — macro-eligible |
| B: BeginTokenPanel + early-return before BeginPanel | inspector, road_editor | 60% — partial macro |
| C: BeginTokenPanel + HFSM state gate | lot_control, building_control, zoning_control | 40% — gate is unique |
| D: BeginDockableWindow | axiom_editor, axiom_bar, dev_shell | 30% — different window API |

**Variant A example** (`rc_panel_validation.cpp` lines 41–66):
```cpp
void Draw(float dt) {
    const bool open = Components::BeginTokenPanel("Validation", UITokens::AmberGlow);
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "Validation", "Validation", "validation", "Bottom",
            "visualizer/src/ui/panels/rc_panel_validation.cpp",
            {"validation", "runtime"}
        },
        open
    );
    if (!open) {
        uiint.EndPanel();
        Components::EndTokenPanel();
        return;
    }
    DrawContent(dt);
    uiint.EndPanel();
    Components::EndTokenPanel();
}
```

This 26-line function is structurally identical in ~10+ panels, with only 6 values varying. A single macro `RC_PANEL_DRAW_IMPL` can collapse it to one 3-line call per panel.

**Estimated savings (Variant A panels only):** ~10 panels × 24 lines = **240 LOC → 30 LOC (87% reduction)**

### 1b. RcPanelDrawers.cpp drawer classes

582 LOC total. 23 Drawer classes. Each is structurally identical (~18–25 lines) with only 4 values varying:

```cpp
namespace RoadIndex {
class Drawer : public IPanelDrawer {
public:
  PanelType type() const override { return PanelType::RoadIndex; }
  const char *display_name() const override { return "Road Index"; }
  PanelCategory category() const override { return PanelCategory::Indices; }
  const char *source_file() const override { return "..."; }
  std::vector<std::string> tags() const override { return {"index", "road", "data"}; }
  void draw(DrawContext &ctx) override { /* call into panel */ }
};
IPanelDrawer *CreateDrawer() { return new Drawer(); }
}
```

**Repetitions:** 23 drawer classes × ~20 lines = ~460 LOC. A `DEFINE_SIMPLE_DRAWER` macro reduces each to 2 lines.
**Estimated savings:** 582 → ~120 LOC = **~460 LOC (79% reduction)**

---

## Zone 2 — Index Trait Selection/Hover Handlers (33% duplicate)

`rc_panel_data_index_traits.h` (535 LOC): 5 trait structs for Road, District, Lot, Building, River.

Each trait contains:
- `OnEntitySelected`: ~30 lines, 95% identical — clears all other selection handles, sets the entity handle, calls `ApplySelectionModifier`. Only the entity type and `VpEntityKind` enum differ.
- `OnEntityHovered`: ~10 lines, 95% identical — calls `RC_UI::Viewport::GetViewportOverlays().SetHovered<Type>(...)`. Only the method name per entity type varies.

**Total duplicated:** 5 traits × 40 lines = **200 LOC → extractable to 5 × 4 lines = 20 LOC via template helpers.**
**Estimated savings: ~180 LOC (34% of file)**

---

## Zone 3 — Property Editor DrawSingle* Functions (34% duplicate)

`rc_property_editor.cpp` (1,292 LOC) contains 4 per-entity draw functions:

| Function | Lines | Entity |
|---|---|---|
| DrawSingleRoad | 92 | Road |
| DrawSingleDistrict | 160 | District |
| DrawSingleLot | 101 | Lot |
| DrawSingleBuilding | ~155 | Building |

**Total: 508 LOC**. Each repeats:

1. **User Placed flag + generation_tag sync** (~20 lines × 4 = 80 LOC):
```cpp
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
    CommitValueChange(history, "<Label> User Flag", before, after, ...);
    MarkDirtyForKind(gs, VpEntityKind::<Kind>);
}
```

2. **Generation Locked checkbox** (~15 lines × 4 = 60 LOC) — identical except entity type

3. **Layer assignment block** (~4 lines × 4 = 16 LOC):
```cpp
const uint8_t current_layer = gs.GetEntityLayer(VpEntityKind::<Kind>, entity.id);
int layer_idx = static_cast<int>(current_layer);
if (ImGui::InputInt("Layer", &layer_idx)) {
    layer_idx = std::clamp(layer_idx, 0, 255);
    gs.SetEntityLayer(VpEntityKind::<Kind>, entity.id, static_cast<uint8_t>(layer_idx));
    gs.dirty_layers.MarkDirty(DirtyLayer::ViewportIndex);
}
```

**Total extractable boilerplate: ~156 LOC (31% of target functions)**

---

## Zone 4 — Control Panel Pulse Animation (minor, ~60 LOC)

`rc_panel_lot_control.cpp`, `rc_panel_building_control.cpp`, `rc_panel_zoning_control.cpp` share an identical pulse animation block (~20 lines each):

```cpp
if (s_is_generating) {
    float pulse = 0.5f + 0.5f * sinf(ImGui::GetTime() * 4.0f);
    ImU32 pulse_color = LerpColor(UITokens::GreenHUD, UITokens::CyanAccent, 1.0f - pulse);
    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(pulse_color));
    // ... button drawing
    ImGui::PopStyleColor();
}
```

**Savings: ~40 LOC** (extract to `Components::DrawGenerationPulse(bool, float)`)

---

## Test Coverage Gaps

The test_ai.cpp schema covers **12 of 23 registered panels (52%)**.

| Panel | In test_ai.cpp | In headless JSON |
|---|---|---|
| AxiomEditor | ✓ | ✗ (headless collapsed) |
| Inspector | ✓ | ✗ |
| SystemMap | ✓ | ✓ (visible: false) |
| ZoningControl | ✓ | ✗ |
| RoadEditor | ✓ | ✗ |
| LotControl / BuildingControl / WaterControl | ✓ | ✗ |
| Tools | ✓ | ✗ |
| DevShell / AiConsole | ✓ | ✗ |
| Workspace | ✓ | ✓ (WorkspaceSelector) |
| AxiomBar | **✗** | ✗ |
| Telemetry | **✗** | ✗ |
| Log | **✗** | ✗ |
| Validation | **✗** | ✗ |
| 5 Index panels | **✗** | ✗ |

**Notable:** Headless introspection only captures 6 panels (Master Panel, Status Bar, Titlebar, Viewport, System Map, WorkspaceSelector). Control panels and diagnostics panels are hidden/collapsed in headless mode, making headless snapshot insufficient for full UI coverage verification.

---

## Placeholder Code

- `rc_panel_axiom_editor.cpp` lines 2594–2626: 7 `ImGui::BeginDisabled()` placeholder buttons for unimplemented operations ("Paint Select (Planned)", "Add/Split (Planned)", "Delete/Trim (Planned)", "Merge/Snap (Planned)" etc.) — ~40 LOC of visual noise with no functionality
- `rc_panel_tools.cpp` lines 850–857: "Policy B (stub): Fixed world extent. Not yet active." comment and disabled control

---

## Savings Summary

| Zone | Target | Before | After | Saved | Reduction |
|---|---|---|---|---|---|
| 1a | ~10 clean BeginTokenPanel Draw() bodies | ~240 | ~30 | ~210 | **87%** |
| 1b | RcPanelDrawers.cpp | 582 | ~120 | ~462 | **79%** |
| 2 | data_index_traits OnSelected/Hovered | ~200 | ~20 | ~180 | **90%** |
| 3 | rc_property_editor DrawSingle* helpers | 508 | ~352 | ~156 | **31%** |
| 4 | Control panel pulse animation | ~60 | ~20 | ~40 | 67% |
| **TOTAL** | **Targeted code** | **~1,590** | **~542** | **~1,048** | **~66%** |

**Overall: ~1,048 LOC reduction from 15,001 = 7% global, 31–90% within each targeted zone.**

The 40% threshold is met or exceeded in Zones 1, 2, and 4.
Zone 3 (property editor) reaches 31% — below threshold — but represents high correctness value (deduplicating a bug-prone copy-paste pattern).
