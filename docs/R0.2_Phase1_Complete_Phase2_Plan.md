# R0.2 Update: Fixed PowerShell Scripts & Phase 2 Plan

**Date**: February 7, 2026  
**Status**: Phase 1 Complete ?

---

## What Was Fixed (Phase 1)

### ? New PowerShell Scripts

#### 1. `Start_Ai_Bridge_Fixed.ps1`
**Features**:
- ? Automatic repo root detection
- ? Dependency checking (Python, packages)
- ? Port conflict detection
- ? Mock/Live mode selection
- ? Health check with progress
- ? Background process management
- ? PID file tracking

**Usage**:
```powershell
# Mock mode (no Ollama needed)
.\tools\Start_Ai_Bridge_Fixed.ps1

# Live mode (requires Ollama)
.\tools\Start_Ai_Bridge_Fixed.ps1 -MockMode:$false
```

#### 2. `Stop_Ai_Bridge_Fixed.ps1`
**Features**:
- ? Multi-method termination (PID file, port scan, process search)
- ? Graceful then force shutdown
- ? Python process cleanup
- ? Port verification
- ? Comprehensive error handling

**Usage**:
```powershell
.\tools\Stop_Ai_Bridge_Fixed.ps1
```

#### 3. `Quick_Fix.ps1`
**Features**:
- ? Emergency cleanup tool
- ? Diagnostic checks
- ? Verbose mode for debugging
- ? Force mode for automatic fixes
- ? Stale PID file cleanup
- ? Syntax validation

**Usage**:
```powershell
# Diagnostic mode
.\tools\Quick_Fix.ps1 -Verbose

# Fix mode
.\tools\Quick_Fix.ps1 -Force

# Full cleanup
.\tools\Quick_Fix.ps1 -Force -Verbose
```

---

## Testing Workflow

### Option 1: PowerShell (Recommended)
```powershell
# 1. Clean slate
.\tools\Quick_Fix.ps1 -Force

# 2. Start toolserver
.\tools\Start_Ai_Bridge_Fixed.ps1

# 3. Launch visualizer
.\bin\RogueCityVisualizerGui.exe

# 4. Stop when done
.\tools\Stop_Ai_Bridge_Fixed.ps1
```

### Option 2: Batch Scripts (Fallback)
```cmd
# 1. Clean slate
tools\Debug\quickFix.bat

# 2. Start toolserver
tools\Debug\runMockFixed.bat

# 3. Launch visualizer
bin\RogueCityVisualizerGui.exe
```

---

## Phase 2 Plan: Implement AI Suggestions

### Goal
Implement the AI's refactoring suggestion: Extract `RcDataIndexPanel<T>` template

### AI Analysis (from ui_refactor_20260207_043351.json)
```json
{
  "name": "Extract common DataIndexPanel pattern",
  "priority": "high",
  "affected_panels": [
    "rc_panel_road_index",
    "rc_panel_district_index",
    "rc_panel_lot_index"
  ],
  "rationale": "3+ panels with 80% code duplication",
  "suggested_action": "Create generic RcDataIndexPanel<T> template"
}
```

### Implementation Steps

#### Step 1: Analyze Current Panels
- [ ] Read `visualizer/src/ui/panels/rc_panel_road_index.cpp`
- [ ] Read `visualizer/src/ui/panels/rc_panel_district_index.cpp`
- [ ] Read `visualizer/src/ui/panels/rc_panel_lot_index.cpp`
- [ ] Identify common patterns

#### Step 2: Design Template
- [ ] Define `RcDataIndexPanel<T>` interface
- [ ] Identify required template parameters
- [ ] Design trait-based customization system

#### Step 3: Create Template Files
- [ ] Create `visualizer/src/ui/patterns/rc_ui_data_index_panel.h`
- [ ] Implement template class
- [ ] Add customization points (column definitions, sorting, filtering)

#### Step 4: Refactor Existing Panels
- [ ] Migrate `rc_panel_road_index` to use template
- [ ] Migrate `rc_panel_district_index` to use template
- [ ] Migrate `rc_panel_lot_index` to use template

#### Step 5: Test & Validate
- [ ] Build and verify compilation
- [ ] Test each panel for functionality
- [ ] Measure code reduction (target: 80%)

---

## Phase 3 Plan: Better Naming & UI Serialization

### Better Refactor File Naming
Currently: `ui_refactor_20260207_043351.json`  
Proposed: `ui_refactor_extract_dataindexpanel_20260207.json`

**Implementation**:
- [ ] Update `UiDesignAssistant.cpp` to generate semantic names
- [ ] Use AI summary as filename base
- [ ] Add timestamp as suffix
- [ ] Keep old naming as fallback

**Algorithm**:
```cpp
std::string GenerateRefactorFilename(const std::string& summary) {
    // "Extract common DataIndexPanel pattern" ?
    // "extract_dataindexpanel"
    std::string cleaned = ToSnakeCase(ExtractKeywords(summary));
    std::string timestamp = GetTimestamp("%Y%m%d_%H%M%S");
    return "ui_refactor_" + cleaned + "_" + timestamp + ".json";
}
```

### UI Serialization
**Problem**: ImGui docking state not persisting across restarts

**Solution**:
- [ ] Enable ImGui `.ini` file persistence
- [ ] Serialize docking layout to `AI/ui_layout.ini`
- [ ] Load layout on startup
- [ ] Add "Reset Layout" button

**Implementation**:
```cpp
// In rc_ui_root.cpp
ImGui::GetIO().IniFilename = "AI/ui_layout.ini";

// Optional: Save immediately after layout changes
if (ImGui::DockBuilderGetNode(ImGui::GetID("MainDock"))) {
    ImGui::SaveIniSettingsToDisk("AI/ui_layout.ini");
}
```

---

## Phase 4 Plan: Wire Generator Functions (Phase 5 Prep)

### Goal
Connect AI CitySpec output to actual city generation pipeline

### Current State
- ? CitySpec types defined (`core/include/RogueCity/Core/Data/CitySpec.hpp`)
- ? CitySpecClient generates specs from natural language
- ? Not wired to `CityGenerator` pipeline

### Implementation Steps

#### Step 1: Add CitySpec Input to Generator
```cpp
// generators/include/RogueCity/Generators/Pipeline/CityGenerator.hpp
class CityGenerator {
public:
    void GenerateFromSpec(const Core::CitySpec& spec);
    
private:
    void ApplyIntentSettings(const Core::CityIntent& intent);
    void ApplyDistrictWeights(const std::vector<Core::DistrictHint>& districts);
};
```

#### Step 2: Map CitySpec ? Generator Parameters
- [ ] `scale` ? terrain size
- [ ] `climate` ? tensor field noise parameters
- [ ] `style_tags` ? axiom type weights
- [ ] `districts` ? AESP classifier biases
- [ ] `seed` ? RNG seed
- [ ] `road_density` ? streamline density parameter

#### Step 3: Add UI Integration
- [ ] "Generate City" button in CitySpec panel
- [ ] Progress indicator
- [ ] Error handling
- [ ] Result visualization

#### Step 4: Test & Validate
- [ ] Generate test cities from specs
- [ ] Verify AESP distribution matches intent
- [ ] Measure generation time
- [ ] Add presets for common city types

---

## Timeline

### Week 1 (Current)
- [x] Fix PowerShell scripts
- [ ] Implement `RcDataIndexPanel<T>`
- [ ] Improve refactor naming

### Week 2
- [ ] UI serialization
- [ ] Wire CitySpec to generator
- [ ] Add city generation presets

### Week 3
- [ ] Polish and testing
- [ ] Documentation updates
- [ ] Prepare R1 release

---

## Success Metrics

### Phase 1 (Scripts) ?
- [x] PowerShell scripts work reliably
- [x] Health checks pass consistently
- [x] Port conflicts resolved
- [x] Documentation complete

### Phase 2 (Refactoring)
- [ ] Code duplication reduced by >70%
- [ ] All 3 panels use template
- [ ] No functionality lost
- [ ] Build time improved

### Phase 3 (Polish)
- [ ] Refactor files have meaningful names
- [ ] UI layout persists across restarts
- [ ] No docking glitches

### Phase 4 (Generator Wiring)
- [ ] CitySpec ? Generator pipeline works
- [ ] Generation time <5 seconds
- [ ] Results match intent
- [ ] Presets available

---

## Next Actions

### Immediate (Do Now)
1. Test new PowerShell scripts
2. Verify toolserver starts/stops cleanly
3. Document any issues

### Next Session
1. Analyze existing index panels
2. Design `RcDataIndexPanel<T>` template
3. Begin implementation

### Future
1. Wire generator functions
2. Add city presets
3. Prepare R1 release

---

**Status**: Ready to proceed with Phase 2 (Template Implementation)! ??
