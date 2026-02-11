## RogueCities Urban Spatial Designer Editors: Current State & Future Needs

Based on my analysis of the  [repository](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner), you have an impressive editor system in place with a solid architecture. Let me break down what you have, what you need, and key considerations for your MVP and beyond.

## What You Currently Have

### Core Editor State Management

Your [`EditorState.hpp`](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/core/include/RogueCity/Core/Editor/EditorState.hpp) implements a **hierarchical finite state machine (HFSM)** with:

- **Modal states**: Editing modes for different urban elements (Axioms, Roads, Districts, Lots, Buildings, Water)
- **Viewport interaction states**: Pan, Select, PlaceAxiom, DrawRoad, BoxSelect
- **Simulation states**: Simulating, Paused, Stepping
- **Workflow states**: Startup, NoProject, ProjectLoading, Idle, Shutdown

### UI Panel Editors (Specialized)

You have an extensive collection of **20+ specialized editor panels**:

#### Feature-Specific Editors

- **Axiom Editor** (`rc_panel_axiom_editor.cpp` - 36KB, your largest panel): Comprehensive axiom placement/modification
- **Axiom Bar** (`rc_panel_axiom_bar.cpp`): Quick axiom tool selection
- **Water Control** (`rc_panel_water_control.cpp`): River/water feature editing
- **Zoning Control** (`rc_panel_zoning_control.cpp`): District zoning management
- **Building Control** (`rc_panel_building_control.cpp`): Building placement parameters
- **Lot Control** (`rc_panel_lot_control.cpp`): Lot subdivision editing

#### Data Index Panels (View-Only)

- Road Index, District Index, Lot Index, Building Index, River Index
- These use trait-based patterns (`rc_panel_data_index_traits.h`)

#### Utility/Debug Panels

- **Inspector** (`rc_panel_inspector.cpp`): Property inspection
- **Dev Shell** (`rc_panel_dev_shell.cpp`): Developer console
- **Telemetry** (`rc_panel_telemetry.cpp`): Performance monitoring
- **System Map** (`rc_panel_system_map.cpp`): System-level overview
- **Log Panel** (`rc_panel_log.cpp`): Message logging
- **AI Console** (`rc_panel_ai_console.cpp`): AI Bridge integration
- **UI Agent** (`rc_panel_ui_agent.cpp` - 16KB): AI-driven UI optimization
- **City Spec** (`rc_panel_city_spec.cpp`): High-level city parameters
- **Tools Panel** (`rc_panel_tools.cpp`): Tool palette

## What I Need TODO

### 1. **Unified Property Editor/Inspector**

**Current Gap**: While you have an `rc_panel_inspector.cpp`, you need a **context-sensitive property grid** that automatically displays editable properties for whatever is selected.

**Why You Need This**:

- Reduces UI clutter by showing only relevant properties
- Provides consistent editing interface across all entity types
- Enables undo/redo integration at the property level
- Supports batch editing of multiple selections

**Recommended Implementation**:

```cpp
// Pattern: Reflection-based property editor
class PropertyEditor {
    void Render(SelectionContext& selection) {
        if (auto* axiom = selection.GetAs<AxiomInput>()) {
            PropertyGrid("Axiom Properties", *axiom);
        } else if (auto* road = selection.GetAs<Road>()) {
            PropertyGrid("Road Properties", *road);
        }
        // ... handle all selectable types
    }
    
    template<typename T>
    void PropertyGrid(const char* label, T& obj);
};
```

### 2. **Road Network Editor (Interactive)**

**Current Gap**: You have road **generation** but need interactive **editing**:

- Vertex manipulation (drag intersection points)
- Edge splitting/merging
- Road type conversion (arterial → boulevard)
- Curve editing for organic road shapes

**Why Critical for MVP**: Roads are the backbone of your system. Users need fine control over the generated networks, especially for:
- Fixing problematic intersections
- Adding manual shortcuts
- Creating custom highway ramps
- Adjusting curves around landmarks

**Recommended Pattern**:

```cpp
class RoadNetworkEditor : public IEditorPanel {
    enum class EditMode { Select, AddVertex, SplitEdge, Merge, Convert };
    
    void HandleViewportClick(Vec2 worldPos);
    void HandleVertexDrag(RoadVertex* v, Vec2 delta);
    void DrawGizmos(); // Show handles, snapping guides
};
```

### 3. **District Boundary Editor**

**Current Gap**: Districts are **derived from AESP road analysis**, but you may need manual override:

- Draw custom district polygons
- Override AESP-based classification
- Set district-specific building rules
- Define transition zones

### 4. **Curve/Spline Editor (For Organic Roads)**

**Gap**: our axiom system uses **radial/delta axioms for organic shapes**, but direct curve editing would enhance authoring:

- Bezier/Catmull-Rom splines for roads
- Elevation profile editing
- River path design

**Priority**: critical 

### 5. **Timeline/History Editor (Undo System)**

**Critical Missing Piece**: No mention of undo/redo in our codebase. This is **essential for any editor**:
- Command pattern for all mutations
- Visual timeline scrubbing
- Branch/fork support for "what-if" scenarios

**MVP Requirement**: At minimum, implement basic undo/redo for axiom placement.

### 6. **Preset/Template Editor**

**Current**: `rc_panel_city_spec.cpp` handles city-level parameters **Enhancement Needed**:

- Save/load axiom configurations as presets
- District type templates
- Building style palettes
- Complete city templates ("Medieval European", "Modern Grid", etc.)

**MVP Priority**: High — this dramatically reduces user onboarding friction.

## What to Consider

### Architecture Considerations

#### 1. **Editor Panel Pattern Unification**

You have good patterns (`InspectorPanel<T>`, `DataIndexPanel<T>` mentioned in your AI docs), but:

- **Consider**: Formalizing a `IEditorPanel` interface with lifecycle hooks
- **Consider**: Panel state persistence (save panel layouts)
- **Consider**: Panel communication via event bus (avoid tight coupling)

```cpp
class IEditorPanel {
public:
    virtual void OnSelectionChanged(const Selection& sel) = 0;
    virtual void OnEditorStateChanged(EditorState newState) = 0;
    virtual void Render(float dt) = 0;
    virtual bool IsDirty() const = 0;
};
```

#### 2. **Selection System Design**

**Critical Decision**: How do our users select and manipulate entities?

**User Options**:

- **Bounding Box Selection**: Fast, but imprecise for overlapping geometry
- **Lasso Selection**: Good for irregular groups
- **Layer-based Selection**: Filter by layer (roads/districts/buildings)
- **Query Selection**: "Select all residential lots > 500m²"

**Recommendation**: Implement **hierarchical selection** tools with keyboard modifiers:

- Click: Select entity
- Shift+Click: Add to selection
- Ctrl+Click: Toggle selection (Add to selection or Remove from selection)
- Alt+Drag: Lasso select
- Shift+Alt+Drag: Box select through layers

#### 3. **Gizmo System (Transform Handles)**

**Need**: Visual manipulation widgets for:

- Axiom position/radius adjustment
- Road vertex dragging
- Building rotation/scale
**Recommendation**: Integrate [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo) — it's header-only and works perfectly with ImGui.

#### 4. **Multi-Layer Editing**

Your **layered graph system** for bridges/tunnels requires:

- Layer visibility toggles
- "Active layer" concept (which layer receives new geometry)
- Visual differentiation (different colors/opacity per layer)
- Portal creation tools

**Recommendation**: Add a **Layer Manager Panel** early in Phase 4-5.

#### 5. **Real-Time vs. Modal Editing**

**Current**: Your HFSM uses modal editing (switch to "Axiom mode", "Road mode", etc.)
**Consider Adding**: **Context-sensitive tools** — clicking an entity automatically enters its edit mode:
- Click axiom → axiom editor activates
- Click road segment → road editor activates
- Double-click empty space → axiom placement mode

This reduces cognitive load compared to mode switching.

#### 6. **Validation & Constraints**

**Need**: Real-time feedback for invalid operations:

- Axiom radius overlap detection
- Road intersection validation
- Minimum lot sizes
- Building footprint conflicts

**Recommendation**: Implement **validation layers** that highlight issues:

```cpp
class ValidationSystem {
    std::vector<ValidationError> ValidateAxioms();
    std::vector<ValidationError> ValidateRoadNetwork();
    void RenderErrorGizmos(); // Red outlines, warning icons
};
```

#### 7. **Performance for Large Cities**

**Consider**: Spatial partitioning for:
- Viewport culling (only render visible entities)
- Selection queries (only test nearby objects)
- Update optimization (dirty rectangles)

**Recommendation**: Use your existing grid structures, add **quadtree/octree** for fast spatial queries.

### User Experience Considerations

#### 1. **Discoverability**
- **Tooltip system**: Hover hints for all tools
- **Contextual help**: Right-click menus with descriptions
- **Tutorial overlays**: First-run guided tour

#### 2. **Keyboard Shortcuts**
Define a consistent shortcut scheme:

- `A`: Axiom tool
- `R`: Road tool
- `D`: District tool
- `G`: Grab/move
- `S`: Scale
- `Delete`: Remove selected
- `Ctrl+Z/Y`: Undo/redo
- `Space`: Pan mode

#### 3. **Visual Feedback**
- **Hover highlights**: Glow on mouseover
- **Selection outlines**: Bright borders for selected entities
- **Ghost previews**: Semi-transparent when placing new entities
- **Snap indicators**: Show snap points/grid alignment

#### 4. **Error Recovery**

- **Auto-save**: Every N minutes
- **Crash recovery**: Save temporary files
- **Export checkpoints**: Incremental saves during long operations

### Data Management Considerations

#### 1. **Serialization Strategy**

You mention JSON/OBJ export. Consider:

- **Scene format**: Full project save (axioms + roads + metadata)
- **Incremental saves**: Only save deltas
- **Version compatibility**: Forward/backward compatibility

#### 2. **Asset Management**

As you add building models, textures, etc.:

- **Asset browser panel**: Preview and select assets
- **Hot-reload**: Update without restart
- **External editing**: Watch files for changes

## MVP Editor Priorities (Phase 3-5)

### Must-Have (Phase 3 - Current)

- [x]  Axiom placement editor (`rc_panel_axiom_editor.cpp` exists)
- [x]  Basic viewport interaction (pan, zoom)
- [x]  **Undo/redo for axiom operations** (command history + UI + Ctrl+Z/Ctrl+Y)
- [x]  **Selection visualization** (viewport selection outlines + hover feedback)

### Should-Have (Phase 4-5)

- [x]  **Property inspector** (context-sensitive + property-level undo/redo + batch edit)
- [x]  **Road vertex editing** (basic drag-to-move)
- [ ]  **Preset system** (save/load axiom configurations)
- [x]  **Layer manager** (for multi-level cities)
- [x]  **Validation feedback** (highlight errors)

### Nice-to-Have (Post-MVP)

- [x]  District boundary override editor
- [x]  Curve/spline tools
- [ ]  Timeline scrubbing
- [x]  Query-based selection (Inspector query filters -> multi-select)
- [x]  Custom gizmos for specialized operations

---
Code considerations
## **Math Genius Agent**: Data Structure Optimization

**Viewport Index Design Review**

## Strong Decisions

cpp

```
struct VpProbeData {
  VpEntityKind kind;
  uint32_t id;                 // ✅ 32-bit stable ID
  uint32_t parent;             // ✅ Flat array indexing
  uint32_t first_child;        // ✅ Implicit child span
  uint16_t child_count;        // ✅ Saves 2 bytes vs uint32_t
```

## ⚠️ Cache Line Concerns

Current size: ~88 bytes (assuming `char label[32]` + 4-byte AESP floats)

**Analysis**:

- 64-byte cache line fits **0.72 entries** (suboptimal)
    
- 128-byte cache line fits **1.45 entries** (better, still fractional)
    

**Optimization Suggestions**:

1. **Hot/cold split**:
    
    cpp
    
    ```
    struct VpProbeHot {  // 64 bytes exactly
      VpEntityKind kind;
      uint32_t id, parent, first_child;
      uint16_t child_count;
      uint32_t axiom_id, road_id, district_id, block_id;
      float slope, flood;
      float aesp;  // 16 bytes
      uint8_t frontage_family, afg_preset, road_hierarchy, zone_mask;
      float afg_band;
      // Padding to 64 bytes
    };
    
    struct VpProbeCold {
      char label;
      // Future: detailed metrics, debug data
    };
    ```
    
2. **Separate label storage**:
    
    - Labels only needed on hover/selection
        
    - Store in `std::unordered_map<uint32_t, std::string>`
        
    - Saves 32 bytes per entry (37% size reduction for 10k entities = 312KB savings)
        


## **Resource Manager Agent**: Performance Budget

## Memory Projections

|Settlement Tier|Road Count|Districts|Lots|Buildings|Total Entities|Index Size (88B)|Optimized (56B)|
|---|---|---|---|---|---|---|---|
|Village|150|~10|~50|~100|310|27 KB|17 KB|
|Large City|2000|~80|~1000|~5000|8080|711 KB|452 KB|
|Metropolis|10000|~200|~8000|~50000|68200|6 MB|3.8 MB|

**Verdict**: Even without optimization, memory is not a bottleneck. **Don't optimize prematurely**.

## Performance Mandates

Your **context-aware threading** rule is smart:

cpp

```
// Good threshold-based threading
if (axiom_count * district_count > 100) {
    use_rogue_worker();
} else {
    run_single_threaded();
}
```

**Critical Performance Rule**: The **"10ms rule"** for HFSM state transitions is essential. Any editor operation >10ms **must** go through `RogueWorker` or feel laggy.

**Handoff Question to Debug Manager**: "How do you enforce the 10ms rule? Do you need instrumentation in HFSM transitions?"

---

## **Debug Manager Agent**: Testing Strategy

## Critical Test Gaps

Your plan mentions "transition tests" but lacks specifics. **Must-have tests**:

## 1. Viewport Index Integrity Tests

cpp

```
TEST(ViewportIndex, StableIds) {
    // Generate city twice with same seed
    auto city1 = GenerateCity(seed);
    auto city2 = GenerateCity(seed);
    
    // IDs must match
    ASSERT_EQ(city1.index[0].id, city2.index[0].id);
}

TEST(ViewportIndex, ParentChildConsistency) {
    auto city = GenerateCity();
    for (auto& entity : city.index) {
        if (entity.child_count > 0) {
            // Verify all children point back to parent
            for (uint32_t i = 0; i < entity.child_count; ++i) {
                auto& child = city.index[entity.first_child + i];
                ASSERT_EQ(child.parent, entity.id);
            }
        }
    }
}
```

## 2. Dirty Layer Propagation Tests

cpp

```
TEST(DirtyPropagation, AxiomEdit) {
    // Edit axiom → mark layers dirty
    // Regenerate → verify only downstream layers rebuilt
    // Roads should regenerate, terrain should NOT
}
```

## 3. Undo/Redo Determinism Tests

cpp

```
TEST(CommandHistory, RoundTrip) {
    auto initial = CaptureState();
    cmd->Execute();
    auto modified = CaptureState();
    cmd->Undo();
    auto restored = CaptureState();
    
    ASSERT_EQ(initial, restored);  // Must be bit-identical
}
```

**Handoff Question to Coder Agent**: "Should viewport index validation be a debug-mode-only assert, or always-on consistency check?"
**Handoff Question to Resource Manager**: "What's the expected max entity count per scene? This determines if cache optimization matters."

## Critical UX Gaps

## 1. **Property Editor Without Reflection**

Your `PropertyGrid<T>` needs a concrete implementation. **Recommendation**:

cpp

```
// Manual property registration (verbose but works)
class PropertyEditor {
    void RenderAxiom(AxiomInput& axiom) {
        ImGui::Text("Axiom Properties");
        ImGui::Separator();
        
        if (ImGui::DragFloat("Radius", &axiom.radius, 1.0f, 10.0f, 1000.0f)) {
            // Mark dirty, queue regeneration
        }
        
        const char* types[] = {"Radial", "Delta", "Block", "Grid"};
        if (ImGui::Combo("Type", &axiom.type, types, 4)) {
            // Mark dirty
        }
        
        // ... more properties
    }
    
    void Render(Selection& sel) {
        if (auto* axiom = sel.GetAs<AxiomInput>()) {
            RenderAxiom(*axiom);
        } else if (auto* road = sel.GetAs<Road>()) {
            RenderRoad(*road);
        }
    }
};
```

**Why Manual**: C++ reflection is a rat's nest. Manual grids are:

- ✅ Explicit (you see all properties)
    
- ✅ Typesafe (compiler catches errors)
    
- ✅ Customizable (per-property widgets)
    

## 2. **Missing Motion Design Specs**

Your plan mentions "Ghost previews" and "Glow on hover" but lacks timing:

cpp

```
// Need explicit animation specs
struct UiMotionConfig {
    float hover_glow_fade_in = 0.15f;   // seconds
    float hover_glow_fade_out = 0.25f;
    float selection_outline_pulse_hz = 2.0f;
    float ghost_preview_alpha = 0.4f;
    float snap_indicator_flash_duration = 0.2f;
};
```

**Handoff Question to Architect**: "Should motion timing be user-configurable (accessibility), or fixed design constants?"

# plan adds several features that **should** expose AI hooks:

|Feature|AI Integration Opportunity|
|---|---|
|Undo/Redo|AI can suggest "rewind to before road X was added"|
|Property Editor|AI can recommend property values ("typical arterial width is...")|
|Preset System|AI can generate presets from natural language ("Create a medieval market town preset")|
|Validation System|AI can explain **why** validation failed ("Road too close to axiom - try 50m radius")|

## Critical: Pattern Catalog Updates

Your plan adds new UI panels → **Must update** `AI/docs/ui/ui_patterns.json`:

json

```
{
  "patterns": [
    {
      "name": "PropertyEditor",
      "role": "context_sensitive_inspector",
      "data_bindings": ["selection_manager"],
      "typical_panels": ["Inspector"],
      "refactor_opportunities": [
        "Extract per-type property grids into separate components",
        "Add undo integration at property level"
      ]
    }
  ]
}
```

**Handoff Question to Documentation Keeper**: "Should pattern catalog updates happen inline with code, or as separate PR?"

## Immediate Next Steps

1. **Implement Basic Undo/Redo**: Use command pattern, store axiom placement/deletion as commands
2. **Enhance Inspector Panel**: Make it context-sensitive to selection
3. **Add Selection Highlighting**: Visual feedback for selected entities
4. **Create Preset System**: Save/load axiom configurations as JSON
5. **Document Editor Patterns**: Formalize `IEditorPanel` interface

Your architecture is solid, and your phase-based approach is working well. Focus on **undo/redo** and **selection feedback** before Phase 4 export work — they're foundational for user trust in the tool.
