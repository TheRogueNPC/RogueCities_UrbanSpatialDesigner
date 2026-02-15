# UI Architecture: Axiom Placement & Dual-Viewport System

## Overview
This document defines the architecture for integrating the RogueCity generators with the UI/UX system, implementing the **Cockpit Doctrine** for state-reactive, affordance-rich control surfaces.

## Core Components

### 1. Dual-Viewport System

#### 1.1 Viewport Architecture
```
ViewportManager
├── PrimaryViewport (3D/2D hybrid)
│   ├── Camera3D (orbital, pan, zoom)
│   ├── Renderer (OpenGL/ImGui integration)
│   └── ViewportOverlay (grid, axiom rings, selection)
└── MinimapViewport (2D top-down)
    ├── Camera2D (orthographic, synced XY)
    ├── Renderer (simplified, fast)
    └── MinimapOverlay (player marker, axiom indicators)
```

#### 1.2 Camera Synchronization
- **Sync Mode (default):** Minimap camera XY tracks primary camera XY
- **Unsync Mode:** Independent navigation
- **Sync Control:** Toggle button in toolbar with visual indicator (chain icon)
- **Implementation:** `ViewportSyncManager` updates minimap camera each frame when synced

#### 1.3 Render Targets
- Primary: Full ImGui viewport with docking support
- Minimap: Dedicated ImGui child window with fixed render texture
- Both viewports share the same `CityGenerator::CityOutput` data

---

### 2. Axiom Placement System

#### 2.1 Axiom Visual Representation
Use ImDesignManager's **reactive object** (Shape with animation system) as Axiom visualization:

```
AxiomVisual (ShapeItem in ImDesignManager)
├── Core Circle (solid fill, represents axiom center)
├── Influence Rings (3 rings, expanding animation)
│   ├── Ring 1: Immediate influence (radius * 0.33)
│   ├── Ring 2: Medium influence (radius * 0.67)
│   └── Ring 3: Far influence (radius * 1.0)
├── Control Knobs (per ring, draggable)
│   ├── Knob visual: Y2K capsule with label (RING-1, RING-2, RING-3)
│   ├── Drag behavior: Adjusts ring radius
│   └── Double-click: Context window popup with numeric entry
└── Type Indicator (color-coded by AxiomInput::Type)
    ├── Radial: Blue gradient
    ├── Grid: Green with grid lines
    ├── Delta: Red with directional arrow
    └── GridCorrective: Orange with correction icon
```

#### 2.2 Ring Expansion Animation (Affordance)
**On axiom placement:**
1. Core appears instantly
2. Rings expand outward with ease-out curve (0.8s duration)
3. Glow pulses 2x during expansion (teaches "this is active")
4. Control knobs fade in after ring expansion completes
5. **Animation toggle:** Settings panel option disables expansion, shows static rings

**Implementation:** `AxiomAnimationController` drives ShapeItem keyframe animations

#### 2.3 Mouse Interaction
```
AxiomPlacementTool (HFSM state: Editing_Axioms)
├── Click: Place axiom at mouse position
├── Drag (initial): Set axiom radius (ghost preview ring)
├── Hover: Highlight existing axiom + control knobs
├── Drag (knob): Adjust ring radius in real-time
├── Double-click (knob): Open ContextWindow for numeric entry
└── Right-click: Delete axiom (with confirmation)
```

#### 2.4 Control Knob System
```cpp
struct RingControlKnob {
    int ring_index;           // 0, 1, 2
    Vec2 screen_position;     // Computed from ring radius + angle
    float angle;              // Position on ring (0-360°)
    bool is_hovered;
    bool is_dragging;
    float value;              // Current ring radius multiplier
};
```

**Double-click Context Window:**
- Popup at knob position
- Numeric input field (ImGui::InputFloat)
- Validation: clamp to [min_radius, max_radius]
- Smooth interpolation on apply (0.3s ease-in-out)
- ESC to cancel, Enter to apply

---

### 3. HFSM Integration: Dock Manager

#### 3.1 Editor States (Extended)
```
EditorHFSM (extended states)
├── Startup
├── NoProject
├── ProjectLoading
├── Idle
├── Editing_Axioms ← NEW STATE
│   ├── Tool: AxiomPlacementTool active
│   ├── Docks: Axiom Inspector, Properties Panel
│   └── Viewport: Rings visible, knobs interactive
├── Editing_Roads
├── Simulating
├── Simulation_Paused
└── Simulation_Stepping
```

#### 3.2 Dock Manager State Tracking
```cpp
struct DockLayoutState {
    EditorState state;
    std::vector<std::string> visible_panels;
    std::map<std::string, ImGuiID> panel_docks;  // Panel -> DockSpace ID
    bool is_optimized;  // Panels hidden for performance
};
```

**Dock Manager Behavior:**
- **On state enter:** Restore panel layout for that state
- **On state exit:** Save current layout, optionally hide/optimize panels
- **Context preservation:** Panels retain content when hidden
- **Optimization:** Panels marked `is_optimized` skip rendering when not visible

#### 3.3 Docking Implementation
Use ImGui's native docking:
```cpp
ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
ImGui::Begin("Primary Viewport", nullptr, ImGuiWindowFlags_NoScrollbar);
ImGui::Begin("Minimap", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
ImGui::Begin("Axiom Inspector", nullptr, ImGuiWindowFlags_None);
```

**Persistent Docking:**
- Save/load `.ini` file per EditorState
- `DockLayoutManager` handles state-specific layouts
- Smooth transitions between layouts (0.5s blend)

---

### 4. Generator Integration

#### 4.1 Data Flow
```
User Interaction → AxiomPlacementTool → CityGenerator::AxiomInput
                                      ↓
                              CityGenerator::generate()
                                      ↓
                              CityOutput (roads, districts, lots)
                                      ↓
                              ViewportRenderer → Display
```

#### 4.2 Real-time Preview
- **Lazy regeneration:** Only regenerate on axiom change + 0.5s debounce
- **Partial updates:** Regenerate tensor field → roads only (skip districts/lots for speed)
- **Background generation:** Use `RogueWorker` for >10ms operations
- **Progress indicator:** Pulse animation on generating state

#### 4.3 Axiom → Generator Mapping
```cpp
CityGenerator::AxiomInput FromUIAxiom(const AxiomVisual& visual) {
    return {
        .type = visual.type,
        .position = ScreenToWorld(visual.position),
        .radius = visual.rings[2].radius,  // Outermost ring
        .theta = visual.rotation,
        .decay = ComputeDecayFromRings(visual.rings)
    };
}
```

---

### 5. Options & View Menu

#### 5.1 Animation Settings
```
Options → Animations
├── [x] Enable Axiom Ring Expansion (default: ON)
├── [x] Enable Control Knob Glow (default: ON)
├── [x] Enable HFSM State Transitions (default: ON)
├── Animation Speed: [========] 1.0x
└── Reduce Motion (accessibility)
```

#### 5.2 Viewport Settings
```
View → Viewports
├── [x] Show Minimap (default: ON)
├── [x] Sync Cameras (default: ON)
├── Minimap Size: [Small | Medium | Large]
├── Grid Overlay: [None | Major | All]
└── Axiom Rings: [Always | On Hover | Selected Only]
```

---

### 6. Implementation Files

#### 6.1 New Headers (app/include/RogueCity/App/)
```
Viewports/
├── ViewportManager.hpp         # Dual-viewport orchestration
├── PrimaryViewport.hpp         # 3D/2D main view
├── MinimapViewport.hpp         # 2D minimap
└── ViewportSyncManager.hpp     # Camera sync logic

Tools/
├── AxiomPlacementTool.hpp      # Mouse interaction, placement
├── AxiomVisual.hpp             # ImDesignManager integration
├── RingControlKnob.hpp         # Ring adjustment UI
└── ContextWindowPopup.hpp      # Double-click numeric entry

Docking/
├── DockLayoutManager.hpp       # State-specific layouts
├── DockLayoutState.hpp         # Layout persistence
└── PanelRegistry.hpp           # Panel lifecycle management

Integration/
├── GeneratorBridge.hpp         # UI → Generator adapter
└── RealTimePreview.hpp         # Debounced regeneration
```

#### 6.2 HFSM Extension (core/include/RogueCity/Core/Editor/)
```cpp
// Add to EditorState.hpp
enum class EditorState {
    // ... existing states ...
    Editing_Axioms,      // NEW
    // ...
};

// Add to EditorEvent.hpp
enum class EditorEvent {
    // ... existing events ...
    Tool_Axioms,         // NEW: Switch to axiom tool
    AxiomPlaced,         // NEW: User placed axiom
    AxiomModified,       // NEW: Ring adjusted
    // ...
};
```

---

### 7. Y2K/Cockpit Design Elements

#### 7.1 Visual Language
- **Control Knobs:** Rounded capsules with segment labels (RING-1, MOD-X3, etc.)
- **Context Windows:** Hard-edged panels with warning-stripe borders
- **Minimap Frame:** Segmented border with Y2K corner caps
- **Sync Indicator:** Chain icon with glow when active
- **State Label:** "MODE: AXIOM-EDIT" in fixed-width font, top-left chrome

#### 7.2 Affordance Patterns
- **Wiggle on first launch:** Axiom tool button wiggles 2x when entering Idle state
- **Glow on relevance:** Axiom Inspector panel glows softly when axiom selected
- **Color shift by state:** Panel borders shift color based on EditorState
  - Idle: Blue
  - Editing_Axioms: Green
  - Simulating: Orange
  - Error: Red pulse
- **Pulse for progress:** Generation progress bar pulses instead of static fill
- **Index highlights:** When axiom selected, its DistrictIndex/RoadIndex highlights in data panel

---

### 8. Performance Considerations

#### 8.1 Optimization Targets
- **Ring rendering:** Use ImGui AddPolyline with < 64 segments
- **Animation updates:** Only update visible axioms (viewport frustum cull)
- **Debouncing:** 500ms delay on axiom modification before regeneration
- **Background work:** Tensor field + road tracing in `RogueWorker` thread
- **Dock optimization:** Hidden panels skip ImGui::Begin/End logic entirely

#### 8.2 Memory Budget
- **Axiom visuals:** ~1KB per axiom (ShapeItem overhead)
- **Viewport textures:** Primary (configurable), Minimap (512x512 fixed)
- **Dock state cache:** ~10KB per EditorState layout
- **Total expected:** <5MB additional UI overhead

---

## Implementation Priority

1. **Phase 1 (Foundations):**
   - Dual-viewport system with basic rendering
   - Camera synchronization toggle
   - HFSM Editing_Axioms state

2. **Phase 2 (Axiom Tool):**
   - AxiomPlacementTool with mouse interaction
   - Axiom visual with static rings (no animation yet)
   - Generator bridge for AxiomInput → generate()

3. **Phase 3 (Interactivity):**
   - Ring control knobs with drag
   - Context window popup on double-click
   - Real-time preview with debouncing

4. **Phase 4 (Affordance):**
   - Ring expansion animation
   - State-reactive panel colors/glow
   - Motion settings toggle

5. **Phase 5 (Docking):**
   - Dock manager with state persistence
   - Panel optimization lifecycle
   - Layout save/load

---

## Testing Strategy

### Unit Tests
- `test_viewport_sync.cpp`: Camera XY synchronization, sync/unsync toggle
- `test_axiom_placement.cpp`: Mouse to world coords, axiom creation
- `test_ring_control.cpp`: Knob drag, radius clamping, context window
- `test_dock_manager.cpp`: State persistence, panel lifecycle

### Integration Tests
- `test_generator_bridge.cpp`: AxiomVisual → AxiomInput → CityOutput
- `test_realtime_preview.cpp`: Debouncing, background generation

### Visual Tests
- `test_ring_animation.cpp`: Expansion timing, ease curves
- `test_affordance_patterns.cpp`: Glow, pulse, color shifts

---

## References
- [TheRogueCityDesignerSoft.md](TheRogueCityDesignerSoft.md) - Axiom semantics, tensor fields
- [test_editor_hfsm.cpp](../tests/test_editor_hfsm.cpp) - HFSM usage examples
- [ImDesignManager](../3rdparty/ImDesignManager/) - ShapeItem animation system
- [AGENTS.md](../.github/AGENTS.md) - UI/UX/ImGui/ImVue Master mandate
