# 🏗️ V1 Completion Plan: Phases 3-5 Integration

## Current Status Analysis

### ✅ **Completed (Phases 1-4)**

- Core data contracts (`CityTypes.hpp`, `GlobalState.hpp`)
- Generator pipeline (roads → districts → lots → buildings)
- AI integration layer (4 phases complete)
- UI scaffold with index panels and viewport overlays
- CitySpec pipeline wired through generators


### ⚠️ **Outstanding Issues**

- **Build Warnings**: LNK4098 (MSVCRT conflict), float narrowing
- **Tool Wiring**: HFSM tool-mode switching fragmented
- **Missing Features**: Water editing, river placement, full district editing
- **Export System**: Phase 4 incomplete

---

## 🎯 Two-Pass Execution Plan

### **PASS 1: Build Stability + Tool-Generator Wiring** (Critical Path)

**Goal**: Zero build errors, all tools trigger generators, HFSM controls functional

### **PASS 2: Feature Completion + Export + Cleanup** (V1 Features)

**Goal**: Full editor capabilities, export system, _Temp deletion gate

---

# 📋 PASS 1 EXECUTION PLAN

## **Task 1.1: Build System Hardening** (Debug Manager + Coder)

**Priority**: CRITICAL
**Estimated Time**: 2-3 hours

### Objectives

1. **Fix MSVCRT Conflict** (LNK4098)
    - Audit CMakeLists for mixed runtime linkage
    - Ensure consistent `/MD` or `/MT` across all targets
    - Verify vcpkg triplet alignment
2. **Resolve Float Narrowing Warnings**
    - Add explicit casts in `GeneratorBridge.cpp`, viewport files
    - Enable `/W4` compliance across app layer
3. **Verify Link Dependencies**
    - Ensure `sol2::sol2` links only when Lua enabled
    - Validate WinHTTP linkage in AI tools

### Deliverables

```cpp
// CMakeLists.txt
if(MSVC)
  add_compile_options(/W4 /WX-)  # Warnings as errors (except specific)
  # Force consistent runtime
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()
```


### Success Criteria

- ✅ `cmake --build build --config Release` → **0 errors, 0 warnings**
- ✅ All targets link successfully

---

## **Task 1.2: HFSM Tool-Mode Unification** (Coder + UI/UX Master)

**Priority**: CRITICAL
**Estimated Time**: 3-4 hours

### Objectives

Wire **all editor tools** through HFSM states with deterministic transitions:

#### Current Tools (from manifest)

1. **Axiom Placement** → `AxiomPlacementState`
2. **Road Editing** → `RoadEditingState`
3. **District Zoning** → `DistrictZoningState`
4. **Lot Subdivision** → `LotSubdivisionState`
5. **Building Placement** → `BuildingPlacementState`
6. **Water/River Editing** → `WaterEditingState` (NEW)

### Implementation Pattern

```cpp
// app/include/RogueCity/App/HFSM/HFSM.hpp
enum class EditorMode {
    AxiomPlacement,
    RoadEditing,      // NEW
    DistrictZoning,
    LotSubdivision,
    BuildingPlacement,
    WaterEditing      // NEW
};

// app/src/HFSM/HFSMStates.cpp
struct RoadEditingState : public HFSMState {
    void enter() override {
        // Show road inspector panel
        // Enable road vertex editing
        // Highlight selected road in viewport
    }
    
    void update(float dt) override {
        // Handle road manipulation input
        // Update road geometry in GlobalState
    }
    
    void exit() override {
        // Commit road changes to generator
        // Hide inspector panel
    }
};
```


### Tool Panel Wiring (visualizer layer)

```cpp
// visualizer/src/ui/panels/rc_panel_tools.cpp
void RcPanelTools::Render() {
    if (ImGui::Button("Axiom Tool")) {
        hfsm->TransitionTo(EditorMode::AxiomPlacement);
    }
    if (ImGui::Button("Road Tool")) {
        hfsm->TransitionTo(EditorMode::RoadEditing);
    }
    // ... etc for all tools
    
    // State-reactive highlight
    if (hfsm->current() == EditorMode::RoadEditing) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
    }
}
```


### Success Criteria

- ✅ Each tool button triggers HFSM state transition
- ✅ Panel visibility/highlighting responds to current state
- ✅ Viewport cursor/overlays change per mode
- ✅ No stale state when switching tools

---

## **Task 1.3: Generator Bridge Completion** (Coder + Math Genius)

**Priority**: HIGH
**Estimated Time**: 4-5 hours

### Objectives

Complete `GeneratorBridge` methods for **all pipeline stages**:

#### Current Bridges (from manifest)

1. ✅ `generateTensorField()` - DONE
2. ✅ `traceRoads()` - DONE
3. ✅ `subdivideDistricts()` - DONE (via ZoningGenerator)
4. ⚠️ `generateLots()` - PARTIAL (needs UI connection)
5. ❌ `placeBuildingSites()` - MISSING
6. ❌ `generateWaterbodies()` - MISSING
7. ❌ `traceRivers()` - MISSING

### Implementation Template

```cpp
// app/include/RogueCity/App/Integration/GeneratorBridge.hpp
namespace RogueCity::App::Integration {

struct LotGenParams {
    float min_lot_area = 100.0f;
    float max_lot_area = 500.0f;
    bool respect_zoning = true;
};

struct BuildingPlacementParams {
    float setback_min = 3.0f;
    float height_variance = 0.2f;
    bool use_aesp_height = true;
};

struct WaterGenParams {
    std::vector<glm::vec2> boundary_points;
    float depth = 5.0f;
    bool generate_shore = true;
};

class GeneratorBridge {
public:
    // Existing...
    void generateTensorField(const TensorFieldParams& params);
    void traceRoads(const RoadGenParams& params);
    void subdivideDistricts(const DistrictParams& params);
    
    // NEW: Complete pipeline
    void generateLots(const LotGenParams& params);
    void placeBuildingSites(const BuildingPlacementParams& params);
    void generateWaterbodies(const WaterGenParams& params);
    void traceRivers(const std::vector<glm::vec2>& controlPoints);
    
private:
    GlobalState* state_;
    std::unique_ptr<Generators::LotGenerator> lot_gen_;
    std::unique_ptr<Generators::SiteGenerator> site_gen_;
    std::unique_ptr<Generators::WaterGenerator> water_gen_;
};

} // namespace RogueCity::App::Integration
```


### LotGenerator Integration

```cpp
// app/src/Integration/GeneratorBridge.cpp
void GeneratorBridge::generateLots(const LotGenParams& params) {
    using namespace Generators;
    
    // Prepare input from current GlobalState districts
    LotInput input;
    input.districts = state_->districts.data(); // FVA access
    input.roads = state_->roads.data();
    input.min_area = params.min_lot_area;
    input.max_area = params.max_lot_area;
    
    // Execute generator (delegates to RogueWorker if >10ms)
    LotOutput output = lot_gen_->Generate(input);
    
    // Populate GlobalState (use FVA for UI stability)
    state_->lots.clear();
    state_->lots.reserve(output.lots.size());
    for (const auto& lot : output.lots) {
        state_->lots.push_back(lot);
    }
    
    // Trigger UI refresh
    state_->NotifyLotsChanged();
}
```


### WaterGenerator Integration (NEW)

```cpp
// generators/include/RogueCity/Generators/Urban/WaterGenerator.hpp
namespace RogueCity::Generators {

struct WaterBody {
    std::vector<glm::vec2> boundary;
    float depth;
    WaterType type; // Lake, River, Ocean
};

class WaterGenerator {
public:
    std::vector<WaterBody> GenerateLakes(const TensorField& field);
    WaterBody TraceRiver(const std::vector<glm::vec2>& control_points);
};

} // namespace
```


### Success Criteria

- ✅ All 7 bridge methods implemented
- ✅ Each method updates GlobalState deterministically
- ✅ UI panels reflect changes immediately
- ✅ RogueWorker used for operations >10ms

---

## **Task 1.4: UI Panel → Generator Wiring** (UI/UX Master + Coder)

**Priority**: HIGH
**Estimated Time**: 3-4 hours

### Objectives

Connect **all control panels** to generator bridge methods:

#### Panel → Bridge Mapping

| Panel | Bridge Method | HFSM State |
| :-- | :-- | :-- |
| AxiomControlPanel | `generateTensorField()` | AxiomPlacement |
| RoadControlPanel | `traceRoads()` | RoadEditing |
| DistrictControlPanel | `subdivideDistricts()` | DistrictZoning |
| LotControlPanel | `generateLots()` | LotSubdivision |
| BuildingControlPanel | `placeBuildingSites()` | BuildingPlacement |
| WaterControlPanel | `generateWaterbodies()` | WaterEditing |

### Implementation Example

```cpp
// visualizer/src/ui/panels/rc_panel_lot_control.cpp
void RcPanelLotControl::Render() {
    ImGui::Begin("Lot Subdivision");
    
    // Parameters (state-reactive)
    ImGui::SliderFloat("Min Area", &params_.min_lot_area, 50.0f, 200.0f);
    ImGui::SliderFloat("Max Area", &params_.max_lot_area, 200.0f, 1000.0f);
    ImGui::Checkbox("Respect Zoning", &params_.respect_zoning);
    
    // Trigger generation
    if (ImGui::Button("Generate Lots")) {
        bridge_->generateLots(params_);
    }
    
    // Status display
    ImGui::Text("Lots: %d", state_->lots.size());
    
    ImGui::End();
}
```


### Cockpit Doctrine Compliance

```cpp
// Y2K affordance: pulse on generation
if (is_generating_) {
    float pulse = 0.5f + 0.5f * sinf(ImGui::GetTime() * 4.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, pulse, 0.2f, 1.0f));
}
```


### Success Criteria

- ✅ All control panels trigger generator methods
- ✅ Parameters flow from UI → Bridge → Generator
- ✅ Results update GlobalState and viewport immediately
- ✅ Y2K motion affordances implemented

---

## **Task 1.5: Viewport Overlay Completion** (UI/UX Master)

**Priority**: MEDIUM
**Estimated Time**: 2-3 hours

### Objectives

Add **missing overlays** for new features:

#### New Overlays

1. **Water Bodies** (blue polygons with shore lines)
2. **River Paths** (flowing curves with direction arrows)
3. **Building Sites** (footprint outlines with height indicators)
4. **Lot Boundaries** (already done, verify rendering)

### Implementation

```cpp
// visualizer/src/ui/viewport/rc_viewport_overlays.cpp
void RcViewportOverlays::RenderWaterbodies(const GlobalState& state) {
    for (const auto& water : state.waterbodies) {
        // Fill polygon
        ImU32 water_color = IM_COL32(50, 120, 200, 180);
        for (size_t i = 0; i < water.boundary.size(); ++i) {
            glm::vec2 p1 = water.boundary[i];
            glm::vec2 p2 = water.boundary[(i + 1) % water.boundary.size()];
            draw_list->AddLine(
                ToScreenSpace(p1), 
                ToScreenSpace(p2), 
                water_color, 
                2.0f
            );
        }
        
        // Shore detail
        if (water.generate_shore) {
            ImU32 shore_color = IM_COL32(200, 180, 140, 255);
            // ... render shore vertices
        }
    }
}
```


### Success Criteria

- ✅ All entity types render in viewport
- ✅ Selection highlighting works per type
- ✅ Overlays respect HFSM state (show only relevant data)

---

## **PASS 1 Summary**

### Deliverables

1. ✅ Zero build errors/warnings
2. ✅ All 6 editor tools wired through HFSM
3. ✅ 7 generator bridge methods complete
4. ✅ All UI panels trigger generators
5. ✅ Viewport overlays for all entity types

### Validation Gates

```sh
# Build verification
cmake --build build --config Release
# Should output: 0 errors, 0 warnings

# Runtime test
.\\build\\Release\\RogueCityApp.exe
# Test each tool mode:
# 1. Place axiom → tensor field generates
# 2. Trace roads → roads appear in index
# 3. Subdivide districts → districts appear
# 4. Generate lots → lots appear
# 5. Place buildings → buildings appear
# 6. Add waterbody → water renders
```


### Time Estimate

**Total: 14-19 hours** (2-3 work days)

---

# 📋 PASS 2 EXECUTION PLAN

## **Task 2.1: Water/River Editing Tools** (City Planner + Coder)

**Priority**: HIGH
**Estimated Time**: 4-5 hours

### Objectives

Implement **interactive editing** for water features:

#### Water Tool Features

1. **Polygon Drawing** (click to add boundary points)
2. **River Curve** (spline with control points)
3. **Shore Generation** (automatic coastline detail)
4. **Depth Painting** (brush-based depth map)

### Implementation

```cpp
// app/include/RogueCity/App/Tools/WaterTool.hpp
class WaterTool {
public:
    enum class Mode { Lake, River, Ocean };
    
    void OnMouseDown(glm::vec2 world_pos);
    void OnMouseDrag(glm::vec2 world_pos);
    void OnMouseUp();
    
    void CommitWaterbody(); // Sends to GeneratorBridge
    
private:
    Mode mode_ = Mode::Lake;
    std::vector<glm::vec2> boundary_points_;
    bool is_editing_ = false;
};
```


### Success Criteria

- ✅ Can draw lake boundaries
- ✅ Can place river control points
- ✅ Shore generation works
- ✅ Changes commit to GlobalState

---

## **Task 2.2: District/Lot Editing** (City Planner + UI/UX Master)

**Priority**: MEDIUM
**Estimated Time**: 3-4 hours

### Objectives

Enable **manual override** of generated districts/lots:

#### Editing Features

1. **District Type Override** (right-click → change zoning)
2. **Lot Merging** (select multiple → merge)
3. **Lot Splitting** (draw line → subdivide)
4. **AESP Adjustment** (slider per district)

### Implementation

```cpp
// Context menu in RcDataIndexPanel<District>
if (ImGui::BeginPopupContextItem()) {
    if (ImGui::MenuItem("Change to Residential")) {
        district.type = DistrictType::Residential;
        district.RecalculateAESP();
    }
    if (ImGui::MenuItem("Recalculate Budget")) {
        bridge_->RecalculateDistrictBudget(district.id);
    }
    ImGui::EndPopup();
}
```


### Success Criteria

- ✅ Can manually change district types
- ✅ Can merge/split lots
- ✅ AESP updates propagate to buildings

---

## **Task 2.3: Export System** (Coder + Documentation Keeper)

**Priority**: HIGH
**Estimated Time**: 4-6 hours

### Objectives

Implement **3 export formats**:

#### Export Targets

1. **JSON** (full city data for save/load)
2. **OBJ** (geometry for Blender import)
3. **GLTF** (PBR materials + animation)

### Implementation

```cpp
// generators/include/RogueCity/Generators/Export/CityExporter.hpp
class CityExporter {
public:
    void ExportJSON(const GlobalState& state, const std::string& path);
    void ExportOBJ(const GlobalState& state, const std::string& path);
    void ExportGLTF(const GlobalState& state, const std::string& path);
    
private:
    nlohmann::json SerializeGlobalState(const GlobalState& state);
    void WriteMesh(const std::vector<glm::vec3>& vertices, std::ofstream& file);
};
```


### JSON Schema

```json
{
  "version": "1.0",
  "city": {
    "axioms": [...],
    "roads": [...],
    "districts": [...],
    "lots": [...],
    "buildings": [...],
    "waterbodies": [...]
  },
  "metadata": {
    "seed": 12345,
    "generation_time": "2026-02-08T12:00:00Z"
  }
}
```


### Success Criteria

- ✅ Can export to all 3 formats
- ✅ JSON can be re-imported
- ✅ OBJ opens in Blender correctly

---

## **Task 2.4: AI Pattern Catalog Update** (AI Integration Agent)

**Priority**: MEDIUM
**Estimated Time**: 2-3 hours

### Objectives

Update **pattern catalog** with new tool panels:

```json
// AI/docs/ui/ui_patterns.json
{
  "patterns": [
    {
      "name": "WaterControlPanel",
      "type": "ControlPanel",
      "role": "trigger_generation",
      "data_bindings": ["GlobalState.waterbodies"],
      "hfsm_states": ["WaterEditing"],
      "refactor_opportunities": [
        "Template into FeatureControlPanel<WaterBody>"
      ]
    },
    {
      "name": "BuildingControlPanel",
      "type": "ControlPanel",
      "role": "trigger_generation",
      "data_bindings": ["GlobalState.buildings"],
      "hfsm_states": ["BuildingPlacement"]
    }
  ]
}
```


### Success Criteria

- ✅ All new panels documented
- ✅ UiDesignAssistant recognizes patterns
- ✅ Refactoring suggestions work

---

## **Task 2.5: Testing \& Polish** (Debug Manager + Resource Manager)

**Priority**: HIGH
**Estimated Time**: 4-6 hours

### Objectives

#### Test Suite

1. **Deterministic Generation** (seeded runs produce identical output)
2. **Performance Regression** (no slowdowns vs baseline)
3. **Memory Budgets** (respect caps from Resource Manager)
4. **UI Responsiveness** (no blocking operations >10ms)

#### Test Implementation

```cpp
// tests/test_full_pipeline.cpp
TEST_CASE("Full Pipeline Deterministic") {
    GlobalState state;
    GeneratorBridge bridge(&state);
    
    // Seeded generation
    state.SetSeed(12345);
    bridge.generateTensorField({...});
    bridge.traceRoads({...});
    bridge.subdivideDistricts({...});
    bridge.generateLots({...});
    bridge.placeBuildingSites({...});
    
    // Verify output
    REQUIRE(state.roads.size() == EXPECTED_ROADS);
    REQUIRE(state.districts.size() == EXPECTED_DISTRICTS);
    
    // Verify hash for byte-identical output
    REQUIRE(HashGlobalState(state) == KNOWN_HASH);
}
```


### Polish Checklist

- [ ] Fix remaining float narrowing warnings
- [ ] Add tooltips to all tool buttons
- [ ] Implement keyboard shortcuts (Ctrl+A = Axiom tool)
- [ ] Add progress bars for long operations
- [ ] Implement undo/redo for edits


### Success Criteria

- ✅ All tests pass
- ✅ No performance regressions
- ✅ UI feels polished and responsive

---

## **Task 2.6: Documentation \& _Temp Cleanup** (Documentation Keeper)

**Priority**: MEDIUM
**Estimated Time**: 2-3 hours

### Objectives

#### Documentation Updates

1. Update `ReadMe.md` with V1 feature list
2. Create `docs/V1_Complete.md` with:
    - All features implemented
    - Tool usage guide
    - Export workflow
    - Troubleshooting

#### _Temp Deletion Gate

Only delete `_Temp/` after:

- ✅ All generators ported
- ✅ All tests pass
- ✅ Export works
- ✅ Documentation complete


### Final Documentation Structure

```
docs/
├── V1_Complete.md (NEW)
├── V1_Tool_Guide.md (NEW)
├── V1_Export_Guide.md (NEW)
├── AI_Integration_Summary.md (existing)
└── TheRogueCityDesignerSoft.md (reference)
```


### Success Criteria

- ✅ Documentation matches implementation
- ✅ _Temp/ moved to _Del/
- ✅ Build instructions verified

---

## **PASS 2 Summary**

### Deliverables

1. ✅ Water/river editing tools
2. ✅ District/lot manual editing
3. ✅ Export system (JSON/OBJ/GLTF)
4. ✅ AI pattern catalog updated
5. ✅ Full test suite passing
6. ✅ Documentation complete
7. ✅ _Temp/ cleanup

### Validation Gates

```sh
# Full pipeline test
.\\build\\Release\\test_generators.exe --test=FullPipeline

# UI test (manual)
.\\build\\Release\\RogueCityApp.exe
# 1. Generate city from scratch
# 2. Edit districts/lots
# 3. Add water features
# 4. Export to JSON/OBJ
# 5. Re-import JSON

# Performance test
.\\build\\Release\\RogueCityApp.exe --benchmark
# Should show: <100ms for 1000-road city generation
```


### Time Estimate

**Total: 19-27 hours** (3-4 work days)

---

# 🎯 V1 COMPLETE CHECKLIST

## Build System

- [ ] Zero build errors
- [ ] Zero build warnings
- [ ] All targets link successfully
- [ ] CMake configuration deterministic


## Generator Pipeline

- [ ] Tensor field generation
- [ ] Road tracing
- [ ] District subdivision
- [ ] Lot generation
- [ ] Building placement
- [ ] Water/river generation


## Editor Tools (HFSM-driven)

- [ ] Axiom placement tool
- [ ] Road editing tool
- [ ] District zoning tool
- [ ] Lot subdivision tool
- [ ] Building placement tool
- [ ] Water/river editing tool


## UI/UX (Cockpit Doctrine)

- [ ] All panels state-reactive
- [ ] Y2K motion affordances
- [ ] Context menus functional
- [ ] Viewport overlays complete
- [ ] Index panels template-based


## Export System

- [ ] JSON export/import
- [ ] OBJ export (Blender-compatible)
- [ ] GLTF export (PBR materials)


## AI Integration

- [ ] Pattern catalog updated
- [ ] UiDesignAssistant functional
- [ ] CitySpec pipeline wired
- [ ] Toolserver endpoints tested


## Testing

- [ ] Deterministic generation tests
- [ ] Performance regression tests
- [ ] Memory budget tests
- [ ] UI responsiveness tests


## Documentation

- [ ] ReadMe.md updated
- [ ] V1_Complete.md written
- [ ] Tool guides created
- [ ] _Temp/ cleanup complete

---

# 📅 EXECUTION TIMELINE

Tasks 1.1-1.3 (Build + HFSM + Bridge)
Tasks 1.4-1.5 (UI Wiring + Overlays)

- **Checkpoint**: All tools functional, zero build errors

Tasks 2.1-2.2 (Water/District editing)
Task 2.3 (Export system)
Tasks 2.4-2.5 (AI + Testing)
Task 2.6 (Documentation + Cleanup)

- **Checkpoint**:
test V1 for feature completeness, ready for hard testing
.

---

# 🚀 IMMEDIATE NEXT STEPS

1. **Run Build Verification**:
```sh
cmake --build build --config Release 2>&1 | Tee-Object build_log.txt
```

2. **Create Task Branches**:
```sh
git checkout -b pass1-build-stability
```

3. **Begin Task 1.1** (Build System Hardening):
    - Fix MSVCRT conflict in root CMakeLists.txt
    - Add `/W4` compliance
    - Resolve float narrowing warnings
4. **Coordinate Agent Assignments**:
    - **Debug Manager**: Task 1.1 (build fixes)
    - **Coder Agent**: Tasks 1.2-1.3 (HFSM + Bridge)
    - **UI/UX Master**: Tasks 1.4-1.5 (UI wiring)
    - **City Planner**: Task 2.1 (water tools)
    - **AI Integration Agent**: Task 2.4 (pattern catalog)
    - **Documentation Keeper**: Task 2.6 (docs + cleanup)

5**continue with the rest of the plan**

i would like specific information to pass along with a prompt to my ai coding agent to implament all this in 2 runs target model Claude Sonnet 4.5

Now let me create the comprehensive two-pass prompt packages for Claude Sonnet 4.5:

***

# 🤖 PASS 1 PROMPT: Build Stability + Tool-Generator Wiring

## Context Summary

You are implementing **PASS 1** of a V1 completion plan for **RogueCities Urban Spatial Designer**, a procedural city generation tool built in C++17+ with ImGui. This pass focuses on:

1. Eliminating build errors/warnings
2. Wiring all editor tools through HFSM state machine
3. Completing GeneratorBridge integration layer
4. Connecting UI panels to generators
5. Adding viewport overlays for all entity types

**Target**: Zero build errors, all tools functional, deterministic pipeline

***

## Repository Structure

```
RogueCities_UrbanSpatialDesigner/
├── core/               # Data types, HFSM, math utilities
│   ├── include/RogueCity/Core/
│   │   ├── Data/      # CityTypes.hpp, CitySpec.hpp, TensorTypes.hpp
│   │   └── Editor/    # HFSM states
├── generators/         # Algorithm layer (roads, districts, lots, buildings)
├── app/               # Integration layer (GeneratorBridge, tools)
│   ├── include/RogueCity/App/
│   │   ├── Integration/  # GeneratorBridge.hpp
│   │   ├── Tools/        # Editor tool classes
│   │   ├── UI/           # Panel base classes
│   │   └── Viewports/    # Viewport overlays
├── visualizer/        # ImGui frontend
│   └── src/ui/panels/ # Control panels
├── CMakeLists.txt     # Root build config
└── tests/
```


***

## TASK 1.1: Build System Hardening

### Objectives

1. **Fix LNK4098 (MSVCRT conflict)**: Ensure consistent runtime library linkage across all targets
2. **Resolve float narrowing warnings**: Add explicit casts in affected files
3. **Enable `/W4` compliance**: Treat warnings seriously (but don't break build)

### Implementation Steps

#### A. Fix MSVCRT Conflict (Root `CMakeLists.txt`)

```cmake
# Add after project() declaration
if(MSVC)
    # Force consistent MSVC runtime library
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    
    # Enable high warning level, but don't break on warnings yet
    add_compile_options(/W4 /WX-)
    
    # Disable specific noisy warnings
    add_compile_options(
        /wd4100  # Unreferenced formal parameter
        /wd4127  # Conditional expression is constant
    )
endif()
```


#### B. Fix Float Narrowing Warnings

**Files to target**:

- `app/src/Integration/GeneratorBridge.cpp`
- `visualizer/src/ui/viewport/*.cpp`

**Pattern to apply**:

```cpp
// BEFORE (causes warning)
float value = some_double_expression;
glm::vec2 pos = {x, y};  // If x, y are doubles

// AFTER (explicit cast)
float value = static_cast<float>(some_double_expression);
glm::vec2 pos = {static_cast<float>(x), static_cast<float>(y)};
```


#### C. Verify Link Dependencies

Check `app/CMakeLists.txt` and ensure:

```cmake
# Only link sol2 if Lua is enabled
if(ROGUECITY_HAS_SOL2)
    target_link_libraries(RogueCityApp PRIVATE sol2::sol2)
endif()

# Ensure WinHTTP is linked for AI tools (Windows only)
if(WIN32)
    target_link_libraries(RogueCityApp PRIVATE winhttp)
endif()
```


### Success Criteria

✅ `cmake --build build --config Release` outputs **0 errors, 0 warnings**
✅ All targets link successfully
✅ No MSVCRT conflicts in linker output

***

## TASK 1.2: HFSM Tool-Mode Unification

### Objectives

Wire **6 editor tools** through HFSM with deterministic state transitions and UI reactivity.

### Current Tools to Implement

1. **Axiom Placement** → `AxiomPlacementState` (already exists, verify)
2. **Road Editing** → `RoadEditingState` (NEW)
3. **District Zoning** → `DistrictZoningState` (NEW)
4. **Lot Subdivision** → `LotSubdivisionState` (NEW)
5. **Building Placement** → `BuildingPlacementState` (NEW)
6. **Water/River Editing** → `WaterEditingState` (NEW)

### Implementation Pattern

#### A. Define Editor Modes (`core/include/RogueCity/Core/Editor/HFSM.hpp`)

```cpp
namespace RogueCity::Core::Editor {

enum class EditorMode {
    None,
    AxiomPlacement,
    RoadEditing,
    DistrictZoning,
    LotSubdivision,
    BuildingPlacement,
    WaterEditing
};

class EditorHFSM {
public:
    void TransitionTo(EditorMode mode);
    EditorMode GetCurrentMode() const { return current_mode_; }
    
    void Update(float dt);
    
private:
    EditorMode current_mode_ = EditorMode::None;
    std::unique_ptr<HFSMState> current_state_;
    
    // State factory
    std::unique_ptr<HFSMState> CreateState(EditorMode mode);
};

} // namespace
```


#### B. Implement State Classes (`core/src/Editor/HFSMStates.cpp`)

```cpp
// Example: Road Editing State
struct RoadEditingState : public HFSMState {
    void Enter(GlobalState* state) override {
        // Show road inspector panel
        state->ui_flags.show_road_inspector = true;
        // Enable road vertex editing mode
        state->interaction_mode = InteractionMode::EditRoads;
        // Highlight currently selected road
        if (state->selected_road_id != -1) {
            state->roads[state->selected_road_id].highlighted = true;
        }
    }
    
    void Update(float dt, GlobalState* state) override {
        // Handle road manipulation input (vertex dragging, etc.)
        // This logic might delegate to a RoadEditTool class
    }
    
    void Exit(GlobalState* state) override {
        // Commit road changes to generator
        state->ui_flags.show_road_inspector = false;
        state->interaction_mode = InteractionMode::None;
        // Unhighlight roads
        for (auto& road : state->roads) {
            road.highlighted = false;
        }
    }
};

// Similarly implement:
// - DistrictZoningState
// - LotSubdivisionState
// - BuildingPlacementState
// - WaterEditingState
```


#### C. Wire Tool Panel (`visualizer/src/ui/panels/rc_panel_tools.cpp`)

```cpp
void RcPanelTools::Render() {
    ImGui::Begin("Tools", nullptr, ImGuiWindowFlags_NoCollapse);
    
    // Determine current mode for highlighting
    auto current_mode = hfsm_->GetCurrentMode();
    
    // Axiom Tool
    if (current_mode == EditorMode::AxiomPlacement) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
    }
    if (ImGui::Button("Axiom Tool", ImVec2(-1, 40))) {
        hfsm_->TransitionTo(EditorMode::AxiomPlacement);
    }
    if (current_mode == EditorMode::AxiomPlacement) {
        ImGui::PopStyleColor();
    }
    
    // Road Tool (NEW)
    if (current_mode == EditorMode::RoadEditing) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
    }
    if (ImGui::Button("Road Tool", ImVec2(-1, 40))) {
        hfsm_->TransitionTo(EditorMode::RoadEditing);
    }
    if (current_mode == EditorMode::RoadEditing) {
        ImGui::PopStyleColor();
    }
    
    // Repeat for: District, Lot, Building, Water tools
    
    ImGui::End();
}
```


### Success Criteria

✅ Each tool button triggers HFSM state transition
✅ Panel visibility/highlighting responds to current state
✅ Viewport cursor/overlays change per mode
✅ No stale state when switching tools

***

## TASK 1.3: Generator Bridge Completion

### Objectives

Complete all 7 generator bridge methods to connect UI → Generators → GlobalState.

### Current Status

1. ✅ `generateTensorField()` - DONE
2. ✅ `traceRoads()` - DONE
3. ✅ `subdivideDistricts()` - DONE
4. ⚠️ `generateLots()` - PARTIAL (needs completion)
5. ❌ `placeBuildingSites()` - MISSING
6. ❌ `generateWaterbodies()` - MISSING
7. ❌ `traceRivers()` - MISSING

### Implementation

#### A. Update `GeneratorBridge.hpp`

```cpp
// app/include/RogueCity/App/Integration/GeneratorBridge.hpp
namespace RogueCity::App::Integration {

// Parameter structs
struct LotGenParams {
    float min_lot_area = 100.0f;
    float max_lot_area = 500.0f;
    bool respect_zoning = true;
};

struct BuildingPlacementParams {
    float setback_min = 3.0f;
    float height_variance = 0.2f;
    bool use_aesp_height = true;
};

struct WaterGenParams {
    std::vector<glm::vec2> boundary_points;
    float depth = 5.0f;
    bool generate_shore = true;
};

class GeneratorBridge {
public:
    explicit GeneratorBridge(GlobalState* state);
    
    // Existing methods (verify these exist)
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


#### B. Implement `generateLots()` (`app/src/Integration/GeneratorBridge.cpp`)

```cpp
void GeneratorBridge::generateLots(const LotGenParams& params) {
    using namespace Generators;
    
    // Prepare input from current GlobalState
    LotInput input;
    input.districts = state_->districts;  // Access district data
    input.roads = state_->roads;
    input.min_area = params.min_lot_area;
    input.max_area = params.max_lot_area;
    input.respect_zoning = params.respect_zoning;
    
    // Execute generator
    // NOTE: If generation takes >10ms, consider delegating to RogueWorker
    LotOutput output = lot_gen_->Generate(input);
    
    // Populate GlobalState
    state_->lots.clear();
    state_->lots.reserve(output.lots.size());
    for (const auto& lot : output.lots) {
        state_->lots.push_back(lot);
    }
    
    // Trigger UI refresh notification
    state_->NotifyLotsChanged();  // Implement this if not present
}
```


#### C. Implement `placeBuildingSites()` (NEW)

```cpp
void GeneratorBridge::placeBuildingSites(const BuildingPlacementParams& params) {
    using namespace Generators;
    
    // Prepare input
    SiteInput input;
    input.lots = state_->lots;
    input.setback_min = params.setback_min;
    input.height_variance = params.height_variance;
    input.use_aesp_height = params.use_aesp_height;
    
    // Execute generator
    SiteOutput output = site_gen_->Generate(input);
    
    // Populate GlobalState
    state_->buildings.clear();
    state_->buildings.reserve(output.sites.size());
    for (const auto& site : output.sites) {
        state_->buildings.push_back(site);
    }
    
    state_->NotifyBuildingsChanged();
}
```


#### D. Implement `generateWaterbodies()` (NEW)

```cpp
void GeneratorBridge::generateWaterbodies(const WaterGenParams& params) {
    using namespace Generators;
    
    // Prepare input
    WaterInput input;
    input.boundary_points = params.boundary_points;
    input.depth = params.depth;
    input.generate_shore = params.generate_shore;
    
    // Execute generator
    WaterOutput output = water_gen_->Generate(input);
    
    // Populate GlobalState
    state_->waterbodies.push_back(output.waterbody);
    
    state_->NotifyWaterbodiesChanged();
}
```


#### E. Implement `traceRivers()` (NEW)

```cpp
void GeneratorBridge::traceRivers(const std::vector<glm::vec2>& control_points) {
    using namespace Generators;
    
    // Generate river from spline control points
    WaterBody river = water_gen_->TraceRiver(control_points);
    river.type = WaterType::River;
    
    state_->waterbodies.push_back(river);
    state_->NotifyWaterbodiesChanged();
}
```


### Success Criteria

✅ All 7 bridge methods implemented
✅ Each method updates GlobalState deterministically
✅ UI panels reflect changes immediately
✅ Operations >10ms use RogueWorker (async execution)

***

## TASK 1.4: UI Panel → Generator Wiring

### Objectives

Connect all control panels to generator bridge methods with proper parameter flow.

### Panel Mapping

| Panel | Bridge Method | HFSM State | Priority |
| :-- | :-- | :-- | :-- |
| AxiomControlPanel | `generateTensorField()` | AxiomPlacement | Verify |
| RoadControlPanel | `traceRoads()` | RoadEditing | NEW |
| DistrictControlPanel | `subdivideDistricts()` | DistrictZoning | Verify |
| LotControlPanel | `generateLots()` | LotSubdivision | NEW |
| BuildingControlPanel | `placeBuildingSites()` | BuildingPlacement | NEW |
| WaterControlPanel | `generateWaterbodies()` | WaterEditing | NEW |

### Implementation Example: Lot Control Panel

#### File: `visualizer/src/ui/panels/rc_panel_lot_control.cpp`

```cpp
#include "rc_panel_lot_control.hpp"
#include <RogueCity/App/Integration/GeneratorBridge.hpp>

void RcPanelLotControl::Render() {
    // Only show if HFSM is in LotSubdivision mode
    if (hfsm_->GetCurrentMode() != EditorMode::LotSubdivision) {
        return;
    }
    
    ImGui::Begin("Lot Subdivision Control", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    
    // === Parameters Section ===
    ImGui::SeparatorText("Parameters");
    ImGui::SliderFloat("Min Area (m²)", &params_.min_lot_area, 50.0f, 200.0f);
    ImGui::SliderFloat("Max Area (m²)", &params_.max_lot_area, 200.0f, 1000.0f);
    ImGui::Checkbox("Respect Zoning", &params_.respect_zoning);
    
    ImGui::Spacing();
    
    // === Generation Button (with Y2K pulse affordance) ===
    if (is_generating_) {
        float pulse = 0.5f + 0.5f * sinf(ImGui::GetTime() * 4.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, pulse, 0.2f, 1.0f));
    }
    
    if (ImGui::Button("Generate Lots", ImVec2(-1, 40))) {
        bridge_->generateLots(params_);
        is_generating_ = true;
    }
    
    if (is_generating_) {
        ImGui::PopStyleColor();
        // Reset flag after 1 second (for visual feedback)
        if (ImGui::GetTime() - gen_start_time_ > 1.0f) {
            is_generating_ = false;
        }
    }
    
    ImGui::Spacing();
    
    // === Status Display ===
    ImGui::SeparatorText("Status");
    ImGui::Text("Total Lots: %zu", state_->lots.size());
    if (!state_->lots.empty()) {
        float avg_area = CalculateAverageLotArea(state_->lots);
        ImGui::Text("Average Area: %.1f m²", avg_area);
    }
    
    ImGui::End();
}
```


### Repeat for All Panels

Apply the same pattern to:

1. **RoadControlPanel**: Parameters for road width, curve smoothness, etc.
2. **BuildingControlPanel**: Setback, height variance, AESP usage
3. **WaterControlPanel**: Boundary drawing, depth, shore generation

### Success Criteria

✅ All control panels trigger generator methods
✅ Parameters flow correctly from UI → Bridge → Generator
✅ Results update GlobalState and viewport immediately
✅ Y2K motion affordances implemented (pulsing buttons during generation)

***

## TASK 1.5: Viewport Overlay Completion

### Objectives

Add missing overlays for new entity types to ensure all data is visualizable.

### New Overlays to Implement

1. **Water Bodies** (blue polygons with shore lines)
2. **River Paths** (flowing curves with direction arrows)
3. **Building Sites** (footprint outlines with height indicators)
4. **Lot Boundaries** (verify existing implementation)

### Implementation

#### File: `visualizer/src/ui/viewport/rc_viewport_overlays.cpp`

```cpp
void RcViewportOverlays::RenderWaterbodies(const GlobalState& state) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    for (const auto& water : state.waterbodies) {
        // Determine color based on water type
        ImU32 water_color;
        if (water.type == WaterType::River) {
            water_color = IM_COL32(80, 150, 220, 200);
        } else if (water.type == WaterType::Lake) {
            water_color = IM_COL32(50, 120, 200, 180);
        } else {
            water_color = IM_COL32(30, 90, 180, 180);
        }
        
        // Render boundary polygon
        for (size_t i = 0; i < water.boundary.size(); ++i) {
            glm::vec2 p1 = water.boundary[i];
            glm::vec2 p2 = water.boundary[(i + 1) % water.boundary.size()];
            
            ImVec2 screen_p1 = WorldToScreen(p1);
            ImVec2 screen_p2 = WorldToScreen(p2);
            
            draw_list->AddLine(screen_p1, screen_p2, water_color, 2.0f);
        }
        
        // Render shore detail if enabled
        if (water.generate_shore && !water.shore_vertices.empty()) {
            ImU32 shore_color = IM_COL32(200, 180, 140, 255);
            for (const auto& vertex : water.shore_vertices) {
                ImVec2 screen_pos = WorldToScreen(vertex);
                draw_list->AddCircleFilled(screen_pos, 2.0f, shore_color);
            }
        }
        
        // Label (center of mass)
        if (viewport_->show_labels) {
            glm::vec2 center = CalculateCentroid(water.boundary);
            ImVec2 screen_center = WorldToScreen(center);
            draw_list->AddText(screen_center, IM_COL32(255, 255, 255, 255), 
                               water.type == WaterType::River ? "River" : "Lake");
        }
    }
}

void RcViewportOverlays::RenderBuildingSites(const GlobalState& state) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    for (const auto& building : state.buildings) {
        // Footprint outline
        ImU32 outline_color = building.highlighted ? 
            IM_COL32(255, 200, 0, 255) : 
            IM_COL32(150, 150, 150, 200);
        
        for (size_t i = 0; i < building.footprint.size(); ++i) {
            glm::vec2 p1 = building.footprint[i];
            glm::vec2 p2 = building.footprint[(i + 1) % building.footprint.size()];
            
            draw_list->AddLine(WorldToScreen(p1), WorldToScreen(p2), outline_color, 1.5f);
        }
        
        // Height indicator (vertical line at center)
        if (viewport_->show_height_indicators) {
            glm::vec2 center = CalculateCentroid(building.footprint);
            ImVec2 screen_center = WorldToScreen(center);
            ImVec2 screen_top = screen_center;
            screen_top.y -= building.height * viewport_->height_scale;
            
            draw_list->AddLine(screen_center, screen_top, IM_COL32(255, 100, 100, 200), 2.0f);
            draw_list->AddCircleFilled(screen_top, 3.0f, IM_COL32(255, 100, 100, 255));
        }
    }
}

void RcViewportOverlays::RenderLots(const GlobalState& state) {
    // Verify existing implementation or add if missing
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    for (const auto& lot : state.lots) {
        ImU32 lot_color = GetLotColor(lot.zoning_type);
        
        for (size_t i = 0; i < lot.boundary.size(); ++i) {
            glm::vec2 p1 = lot.boundary[i];
            glm::vec2 p2 = lot.boundary[(i + 1) % lot.boundary.size()];
            
            draw_list->AddLine(WorldToScreen(p1), WorldToScreen(p2), lot_color, 1.0f);
        }
    }
}
```


### Overlay Selection Logic

```cpp
void RcViewportOverlays::Render(const GlobalState& state) {
    // Render based on current HFSM mode
    auto mode = hfsm_->GetCurrentMode();
    
    // Always render roads and districts (foundational)
    RenderRoads(state);
    RenderDistricts(state);
    
    // Mode-specific overlays
    switch (mode) {
        case EditorMode::LotSubdivision:
            RenderLots(state);
            break;
        case EditorMode::BuildingPlacement:
            RenderLots(state);
            RenderBuildingSites(state);
            break;
        case EditorMode::WaterEditing:
            RenderWaterbodies(state);
            break;
        default:
            // Render all in default view
            RenderLots(state);
            RenderBuildingSites(state);
            RenderWaterbodies(state);
            break;
    }
}
```


### Success Criteria

✅ All entity types render in viewport
✅ Selection highlighting works per type
✅ Overlays respect HFSM state (show only relevant data)
✅ Performance remains acceptable (>30 FPS with 1000+ entities)

***

## PASS 1 VALIDATION CHECKLIST

Before moving to PASS 2, verify:

### Build System

- [ ] `cmake --build build --config Release` → 0 errors, 0 warnings
- [ ] All targets link successfully
- [ ] MSVCRT conflict resolved
- [ ] Float narrowing warnings fixed


### HFSM Integration

- [ ] All 6 tool buttons exist in Tools panel
- [ ] Clicking each button transitions HFSM state
- [ ] Active tool button highlights correctly
- [ ] State transitions are deterministic


### Generator Bridge

- [ ] All 7 bridge methods compile
- [ ] Each method updates GlobalState
- [ ] Notification system works (`NotifyLotsChanged()`, etc.)
- [ ] No crashes when calling generators


### UI Wiring

- [ ] All control panels render when in correct HFSM state
- [ ] Parameter sliders update correctly
- [ ] Generate buttons trigger bridge methods
- [ ] Status displays show current entity counts


### Viewport Overlays

- [ ] Roads render
- [ ] Districts render
- [ ] Lots render
- [ ] Buildings render
- [ ] Water bodies render
- [ ] Selection highlighting works

***

## Execution Commands

```bash
# 1. Create working branch
git checkout -b pass1-build-stability

# 2. Build and verify
cmake --build build --config Release 2>&1 | Tee-Object pass1_build_log.txt

# 3. Run tests
./build/Release/test_generators.exe
./build/Release/test_editor_hfsm.exe

# 4. Launch application for manual testing
./build/Release/RogueCityApp.exe

# 5. Commit progress
git add .
git commit -m "PASS 1: Build stability + HFSM wiring complete"
```


***

## Expected Time

**14-19 hours** (concentrated implementation)

***

## Notes for Claude

- **Prioritize**: Tasks 1.1-1.3 first (build + core systems)
- **Verify**: Check existing code before rewriting (e.g., AxiomPlacementState may already exist)
- **Consistency**: Use existing naming conventions from `CityTypes.hpp`
- **Safety**: Add null checks when accessing GlobalState collections
- **Performance**: Profile any loops over large collections (districts, lots, buildings)

***

# 🤖 PASS 2 PROMPT: Feature Completion + Export + Polish

## Context Summary

You are implementing **PASS 2** of the V1 completion plan for **RogueCities Urban Spatial Designer**. PASS 1 has established:

- ✅ Zero build errors/warnings
- ✅ All editor tools wired through HFSM
- ✅ Generator bridge complete
- ✅ UI panels connected to generators
- ✅ Viewport overlays functional

**PASS 2 Goals**:

1. Interactive water/river editing tools
2. Manual district/lot editing capabilities
3. Export system (JSON/OBJ/GLTF)
4. AI pattern catalog updates
5. Comprehensive testing
6. Documentation and `_Temp/` cleanup

***

## TASK 2.1: Water/River Editing Tools

### Objectives

Implement **interactive editing** for water features with polygon drawing and spline control.

### Features to Implement

#### A. Water Tool Modes

```cpp
// app/include/RogueCity/App/Tools/WaterTool.hpp
namespace RogueCity::App::Tools {

enum class WaterToolMode {
    Lake,       // Polygon boundary drawing
    River,      // Spline curve with control points
    Ocean,      // Large boundary with shore detail
    None
};

class WaterTool {
public:
    explicit WaterTool(GeneratorBridge* bridge, GlobalState* state);
    
    void SetMode(WaterToolMode mode);
    WaterToolMode GetMode() const { return mode_; }
    
    // Mouse event handlers
    void OnMouseDown(glm::vec2 world_pos);
    void OnMouseMove(glm::vec2 world_pos);
    void OnMouseUp();
    
    // Keyboard events
    void OnKeyPress(int key);  // ESC = cancel, Enter = commit
    
    // Commit current water feature to GlobalState
    void CommitWaterbody();
    void CancelEditing();
    
    // Preview rendering (before commit)
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


#### B. Lake Drawing Implementation

```cpp
// app/src/Tools/WaterTool.cpp
void WaterTool::OnMouseDown(glm::vec2 world_pos) {
    if (mode_ == WaterToolMode::Lake) {
        if (!is_editing_) {
            // Start new lake
            boundary_points_.clear();
            is_editing_ = true;
        }
        
        // Add boundary point
        boundary_points_.push_back(world_pos);
        
        // Auto-close if near first point
        if (boundary_points_.size() > 2) {
            float dist = glm::distance(boundary_points_.front(), world_pos);
            if (dist < 10.0f) {  // 10m threshold
                CommitWaterbody();
            }
        }
    }
}

void WaterTool::CommitWaterbody() {
    if (boundary_points_.size() < 3) {
        // Invalid polygon
        CancelEditing();
        return;
    }
    
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

void WaterTool::RenderPreview(ImDrawList* draw_list) {
    if (!is_editing_ || boundary_points_.empty()) {
        return;
    }
    
    // Draw boundary preview
    ImU32 preview_color = IM_COL32(100, 180, 255, 200);
    for (size_t i = 0; i < boundary_points_.size(); ++i) {
        glm::vec2 p1 = boundary_points_[i];
        glm::vec2 p2 = (i + 1 < boundary_points_.size()) ? 
            boundary_points_[i + 1] : 
            boundary_points_.front();
        
        draw_list->AddLine(WorldToScreen(p1), WorldToScreen(p2), preview_color, 2.0f);
        draw_list->AddCircleFilled(WorldToScreen(p1), 4.0f, IM_COL32(255, 200, 0, 255));
    }
    
    // Hint text
    ImVec2 screen_pos = WorldToScreen(boundary_points_.back());
    screen_pos.y -= 20.0f;
    draw_list->AddText(screen_pos, IM_COL32(255, 255, 255, 255), 
                       "Click near first point to close");
}
```


#### C. River Spline Implementation

```cpp
void WaterTool::OnMouseDown(glm::vec2 world_pos) {
    if (mode_ == WaterToolMode::River) {
        if (!is_editing_) {
            river_control_points_.clear();
            is_editing_ = true;
        }
        
        river_control_points_.push_back(world_pos);
        
        // Rivers need at least 2 control points to commit
        // User presses Enter to finish
    }
}

void WaterTool::OnKeyPress(int key) {
    if (key == GLFW_KEY_ENTER && is_editing_) {
        if (mode_ == WaterToolMode::River && river_control_points_.size() >= 2) {
            // Commit river
            bridge_->traceRivers(river_control_points_);
            river_control_points_.clear();
            is_editing_ = false;
        }
    } else if (key == GLFW_KEY_ESCAPE) {
        CancelEditing();
    }
}
```


#### D. Water Control Panel Integration

```cpp
// visualizer/src/ui/panels/rc_panel_water_control.cpp
void RcPanelWaterControl::Render() {
    if (hfsm_->GetCurrentMode() != EditorMode::WaterEditing) {
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


### Success Criteria

✅ Can draw lake boundaries with polygon closure
✅ Can place river control points with spline generation
✅ Shore generation works
✅ Changes commit to GlobalState and render in viewport

***

## TASK 2.2: District/Lot Manual Editing

### Objectives

Enable **manual overrides** of generated districts and lots for fine-tuned control.

### Features to Implement

#### A. District Type Override (Context Menu)

```cpp
// visualizer/src/ui/panels/rc_data_index_panel.cpp
// Template specialization for District

template<>
void RcDataIndexPanel<District>::RenderContextMenu(District& district, int index) {
    if (ImGui::BeginPopupContextItem()) {
        ImGui::SeparatorText("Change District Type");
        
        if (ImGui::MenuItem("Residential")) {
            district.type = DistrictType::Residential;
            district.RecalculateAESP();  // Recalculate budget
            bridge_->UpdateDistrict(district);
        }
        if (ImGui::MenuItem("Commercial")) {
            district.type = DistrictType::Commercial;
            district.RecalculateAESP();
            bridge_->UpdateDistrict(district);
        }
        if (ImGui::MenuItem("Industrial")) {
            district.type = DistrictType::Industrial;
            district.RecalculateAESP();
            bridge_->UpdateDistrict(district);
        }
        if (ImGui::MenuItem("Park")) {
            district.type = DistrictType::Park;
            district.RecalculateAESP();
            bridge_->UpdateDistrict(district);
        }
        
        ImGui::Separator();
        
        if (ImGui::MenuItem("Recalculate Budget")) {
            bridge_->RecalculateDistrictBudget(district.id);
        }
        
        if (ImGui::MenuItem("Delete District")) {
            // Mark for deletion (handled in update loop)
            district.marked_for_deletion = true;
        }
        
        ImGui::EndPopup();
    }
}
```


#### B. Lot Merging Tool

```cpp
// app/include/RogueCity/App/Tools/LotEditTool.hpp
namespace RogueCity::App::Tools {

class LotEditTool {
public:
    explicit LotEditTool(GeneratorBridge* bridge, GlobalState* state);
    
    // Multi-selection
    void SelectLot(int lot_id);
    void DeselectLot(int lot_id);
    void ClearSelection();
    
    // Operations
    void MergeSelectedLots();
    void SplitLot(int lot_id, glm::vec2 split_line_start, glm::vec2 split_line_end);
    
    // Rendering
    void RenderSelectionOverlay(ImDrawList* draw_list);
    
private:
    GeneratorBridge* bridge_;
    GlobalState* state_;
    std::vector<int> selected_lot_ids_;
};

} // namespace
```


#### C. Lot Merge Implementation

```cpp
// app/src/Tools/LotEditTool.cpp
void LotEditTool::MergeSelectedLots() {
    if (selected_lot_ids_.size() < 2) {
        // Need at least 2 lots to merge
        return;
    }
    
    // Gather lots
    std::vector<Lot> lots_to_merge;
    for (int id : selected_lot_ids_) {
        auto it = std::find_if(state_->lots.begin(), state_->lots.end(),
                               [id](const Lot& l) { return l.id == id; });
        if (it != state_->lots.end()) {
            lots_to_merge.push_back(*it);
        }
    }
    
    // Compute merged boundary (convex hull or union)
    Lot merged_lot = ComputeMergedLot(lots_to_merge);
    
    // Remove old lots
    for (int id : selected_lot_ids_) {
        auto it = std::remove_if(state_->lots.begin(), state_->lots.end(),
                                 [id](const Lot& l) { return l.id == id; });
        state_->lots.erase(it, state_->lots.end());
    }
    
    // Add merged lot
    state_->lots.push_back(merged_lot);
    state_->NotifyLotsChanged();
    
    ClearSelection();
}

Lot LotEditTool::ComputeMergedLot(const std::vector<Lot>& lots) {
    // Implementation: Compute convex hull or GEOS union
    // For simplicity, use bounding polygon union
    
    Lot merged;
    merged.id = state_->lots.size();  // New ID
    merged.district_id = lots[0].district_id;  // Inherit from first
    
    // Collect all boundary points
    std::vector<glm::vec2> all_points;
    for (const auto& lot : lots) {
        all_points.insert(all_points.end(), lot.boundary.begin(), lot.boundary.end());
    }
    
    // Compute convex hull (or use GEOS for proper union)
    merged.boundary = ComputeConvexHull(all_points);
    merged.area = ComputePolygonArea(merged.boundary);
    
    return merged;
}
```


#### D. Lot Splitting Tool

```cpp
void LotEditTool::SplitLot(int lot_id, glm::vec2 split_start, glm::vec2 split_end) {
    // Find lot
    auto it = std::find_if(state_->lots.begin(), state_->lots.end(),
                           [lot_id](const Lot& l) { return l.id == lot_id; });
    if (it == state_->lots.end()) {
        return;
    }
    
    Lot& lot = *it;
    
    // Split polygon by line (requires geometric algorithm)
    auto [lot1, lot2] = SplitPolygonByLine(lot.boundary, split_start, split_end);
    
    // Replace original lot with two new lots
    lot.boundary = lot1;
    lot.area = ComputePolygonArea(lot1);
    
    Lot new_lot;
    new_lot.id = state_->lots.size();
    new_lot.district_id = lot.district_id;
    new_lot.boundary = lot2;
    new_lot.area = ComputePolygonArea(lot2);
    
    state_->lots.push_back(new_lot);
    state_->NotifyLotsChanged();
}
```


#### E. AESP Manual Adjustment

```cpp
// Add to District inspector panel
void RcDistrictInspector::Render(District& district) {
    ImGui::Begin("District Inspector");
    
    ImGui::Text("District ID: %d", district.id);
    ImGui::Text("Type: %s", magic_enum::enum_name(district.type).data());
    
    ImGui::Spacing();
    ImGui::SeparatorText("AESP Budget");
    
    // Manual sliders for AESP components
    bool changed = false;
    changed |= ImGui::SliderFloat("Aesthetics", &district.aesp.aesthetics, 0.0f, 10.0f);
    changed |= ImGui::SliderFloat("Economy", &district.aesp.economy, 0.0f, 10.0f);
    changed |= ImGui::SliderFloat("Social", &district.aesp.social, 0.0f, 10.0f);
    changed |= ImGui::SliderFloat("Prestige", &district.aesp.prestige, 0.0f, 10.0f);
    
    if (changed) {
        // Recalculate total budget
        district.aesp.total_budget = district.aesp.aesthetics + 
                                     district.aesp.economy + 
                                     district.aesp.social + 
                                     district.aesp.prestige;
        
        // Trigger building height recalculation
        bridge_->RecalculateDistrictBuildings(district.id);
    }
    
    if (ImGui::Button("Reset to Default")) {
        district.RecalculateAESP();
        bridge_->RecalculateDistrictBuildings(district.id);
    }
    
    ImGui::End();
}
```


### Success Criteria

✅ Can manually change district types via context menu
✅ Can merge multiple lots into one
✅ Can split lots with visual line tool
✅ AESP adjustments propagate to building heights

***

## TASK 2.3: Export System

### Objectives

Implement **3 export formats** for interoperability with external tools.

### Export Formats

#### A. JSON Export (Full City Data)

```cpp
// generators/include/RogueCity/Generators/Export/CityExporter.hpp
namespace RogueCity::Generators::Export {

class CityExporter {
public:
    // Export full city state to JSON (save/load compatible)
    void ExportJSON(const GlobalState& state, const std::string& path);
    
    // Export geometry to OBJ (Blender-compatible)
    void ExportOBJ(const GlobalState& state, const std::string& path);
    
    // Export to GLTF (PBR materials + animation support)
    void ExportGLTF(const GlobalState& state, const std::string& path);
    
    // Import JSON back into GlobalState
    void ImportJSON(const std::string& path, GlobalState& state);
    
private:
    nlohmann::json SerializeGlobalState(const GlobalState& state);
    void DeserializeGlobalState(const nlohmann::json& json, GlobalState& state);
    
    void WriteMesh(const std::vector<glm::vec3>& vertices, 
                   const std::vector<uint32_t>& indices,
                   std::ofstream& file);
};

} // namespace
```


#### B. JSON Schema Implementation

```cpp
// generators/src/Export/CityExporter.cpp
nlohmann::json CityExporter::SerializeGlobalState(const GlobalState& state) {
    nlohmann::json j;
    
    j["version"] = "1.0";
    j["metadata"] = {
        {"seed", state.seed},
        {"generation_time", GetCurrentISO8601Time()},
        {"city_name", state.city_name}
    };
    
    // Serialize axioms
    j["axioms"] = nlohmann::json::array();
    for (const auto& axiom : state.axioms) {
        j["axioms"].push_back({
            {"id", axiom.id},
            {"position", {axiom.position.x, axiom.position.y}},
            {"type", magic_enum::enum_name(axiom.type)}
        });
    }
    
    // Serialize roads
    j["roads"] = nlohmann::json::array();
    for (const auto& road : state.roads) {
        j["roads"].push_back({
            {"id", road.id},
            {"vertices", SerializeVec2Array(road.vertices)},
            {"width", road.width},
            {"type", magic_enum::enum_name(road.type)}
        });
    }
    
    // Serialize districts
    j["districts"] = nlohmann::json::array();
    for (const auto& district : state.districts) {
        j["districts"].push_back({
            {"id", district.id},
            {"boundary", SerializeVec2Array(district.boundary)},
            {"type", magic_enum::enum_name(district.type)},
            {"aesp", {
                {"aesthetics", district.aesp.aesthetics},
                {"economy", district.aesp.economy},
                {"social", district.aesp.social},
                {"prestige", district.aesp.prestige}
            }}
        });
    }
    
    // Serialize lots
    j["lots"] = nlohmann::json::array();
    for (const auto& lot : state.lots) {
        j["lots"].push_back({
            {"id", lot.id},
            {"district_id", lot.district_id},
            {"boundary", SerializeVec2Array(lot.boundary)},
            {"area", lot.area}
        });
    }
    
    // Serialize buildings
    j["buildings"] = nlohmann::json::array();
    for (const auto& building : state.buildings) {
        j["buildings"].push_back({
            {"id", building.id},
            {"lot_id", building.lot_id},
            {"footprint", SerializeVec2Array(building.footprint)},
            {"height", building.height}
        });
    }
    
    // Serialize waterbodies
    j["waterbodies"] = nlohmann::json::array();
    for (const auto& water : state.waterbodies) {
        j["waterbodies"].push_back({
            {"id", water.id},
            {"type", magic_enum::enum_name(water.type)},
            {"boundary", SerializeVec2Array(water.boundary)},
            {"depth", water.depth}
        });
    }
    
    return j;
}

void CityExporter::ExportJSON(const GlobalState& state, const std::string& path) {
    nlohmann::json j = SerializeGlobalState(state);
    
    std::ofstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for JSON export: " + path);
    }
    
    file << j.dump(4);  // Pretty-print with 4-space indent
    file.close();
}
```


#### C. OBJ Export Implementation

```cpp
void CityExporter::ExportOBJ(const GlobalState& state, const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for OBJ export: " + path);
    }
    
    file << "# RogueCity Procedural City Export\n";
    file << "# Generated: " << GetCurrentISO8601Time() << "\n\n";
    
    int vertex_offset = 1;  // OBJ indices start at 1
    
    // Export buildings as extruded polygons
    for (const auto& building : state.buildings) {
        file << "# Building " << building.id << "\n";
        file << "o Building_" << building.id << "\n";
        
        // Bottom vertices
        for (const auto& v : building.footprint) {
            file << "v " << v.x << " 0.0 " << v.y << "\n";
        }
        
        // Top vertices (extruded by height)
        for (const auto& v : building.footprint) {
            file << "v " << v.x << " " << building.height << " " << v.y << "\n";
        }
        
        int n = building.footprint.size();
        
        // Bottom face
        file << "f";
        for (int i = 0; i < n; ++i) {
            file << " " << (vertex_offset + i);
        }
        file << "\n";
        
        // Top face
        file << "f";
        for (int i = 0; i < n; ++i) {
            file << " " << (vertex_offset + n + i);
        }
        file << "\n";
        
        // Side faces
        for (int i = 0; i < n; ++i) {
            int next = (i + 1) % n;
            file << "f " << (vertex_offset + i) << " "
                 << (vertex_offset + next) << " "
                 << (vertex_offset + n + next) << " "
                 << (vertex_offset + n + i) << "\n";
        }
        
        vertex_offset += 2 * n;
        file << "\n";
    }
    
    // Export roads as flat polygons
    for (const auto& road : state.roads) {
        file << "# Road " << road.id << "\n";
        file << "o Road_" << road.id << "\n";
        
        // Generate road polygon from centerline + width
        auto road_polygon = GenerateRoadPolygon(road);
        
        for (const auto& v : road_polygon) {
            file << "v " << v.x << " 0.1 " << v.y << "\n";  // Slight elevation
        }
        
        file << "f";
        for (size_t i = 0; i < road_polygon.size(); ++i) {
            file << " " << (vertex_offset + i);
        }
        file << "\n\n";
        
        vertex_offset += road_polygon.size();
    }
    
    file.close();
}
```


#### D. GLTF Export (Stub for Future)

```cpp
void CityExporter::ExportGLTF(const GlobalState& state, const std::string& path) {
    // For V1, implement basic geometry export
    // Future: Add PBR materials, animations, etc.
    
    // Use tinygltf library (add to vcpkg dependencies)
    tinygltf::Model model;
    tinygltf::Scene scene;
    
    // Build nodes for buildings, roads, etc.
    for (const auto& building : state.buildings) {
        tinygltf::Node node;
        node.name = "Building_" + std::to_string(building.id);
        
        // Create mesh from building geometry
        int mesh_index = CreateBuildingMesh(model, building);
        node.mesh = mesh_index;
        
        scene.nodes.push_back(model.nodes.size());
        model.nodes.push_back(node);
    }
    
    model.scenes.push_back(scene);
    model.defaultScene = 0;
    
    tinygltf::TinyGLTF loader;
    loader.WriteGltfSceneToFile(&model, path, false, true, true, false);
}
```


#### E. Export UI Panel

```cpp
// visualizer/src/ui/panels/rc_panel_export.cpp
void RcPanelExport::Render() {
    ImGui::Begin("Export City");
    
    ImGui::SeparatorText("Export Format");
    
    static int selected_format = 0;
    ImGui::RadioButton("JSON (Full Data)", &selected_format, 0);
    ImGui::RadioButton("OBJ (Geometry)", &selected_format, 1);
    ImGui::RadioButton("GLTF (PBR)", &selected_format, 2);
    
    ImGui::Spacing();
    
    // File path input
    static char file_path[256] = "export/city_export";
    ImGui::InputText("File Name", file_path, sizeof(file_path));
    
    ImGui::Spacing();
    
    if (ImGui::Button("Export", ImVec2(-1, 40))) {
        std::string full_path = std::string(file_path);
        
        try {
            switch (selected_format) {
                case 0:
                    exporter_->ExportJSON(*state_, full_path + ".json");
                    ShowNotification("Exported to JSON successfully");
                    break;
                case 1:
                    exporter_->ExportOBJ(*state_, full_path + ".obj");
                    ShowNotification("Exported to OBJ successfully");
                    break;
                case 2:
                    exporter_->ExportGLTF(*state_, full_path + ".gltf");
                    ShowNotification("Exported to GLTF successfully");
                    break;
            }
        } catch (const std::exception& e) {
            ShowError("Export failed: " + std::string(e.what()));
        }
    }
    
    ImGui::End();
}
```


### Success Criteria

✅ Can export to all 3 formats
✅ JSON can be re-imported without data loss
✅ OBJ opens correctly in Blender
✅ GLTF has valid structure

***

## TASK 2.4: AI Pattern Catalog Update

### Objectives

Update AI pattern catalog to recognize new UI components for automated refactoring.

### Implementation

#### File: `AI/docs/ui/ui_patterns.json`

```json
{
  "version": "1.0",
  "last_updated": "2026-02-08",
  "patterns": [
    {
      "name": "WaterControlPanel",
      "type": "ControlPanel",
      "role": "trigger_generation",
      "data_bindings": ["GlobalState.waterbodies"],
      "hfsm_states": ["WaterEditing"],
      "parameters": [
        {"name": "depth", "type": "float", "range": [1.0, 20.0]},
        {"name": "generate_shore", "type": "bool"}
      ],
      "refactor_opportunities": [
        "Template into FeatureControlPanel<WaterBody>",
        "Consolidate depth slider with other float parameters"
      ]
    },
    {
      "name": "BuildingControlPanel",
      "type": "ControlPanel",
      "role": "trigger_generation",
      "data_bindings": ["GlobalState.buildings"],
      "hfsm_states": ["BuildingPlacement"],
      "parameters": [
        {"name": "setback_min", "type": "float", "range": [0.0, 10.0]},
        {"name": "height_variance", "type": "float", "range": [0.0, 1.0]},
        {"name": "use_aesp_height", "type": "bool"}
      ],
      "refactor_opportunities": [
        "Template into FeatureControlPanel<Building>"
      ]
    },
    {
      "name": "LotControlPanel",
      "type": "ControlPanel",
      "role": "trigger_generation",
      "data_bindings": ["GlobalState.lots"],
      "hfsm_states": ["LotSubdivision"],
      "parameters": [
        {"name": "min_lot_area", "type": "float", "range": [50.0, 200.0]},
        {"name": "max_lot_area", "type": "float", "range": [200.0, 1000.0]},
        {"name": "respect_zoning", "type": "bool"}
      ],
      "refactor_opportunities": [
        "Template into FeatureControlPanel<Lot>"
      ]
    },
    {
      "name": "RcDataIndexPanel<District>",
      "type": "IndexPanel",
      "role": "data_display",
      "data_bindings": ["GlobalState.districts"],
      "context_menu_actions": [
        "ChangeType",
        "RecalculateBudget",
        "Delete"
      ],
      "refactor_opportunities": [
        "Add bulk selection for multi-district operations"
      ]
    },
    {
      "name": "RcDataIndexPanel<Lot>",
      "type": "IndexPanel",
      "role": "data_display",
      "data_bindings": ["GlobalState.lots"],
      "context_menu_actions": [
        "Merge",
        "Split",
        "Delete"
      ],
      "refactor_opportunities": [
        "Add multi-select for merge operations"
      ]
    },
    {
      "name": "WaterTool",
      "type": "InteractiveTool",
      "role": "geometry_editing",
      "modes": ["Lake", "River", "Ocean"],
      "input_methods": ["MouseDown", "MouseMove", "MouseUp", "KeyPress"],
      "refactor_opportunities": [
        "Generalize polygon drawing to shared PolygonEditTool base class"
      ]
    },
    {
      "name": "LotEditTool",
      "type": "InteractiveTool",
      "role": "geometry_editing",
      "operations": ["Merge", "Split", "Select"],
      "refactor_opportunities": [
        "Share selection logic with other edit tools"
      ]
    }
  ],
  "refactoring_suggestions": [
    {
      "priority": "high",
      "description": "Template ControlPanel pattern into FeatureControlPanel<T>",
      "affected_files": [
        "rc_panel_water_control.cpp",
        "rc_panel_building_control.cpp",
        "rc_panel_lot_control.cpp"
      ],
      "estimated_savings": "~200 lines of duplicate code"
    },
    {
      "priority": "medium",
      "description": "Create PolygonEditTool base class for shared polygon drawing logic",
      "affected_files": [
        "WaterTool.cpp",
        "DistrictTool.cpp",
        "LotEditTool.cpp"
      ],
      "estimated_savings": "~150 lines of duplicate code"
    }
  ]
}
```


### Success Criteria

✅ All new panels documented in pattern catalog
✅ UiDesignAssistant recognizes patterns
✅ Refactoring suggestions are actionable

***

## TASK 2.5: Testing \& Polish

### Objectives

Implement comprehensive test suite and polish UI for V1 release quality.

### A. Deterministic Generation Test

```cpp
// tests/test_full_pipeline.cpp
TEST_CASE("Full Pipeline Deterministic Generation") {
    GlobalState state;
    GeneratorBridge bridge(&state);
    
    // Set seed for deterministic output
    state.SetSeed(12345);
    
    // Execute full pipeline
    TensorFieldParams tensor_params;
    tensor_params.grid_size = 100;
    bridge.generateTensorField(tensor_params);
    
    RoadGenParams road_params;
    road_params.num_seeds = 10;
    bridge.traceRoads(road_params);
    
    DistrictParams district_params;
    district_params.min_district_size = 500.0f;
    bridge.subdivideDistricts(district_params);
    
    LotGenParams lot_params;
    lot_params.min_lot_area = 100.0f;
    lot_params.max_lot_area = 500.0f;
    bridge.generateLots(lot_params);
    
    BuildingPlacementParams building_params;
    building_params.setback_min = 3.0f;
    bridge.placeBuildingSites(building_params);
    
    // Verify output counts
    REQUIRE(state.roads.size() > 0);
    REQUIRE(state.districts.size() > 0);
    REQUIRE(state.lots.size() > 0);
    REQUIRE(state.buildings.size() > 0);
    
    // Verify determinism by running again with same seed
    GlobalState state2;
    GeneratorBridge bridge2(&state2);
    state2.SetSeed(12345);
    
    bridge2.generateTensorField(tensor_params);
    bridge2.traceRoads(road_params);
    bridge2.subdivideDistricts(district_params);
    bridge2.generateLots(lot_params);
    bridge2.placeBuildingSites(building_params);
    
    // Compare outputs (should be identical)
    REQUIRE(state.roads.size() == state2.roads.size());
    REQUIRE(state.districts.size() == state2.districts.size());
    REQUIRE(state.lots.size() == state2.lots.size());
    REQUIRE(state.buildings.size() == state2.buildings.size());
    
    // Hash comparison for byte-level identity
    uint64_t hash1 = HashGlobalState(state);
    uint64_t hash2 = HashGlobalState(state2);
    REQUIRE(hash1 == hash2);
}
```


### B. Performance Regression Test

```cpp
TEST_CASE("Performance Regression Baseline") {
    GlobalState state;
    GeneratorBridge bridge(&state);
    state.SetSeed(99999);
    
    // Benchmark: 1000-road city generation
    auto start = std::chrono::high_resolution_clock::now();
    
    TensorFieldParams tensor_params;
    tensor_params.grid_size = 200;
    bridge.generateTensorField(tensor_params);
    
    RoadGenParams road_params;
    road_params.num_seeds = 100;  // Should generate ~1000 road segments
    bridge.traceRoads(road_params);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Target: < 100ms for 1000-road city
    REQUIRE(duration.count() < 100);
    
    std::cout << "1000-road generation: " << duration.count() << "ms\n";
}
```


### C. Memory Budget Test

```cpp
TEST_CASE("Memory Budget Compliance") {
    GlobalState state;
    GeneratorBridge bridge(&state);
    
    // Generate large city
    state.SetSeed(11111);
    
    size_t initial_memory = GetProcessMemoryUsage();
    
    // Generate max-size city
    TensorFieldParams tensor_params;
    tensor_params.grid_size = 500;
    bridge.generateTensorField(tensor_params);
    
    RoadGenParams road_params;
    road_params.num_seeds = 500;
    bridge.traceRoads(road_params);
    
    // ... full pipeline
    
    size_t final_memory = GetProcessMemoryUsage();
    size_t memory_used = final_memory - initial_memory;
    
    // Target: < 500MB for large city
    REQUIRE(memory_used < 500 * 1024 * 1024);
    
    std::cout << "Memory used: " << (memory_used / (1024 * 1024)) << "MB\n";
}
```


### D. UI Responsiveness Test (Manual)

```cpp
// tests/manual/test_ui_responsiveness.cpp
// This is a manual test script, not automated

int main() {
    // Launch application in test mode
    RogueCityApp app;
    app.Initialize();
    
    // Test 1: Tool switching latency
    std::cout << "Test 1: Tool Switching\n";
    std::cout << "Click through all tool buttons rapidly.\n";
    std::cout << "Expected: < 16ms latency per switch (60 FPS)\n";
    
    // Test 2: Generation latency
    std::cout << "\nTest 2: Generation Latency\n";
    std::cout << "Trigger each generator (axiom, road, district, lot, building)\n";
    std::cout << "Expected: UI remains responsive, no blocking >10ms\n";
    
    // Test 3: Viewport overlay performance
    std::cout << "\nTest 3: Viewport Rendering\n";
    std::cout << "Generate large city (1000+ roads, 500+ buildings)\n";
    std::cout << "Expected: >30 FPS with all overlays enabled\n";
    
    app.Run();
    return 0;
}
```


### E. Polish Checklist

#### Float Narrowing Warnings

```bash
# Run final warning scan
cmake --build build --config Release 2>&1 | grep "warning C4244"
# Should output: 0 results
```


#### Tooltips

```cpp
// Add tooltips to all tool buttons
if (ImGui::Button("Axiom Tool")) {
    // ...
}
if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Place axioms to define tensor field seeds.\nHotkey: Ctrl+1");
}

// Repeat for all tools
```


#### Keyboard Shortcuts

```cpp
// app/src/Input/InputHandler.cpp
void InputHandler::ProcessKeyboard() {
    if (ImGui::IsKeyPressed(ImGuiKey_1) && ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
        hfsm_->TransitionTo(EditorMode::AxiomPlacement);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_2) && ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
        hfsm_->TransitionTo(EditorMode::RoadEditing);
    }
    // ... etc for tools 3-6
    
    // ESC = cancel current operation
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        current_tool_->CancelOperation();
    }
}
```


#### Progress Bars

```cpp
// For long operations, show progress
if (is_generating_lots_) {
    ImGui::ProgressBar(lot_generation_progress_, ImVec2(-1, 0));
}
```


#### Undo/Redo (Basic)

```cpp
// app/include/RogueCity/App/History/UndoStack.hpp
class UndoStack {
public:
    void PushState(const GlobalState& state);
    void Undo(GlobalState& state);
    void Redo(GlobalState& state);
    
    bool CanUndo() const { return undo_stack_.size() > 0; }
    bool CanRedo() const { return redo_stack_.size() > 0; }
    
private:
    std::vector<GlobalState> undo_stack_;
    std::vector<GlobalState> redo_stack_;
    size_t max_stack_size_ = 10;
};
```


### Success Criteria

✅ All tests pass
✅ No performance regressions
✅ UI feels polished and responsive
✅ Tooltips and shortcuts implemented

***

## TASK 2.6: Documentation \& _Temp Cleanup

### Objectives

Finalize V1 documentation and safely remove temporary files.

### A. Update `ReadMe.md`

```markdown
# RogueCity Urban Spatial Designer V1.0

## ✨ Features
- **Procedural Generation Pipeline**: Tensor fields → Roads → Districts → Lots → Buildings
- **Interactive Tools**: 6 editor modes (Axiom, Road, District, Lot, Building, Water)
- **Manual Editing**: Override district types, merge/split lots, adjust AESP budgets
- **Water Features**: Lakes, rivers, oceans with shore detail
- **Export System**: JSON, OBJ, GLTF formats
- **HFSM-Driven UI**: State-reactive panels and viewport overlays

## 🚀 Quick Start
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./bin/RogueCityApp.exe
```


## 📖 Documentation

- [V1 Complete Feature List](docs/V1_Complete.md)
- [Tool Usage Guide](docs/V1_Tool_Guide.md)
- [Export Workflow](docs/V1_Export_Guide.md)
- [AI Integration](docs/AI_Integration_Summary.md)


## 🎮 Controls

- **Ctrl+1-6**: Switch tools
- **ESC**: Cancel current operation
- **Enter**: Commit (rivers, polygons)
- **Right-click**: Context menu (districts, lots)


## 🧪 Testing

```bash
./bin/test_generators.exe
./bin/test_editor_hfsm.exe
```


## 📦 Export

1. Generate city
2. Open Export panel (File → Export)
3. Select format (JSON/OBJ/GLTF)
4. Click Export
```

### B. Create `docs/V1_Complete.md`
```markdown
# V1 Complete Feature List

## Generator Pipeline
### ✅ Tensor Field Generation
- Axiom-based seed placement
- Radial and grid decay patterns
- Interactive preview

### ✅ Road Tracing
- Streamline integration
- Road hierarchy (highway, main, residential)
- Intersection handling

### ✅ District Subdivision
- Voronoi-based zoning
- AESP budget calculation
- District type assignment (residential, commercial, industrial, park)

### ✅ Lot Generation
- Polygon subdivision within districts
- Area constraints (min/max)
- Zoning respect

### ✅ Building Placement
- Setback calculation
- AESP-driven height assignment
- Footprint generation within lots

### ✅ Water Features
- Lake polygon drawing
- River spline tracing
- Shore detail generation

## Editor Tools
### 1. Axiom Placement Tool
- Click to place axioms
- Adjust influence radius
- Realtime tensor field preview

### 2. Road Editing Tool
- Vertex dragging
- Road type change
- Width adjustment

### 3. District Zoning Tool
- Right-click context menu for type override
- AESP manual adjustment
- Budget recalculation

### 4. Lot Subdivision Tool
- Generate lots within districts
- Multi-select for merging
- Line-split tool

### 5. Building Placement Tool
- Setback control
- Height variance adjustment
- AESP toggle

### 6. Water/River Editing Tool
- Lake: Polygon drawing with auto-close
- River: Spline control points
- Depth and shore settings

## Export System
### JSON
- Full city data (save/load compatible)
- Metadata (seed, timestamp)
- All entities (axioms, roads, districts, lots, buildings, water)

### OBJ
- Blender-compatible geometry
- Buildings as extruded polygons
- Roads as flat meshes

### GLTF
- PBR materials (future expansion)
- Scene hierarchy
- Animation support (future)

## UI Features
### Cockpit Doctrine Compliance
- State-reactive panels
- Y2K motion affordances (pulsing buttons)
- Context menus for quick actions

### Data Index Panels
- Filterable lists (districts, lots, buildings)
- Right-click context menus
- Selection highlighting in viewport

### Viewport Overlays
- Layered rendering (roads → districts → lots → buildings → water)
- Mode-specific visibility
- Selection highlighting

## Performance Targets
- **1000-road city**: < 100ms generation
- **Large city memory**: < 500MB
- **UI responsiveness**: >30 FPS with 500+ buildings
- **Tool switching**: < 16ms latency

## Known Limitations
- Undo/redo: Basic implementation (10-state stack)
- GLTF export: Geometry only (PBR materials in future)
- Water physics: Static only (no flow simulation)
```


### C. Create `docs/V1_Tool_Guide.md`

```markdown
# V1 Tool Usage Guide

## Axiom Placement Tool (Ctrl+1)
**Purpose**: Define seeds for tensor field generation

**Workflow**:
1. Click "Axiom Tool" or press Ctrl+1
2. Click in viewport to place axioms
3. Adjust influence radius in control panel
4. Click "Generate Tensor Field"
5. Preview field overlays in viewport

**Parameters**:
- Influence Radius: 50-500m (how far axiom affects field)
- Decay Type: Radial or Grid
- Intensity: 0.0-1.0 (field strength)

---

## Road Editing Tool (Ctrl+2)
**Purpose**: Manually adjust generated roads

**Workflow**:
1. Generate tensor field and roads first
2. Switch to Road Tool (Ctrl+2)
3. Click road to select
4. Drag vertices to adjust path
5. Right-click for context menu:
   - Change Type (Highway/Main/Residential)
   - Adjust Width
   - Delete Road

**Tips**:
- Hold Shift to multi-select roads
- Use Ctrl+Z to undo changes

---

## District Zoning Tool (Ctrl+3)
**Purpose**: Assign district types and AESP budgets

**Workflow**:
1. Generate roads and districts first
2. Switch to District Tool (Ctrl+3)
3. Click district in viewport or index panel
4. Right-click for context menu:
   - Change to Residential/Commercial/Industrial/Park
   - Recalculate Budget
   - Delete District
5. Use inspector panel to manually adjust AESP values

**AESP Components**:
- Aesthetics: Visual appeal (parks, landscaping)
- Economy: Commercial activity (shops, offices)
- Social: Community spaces (schools, centers)
- Prestige: High-value development (luxury buildings)

---

## Lot Subdivision Tool (Ctrl+4)
**Purpose**: Generate and edit lots within districts

**Workflow**:
1. Generate districts first
2. Switch to Lot Tool (Ctrl+4)
3. Adjust parameters:
   - Min/Max Area
   - Respect Zoning toggle
4. Click "Generate Lots"
5. Multi-select lots (Shift+Click) to merge
6. Use line-split tool to subdivide

**Operations**:
- **Merge**: Select 2+ lots, right-click → Merge
- **Split**: Select lot, activate line tool, draw split line

---

## Building Placement Tool (Ctrl+5)
**Purpose**: Generate buildings within lots

**Workflow**:
1. Generate lots first
2. Switch to Building Tool (Ctrl+5)
3. Adjust parameters:
   - Setback Min (distance from lot boundary)
   - Height Variance (randomness)
   - Use AESP Height (tie to district budget)
4. Click "Place Buildings"
5. Click building for inspector panel

---

## Water/River Editing Tool (Ctrl+6)
**Purpose**: Add water features (lakes, rivers, oceans)

**Workflow**:

### Lake Mode
1. Select "Lake" in control panel
2. Click in viewport to place boundary points
3. Click near first point to auto-close polygon
4. Adjust depth and shore settings
5. Polygon commits to GlobalState

### River Mode
1. Select "River" in control panel
2. Click to place spline control points
3. Press Enter to finish river (ESC to cancel)
4. River generates with automatic banks

### Parameters
- Depth: 1-20m (visual only, no physics)
- Generate Shore: Adds coastal detail vertices

---

## General Tips
- **ESC**: Cancel current operation
- **Ctrl+Z / Ctrl+Y**: Undo / Redo
- **Ctrl+S**: Quick save to JSON
- **Ctrl+E**: Open export panel
- **F**: Frame selected in viewport
- **G**: Toggle grid overlay
```


### D. Create `docs/V1_Export_Guide.md`

```markdown
# V1 Export Workflow Guide

## JSON Export (Save/Load)
**Use Case**: Save city for later editing or backup

**Steps**:
1. File → Export → Select JSON
2. Enter filename (e.g., `my_city_v1`)
3. Click Export
4. File saved to `export/my_city_v1.json`

**To Re-Import**:
1. File → Import → Select JSON file
2. City loads into GlobalState
3. Continue editing

**JSON Structure**:
```json
{
  "version": "1.0",
  "metadata": {
    "seed": 12345,
    "generation_time": "2026-02-08T12:00:00Z",
    "city_name": "Procedural City"
  },
  "axioms": [...],
  "roads": [...],
  "districts": [...],
  "lots": [...],
  "buildings": [...],
  "waterbodies": [...]
}
```


---

## OBJ Export (Blender Import)

**Use Case**: Import geometry into Blender for rendering

**Steps**:

1. File → Export → Select OBJ
2. Enter filename (e.g., `city_mesh`)
3. Click Export
4. File saved to `export/city_mesh.obj`

**In Blender**:

1. File → Import → Wavefront (.obj)
2. Select `city_mesh.obj`
3. Geometry loads as separate objects:
    - `Building_001`, `Building_002`, ...
    - `Road_001`, `Road_002`, ...

**Tips**:

- Buildings are extruded polygons (height applied)
- Roads are flat meshes (0.1m elevation for Z-fighting)
- No materials or textures (apply in Blender)

---

## GLTF Export (PBR-Ready)

**Use Case**: Export for game engines (Unity, Unreal) or web viewers

**Steps**:

1. File → Export → Select GLTF
2. Enter filename (e.g., `city_scene`)
3. Click Export
4. File saved to `export/city_scene.gltf`

**Current Limitations (V1)**:

- Geometry only (no PBR materials yet)
- No animations
- Basic scene hierarchy

**Future Enhancements (V2)**:

- PBR materials (albedo, roughness, metallic)
- Baked lighting
- LOD (Level of Detail) meshes

---

## Batch Export Script

**For exporting all formats at once**:

```bash
# PowerShell script: export_all.ps1
param([string]$CityName = "my_city")

Write-Host "Exporting $CityName to all formats..."

# Launch app in headless mode with export flags
.\\bin\\RogueCityApp.exe --headless --export-json "export/$CityName.json" `
                                    --export-obj "export/$CityName.obj" `
                                    --export-gltf "export/$CityName.gltf"

Write-Host "Export complete!"
```

**Usage**:

```powershell
.\\export_all.ps1 -CityName "procedural_city_001"
```

```

### E. _Temp Cleanup Gate

#### Verification Checklist
Before deleting `_Temp/`:
- [ ] All generators ported to `generators/`
- [ ] All tests pass (run full suite)
- [ ] Export system works (verify JSON import roundtrip)
- [ ] Documentation complete (ReadMe, V1 docs)
- [ ] No references to `_Temp/` in CMakeLists.txt or code

#### Cleanup Script
```powershell
# cleanup_temp.ps1
param([switch]$Force)

# Safety check
if (-not $Force) {
    Write-Host "This will delete the _Temp/ directory permanently."
    Write-Host "Run with -Force to confirm deletion."
    exit 1
}

# Verify tests pass
Write-Host "Running tests before cleanup..."
.\\bin\\test_generators.exe
if ($LASTEXITCODE -ne 0) {
    Write-Host "Tests failed! Aborting cleanup."
    exit 1
}

# Move to _Del/ for recovery if needed
if (Test-Path "_Temp") {
    Move-Item "_Temp" "_Del/_Temp_$(Get-Date -Format 'yyyyMMdd_HHmmss')"
    Write-Host "_Temp moved to _Del/ (safe to delete after verification)"
}

Write-Host "Cleanup complete!"
```

**Usage**:

```powershell
.\\cleanup_temp.ps1 -Force
```


### Success Criteria

✅ Documentation matches implementation
✅ _Temp/ moved to _Del/
✅ Build instructions verified
✅ All links in docs are valid

***

## PASS 2 VALIDATION CHECKLIST

Before marking V1 complete:

### Water/River Editing

- [ ] Can draw lake boundaries
- [ ] Can place river control points
- [ ] Shore generation works
- [ ] Water renders correctly in viewport
- [ ] Changes commit to GlobalState


### District/Lot Editing

- [ ] Can change district types via context menu
- [ ] Can merge multiple lots
- [ ] Can split lots with line tool
- [ ] AESP adjustments propagate to buildings
- [ ] Manual edits persist after save/load


### Export System

- [ ] JSON export works
- [ ] JSON import restores city exactly
- [ ] OBJ export works
- [ ] OBJ opens correctly in Blender
- [ ] GLTF export generates valid file


### AI Integration

- [ ] Pattern catalog includes all new panels
- [ ] UiDesignAssistant recognizes patterns
- [ ] Refactoring suggestions are actionable


### Testing

- [ ] Deterministic generation test passes
- [ ] Performance regression test passes
- [ ] Memory budget test passes
- [ ] UI responsiveness manually verified


### Polish

- [ ] All float narrowing warnings fixed
- [ ] Tooltips added to all tools
- [ ] Keyboard shortcuts work (Ctrl+1-6, ESC, Enter)
- [ ] Progress bars show for long operations
- [ ] Undo/redo basic implementation works


### Documentation

- [ ] ReadMe.md updated with V1 features
- [ ] V1_Complete.md written
- [ ] V1_Tool_Guide.md created
- [ ] V1_Export_Guide.md created
- [ ] All docs have valid links
- [ ] _Temp/ cleanup complete

***

## Execution Commands

```bash
# 1. Create PASS 2 branch
git checkout -b pass2-features-export

# 2. Build and verify
cmake --build build --config Release

# 3. Run full test suite
./bin/test_generators.exe
./bin/test_editor_hfsm.exe
./bin/test_viewport.exe

# 4. Manual UI testing
./bin/RogueCityApp.exe

# 5. Export verification
# - Generate city
# - Export to JSON/OBJ/GLTF
# - Import JSON, verify identical

# 6. Documentation review
# - Open all docs/*.md files
# - Verify formatting, links, accuracy

# 7. Cleanup
./cleanup_temp.ps1 -Force

# 8. Commit
git add .
git commit -m "PASS 2: V1 feature completion + export + polish"

# 9. Merge to main
git checkout main
git merge pass2-features-export
git tag v1.0.0
```


***

## Expected Time

**19-27 hours** (concentrated implementation)

***

## Notes for Claude

- **Water Tool**: Focus on polygon drawing first (lake mode), then spline (river mode)
- **Export**: JSON is highest priority (enables save/load), OBJ second (Blender workflow)
- **Testing**: Automate what you can, manual tests for UI feel
- **Documentation**: Be thorough—good docs prevent future confusion
- **Cleanup**: Do NOT delete _Temp/ until ALL tests pass and docs are verified

***

## Final V1 Deliverable

When PASS 2 is complete, you will have:

1. ✅ Fully functional procedural city generator
2. ✅ 6 interactive editor tools (HFSM-driven)
3. ✅ Manual editing capabilities (districts, lots)
4. ✅ Water features (lakes, rivers)
5. ✅ Export system (JSON/OBJ/GLTF)
6. ✅ Comprehensive documentation
7. ✅ Clean codebase (no _Temp/)

**Congratulations on V1 completion!** 🎉

***

This completes the two-pass implementation plan. Each pass is designed to be self-contained with clear objectives, implementation patterns, success criteria, and validation gates. The prompts provide sufficient context for Claude Sonnet 4.5 to implement the features independently while maintaining consistency with your existing codebase architecture.

