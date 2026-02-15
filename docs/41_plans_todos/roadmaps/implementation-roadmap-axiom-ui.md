# Implementation Roadmap: Axiom UI Integration

## Status: Architecture Complete, Implementation Ready

This document provides the implementation roadmap for the Axiom placement system, dual-viewport architecture, and dock manager integration following the **Cockpit Doctrine**.

---

## Completed (Architecture Phase)

✅ **Design Documentation**
- Comprehensive architecture in `docs/UI_Architecture_Axiom_System.md`
- Cockpit Doctrine principles defined and integrated into agent roles
- UI/UX/ImGui/ImVue Master agent created with full mandate

✅ **Header Structure**
- All header files created with full API surface definitions
- Directory structure established (`app/include/RogueCity/App/`)
- Layer separation enforced (Core, Generators, App)

✅ **HFSM Extension**
- `EditorState::Editing_Axioms` added
- `EditorEvent::Tool_Axioms`, `AxiomPlaced`, `AxiomModified`, `AxiomDeleted` added
- State machine ready for axiom workflow integration

---

## Next Steps: Implementation Priority

### Phase 1: Viewport Foundation (Week 1)
**Goal:** Get dual viewports rendering with camera sync.

#### Tasks:
1. **ViewportManager Implementation** (`app/src/Viewports/ViewportManager.cpp`)
   - Initialize both viewports
   - Handle ImGui docking integration
   - Update/render loop orchestration

2. **PrimaryViewport Implementation** (`app/src/Viewports/PrimaryViewport.cpp`)
   - OpenGL render target setup
   - Camera controls (orbit, pan, zoom)
   - Screen-to-world coordinate conversion
   - Basic grid overlay rendering

3. **MinimapViewport Implementation** (`app/src/Viewports/MinimapViewport.cpp`)
   - 512x512 render texture
   - Simplified 2D orthographic rendering
   - Fixed ImGui child window integration

4. **ViewportSyncManager Implementation** (`app/src/Viewports/ViewportSyncManager.cpp`)
   - XY position sync with lerp smoothing
   - Sync toggle UI button (chain icon)
   - Frame-by-frame camera updates

**Acceptance Criteria:**
- Both viewports render simultaneously
- Camera sync toggle works correctly
- Minimap follows primary viewport XY
- No performance degradation (<1ms overhead)

---

### Phase 2: Axiom Placement Core (Week 2)
**Goal:** Place axioms with mouse, see static rings.

#### Tasks:
1. **AxiomVisual Implementation** (`app/src/Tools/AxiomVisual.cpp`)
   - Ring rendering (3 concentric circles)
   - Color-coded by axiom type
   - Hover detection logic
   - to_axiom_input() conversion

2. **AxiomPlacementTool Implementation** (`app/src/Tools/AxiomPlacementTool.cpp`)
   - Mouse event handling (down, up, move)
   - Click-to-place workflow
   - Drag-to-size ghost preview
   - Right-click to delete

3. **GeneratorBridge Implementation** (`app/src/Integration/GeneratorBridge.cpp`)
   - AxiomVisual → AxiomInput conversion
   - Validation logic
   - Decay computation from ring distribution

4. **HFSM State Handlers** (extend `core/src/Editor/EditorState.cpp`)
   - on_enter(Editing_Axioms): Activate tool, show panels
   - on_exit(Editing_Axioms): Save axioms, hide panels
   - handle_event(Tool_Axioms): Transition to Editing_Axioms

**Acceptance Criteria:**
- Click to place axiom in viewport
- Axioms render as static rings
- Axioms convert correctly to generator inputs
- HFSM transitions to Editing_Axioms state

---

### Phase 3: Ring Control Knobs (Week 3)
**Goal:** Drag knobs to adjust ring radius, double-click for numeric entry.

#### Tasks:
1. **RingControlKnob Implementation** (in `AxiomVisual.cpp`)
   - Y2K capsule rendering (rounded rect with label)
   - Hover detection
   - Drag behavior (adjust ring radius)
   - Position on ring at fixed angle

2. **ContextWindowPopup Implementation** (`app/src/Tools/ContextWindowPopup.cpp`)
   - ImGui popup window with Y2K styling
   - Numeric input field with validation
   - Smooth value interpolation on apply
   - ESC to cancel, Enter to confirm

3. **Double-Click Detection** (in `AxiomPlacementTool.cpp`)
   - Track last_click_time per knob
   - Trigger ContextWindowPopup on double-click
   - Wire popup callbacks to knob value updates

**Acceptance Criteria:**
- Knobs render at ring perimeter
- Drag knob adjusts ring radius in real-time
- Double-click opens context window
- Numeric entry applies with smooth interpolation

---

### Phase 4: Animation & Affordance (Week 4)
**Goal:** Ring expansion animation, glow effects, motion-based teaching.

#### Tasks:
1. **AxiomAnimationController Implementation** (`app/src/Tools/AxiomAnimationController.cpp`)
   - Ease-out cubic interpolation
   - Ring expansion sequence (0.8s duration)
   - Glow pulse effect (2x during expansion)
   - Knob fade-in after expansion complete

2. **Animation Toggle** (add to settings panel)
   - Options → Animations checkbox
   - Propagate to all AxiomVisual instances
   - Static rings when disabled

3. **State-Reactive Panel Colors** (in DockLayoutManager)
   - Panel border color shifts based on EditorState
   - Idle: Blue, Editing_Axioms: Green, Simulating: Orange
   - Soft glow on relevant panels (Axiom Inspector when axiom selected)

**Acceptance Criteria:**
- Rings expand smoothly on placement
- Animation can be toggled off
- Panels glow when contextually relevant
- No animation causes stutter (<16ms frame time)

---

### Phase 5: Docking & State Persistence (Week 5)
**Goal:** Dock manager saves/loads layouts per EditorState.

#### Tasks:
1. **DockLayoutManager Implementation** (`app/src/Docking/DockLayoutManager.cpp`)
   - Save/load `.ini` files per EditorState
   - apply_layout() with ImGui docking API
   - transition_to_state() with smooth blend (0.5s)

2. **PanelRegistry Implementation** (`app/src/Docking/PanelRegistry.cpp`)
   - Register panels with render callbacks
   - Optimization flags (skip rendering when hidden)
   - Content preservation when panels hidden

3. **Per-State Layouts**
   - Define default layouts for Idle, Editing_Axioms, Editing_Roads
   - Axiom Inspector + Properties visible in Editing_Axioms
   - Minimap always visible (configurable)

**Acceptance Criteria:**
- Layouts persist across sessions
- State transitions morph panel layouts smoothly
- Hidden panels skip rendering (>5ms saved when optimized)
- Panels retain content when restored

---

### Phase 6: Real-Time Preview (Week 6)
**Goal:** Generate city in background, update viewport on axiom changes.

#### Tasks:
1. **RealTimePreview Implementation** (`app/src/Integration/RealTimePreview.cpp`)
   - Debounce logic (0.5s delay after axiom modification)
   - RogueWorker integration for tensor field + road tracing
   - Progress indicator (pulse animation)
   - Callback on generation complete

2. **Viewport Rendering Integration**
   - Render roads from CityOutput in PrimaryViewport
   - Simplified road rendering in MinimapViewport
   - Highlight selected axiom's influence zones

3. **Optimization**
   - Partial regeneration (tensor field + roads only, skip districts)
   - Spatial hash for road rendering (viewport frustum culling)

**Acceptance Criteria:**
- Axiom changes trigger regeneration after 0.5s
- Generation runs in background (no UI freeze)
- Roads appear in both viewports
- <100ms frame time during generation

---

### Phase 7: Options & Polish (Week 7)
**Goal:** Settings panel, view menu, final UX polish.

#### Tasks:
1. **Options Panel**
   - Animations toggle
   - Animation speed slider
   - Reduce motion (accessibility)
   - Minimap size selection

2. **View Menu**
   - Show/hide minimap
   - Sync/unsync cameras
   - Grid overlay options
   - Axiom ring visibility modes (Always, On Hover, Selected Only)

3. **Y2K Visual Polish**
   - Fixed-width font for indices (DistrictIndex/RoadIndex)
   - "MODE: AXIOM-EDIT" label in top-left chrome
   - Warning-stripe borders on context windows
   - Segmented minimap frame with corner caps

**Acceptance Criteria:**
- All options functional and persist
- Y2K design language consistent across UI
- Accessibility options work correctly

---

### Phase 8: Testing & Validation (Week 8)
**Goal:** Comprehensive test coverage and bug fixes.

#### Tasks:
1. **Unit Tests**
   - `test_viewport_sync.cpp`
   - `test_axiom_placement.cpp`
   - `test_ring_control.cpp`
   - `test_dock_manager.cpp`

2. **Integration Tests**
   - `test_generator_bridge.cpp`
   - `test_realtime_preview.cpp`

3. **Visual Tests**
   - `test_ring_animation.cpp`
   - `test_affordance_patterns.cpp`

4. **Performance Profiling**
   - Frame time analysis during generation
   - Memory usage validation (<5MB overhead)
   - Docking transition timing

**Acceptance Criteria:**
- All tests pass
- No memory leaks
- Frame time <16ms (60 FPS)
- Total UI overhead <5MB

---

## Development Guidelines

### Code Style
- Follow existing RogueCity C++20 patterns
- Use `Core::Vec2` instead of `ImVec2` for world coordinates
- Enforce Rogue Protocol (FVA for UI elements, RogueWorker for >10ms tasks)

### Commit Messages
- Prefix with component: `[Viewport]`, `[Axiom]`, `[Dock]`, `[Test]`
- Reference phase number: `[Viewport] Phase 1: Implement camera sync (#2.4)`

### Code Review Checklist
- [ ] Core layer has no UI dependencies (grep for `imgui.h` in `core/`)
- [ ] FVA used for AxiomVisual (stable handles for UI inspector)
- [ ] Animations use ease functions (no linear lerp)
- [ ] State-reactive colors implemented
- [ ] Performance validated (<16ms frame time)

---

## Dependencies & Integration Points

### External Libraries
- **ImGui (docking branch):** Already integrated via `3rdparty/imgui`
- **ImDesignManager:** Available in `3rdparty/ImDesignManager/` for ShapeItem animation system
- **GLM:** Math library for transformations
- **GLFW:** Window/input handling

### Internal Dependencies
- **RogueCity::Core:** Types, Vec2, HFSM
- **RogueCity::Generators:** CityGenerator, AxiomInput, TensorFieldGenerator
- **RogueCity::Core::Editor:** EditorState, EditorEvent, GlobalState

### Build System
- Update `app/CMakeLists.txt` with new source files
- Link against `RogueCityCore`, `RogueCityGenerators`
- Add ImGui docking flags if not already set

---

## Risk Mitigation

### Performance Risks
- **Risk:** Real-time generation causes frame drops
- **Mitigation:** Debouncing + RogueWorker offload, partial regeneration

### UX Risks
- **Risk:** Users find ring expansion distracting
- **Mitigation:** Animation toggle in settings, default ON but easy to disable

### Technical Risks
- **Risk:** ImGui docking API unstable across platforms
- **Mitigation:** Test on Windows/Linux early, fallback to manual layout if needed

---

## Success Metrics

### User Experience
- Users place 10+ axioms in first session (tool is intuitive)
- <2 seconds to understand ring influence (visual clarity)
- Animation perceived as "helpful" not "annoying" (UX survey)

### Performance
- 60 FPS maintained with 20 axioms + 500 roads
- <100ms generation time for tensor field + roads (partial regeneration)
- <5MB memory overhead for UI system

### Code Quality
- 100% test coverage for critical paths (placement, sync, docking)
- Zero Core → UI dependency violations
- All Cockpit Doctrine principles enforced

---

## Questions for Next Review

1. **RogueWorker integration:** Is threading library already available in `core/`?
2. **ImDesignManager usage:** Should we extend ShapeItem or create parallel system?
3. **Render targets:** Use FBO directly or ImGui's native render texture support?
4. **Docking persistence:** Store .ini per user or per project?

---

## Contact

- **Architect Agent:** Overall orchestration, design review
- **Coder Agent:** C++ implementation, build system
- **UI/UX Master:** Cockpit Doctrine enforcement, affordance patterns
- **Debug Manager:** Testing strategy, performance profiling

---

## Timeline Summary

| Phase | Duration | Deliverable |
|-------|----------|-------------|
| Phase 1 | Week 1 | Dual viewports with camera sync |
| Phase 2 | Week 2 | Axiom placement with static rings |
| Phase 3 | Week 3 | Ring control knobs + context windows |
| Phase 4 | Week 4 | Animation & affordance patterns |
| Phase 5 | Week 5 | Docking & state persistence |
| Phase 6 | Week 6 | Real-time preview with background generation |
| Phase 7 | Week 7 | Options panel & Y2K polish |
| Phase 8 | Week 8 | Testing & validation |

**Total Estimated Time:** 8 weeks (assuming 1 developer, part-time)

---

## Next Immediate Action

**Start Phase 1, Task 1: Implement ViewportManager.cpp**

```bash
# Create implementation file
touch app/src/Viewports/ViewportManager.cpp

# Update CMakeLists.txt
# Add: app/src/Viewports/ViewportManager.cpp

# Implement basic structure:
# - Constructor/destructor
# - initialize() with GLFW window context
# - update() stub
# - render() stub with ImGui::DockSpaceOverViewport()
```
