# UI Panel Pattern Contract (2026-03-01)

**Author:** Claude
**Layer:** `visualizer/src/ui/`
**Status:** Enforced — patterns are live in codebase
**Applies to:** All agents writing or modifying visualizer UI panels, drawers, property editors, or index traits

---

## Purpose

This contract documents four canonical patterns extracted during the Zone 1–4 code-reduction audit. Any agent adding new panels, drawers, or property functions **must** follow these patterns. Any agent modifying existing code in the targeted files **must not** revert these patterns to the old verbatim style.

---

## Pattern 1 — `RC_PANEL_DRAW_IMPL` (Variant A panels)

**File:** `visualizer/src/ui/rc_ui_panel_macros.h`

Collapses the 17-line `Draw(float dt)` boilerplate in standard panels:

```cpp
// OLD — 17 lines, repeated in every eligible panel:
void Draw(float dt) {
    const bool open = Components::BeginTokenPanel("Name", UITokens::Color);
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(RogueCity::UIInt::PanelMeta{"Name","Label","id","Dock","file",{tags}}, open);
    if (!open) { uiint.EndPanel(); Components::EndTokenPanel(); return; }
    DrawContent(dt);
    uiint.EndPanel();
    Components::EndTokenPanel();
}

// NEW — macro call:
RC_PANEL_DRAW_IMPL(
    "Name", "Label", "panel_id", "Dock",
    UITokens::Color,
    "visualizer/src/ui/panels/rc_panel_<name>.cpp",
    "tag1", "tag2"
)
```

### When to use

Apply to any new panel whose `Draw(float dt)` follows the **Variant A** pattern:
- Opens with `Components::BeginTokenPanel(title, color)` — no extra ImGui window flags
- Immediately calls `uiint.BeginPanel(...)` with the standard 6-field PanelMeta
- No HFSM state gate before `BeginTokenPanel`
- No `BeginDockableWindow`
- Calls `DrawContent(dt)` and closes symmetrically

### Currently applied
`rc_panel_validation.cpp`, `rc_panel_system_map.cpp`, `rc_panel_telemetry.cpp`

### Variant exclusions — do NOT use the macro for

| Variant | Panels | Reason |
|---|---|---|
| B | `rc_panel_inspector.cpp`, `rc_panel_road_editor.cpp` | Early-return before BeginPanel or no uiint |
| C | `rc_panel_lot_control.cpp`, `rc_panel_building_control.cpp`, `rc_panel_water_control.cpp`, `rc_panel_zoning_control.cpp` | HFSM state gate + `ImGuiWindowFlags_AlwaysAutoResize` before `BeginTokenPanel` |
| D | `rc_panel_axiom_editor.cpp`, `rc_panel_axiom_bar.cpp`, `rc_panel_dev_shell.cpp`, `rc_panel_tools.cpp` | Uses `BeginDockableWindow` |
| Special | `rc_panel_log.cpp` | Custom viewport overlay — no `BeginTokenPanel` at all |
| Special | `rc_panel_workspace.cpp` | `BeginDockableWindow` + `s_open` guard |

### Requirements for the caller's `.cpp`
Must include before using the macro:
```cpp
#include "ui/rc_ui_components.h"
#include "ui/introspection/UiIntrospection.h"
#include "ui/rc_ui_panel_macros.h"
```

---

## Pattern 2 — `RC_DEFINE_INDEX_DRAWER` / `RC_DEFINE_DRAWER` (drawer classes)

**File:** `visualizer/src/ui/panels/RcPanelDrawers.cpp` (macros defined at top of file)

Collapses 18-line `IPanelDrawer` subclass + `CreateDrawer()` factory into a 3-line macro call.

```cpp
// Index panels — draw() calls GetPanel().DrawContent(ctx.global_state, ctx.introspector)
RC_DEFINE_INDEX_DRAWER(RoadIndex, RoadIndex, "Road Index",
    "visualizer/src/ui/panels/rc_panel_road_index.cpp",
    "index", "road", "data")

// Simple panels — draw() calls DrawContent(ctx.dt) with no state overrides
RC_DEFINE_DRAWER(Validation, Validation, "Validation", System,
    "visualizer/src/ui/panels/rc_panel_validation.cpp",
    "system", "validation", "errors")
```

### When to use `RC_DEFINE_INDEX_DRAWER`
The panel namespace exposes `GetPanel()` returning a `RcDataIndexPanel<T>` instance, and `draw()` calls `GetPanel().DrawContent(ctx.global_state, ctx.introspector)`.

### When to use `RC_DEFINE_DRAWER`
The panel namespace exposes `DrawContent(float dt)` and the drawer requires no `is_visible()` override, no `can_popout()` override, and the draw function is simply `DrawContent(ctx.dt)`.

### Cannot use macros — hand-write the drawer class for

| Drawer | Reason |
|---|---|
| `ZoningControl` | `is_visible()` checks 3 HFSM states |
| `RoadEditor` | `is_visible()` checks 1 HFSM state |
| `AxiomBar` | `can_popout() { return false; }` override |
| `WorkspaceSettings` | draw calls `Workspace::DrawContent` (namespace mismatch) |
| `Log` | draw calls `Log::Draw` (not `DrawContent`) |
| `AiConsole`, `UiAgent`, `CitySpec` | Feature-gated (`#if ROGUE_AI_DLC_ENABLED`) + `is_visible()` checking `config.dev_mode_enabled` |

### Macro parameter reference

```
RC_DEFINE_DRAWER(NS, TYPE, DISPLAY, CAT, FILE, tags...)
  NS      — namespace name (e.g. Validation)
  TYPE    — PanelType enum value (e.g. Validation)
  DISPLAY — display_name() string (e.g. "Validation")
  CAT     — PanelCategory value (Indices/Controls/Tools/System/AI/Hidden)
  FILE    — source_file() path string
  tags... — variadic string tags for tags()

RC_DEFINE_INDEX_DRAWER(NS, TYPE, DISPLAY, FILE, tags...)
  (same, but category is always Indices and draw calls GetPanel().DrawContent)
```

---

## Pattern 3 — Entity Flag Helpers in `rc_property_editor.cpp`

**File:** `visualizer/src/ui/panels/rc_property_editor.cpp` (anonymous namespace, before `DrawSingleRoad`)

Three template/static helpers replacing copy-pasted blocks in `DrawSingleRoad/District/Lot/Building`.

### `DrawUserFlagBlock<T>` — User Placed/Created checkbox

```cpp
DrawUserFlagBlock(history, gs, entity, VpEntityKind::Road,
    &RogueCity::Core::Road::is_user_created,   // member pointer to bool field
    "User Created",                             // ImGui checkbox label
    "Road User Flag");                          // undo history description
```

Renders the User Placed/Created checkbox, syncs `generation_tag`/`generation_locked` on change, commits the change to history, and calls `MarkDirtyForKind`. Also renders the `ImGui::Text("Generation Tag: ...")` line immediately after.

**Field name convention:** Road uses `is_user_created`; all other entities (District, Lot, Building) use `is_user_placed`. Use the correct member pointer for each entity type.

### `DrawGenerationLocked<T>` — Generation Locked checkbox

```cpp
DrawGenerationLocked(history, gs, entity, VpEntityKind::Road, "Road Generation Lock");
```

Renders the Generation Locked checkbox, commits change to history, calls `MarkDirtyForKind`.

### `DrawLayerAssignment` — Layer integer input

```cpp
DrawLayerAssignment(gs, VpEntityKind::Road, road.id);
```

Renders `ImGui::InputInt("Layer", ...)`, clamps to `[0, 255]`, calls `gs.SetEntityLayer` and marks `DirtyLayer::ViewportIndex`.

### Call sequence in every `DrawSingleX` function

The three helpers must be called in this order (matching the established UI layout):
1. `DrawUserFlagBlock(...)` — includes the gen-tag text display
2. `DrawGenerationLocked(...)` — immediately after
3. *(entity-specific fields in between)*
4. `DrawLayerAssignment(...)` — just before the geometry/AESP section

### Adding a new entity type
If a new entity (e.g. WaterBody) gains a `DrawSingleWaterBody` function in the property editor:
- Use `DrawUserFlagBlock` with `&RogueCity::Core::WaterBody::is_user_placed`
- Use `DrawGenerationLocked` and `DrawLayerAssignment`
- Do NOT write the 32 lines inline

---

## Pattern 4 — `ClearAllSelections` in `rc_panel_data_index_traits.h`

**File:** `visualizer/src/ui/panels/rc_panel_data_index_traits.h` (anonymous namespace)

```cpp
// Called at the start of OnEntitySelected in Road, District, Lot, Building traits.
ClearAllSelections(gs);
gs.selection.selected_<entity> = gs.<container>.createHandleFromData(index);
ApplySelectionModifier(gs.selection_manager, VpEntityKind::<Kind>, entity.id);
```

### Rule
Every `OnEntitySelected` in a trait struct that manages one of the four core entity types (Road, District, Lot, Building) **must** begin with `ClearAllSelections(gs)` before setting its own handle. This ensures that clicking any entity in an index panel correctly deselects all other entity types.

### Exception
`RiverIndexTraits::OnEntitySelected` intentionally does not call `ClearAllSelections` (it only sets a viewport focus point, not a selection handle). Do not add it there.

### Adding a new index trait
If a new `XxxIndexTraits` struct is added for a fifth entity type with its own handle in `gs.selection`:
1. Add the new selection field to `ClearAllSelections`.
2. Call `ClearAllSelections(gs)` at the start of the new trait's `OnEntitySelected`.

---

## Anti-Patterns — What Agents Must Not Do

| Anti-Pattern | Correct Alternative |
|---|---|
| Write a new 17-line `Draw(float dt)` with inline `BeginTokenPanel`+`uiint.BeginPanel` for a Variant A panel | Use `RC_PANEL_DRAW_IMPL` |
| Write a new 18-line `class Drawer : public IPanelDrawer` in `RcPanelDrawers.cpp` for a plain `DrawContent` panel | Use `RC_DEFINE_DRAWER` or `RC_DEFINE_INDEX_DRAWER` |
| Copy-paste the 17-line user-flag checkbox block into a new `DrawSingleX` function | Call `DrawUserFlagBlock<T>` |
| Copy-paste the 10-line generation-locked checkbox block | Call `DrawGenerationLocked<T>` |
| Copy-paste the 5-line layer assignment block | Call `DrawLayerAssignment` |
| Write `gs.selection.selected_road = {}; gs.selection.selected_district = {}; ...` inline in a new `OnEntitySelected` | Call `ClearAllSelections(gs)` |
| Add `is_visible()` to a drawer that is supposed to use `RC_DEFINE_DRAWER` | Keep it hand-written in the COMPLEX DRAWERS section |

---

## Verification Checklist

After adding any new panel or drawer:

- [ ] If Variant A: `RC_PANEL_DRAW_IMPL` is used and the `.cpp` includes `rc_ui_panel_macros.h`
- [ ] If drawer has no state overrides and calls `DrawContent(dt)`: `RC_DEFINE_DRAWER` is used in `RcPanelDrawers.cpp`
- [ ] If drawer is an index panel: `RC_DEFINE_INDEX_DRAWER` is used
- [ ] If drawer has custom overrides: it is in the `// COMPLEX DRAWERS` section with a comment explaining why
- [ ] Any new `DrawSingleX` in `rc_property_editor.cpp` uses all three helpers
- [ ] Any new index trait with a 4-type selection handle calls `ClearAllSelections` in `OnEntitySelected`
- [ ] `CHANGELOG.md` updated

---

## Files Modified by This Pattern Set

| File | Change |
|---|---|
| `visualizer/src/ui/rc_ui_panel_macros.h` | **CREATED** — `RC_PANEL_DRAW_IMPL` macro |
| `visualizer/src/ui/panels/RcPanelDrawers.cpp` | `RC_DEFINE_INDEX_DRAWER` + `RC_DEFINE_DRAWER` macros + collapsed 15 drawer classes |
| `visualizer/src/ui/panels/rc_panel_validation.cpp` | `RC_PANEL_DRAW_IMPL` applied |
| `visualizer/src/ui/panels/rc_panel_system_map.cpp` | `RC_PANEL_DRAW_IMPL` applied |
| `visualizer/src/ui/panels/rc_panel_telemetry.cpp` | `RC_PANEL_DRAW_IMPL` applied |
| `visualizer/src/ui/panels/rc_property_editor.cpp` | `DrawUserFlagBlock<T>`, `DrawGenerationLocked<T>`, `DrawLayerAssignment` extracted and applied to all 4 `DrawSingleX` functions |
| `visualizer/src/ui/panels/rc_panel_data_index_traits.h` | `ClearAllSelections` extracted and applied to Road/District/Lot/Building `OnEntitySelected` |
