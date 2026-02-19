Repository-specific Addendum — Viewport Tool Masterpass

Purpose
- Provide concrete, repo-aligned next steps and file-level scaffolding to implement the handler pipeline described in `Tools_Masterpass_Plan.md`.

High-level constraints (from repo)
- Keep `core/` UI-free; viewport/tool code lives in `app/` and `visualizer/` layers.
- Many existing systems are already present and must be preserved: `AxiomPlacementTool`, `ViewportIndex` / `VpProbeData`, `EditorManipulation` camera math and `PrimaryViewport` render path.
- Hot-reload: prefer editing `.cpp` files for rapid iteration when testing UI changes.

Concrete next steps (incremental, safe)
1) Add a small handler interface and namespace
- Path: `app/src/Tools/viewport_handlers/`
- Files to create:
  - `viewport_handler.h` — declare `struct ViewportContext; struct HandlerResult; class IViewportHandler { virtual bool Handle(ViewportContext&, HandlerResult&) = 0; virtual ~IViewportHandler() = default; };`
  - `viewport_handler_dispatcher.h/.cpp` — maintain `std::vector<std::unique_ptr<IViewportHandler>>` and call handlers in order until one returns true (consumed).

2) Scaffold the first handlers (one at a time)
- Start with `GlobalPanHandler` (lowest risk):
  - `app/src/Tools/viewport_handlers/global_pan_handler.h/.cpp`
  - Implement only input->camera XY translation logic using existing `EditorManipulation::screen_to_world` and camera setters.
  - Tests: manual pan/zoom sanity; ensure not consumed when modifier not present.

- Next low-risk: `GlobalSelectHandler`:
  - Use `ViewportIndex`/`VpProbeData` to resolve picks; call into `SelectionManager` or existing selection API.

- Later: `GlobalGizmoHandler`, `GlobalLassoBoxHandler`, `DomainPlacementHandler`, `GlobalInspectHandler`.

3) Minimal migration to dispatcher in `PrimaryViewport`
- Add a short dispatcher call in `PrimaryViewport::ProcessNonAxiomViewportInteraction` that forwards to the new dispatcher (initially with only Pan handler registered).
- Keep original monolith code present but behind an `#if 0` or `TODO` guard until handlers are fully tested.

4) CMake / build notes
- Add new sources under `app/CMakeLists.txt` (or the CMakeLists responsible for `app/src/Tools/`) so they compile with existing targets.
- Keep changes incremental to avoid broad rebuilds; add only the small handler sources first.

5) Tests & verification
- Add `tests/test_viewport_handlers.cpp` for unit-testing handler logic where possible (camera delta math, selection filtering).
- Add a debug runtime flag `--viewport-handler-log` to print handler order/consumption for manual verification during interactive sessions.

6) HFSM and EditorState considerations
- Do not move HFSM transitions in the same change as handler extraction. Instead:
  - Implement handler pipeline first, keep current HFSM logic unchanged.
  - For visibility gating, use handler `is_visible()` or `ViewportContext.editor_state` to early-noop without changing HFSM until later.

7) Split `NonAxiomInteractionState` incrementally
- Create per-handler sub-state structs under `app/src/Tools/viewport_handlers/states/`.
- Migrate only the fields a handler needs (anchor point, drag rect, gizmo id) to reduce risk.

8) Manual test checklist (after Pan + Select)
- Run GUI, enter Editor state `Viewport_Pan`, verify Middle/Alt+LMB pan behavior matches previous behavior.
- Verify selection still works and multi-select (Shift+Click) unchanged.
- Verify that other tools still render their previews (axiom placement ghost, etc.).

Scaffold example: `viewport_handler.h`

// (conceptual, small)

namespace RogueCity::App::ViewportHandlers {

struct ViewportContext {
    GlobalState* gs; // existing global state pointer
    InputSnapshot input; // condensed input for handler (mouse pos, buttons, mods)
    // add refs to probe index, selection manager as needed
};

struct HandlerResult {
    bool consumed = false;
};

class IViewportHandler {
public:
    virtual ~IViewportHandler() = default;
    virtual void Handle(ViewportContext& ctx, HandlerResult& out) = 0;
};

}

Migration plan (stepwise)
- Step A: Add interface + dispatcher, register `GlobalPanHandler`, wire into `PrimaryViewport`.
- Step B: Implement `GlobalSelectHandler` and register; verify selection tests.
- Step C: Add `GlobalGizmoHandler` and `GlobalLassoBoxHandler`.
- Step D: Migrate domain placement logic (Axiom) into `DomainPlacementHandler` (highest risk; do last).

Risks & mitigations
- Risk: HFSM/visibility semantics change unexpectedly. Mitigation: avoid editing HFSM; use `ViewportContext.editor_state` for gating.
- Risk: Regressing Axiom placement or gizmo behavior. Mitigation: maintain original code until new handlers pass manual tests; create small test harnesses.
- Risk: Large rebuilds. Mitigation: add only small `.cpp` files in early steps; avoid touching headers used widely.

Estimated effort
- Pan handler + dispatcher scaffold: small (1–2 days including testing and CMake updates).
- Select + Gizmo + Lasso: medium (2–4 days).
- Full migration of domain placement & HFSM cleanup: larger (variable, depends on coverage/tests).

Next immediate action I can do for you
- Create `viewport_handler.h` + dispatcher scaffold and implement `GlobalPanHandler` with CMake entry and a short manual test guide. Approve and I'll scaffold files now.

---

References (from repo scan / plan)
- `PrimaryViewport.cpp` (entry for current monolith)
- `AxiomPlacementTool.cpp` (existing domain placement behavior)
- `EditorManipulation.cpp` (camera math)
- `ViewportIndex.hpp` / `VpProbeData` (picking and probe info)

### Minimap Integration

**Current State:**
- The minimap is implemented as a separate ImGui overlay in `visualizer/src/ui/rc_ui_minimap.cpp`.
- It renders a simplified version of the city layout using `CityGenerator::CityOutput` data.
- Supports zoom levels and panning, but lacks interactive features like click-to-navigate.

**Integration Points:**
- `CityGenerator::CityOutput` provides the data for roads, districts, and lots.
- `EditorManipulation` is used for coordinate transformations between world and minimap space.
- `ViewportIndex` is not currently leveraged for minimap interactions but could be extended for entity highlighting.

**Next Steps:**
1. Add click-to-navigate functionality:
   - Use `EditorManipulation::screen_to_world` to map minimap clicks to world coordinates.
   - Update the camera position in `PrimaryViewport`.
2. Highlight selected entities:
   - Extend `ViewportIndex` to provide entity bounds for minimap rendering.
   - Use `SelectionManager` to sync highlights between the main viewport and minimap.

### Systems Map Integration

**Current State:**
- The systems map is a conceptual overlay for visualizing infrastructure layers (e.g., roads, utilities, zoning).
- No dedicated implementation exists yet; planned as an extension of the minimap.

**Integration Points:**
- `CityGenerator::CityOutput` will provide layer-specific data (e.g., road hierarchy, zoning masks).
- `VpProbeData` can be extended to include infrastructure-specific metadata for interactive queries.
- `GlobalState` will need additional fields to track active layers and visibility toggles.

**Next Steps:**
1. Define data structures for infrastructure layers:
   - Extend `CityGenerator::CityOutput` to include utility networks and zoning overlays.
   - Add layer visibility toggles to `GlobalState`.
2. Implement a layer toggle UI:
   - Use ImGui to create a collapsible menu for enabling/disabling layers.
   - Sync layer visibility with the systems map rendering logic.
3. Prototype the systems map overlay:
   - Start with a static rendering of road hierarchy and zoning masks.
   - Add interactivity (e.g., hover tooltips, click-to-query) in later iterations.

**Risks & Mitigations:**
- **Risk:** Performance impact from rendering additional layers. **Mitigation:** Use level-of-detail (LOD) techniques to simplify rendering at lower zoom levels.
- **Risk:** Data inconsistencies between the main viewport and overlays. **Mitigation:** Ensure all overlays use the same `CityGenerator::CityOutput` instance for data sourcing.
