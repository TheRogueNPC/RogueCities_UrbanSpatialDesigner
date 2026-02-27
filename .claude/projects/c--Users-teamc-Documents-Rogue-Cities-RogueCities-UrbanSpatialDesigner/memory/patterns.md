# RogueCities — Code Patterns & Key APIs

## Panel System (Visualizer)

### Adding a New Panel — Checklist
1. Check if index-style → use `RcDataIndexPanel<T, Traits>`
2. Create `rc_panel_<name>.h/.cpp` in `visualizer/src/ui/panels/`
3. Implement `IPanelDrawer` subclass, implement `draw(DrawContext&)`
4. Register in `visualizer/src/ui/panels/PanelRegistry.cpp` via `InitializePanelRegistry()`
5. Add `PanelType` enum value to `IPanelDrawer.h`
6. Add `PanelCategory` assignment
7. Wire `is_visible(DrawContext&)` to HFSM state if needed

### IPanelDrawer Interface
```cpp
// visualizer/src/ui/panels/IPanelDrawer.h
class IPanelDrawer {
    virtual PanelType type() const = 0;
    virtual const char* display_name() const = 0;
    virtual PanelCategory category() const = 0;
    virtual void draw(DrawContext& ctx) = 0;            // CONTENT ONLY — no Begin/End
    virtual bool is_visible(DrawContext& ctx) const;    // HFSM gate
    virtual void on_activated() {}
    virtual void on_deactivated() {}
};
```

### DrawContext — passed to every drawer
```cpp
struct DrawContext {
    GlobalState& global_state;
    EditorHFSM& hfsm;
    UiIntrospector& introspector;
    CommandHistory* command_history;
    float dt;
    bool is_floating_window;
};
```

### PanelType Enum (complete, as of Feb 2026)
Indices: RoadIndex, DistrictIndex, LotIndex, RiverIndex, BuildingIndex, Indices
Controls: ZoningControl, LotControl, BuildingControl, WaterControl
Tools: AxiomBar, AxiomEditor, RoadEditor
System: Telemetry, Log, Validation, Tools, Inspector, SystemMap, DevShell, UISettings
AI (feature-gated): AiConsole, UiAgent, CitySpec

### RcDataIndexPanel Template
```cpp
// visualizer/src/ui/patterns/rc_ui_data_index_panel.h
template <typename T, typename Traits = DataIndexPanelTraits<T>>
class RcDataIndexPanel {
    void DrawContent(GlobalState& gs, UiIntrospector& uiint);
    int GetSelectedIndex() const;
    void SetSelectedIndex(int index);
    void ClearSelection();
};
```

### Trait interface (required methods)
```cpp
template<typename T> struct DataIndexPanelTraits {
    static ContainerType& GetData(GlobalState& gs);
    static int GetColumnCount();
    static const char* GetColumnHeader(int col);
    static void RenderCell(T& entity, size_t index, int col);
    // Optional: FilterEntity, CompareEntities, OnEntitySelected,
    //           OnEntityHovered, ShowContextMenu, GetPanelTitle
};
```

### Concrete Trait Specializations
File: `visualizer/src/ui/panels/rc_panel_data_index_traits.h`
- RoadIndexTraits → `fva::Container<Road>` — cols: ID, Type, Points
- DistrictIndexTraits → `fva::Container<District>` — cols: ID, Axiom, Borders
- LotIndexTraits → `fva::Container<LotToken>` — cols: ID, DistrictID
- BuildingIndexTraits → `siv::Vector<BuildingSite>` — cols: ID, Type, Lot, Source
- RiverIndexTraits → `fva::Container<WaterBody>` — cols: ID, Type, Depth, Shore

---

## AxiomVisual & PlacementTool

### AxiomVisual — key API
File: `app/include/RogueCity/App/Tools/AxiomVisual.hpp`
```cpp
class AxiomVisual {
    void set_position(const Vec2& pos);
    void set_radius(float r); void set_rotation(float theta);
    void set_type(AxiomType type);
    void set_terminal_feature(TerminalFeature f, bool enabled);
    [[nodiscard]] AxiomInput to_axiom_input() const;  // → CityGenerator input
    // Type-specific setters: set_organic_curviness, set_radial_spokes,
    // set_loose_grid_jitter, set_suburban_loop_strength, set_stem_branch_angle,
    // set_superblock_block_size, set_radial_ring_knob_weight(ring, knob, val)
};
```

### LatticeTopology enum
```cpp
enum class LatticeTopology { BezierPatch, Polygon, Radial, Linear };
```

### AxiomPlacementTool Modes
```cpp
enum class Mode { Idle, Placing, DraggingSize, DraggingAxiom, DraggingKnob, Hovering };
```

Snapshot pattern for undo/redo: `AxiomSnapshot` captures full axiom state.
`consume_dirty()` → poll for changes to trigger regeneration.

---

## GeneratorBridge (App → Generators adapter)
File: `app/include/RogueCity/App/Integration/GeneratorBridge.hpp`
```cpp
class GeneratorBridge {  // All static methods
    static std::vector<AxiomInput> convert_axioms(const vector<unique_ptr<AxiomVisual>>&);
    static AxiomInput convert_axiom(const AxiomVisual&);
    static bool validate_axioms(const vector<AxiomInput>&, const Config&);
    static double compute_decay_from_rings(float r1, float r2, float r3);
};
```

---

## Theme System
File: `app/include/RogueCity/App/UI/ThemeManager.h`

### ThemeProfile — 12 color tokens
primary_accent, secondary_accent, success_color, warning_color, error_color,
background_dark, panel_background, grid_overlay,
text_primary, text_secondary, text_disabled, border_accent

### ThemeManager API
```cpp
ThemeManager::Instance().LoadTheme("DowntownCity");
ThemeManager::Instance().ApplyToImGui();
ImU32 c = ThemeManager::Instance().ResolveToken("color.ui.primary");
```

### Built-in Themes
Default, Soviet (alias→DowntownCity), RedRetro (#D12128/#FAE3AC/#01344F),
DowntownCity (#1B1B2F/#E94560/#0F3460), RedlightDistrict, CyberPunk, Tron

Custom themes saved to: `AI/config/themes/*.json`

---

## GeometryPolicy
File: `app/include/RogueCity/App/Tools/GeometryPolicy.hpp`
```cpp
struct GeometryPolicy {
    double district_placement_half_extent{45.0};  // meters
    double lot_placement_half_extent{16.0};
    double building_default_scale{1.0};
    double water_default_depth{6.0};
    double water_falloff_radius_world{42.0};
    double merge_radius_world{40.0};
    double edge_insert_multiplier{1.5};
    double base_pick_radius{8.0};
};
// Resolve from GlobalState + EditorState:
GeometryPolicy ResolveGeometryPolicy(const GlobalState&, EditorState);
double ComputeFalloffWeight(double distance, double radius);
```

---

## UI Components (Reusable)
File: `visualizer/src/ui/rc_ui_components.h`
```cpp
BeginTokenPanel(title, border_color, ...)   // Styled panel with Y2K border
EndTokenPanel()
AnimatedActionButton(id, label, feedback, dt, active, size)  // Scale+glow
StatusChip(label, color, emphasized)
DrawScanlineBackdrop(min, max, time_sec, tint)  // CRT retro VFX
TextToken(color, fmt, ...)
BoundedText(text, padding)
DraggableSectionDivider(label, popup_id)
```
Min hit target: 28px (P0 requirement).

---

## Docking & Layout
File: `app/include/RogueCity/App/Docking/DockLayoutManager.hpp`
- `DockLayoutManager::transition_to_state(EditorState, float duration)` — animated panel transitions
- Maps EditorState → visible panel set
- `DockLayoutState::to_ini()` / `from_ini()` for persistence

---

## Command History (Undo/Redo)
File: `app/include/RogueCity/App/Editor/CommandHistory.hpp`
```cpp
class ICommand {
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual const char* GetDescription() const = 0;
};
class CommandHistory {
    void Execute(unique_ptr<ICommand>);  // Run + push
    void Commit(unique_ptr<ICommand>);   // Push already-executed cmd
    void Undo(); void Redo();
    bool CanUndo() const; bool CanRedo() const;
    const ICommand* PeekUndo() const; const ICommand* PeekRedo() const;
    void Clear();
};
// Global accessors:
CommandHistory& GetEditorCommandHistory();
void ResetEditorCommandHistory();
```

---

## Generation Pipeline (App Integration)

### CityOutputApplier
File: `app/include/RogueCity/App/Integration/CityOutputApplier.hpp`
```cpp
enum class GenerationScope : uint8_t { RoadsOnly, RoadsAndBounds, FullCity };
struct CityOutputApplyOptions {
    GenerationScope scope{ FullCity };
    bool rebuild_viewport_index{ true };
    bool mark_dirty_layers_clean{ true };
    bool preserve_locked_user_entities{ true };
};
void ApplyCityOutputToGlobalState(const CityOutput&, GlobalState&, const CityOutputApplyOptions& = {});
```

### GenerationCoordinator
File: `app/include/RogueCity/App/Integration/GenerationCoordinator.hpp`
```cpp
enum class GenerationRequestReason : uint8_t { Unknown, LivePreview, ForceGenerate, ExternalRequest };
class GenerationCoordinator {
    void Update(float dt);
    void SetDebounceDelay(float seconds);
    void RequestRegeneration(axioms, config, depth, reason);          // debounced
    void RequestRegenerationIncremental(axioms, config, dirty_stages, depth, reason);
    void ForceRegeneration(axioms, config, depth, reason);            // immediate
    void ForceRegenerationIncremental(axioms, config, dirty_stages, depth, reason);
    void CancelGeneration(); void ClearOutput();
    bool IsGenerating() const; float GetProgress() const;
    const CityOutput* GetOutput() const;
    GenerationPhase Phase() const; float PhaseElapsedSeconds() const;
    // Serial tracking (ensures only latest request triggers callback):
    uint64_t LastScheduledSerial() const; uint64_t LastCompletedSerial() const;
    static const char* ReasonName(GenerationRequestReason);
};
```
Generation policy (domain→method): Axiom=LiveDebounced, others=Explicit (mark dirty, user triggers).

### RealTimePreview
File: `app/include/RogueCity/App/Integration/RealTimePreview.hpp`
```cpp
enum class GenerationDepth : uint8_t { AxiomBounds, FullPipeline };
enum class GenerationPhase : uint8_t { Idle, InitStreetSweeper, Sweeping, Cancelled, StreetsSwept };
// Uses std::jthread for background generation, atomic flags for progress, mutex for output swap
// Default debounce: 0.5s
```

### ViewportIndexBuilder
File: `app/include/RogueCity/App/Editor/ViewportIndexBuilder.hpp`
```cpp
struct ViewportIndexBuilder {
    static void Build(GlobalState& gs);  // Rebuilds gs.viewport_index from entity containers
};
```

### PrimaryViewport
File: `app/include/RogueCity/App/Viewports/PrimaryViewport.hpp`
```cpp
class PrimaryViewport {
    void set_camera_position(const Vec2& xy, float z);
    void set_camera_yaw(float radians);
    Vec2 screen_to_world(const ImVec2&) const;
    ImVec2 world_to_screen(const Vec2&) const;
    float world_to_screen_scale(float world_distance) const;
    bool is_hovered() const;
    void set_active_tool(IViewportTool* tool);
    void set_city_output(const CityOutput*);
    // Initial camera_z = 500.0f, zoom = 1.0f
};
```

---

## Viewport Interaction System
File: `visualizer/src/ui/viewport/rc_viewport_interaction.h`

### InteractionOutcome enum
```cpp
enum class InteractionOutcome : uint8_t {
    None, Mutation, Selection, GizmoInteraction,
    ActivateOnly, BlockedByInputGate, NoEligibleTarget
};
```

### NonAxiomInteractionState (drag state bundle)
```cpp
struct NonAxiomInteractionState {
    SelectionDragState selection_drag;      // lasso_active, box_active, lasso_points
    GizmoDragState gizmo_drag;             // active, pivot, previous_world
    RoadVertexDragState road_vertex_drag;  // road_id, vertex_index, tangent handle
    DistrictBoundaryDragState district_boundary_drag;
    WaterVertexDragState water_vertex_drag;
    SplinePenDragState road_pen_drag;
    SplinePenDragState water_pen_drag;
};
```

### NonAxiomInteractionResult
```cpp
struct NonAxiomInteractionResult {
    bool active, has_world_pos, handled, requires_explicit_generation;
    Vec2 world_pos;
    const char* status_code{ "idle" };
    InteractionOutcome outcome;
};
```

### Key functions
```cpp
AxiomInteractionResult ProcessAxiomViewportInteraction(const AxiomInteractionParams&);
NonAxiomInteractionResult ProcessNonAxiomViewportInteraction(
    const NonAxiomInteractionParams&, NonAxiomInteractionState*);
void ProcessViewportCommandTriggers(const CommandInteractionParams&, const CommandMenuStateBundle&);
const char* InteractionOutcomeString(InteractionOutcome);
```

---

## Viewport Overlays
File: `visualizer/src/ui/viewport/rc_viewport_overlays.h`

### OverlayConfig (30+ toggles)
```cpp
struct OverlayConfig {
    // Rendering layers
    bool show_zone_colors, show_aesp_heatmap, show_road_labels, show_tensor_field;
    bool show_roads, show_districts, show_lots, show_water_bodies, show_building_sites;
    bool show_city_boundary, show_connector_graph, show_validation_errors, show_gizmos;
    bool show_scale_ruler, show_compass_gimbal;
    bool compass_parented; ImVec2 compass_center; float compass_radius{ 36.0f };
    enum class AESPComponent { Access, Exposure, Service, Privacy, Combined };
    AESPComponent aesp_component{ Combined };
};
```

### District Color Scheme (Y2K palette)
Residential=Blue (0.3,0.5,0.9), Commercial=Green (0.3,0.9,0.5),
Industrial=Red (0.9,0.3,0.3), Civic=Orange (0.9,0.7,0.3), Mixed=Gray

### ViewportOverlays key API
```cpp
class ViewportOverlays {
    void Render(GlobalState&, const OverlayConfig&);  // Main call
    void RenderZoneColors, RenderRoadNetwork, RenderAESPHeatmap, ...;
    void RenderFlightDeckHUD, RenderScaleRulerHUD, RenderCompassGimbalHUD(...);
    std::optional<float> requested_yaw_;  // Compass drives camera yaw
    ImVec2 WorldToScreen(const Vec2&) const;
};
ViewportOverlays& GetViewportOverlays();       // Singleton accessor
BuildingSearchOverlay& GetBuildingSearchOverlay();
```
Note: `AI_INTEGRATION_TAG: V1_PASS1_TASK5_*` comments mark AI-wired sections.

---

## VpProbeData & SelectionManager

### VpEntityKind enum
```cpp
enum class VpEntityKind : uint8_t {
    Unknown, Axiom, Road, District, Lot, Building, Water, Block
};
constexpr uint32_t kViewportIndexInvalid = 0xFFFFFFFFu;
```

### VpProbeData (88 bytes, flat cache-friendly)
```cpp
struct VpProbeData {
    VpEntityKind kind;
    uint32_t id, stable_id, parent, first_child;
    uint16_t child_count;
    uint32_t axiom_id, road_id, district_id, block_id;
    float slope, flood;
    std::array<float, 4> aesp;
    uint8_t frontage_family, afg_preset, road_hierarchy, zone_mask, layer_id;
    float afg_band;
    char label[32];  // e.g. "Road #42"
};
void SetViewportLabel(VpProbeData&, const char*);  // safe snprintf wrapper
```
Design rule: "If UI needs data not in index, the index is wrong — not the UI"
Memory: ~27KB (village), ~711KB (large city), ~6MB (metropolis)

### SelectionManager
```cpp
struct SelectionItem { VpEntityKind kind; uint32_t id; };
class SelectionManager {
    void Clear(); void SetItems(vector<SelectionItem>);
    void Select(VpEntityKind, uint32_t id);  // replaces
    void Add(VpEntityKind, uint32_t id);     // append if not present
    void Toggle(VpEntityKind, uint32_t id);
    bool IsSelected(VpEntityKind, uint32_t id);
    size_t Count(); bool IsMultiSelect();
    const SelectionItem* Primary();          // items_[0]
    std::span<const SelectionItem> Items();
};
```

---

## Road Editor Panel
File: `visualizer/src/ui/panels/rc_panel_road_editor.h/.cpp`
```cpp
enum class RoadEditorSubtool { Select, AddVertex, Split, Merge, Convert };
```
Three stations layout: Mode (subtool toggle), Telemetry (road metrics), Mutation (context-dependent)
- Uses `magic_enum` for road type combo display
- Pulsing active state: sine wave at 4.0 Hz
- Panel token: `UITokens::AmberGlow`
