# V1 Finalization Execution Plan
**Date**: February 8, 2026  
**Target**: V1.0 Complete in 2 Execution Runs  
**Estimated Time**: 33-46 hours (5-7 work days)

---

## ?? Execution Overview

### Current Status (Pre-PASS 1)
? **Completed**:
- Core data contracts (`CityTypes.hpp`, `GlobalState.hpp`)
- Generator pipeline foundation (roads ? districts ? lots ? buildings)
- AI integration layer (4 phases complete)
- UI scaffold with index panels and viewport overlays
- CitySpec pipeline wired through generators

?? **Outstanding Issues**:
- Build warnings (LNK4098 MSVCRT conflict, float narrowing)
- HFSM tool-mode switching fragmented
- Missing features: Water editing, river placement, full district editing
- Export system incomplete

---

## ?? TWO-PASS EXECUTION STRATEGY

### PASS 1: Build Stability + Tool-Generator Wiring
**Duration**: 14-19 hours (2-3 work days)  
**Goal**: Zero build errors, all tools trigger generators, HFSM controls functional

**Tasks**:
1. Build System Hardening (Debug Manager + Coder)
2. HFSM Tool-Mode Unification (Coder + UI/UX Master)
3. Generator Bridge Completion (Coder + Math Genius)
4. UI Panel ? Generator Wiring (UI/UX Master + Coder)
5. Viewport Overlay Completion (UI/UX Master)

### PASS 2: Feature Completion + Export + Polish
**Duration**: 19-27 hours (3-4 work days)  
**Goal**: Full editor capabilities, export system, documentation, cleanup

**Tasks**:
1. Water/River Editing Tools (City Planner + Coder)
2. District/Lot Manual Editing (City Planner + UI/UX Master)
3. Export System (Coder + Documentation Keeper)
4. AI Pattern Catalog Update (AI Integration Agent)
5. Testing & Polish (Debug Manager + Resource Manager)
6. Documentation & _Temp Cleanup (Documentation Keeper)

---

## ?? PASS 1 DETAILED EXECUTION

### Task 1.1: Build System Hardening
**Agent**: Debug Manager + Coder  
**Priority**: CRITICAL  
**Duration**: 2-3 hours

#### Objectives
1. Fix LNK4098 (MSVCRT conflict)
2. Resolve float narrowing warnings
3. Enable /W4 compliance

#### Implementation Steps

##### A. Fix MSVCRT Conflict (Root CMakeLists.txt)
```cmake
# Add after project() declaration (around line 15)
if(MSVC)
    # Force consistent MSVC runtime library
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    
    # Enable high warning level
    add_compile_options(/W4 /WX-)
    
    # Disable specific noisy warnings
    add_compile_options(
        /wd4100  # Unreferenced formal parameter
        /wd4127  # Conditional expression is constant
    )
endif()
```

##### B. Float Narrowing Warnings
**Files to scan**:
- `app/src/Integration/GeneratorBridge.cpp`
- `visualizer/src/ui/viewport/*.cpp`

**Fix pattern**:
```cpp
// BEFORE (warning)
float value = some_double_expression;

// AFTER (no warning)
float value = static_cast<float>(some_double_expression);
```

##### C. Verify Link Dependencies
Check `app/CMakeLists.txt`:
```cmake
# Only link sol2 if Lua is enabled
if(ROGUECITY_HAS_SOL2)
    target_link_libraries(RogueCityApp PRIVATE sol2::sol2)
endif()

# Windows-only WinHTTP
if(WIN32)
    target_link_libraries(RogueCityApp PRIVATE winhttp)
endif()
```

#### Validation
```bash
cmake --build build --config Release 2>&1 | Tee-Object pass1_task1_build.log
# Should output: 0 errors, 0 warnings
```

#### Debug Flags for AI System
Add to all modified files:
```cpp
// AI_INTEGRATION_TAG: V1_PASS1_TASK1_BUILD_HARDENING
// AGENT: Debug_Manager + Coder_Agent
// CATEGORY: Build_System
```

---

### Task 1.2: HFSM Tool-Mode Unification
**Agent**: Coder + UI/UX Master  
**Priority**: CRITICAL  
**Duration**: 3-4 hours

#### Objectives
Wire all 6 editor tools through HFSM with deterministic transitions:
1. Axiom Placement ? `AxiomPlacementState` (verify exists)
2. Road Editing ? `RoadEditingState` (NEW)
3. District Zoning ? `DistrictZoningState` (NEW)
4. Lot Subdivision ? `LotSubdivisionState` (NEW)
5. Building Placement ? `BuildingPlacementState` (NEW)
6. Water/River Editing ? `WaterEditingState` (NEW)

#### Implementation Steps

##### A. Check Existing HFSM Infrastructure
```bash
# Search for existing HFSM files
grep -r "EditorMode" core/include/ app/include/
grep -r "HFSMState" core/src/ app/src/
```

##### B. Create/Extend EditorMode Enum
**File**: `core/include/RogueCity/Core/Editor/EditorState.hpp` (or similar)
```cpp
// AI_INTEGRATION_TAG: V1_PASS1_TASK2_HFSM_MODES
// AGENT: Coder_Agent + UI/UX_Master
// CATEGORY: State_Management

namespace RogueCity::Core::Editor {

enum class EditorMode {
    None,
    AxiomPlacement,
    RoadEditing,       // NEW
    DistrictZoning,    // NEW
    LotSubdivision,    // NEW
    BuildingPlacement, // NEW
    WaterEditing       // NEW
};

} // namespace
```

##### C. Implement State Classes
**File**: `app/src/Editor/EditorStates.cpp` (or create new)
```cpp
// AI_INTEGRATION_TAG: V1_PASS1_TASK2_HFSM_STATES
// AGENT: Coder_Agent
// CATEGORY: State_Implementation

struct RoadEditingState : public EditorState {
    void Enter(GlobalState* state) override {
        // DEBUG_TAG: STATE_ENTER_ROAD_EDITING
        state->ui_flags.show_road_inspector = true;
        state->interaction_mode = InteractionMode::EditRoads;
        
        // Highlight selected road
        if (state->selected_road_id != -1) {
            state->roads[state->selected_road_id].highlighted = true;
        }
    }
    
    void Update(float dt, GlobalState* state) override {
        // DEBUG_TAG: STATE_UPDATE_ROAD_EDITING
        // Handle road manipulation input
    }
    
    void Exit(GlobalState* state) override {
        // DEBUG_TAG: STATE_EXIT_ROAD_EDITING
        state->ui_flags.show_road_inspector = false;
        state->interaction_mode = InteractionMode::None;
        
        // Unhighlight roads
        for (auto& road : state->roads) {
            road.highlighted = false;
        }
    }
};

// Implement similarly:
// - DistrictZoningState
// - LotSubdivisionState
// - BuildingPlacementState
// - WaterEditingState
```

##### D. Wire Tool Panel
**File**: `visualizer/src/ui/panels/rc_panel_tools.cpp`
```cpp
// AI_INTEGRATION_TAG: V1_PASS1_TASK2_TOOL_PANEL_WIRING
// AGENT: UI/UX_Master
// CATEGORY: UI_State_Reactive

void RcPanelTools::Render() {
    ImGui::Begin("Tools", nullptr, ImGuiWindowFlags_NoCollapse);
    
    auto current_mode = editor_state_->GetCurrentMode();
    
    // Tool buttons with state-reactive highlighting
    RenderToolButton("Axiom", EditorMode::AxiomPlacement, current_mode);
    RenderToolButton("Road", EditorMode::RoadEditing, current_mode);
    RenderToolButton("District", EditorMode::DistrictZoning, current_mode);
    RenderToolButton("Lot", EditorMode::LotSubdivision, current_mode);
    RenderToolButton("Building", EditorMode::BuildingPlacement, current_mode);
    RenderToolButton("Water", EditorMode::WaterEditing, current_mode);
    
    ImGui::End();
}

void RcPanelTools::RenderToolButton(
    const char* label, 
    EditorMode mode, 
    EditorMode current_mode) 
{
    // Y2K affordance: glow when active
    if (current_mode == mode) {
        float pulse = 0.5f + 0.5f * sinf(ImGui::GetTime() * 4.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, 
            ImVec4(0.2f, 0.8f * pulse + 0.2f, 0.2f, 1.0f));
    }
    
    if (ImGui::Button(label, ImVec2(-1, 40))) {
        // DEBUG_TAG: TOOL_BUTTON_CLICKED
        editor_state_->TransitionTo(mode);
    }
    
    if (current_mode == mode) {
        ImGui::PopStyleColor();
    }
}
```

#### Validation
- [ ] Each tool button exists and is clickable
- [ ] Clicking button transitions HFSM state
- [ ] Active button highlights with pulse animation
- [ ] Panel visibility changes per state
- [ ] No crashes when switching between modes

#### Debug Logging
Add to state transitions:
```cpp
void EditorState::TransitionTo(EditorMode new_mode) {
    // AI_DEBUG_TAG: HFSM_TRANSITION
    std::cout << "[HFSM] Transition: " 
              << magic_enum::enum_name(current_mode_) 
              << " -> " 
              << magic_enum::enum_name(new_mode) 
              << std::endl;
    
    // Execute transition
    if (current_state_) {
        current_state_->Exit(global_state_);
    }
    
    current_mode_ = new_mode;
    current_state_ = CreateState(new_mode);
    
    if (current_state_) {
        current_state_->Enter(global_state_);
    }
}
```

---

### Task 1.3: Generator Bridge Completion
**Agent**: Coder + Math Genius  
**Priority**: HIGH  
**Duration**: 4-5 hours

#### Current Status
- ? `generateTensorField()` - DONE
- ? `traceRoads()` - DONE
- ? `subdivideDistricts()` - DONE (via ZoningGenerator)
- ?? `generateLots()` - PARTIAL (needs UI connection)
- ? `placeBuildingSites()` - MISSING
- ? `generateWaterbodies()` - MISSING
- ? `traceRivers()` - MISSING

#### Implementation Steps

##### A. Check Existing GeneratorBridge
```bash
# Verify current implementation
cat app/include/RogueCity/App/Integration/GeneratorBridge.hpp | grep "void generate"
cat app/src/Integration/GeneratorBridge.cpp | grep "void GeneratorBridge::"
```

##### B. Add Missing Parameter Structs
**File**: `app/include/RogueCity/App/Integration/GeneratorBridge.hpp`
```cpp
// AI_INTEGRATION_TAG: V1_PASS1_TASK3_GENERATOR_PARAMS
// AGENT: Coder_Agent
// CATEGORY: Generator_Bridge

namespace RogueCity::App::Integration {

// NEW: Lot generation parameters
struct LotGenParams {
    float min_lot_area = 100.0f;
    float max_lot_area = 500.0f;
    bool respect_zoning = true;
};

// NEW: Building placement parameters
struct BuildingPlacementParams {
    float setback_min = 3.0f;
    float height_variance = 0.2f;
    bool use_aesp_height = true;
};

// NEW: Water body parameters
struct WaterGenParams {
    std::vector<glm::vec2> boundary_points;
    float depth = 5.0f;
    bool generate_shore = true;
};

class GeneratorBridge {
public:
    explicit GeneratorBridge(GlobalState* state);
    
    // Existing methods (verify)
    void generateTensorField(const TensorFieldParams& params);
    void traceRoads(const RoadGenParams& params);
    void subdivideDistricts(const DistrictParams& params);
    
    // NEW: Complete pipeline
    void generateLots(const LotGenParams& params);
    void placeBuildingSites(const BuildingPlacementParams& params);
    void generateWaterbodies(const WaterGenParams& params);
    void traceRivers(const std::vector<glm::vec2>& control_points);
    
private:
    GlobalState* state_;
    
    // Generator instances (create if needed)
    std::unique_ptr<Generators::LotGenerator> lot_gen_;
    std::unique_ptr<Generators::SiteGenerator> site_gen_;
    std::unique_ptr<Generators::WaterGenerator> water_gen_;
};

} // namespace
```

##### C. Implement generateLots()
**File**: `app/src/Integration/GeneratorBridge.cpp`
```cpp
// AI_INTEGRATION_TAG: V1_PASS1_TASK3_GENERATE_LOTS
// AGENT: Coder_Agent + Math_Genius
// CATEGORY: Generator_Implementation

void GeneratorBridge::generateLots(const LotGenParams& params) {
    using namespace Generators;
    
    // DEBUG_TAG: LOT_GENERATION_START
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Prepare input from GlobalState
    LotInput input;
    input.districts = state_->districts;
    input.roads = state_->roads;
    input.min_area = params.min_lot_area;
    input.max_area = params.max_lot_area;
    input.respect_zoning = params.respect_zoning;
    
    // Execute generator (use RogueWorker if needed)
    LotOutput output = lot_gen_->Generate(input);
    
    // Populate GlobalState (use FVA for UI stability)
    state_->lots.clear();
    state_->lots.reserve(output.lots.size());
    for (const auto& lot : output.lots) {
        state_->lots.push_back(lot);
    }
    
    // Trigger UI refresh
    state_->NotifyLotsChanged();
    
    // DEBUG_TAG: LOT_GENERATION_COMPLETE
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    std::cout << "[GeneratorBridge] generateLots: " 
              << output.lots.size() << " lots in " 
              << duration.count() << "ms" << std::endl;
}
```

##### D. Implement placeBuildingSites()
```cpp
// AI_INTEGRATION_TAG: V1_PASS1_TASK3_PLACE_BUILDINGS
// AGENT: Coder_Agent
// CATEGORY: Generator_Implementation

void GeneratorBridge::placeBuildingSites(const BuildingPlacementParams& params) {
    using namespace Generators;
    
    // DEBUG_TAG: BUILDING_PLACEMENT_START
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Prepare input
    SiteInput input;
    input.lots = state_->lots;
    input.setback_min = params.setback_min;
    input.height_variance = params.height_variance;
    input.use_aesp_height = params.use_aesp_height;
    
    // Execute generator
    SiteOutput output = site_gen_->Generate(input);
    
    // Populate GlobalState (use SIV for buildings - high churn)
    state_->buildings.clear();
    state_->buildings.reserve(output.sites.size());
    for (const auto& site : output.sites) {
        state_->buildings.push_back(site);
    }
    
    state_->NotifyBuildingsChanged();
    
    // DEBUG_TAG: BUILDING_PLACEMENT_COMPLETE
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    std::cout << "[GeneratorBridge] placeBuildingSites: " 
              << output.sites.size() << " buildings in " 
              << duration.count() << "ms" << std::endl;
}
```

##### E. Implement Water Generators (Stubs for PASS 2)
```cpp
// AI_INTEGRATION_TAG: V1_PASS1_TASK3_WATER_STUBS
// AGENT: Coder_Agent
// CATEGORY: Generator_Stub

void GeneratorBridge::generateWaterbodies(const WaterGenParams& params) {
    // TODO: Full implementation in PASS 2 Task 2.1
    // DEBUG_TAG: WATER_GEN_STUB
    std::cout << "[GeneratorBridge] generateWaterbodies: STUB (PASS 2)" << std::endl;
}

void GeneratorBridge::traceRivers(const std::vector<glm::vec2>& control_points) {
    // TODO: Full implementation in PASS 2 Task 2.1
    // DEBUG_TAG: RIVER_GEN_STUB
    std::cout << "[GeneratorBridge] traceRivers: STUB (PASS 2)" << std::endl;
}
```

#### Validation
- [ ] All 7 bridge methods compile without errors
- [ ] `generateLots()` populates `GlobalState::lots`
- [ ] `placeBuildingSites()` populates `GlobalState::buildings`
- [ ] Notification system triggers UI refresh
- [ ] Debug logs show generation times

---

### Task 1.4: UI Panel ? Generator Wiring
**Agent**: UI/UX Master + Coder  
**Priority**: HIGH  
**Duration**: 3-4 hours

#### Panel Mapping
| Panel | Bridge Method | HFSM State | Status |
|-------|--------------|------------|--------|
| AxiomControlPanel | `generateTensorField()` | AxiomPlacement | Verify |
| RoadControlPanel | `traceRoads()` | RoadEditing | NEW |
| DistrictControlPanel | `subdivideDistricts()` | DistrictZoning | Verify |
| LotControlPanel | `generateLots()` | LotSubdivision | NEW |
| BuildingControlPanel | `placeBuildingSites()` | BuildingPlacement | NEW |
| WaterControlPanel | `generateWaterbodies()` | WaterEditing | STUB (PASS 2) |

#### Implementation Steps

##### A. Create Lot Control Panel
**Files**: 
- `visualizer/src/ui/panels/rc_panel_lot_control.h`
- `visualizer/src/ui/panels/rc_panel_lot_control.cpp`

```cpp
// AI_INTEGRATION_TAG: V1_PASS1_TASK4_LOT_CONTROL_PANEL
// AGENT: UI/UX_Master
// CATEGORY: Control_Panel

#pragma once
#include <RogueCity/App/Integration/GeneratorBridge.hpp>
#include <imgui.h>

class RcPanelLotControl {
public:
    explicit RcPanelLotControl(
        GeneratorBridge* bridge, 
        GlobalState* state,
        EditorState* editor_state);
    
    void Render();
    
private:
    GeneratorBridge* bridge_;
    GlobalState* state_;
    EditorState* editor_state_;
    
    LotGenParams params_;
    bool is_generating_ = false;
    float gen_start_time_ = 0.0f;
};

// Implementation
RcPanelLotControl::RcPanelLotControl(
    GeneratorBridge* bridge, 
    GlobalState* state,
    EditorState* editor_state)
    : bridge_(bridge)
    , state_(state)
    , editor_state_(editor_state)
{
}

void RcPanelLotControl::Render() {
    // Only show if in LotSubdivision mode
    if (editor_state_->GetCurrentMode() != EditorMode::LotSubdivision) {
        return;
    }
    
    ImGui::Begin("Lot Subdivision Control", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    
    // === Parameters ===
    ImGui::SeparatorText("Parameters");
    ImGui::SliderFloat("Min Area (m²)", &params_.min_lot_area, 50.0f, 200.0f);
    ImGui::SliderFloat("Max Area (m²)", &params_.max_lot_area, 200.0f, 1000.0f);
    ImGui::Checkbox("Respect Zoning", &params_.respect_zoning);
    
    ImGui::Spacing();
    
    // === Generation Button (Y2K pulse) ===
    if (is_generating_) {
        float pulse = 0.5f + 0.5f * sinf(ImGui::GetTime() * 4.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, 
            ImVec4(0.2f, pulse, 0.2f, 1.0f));
    }
    
    if (ImGui::Button("Generate Lots", ImVec2(-1, 40))) {
        // DEBUG_TAG: LOT_CONTROL_GENERATE_CLICKED
        bridge_->generateLots(params_);
        is_generating_ = true;
        gen_start_time_ = ImGui::GetTime();
    }
    
    if (is_generating_) {
        ImGui::PopStyleColor();
        if (ImGui::GetTime() - gen_start_time_ > 1.0f) {
            is_generating_ = false;
        }
    }
    
    ImGui::Spacing();
    
    // === Status ===
    ImGui::SeparatorText("Status");
    ImGui::Text("Total Lots: %zu", state_->lots.size());
    
    if (!state_->lots.empty()) {
        // Calculate average area
        float total_area = 0.0f;
        for (const auto& lot : state_->lots) {
            total_area += lot.area;
        }
        float avg_area = total_area / state_->lots.size();
        ImGui::Text("Average Area: %.1f m²", avg_area);
    }
    
    ImGui::End();
}
```

##### B. Create Building Control Panel
Similar pattern to LotControlPanel with:
- Parameters: setback_min, height_variance, use_aesp_height
- Trigger: `bridge_->placeBuildingSites(params_)`
- Status: Building count, average height

##### C. Wire Panels to UI Root
**File**: `visualizer/src/ui/rc_ui_root.cpp`
```cpp
// AI_INTEGRATION_TAG: V1_PASS1_TASK4_PANEL_REGISTRATION
// AGENT: Coder_Agent
// CATEGORY: UI_Integration

void RcUiRoot::InitializePanels() {
    // Existing panels...
    
    // NEW: Lot control panel
    lot_control_panel_ = std::make_unique<RcPanelLotControl>(
        generator_bridge_.get(),
        global_state_.get(),
        editor_state_.get()
    );
    
    // NEW: Building control panel
    building_control_panel_ = std::make_unique<RcPanelBuildingControl>(
        generator_bridge_.get(),
        global_state_.get(),
        editor_state_.get()
    );
}

void RcUiRoot::RenderPanels() {
    // Existing panels...
    
    lot_control_panel_->Render();
    building_control_panel_->Render();
}
```

#### Validation
- [ ] All control panels render when in correct HFSM state
- [ ] Parameter sliders update values
- [ ] Generate buttons trigger bridge methods
- [ ] Status displays show entity counts
- [ ] Y2K pulse animation works on generation

---

### Task 1.5: Viewport Overlay Completion
**Agent**: UI/UX Master  
**Priority**: MEDIUM  
**Duration**: 2-3 hours

#### Missing Overlays
1. **Lot Boundaries** (verify existing)
2. **Building Sites** (footprint + height indicators)
3. **Water Bodies** (stub for PASS 2)

#### Implementation Steps

##### A. Check Existing Viewport Overlay System
```bash
# Find viewport renderer
grep -r "RenderRoads\|RenderDistricts" visualizer/src/ui/viewport/
cat visualizer/src/ui/viewport/rc_viewport_overlays.cpp
```

##### B. Add Building Site Overlay
**File**: `visualizer/src/ui/viewport/rc_viewport_overlays.cpp`
```cpp
// AI_INTEGRATION_TAG: V1_PASS1_TASK5_BUILDING_OVERLAY
// AGENT: UI/UX_Master
// CATEGORY: Viewport_Rendering

void RcViewportOverlays::RenderBuildings(const GlobalState& state) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    for (const auto& building : state.buildings) {
        // Determine color
        ImU32 outline_color = building.highlighted ? 
            IM_COL32(255, 200, 0, 255) :  // Yellow highlight
            IM_COL32(150, 150, 150, 200); // Normal gray
        
        // Render footprint outline
        for (size_t i = 0; i < building.footprint.size(); ++i) {
            glm::vec2 p1 = building.footprint[i];
            glm::vec2 p2 = building.footprint[(i + 1) % building.footprint.size()];
            
            ImVec2 screen_p1 = WorldToScreen(p1);
            ImVec2 screen_p2 = WorldToScreen(p2);
            
            draw_list->AddLine(screen_p1, screen_p2, outline_color, 1.5f);
        }
        
        // Height indicator (vertical line at center)
        if (viewport_config_.show_height_indicators) {
            glm::vec2 center = CalculateCentroid(building.footprint);
            ImVec2 screen_center = WorldToScreen(center);
            ImVec2 screen_top = screen_center;
            screen_top.y -= building.height * viewport_config_.height_scale;
            
            draw_list->AddLine(
                screen_center, 
                screen_top, 
                IM_COL32(255, 100, 100, 200), 
                2.0f
            );
            draw_list->AddCircleFilled(
                screen_top, 
                3.0f, 
                IM_COL32(255, 100, 100, 255)
            );
        }
    }
}
```

##### C. Add Lot Boundary Overlay (if missing)
```cpp
// AI_INTEGRATION_TAG: V1_PASS1_TASK5_LOT_OVERLAY
// AGENT: UI/UX_Master
// CATEGORY: Viewport_Rendering

void RcViewportOverlays::RenderLots(const GlobalState& state) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    for (const auto& lot : state.lots) {
        // Color by zoning type
        ImU32 lot_color = GetLotColorByZoning(lot.zoning_type);
        
        // Render boundary
        for (size_t i = 0; i < lot.boundary.size(); ++i) {
            glm::vec2 p1 = lot.boundary[i];
            glm::vec2 p2 = lot.boundary[(i + 1) % lot.boundary.size()];
            
            ImVec2 screen_p1 = WorldToScreen(p1);
            ImVec2 screen_p2 = WorldToScreen(p2);
            
            draw_list->AddLine(screen_p1, screen_p2, lot_color, 1.0f);
        }
    }
}

ImU32 RcViewportOverlays::GetLotColorByZoning(DistrictType type) const {
    switch (type) {
        case DistrictType::Residential:
            return IM_COL32(100, 200, 100, 200);  // Green
        case DistrictType::Commercial:
            return IM_COL32(200, 100, 100, 200);  // Red
        case DistrictType::Industrial:
            return IM_COL32(150, 150, 50, 200);   // Yellow
        case DistrictType::Park:
            return IM_COL32(50, 150, 50, 200);    // Dark green
        default:
            return IM_COL32(150, 150, 150, 200);  // Gray
    }
}
```

##### D. Wire Overlays to Render Loop
**File**: `visualizer/src/ui/viewport/rc_viewport_renderer.cpp`
```cpp
// AI_INTEGRATION_TAG: V1_PASS1_TASK5_OVERLAY_INTEGRATION
// AGENT: UI/UX_Master
// CATEGORY: Viewport_Rendering

void RcViewportRenderer::RenderOverlays(const GlobalState& state) {
    // Mode-specific overlay rendering
    auto mode = editor_state_->GetCurrentMode();
    
    // Always render foundation layers
    overlays_->RenderRoads(state);
    overlays_->RenderDistricts(state);
    
    // Mode-specific overlays
    switch (mode) {
        case EditorMode::LotSubdivision:
            overlays_->RenderLots(state);
            break;
        case EditorMode::BuildingPlacement:
            overlays_->RenderLots(state);
            overlays_->RenderBuildings(state);
            break;
        case EditorMode::None:
        default:
            // Render all in default view
            overlays_->RenderLots(state);
            overlays_->RenderBuildings(state);
            break;
    }
}
```

#### Validation
- [ ] Lot boundaries render with correct colors
- [ ] Building footprints render with outlines
- [ ] Height indicators show vertical lines
- [ ] Overlays respect HFSM state
- [ ] Performance >30 FPS with 500+ buildings

---

## PASS 1 VALIDATION CHECKLIST

Before proceeding to PASS 2:

### Build System ?
```bash
cmake --build build --config Release 2>&1 | Tee-Object pass1_final_build.log
# Verify: 0 errors, 0 warnings
```

### HFSM Integration ?
- [ ] All 6 tool buttons exist
- [ ] State transitions work
- [ ] Active tool highlights correctly
- [ ] No crashes when switching

### Generator Bridge ?
- [ ] All 7 methods compile
- [ ] `generateLots()` works
- [ ] `placeBuildingSites()` works
- [ ] GlobalState updates correctly

### UI Wiring ?
- [ ] All control panels render
- [ ] Generate buttons work
- [ ] Status displays show counts
- [ ] Y2K animations functional

### Viewport Overlays ?
- [ ] Roads render
- [ ] Districts render
- [ ] Lots render
- [ ] Buildings render
- [ ] Selection highlighting works

### Git Checkpoint
```bash
git add .
git commit -m "PASS 1 Complete: Build stability + HFSM + Generator bridge + UI wiring"
git tag v1.0-pass1-complete
```

---

## ?? PASS 2 DETAILED EXECUTION

### Task 2.1: Water/River Editing Tools
**Agent**: City Planner + Coder  
**Priority**: HIGH  
**Duration**: 4-5 hours

#### Objectives
Implement interactive water feature editing:
1. Lake polygon drawing
2. River spline tracing
3. Shore generation
4. Water control panel

#### Implementation Steps

##### A. Create WaterTool Class
**Files**:
- `app/include/RogueCity/App/Tools/WaterTool.hpp`
- `app/src/Tools/WaterTool.cpp`

```cpp
// AI_INTEGRATION_TAG: V1_PASS2_TASK1_WATER_TOOL
// AGENT: City_Planner + Coder_Agent
// CATEGORY: Interactive_Tool

#pragma once
#include <RogueCity/App/Integration/GeneratorBridge.hpp>
#include <vector>
#include <glm/glm.hpp>

namespace RogueCity::App::Tools {

enum class WaterToolMode {
    Lake,
    River,
    Ocean,
    None
};

class WaterTool {
public:
    explicit WaterTool(GeneratorBridge* bridge, GlobalState* state);
    
    void SetMode(WaterToolMode mode);
    WaterToolMode GetMode() const { return mode_; }
    
    // Mouse events
    void OnMouseDown(glm::vec2 world_pos);
    void OnMouseMove(glm::vec2 world_pos);
    void OnMouseUp();
    
    // Keyboard events
    void OnKeyPress(int key);  // ESC = cancel, Enter = commit
    
    // Commit/cancel
    void CommitWaterbody();
    void CancelEditing();
    
    // Preview rendering
    void RenderPreview(ImDrawList* draw_list);
    
private:
    GeneratorBridge* bridge_;
    GlobalState* state_;
    WaterToolMode mode_ = WaterToolMode::None;
    
    // Editing state
    std::vector<glm::vec2> boundary_points_;
    std::vector<glm::vec2> river_control_points_;
    bool is_editing_ = false;
    float current_depth_ = 5.0f;
    bool generate_shore_ = true;
};

} // namespace
```

##### B. Implement Lake Drawing
```cpp
// AI_INTEGRATION_TAG: V1_PASS2_TASK1_LAKE_DRAWING
// AGENT: Coder_Agent
// CATEGORY: Tool_Implementation

void WaterTool::OnMouseDown(glm::vec2 world_pos) {
    if (mode_ == WaterToolMode::Lake) {
        // DEBUG_TAG: WATER_TOOL_LAKE_CLICK
        
        if (!is_editing_) {
            // Start new lake
            boundary_points_.clear();
            is_editing_ = true;
            std::cout << "[WaterTool] Started lake drawing" << std::endl;
        }
        
        // Add boundary point
        boundary_points_.push_back(world_pos);
        
        // Auto-close if near first point
        if (boundary_points_.size() > 2) {
            float dist = glm::distance(boundary_points_.front(), world_pos);
            if (dist < 10.0f) {  // 10m threshold
                // DEBUG_TAG: WATER_TOOL_LAKE_AUTO_CLOSE
                std::cout << "[WaterTool] Auto-closing lake polygon" << std::endl;
                CommitWaterbody();
            }
        }
    }
}

void WaterTool::CommitWaterbody() {
    if (boundary_points_.size() < 3) {
        std::cerr << "[WaterTool] Invalid polygon (< 3 points)" << std::endl;
        CancelEditing();
        return;
    }
    
    // DEBUG_TAG: WATER_TOOL_COMMIT
    std::cout << "[WaterTool] Committing waterbody with " 
              << boundary_points_.size() << " points" << std::endl;
    
    // Build parameters
    WaterGenParams params;
    params.boundary_points = boundary_points_;
    params.depth = current_depth_;
    params.generate_shore = generate_shore_;
    
    // Send to generator
    bridge_->generateWaterbodies(params);
    
    // Reset state
    boundary_points_.clear();
    is_editing_ = false;
}
```

##### C. Implement River Spline
```cpp
// AI_INTEGRATION_TAG: V1_PASS2_TASK1_RIVER_SPLINE
// AGENT: Coder_Agent
// CATEGORY: Tool_Implementation

void WaterTool::OnMouseDown(glm::vec2 world_pos) {
    if (mode_ == WaterToolMode::River) {
        // DEBUG_TAG: WATER_TOOL_RIVER_CLICK
        
        if (!is_editing_) {
            river_control_points_.clear();
            is_editing_ = true;
            std::cout << "[WaterTool] Started river tracing" << std::endl;
        }
        
        river_control_points_.push_back(world_pos);
        std::cout << "[WaterTool] Added control point " 
                  << river_control_points_.size() << std::endl;
    }
}

void WaterTool::OnKeyPress(int key) {
    if (key == GLFW_KEY_ENTER && is_editing_) {
        if (mode_ == WaterToolMode::River && river_control_points_.size() >= 2) {
            // DEBUG_TAG: WATER_TOOL_RIVER_COMMIT
            std::cout << "[WaterTool] Committing river with " 
                      << river_control_points_.size() << " control points" << std::endl;
            
            bridge_->traceRivers(river_control_points_);
            river_control_points_.clear();
            is_editing_ = false;
        }
    } else if (key == GLFW_KEY_ESCAPE) {
        // DEBUG_TAG: WATER_TOOL_CANCEL
        std::cout << "[WaterTool] Cancelled editing" << std::endl;
        CancelEditing();
    }
}
```

##### D. Create Water Control Panel
```cpp
// AI_INTEGRATION_TAG: V1_PASS2_TASK1_WATER_CONTROL_PANEL
// AGENT: UI/UX_Master
// CATEGORY: Control_Panel

void RcPanelWaterControl::Render() {
    if (editor_state_->GetCurrentMode() != EditorMode::WaterEditing) {
        return;
    }
    
    ImGui::Begin("Water Editing Control");
    
    // Mode selection
    ImGui::SeparatorText("Tool Mode");
    if (ImGui::RadioButton("Lake", water_tool_->GetMode() == WaterToolMode::Lake)) {
        water_tool_->SetMode(WaterToolMode::Lake);
    }
    if (ImGui::RadioButton("River", water_tool_->GetMode() == WaterToolMode::River)) {
        water_tool_->SetMode(WaterToolMode::River);
    }
    if (ImGui::RadioButton("Ocean", water_tool_->GetMode() == WaterToolMode::Ocean)) {
        water_tool_->SetMode(WaterToolMode::Ocean);
    }
    
    ImGui::Spacing();
    
    // Parameters
    ImGui::SeparatorText("Parameters");
    ImGui::SliderFloat("Depth (m)", &depth_, 1.0f, 20.0f);
    ImGui::Checkbox("Generate Shore Detail", &generate_shore_);
    
    // Instructions
    ImGui::Spacing();
    ImGui::SeparatorText("Instructions");
    if (water_tool_->GetMode() == WaterToolMode::Lake) {
        ImGui::TextWrapped("Click to place boundary points. Click near the first point to close the polygon.");
    } else if (water_tool_->GetMode() == WaterToolMode::River) {
        ImGui::TextWrapped("Click to place control points. Press Enter to finish river. ESC to cancel.");
    }
    
    ImGui::End();
}
```

#### Validation
- [ ] Can draw lake boundaries
- [ ] Can place river control points
- [ ] ESC cancels editing
- [ ] Enter commits river
- [ ] Auto-close works for lakes
- [ ] Water renders in viewport

---

### Task 2.2: District/Lot Manual Editing
**Agent**: City Planner + UI/UX Master  
**Priority**: MEDIUM  
**Duration**: 3-4 hours

(Continue with similar detailed implementation for remaining tasks...)

---

## AI SYSTEM TAGGING CONVENTIONS

### File Headers
```cpp
// AI_INTEGRATION_TAG: <PASS>_<TASK>_<COMPONENT>
// AGENT: <Agent_Name>
// CATEGORY: <Category>
// LAST_MODIFIED: <Date>
```

### Debug Logging
```cpp
// DEBUG_TAG: <ACTION>_<CONTEXT>
std::cout << "[Component] Message" << std::endl;
```

### Categories
- Build_System
- State_Management
- Generator_Bridge
- UI_State_Reactive
- Control_Panel
- Viewport_Rendering
- Interactive_Tool
- Export_System
- Testing
- Documentation

---

## TESTING STRATEGY

### Build Verification
```bash
# After each task
cmake --build build --config Release 2>&1 | Tee-Object task_X_Y_build.log
```

### Runtime Testing
```bash
# After PASS 1
./bin/RogueCityApp.exe --test-mode --pass1-validation

# After PASS 2
./bin/RogueCityApp.exe --test-mode --full-validation
```

### Performance Profiling
```bash
# Use Windows Performance Analyzer
./bin/RogueCityApp.exe --benchmark --profile-output pass1_profile.json
```

---

## EXECUTION TIMELINE

### Week 1 (PASS 1)
- **Day 1**: Tasks 1.1-1.2 (Build + HFSM)
- **Day 2**: Task 1.3 (Generator Bridge)
- **Day 3**: Tasks 1.4-1.5 (UI Wiring + Overlays)

### Week 2 (PASS 2)
- **Day 1**: Tasks 2.1-2.2 (Water + Editing)
- **Day 2**: Task 2.3 (Export)
- **Day 3**: Tasks 2.4-2.5 (AI + Testing)
- **Day 4**: Task 2.6 (Docs + Cleanup)

---

## COMPLETION CRITERIA

V1.0 is complete when:
- [ ] Zero build errors/warnings
- [ ] All 6 editor tools functional
- [ ] Full generator pipeline working
- [ ] Export to JSON/OBJ/GLTF works
- [ ] All tests pass
- [ ] Documentation updated
- [ ] _Temp/ cleaned up

**Target Date**: February 15, 2026
