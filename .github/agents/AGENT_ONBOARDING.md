# RogueCity Agent Onboarding Guide

**Last Updated**: February 16, 2026  
**For**: AI Agents working on RogueCities_UrbanSpatialDesigner

---

## 🎯 Quick Start Checklist

Before working on ANY task, ensure you understand:

- [ ] **The 3-Layer Architecture**: Core → Generators → App (strict separation)
- [ ] **The Rogue Protocol**: Performance mandates (FVA/SIV/CIV/RogueWorker)
- [ ] **The Cockpit Doctrine**: UI/UX design philosophy (state-reactive, motion-driven)
- [ ] **Agent Roles**: Which specialist agent handles which domain
- [ ] **Key Contracts**: Generator-Viewport, Tool-Wiring, UI Patterns

---

## 📚 Architecture Foundation

### The Three-Layer Law (NEVER VIOLATE)

```
┌─────────────────────────────────────────────┐
│  Layer 2: App (Editor + UI + HFSM)         │  Dependencies: Core + Generators
│  Files: app/*, visualizer/*                 │  Uses: ImGui, OpenGL, GLFW
├─────────────────────────────────────────────┤
│  Layer 1: Generators (Algorithms)          │  Dependencies: Core only
│  Files: generators/*                        │  Uses: Math, data structures
├─────────────────────────────────────────────┤
│  Layer 0: Core (Data + Math)               │  Dependencies: Math libs only
│  Files: core/*                              │  FORBIDDEN: ImGui, OpenGL, GLFW
└─────────────────────────────────────────────┘
```

**CRITICAL RULE**: Core MUST NOT depend on UI libraries. Build fails if violated.

### The Rogue Protocol (Performance Mandates)

#### Container Selection Matrix

| Container | Use Case | When to Use | Aliased As |
|-----------|----------|-------------|------------|
| **FVA** (FastVectorArray) | Roads, Districts, UI entities | Need stable IDs for ImGui Inspector | `FVA<T>` |
| **SIV** (StableIndexVector) | Buildings, Agents, Props | High-churn entities, need validity checks | `SIV<T>` + `SIVHandle<T>` |
| **CIV** (ConstantIndexVector) | Internal calculations | Pathfinding, scratch buffers, temp geometry | `CIV<T>` + `CIVRef<T>` |
| **std::vector** | Simple data | ❌ FORBIDDEN for entities exposed to UI/Editor | - |

**Rule**: If it has an ID the user can see/select → FVA. If it's high-churn → SIV. If it's internal only → CIV.

#### Threading Mandates

| Tool | Use Case | When to Use | Threshold |
|------|----------|-------------|-----------|
| **RogueWorker** | Heavy parallel work | Operations >10ms (tensor fields, road tracing) | `N = axioms × districts × density > 100` |
| **Main Thread** | UI updates | All ImGui rendering, state transitions | Always |
| **Context-Aware** | Smart threading | Enable threading only when complexity justifies overhead | Calculate N, compare to threshold |

**Rule**: "If it loops over the grid, thread it. If it touches GPU, queue it."

---

## 🤝 Agent Roles & Responsibilities

### The Architect (Master Director)

**YOU ARE HERE** when coordinating multi-agent tasks or making architectural decisions.

**Authority**: Final say on architecture, performance, and design compliance  
**Tools**: Can delegate to all specialist agents  
**Documents**: Enforces adherence to `TheRogueCityDesignerSoft.md` and Rogue Protocol

### Specialist Agents (When to Consult)

#### 1. **Coder Agent** 
**When**: Implementing C++ code, adding features, refactoring
- Layer separation (Core/Generators/App)
- CMake build hygiene
- Memory safety and performance
- Test implementation
- **Handoff Question**: "What GlobalState containers and metadata fields do you need?"

#### 2. **Math Genius Agent**
**When**: Formulas, metrics, tensor fields, AESP calculations
- Grid Index metrics (Straightness, Orientation, Intersection)
- AESP formula validation
- Numerical stability
- Performance optimization of math kernels
- **Must Read**: `docs/TheRogueCityDesignerSoft.md` (single source of truth for formulas)

#### 3. **City Planner Agent**
**When**: District types, zoning logic, urban design semantics
- AESP zoning behavior (Access, Exposure, Serviceability, Privacy)
- District archetypes (Residential, Commercial, Industrial, Civic, Mixed-Use)
- Player flow and readability
- Axiom influence rules
- **Must Read**: Design doc section on AESP Standard

#### 4. **Resource Manager Agent**
**When**: Memory budgets, performance limits, capacity planning
- Set caps for roads/districts/lots
- Tune config defaults
- Prevent data structure blowups
- Generate test cases for Debug Manager
- **Monitors**: Entity counts, spatial index sizes, generation costs

#### 5. **Debug Manager Agent**
**When**: Diagnostics, profiling, reproducibility, testing
- HFSM transition tests
- Deterministic runs (seeded RNG)
- Performance snapshots
- Regression test maintenance
- **Works With**: Resource Manager for function validation

#### 6. **Documentation Keeper Agent**
**When**: Updating docs, build instructions, architecture diagrams
- Keep `ReadMe.md` aligned with code
- Maintain architecture diagrams
- Update build instructions
- Document design rationale
- **Suggests**: Add glossaries, improve readability

#### 7. **UI/UX/ImGui/ImVue Master**
**When**: UI design, ImGui panels, motion design, editor experience
- **The Cockpit Doctrine**: State-reactive, motion-driven UI
- **Design Language**: Vignelli structure + Y2K geometry + LCARS segmentation
- **Sacred Principles**: Viewport is sacred, Tools are tactile, Properties are contextual
- **Enforces**: No static UIs, motion with purpose, structural honesty
- **Critical**: Ensure Core stays UI-free, validate HFSM integration

#### 8. **AI Integration Agent**
**When**: AI toolserver, protocol types, assistant workflows
- Maintain `tools/toolserver.py` endpoints
- Ensure protocol backward compatibility
- Update pattern catalog (`AI/docs/ui/ui_patterns.json`)
- Mock mode testing
- **Key Files**: `AI/protocol/*`, `AI/client/*`, `tools/toolserver.py`

#### 9. **Commenter/API Alias Keeper**
**When**: Lua bindings, API stability, signature preservation
- Maintain API comment blocks
- Track deprecated names
- Define alias maps for Lua
- **Future**: Will coordinate with Lua Overseer Agent

---

## 🔧 Development Workflows

### Adding a New Generator (8-Step Canonical Pattern)

Follow this workflow for any new generator (Zoning, Lots, Buildings, etc.):

#### Step 1: Define Generator Pipeline (Generators Layer)
**Pattern**: Mirror `CityGenerator.hpp`  
**Location**: `generators/include/RogueCity/Generators/Pipeline/`  
**Create**: Config struct, Input struct, Output struct, Pipeline methods  
**Performance**: Use RogueWorker for >10ms operations  
**Handoff**: "What GlobalState containers do you need?"

#### Step 2: Extend GlobalState (Core Layer)
**File**: `core/include/RogueCity/Core/Editor/GlobalState.hpp`  
**Add**: FVA for UI-stable entities, SIV for high-churn  
**Handoff**: "What bridge API do you need?"

#### Step 3: Create Generator Bridge (App Layer)
**Pattern**: Mirror `GeneratorBridge.hpp`  
**Location**: `app/include/RogueCity/App/Integration/`  
**Purpose**: UI parameters → Generator inputs → GlobalState population  
**Handoff**: "What HFSM states do you need?"

#### Step 4: Add HFSM States (App Layer)
**Files**: `app/include/RogueCity/App/HFSM/HFSM.hpp`, `app/src/HFSM/HFSMStates.cpp`  
**Rule**: Keep transitions <10ms, offload to RogueWorker  
**Testing**: Add tests in `tests/test_editor_hfsm.cpp`  
**Handoff**: "What UI panels do you need?"

#### Step 5: Create Index Panels (UI Layer)
**Pattern**: Use `RcDataIndexPanel<T>` template  
**Location**: `visualizer/src/ui/patterns/rc_ui_data_index_panel.h`  
**Add**: Context menus with HFSM hooks  
**Handoff**: "What viewport overlays do you need?"

#### Step 6: Implement Viewport Overlays (Visualizer Layer)
**File**: `visualizer/src/ui/rc_viewport_renderer.cpp`  
**Add**: Colors, heat maps, labels, indicators  
**Handoff**: "What control panel parameters do you need?"

#### Step 7: Add Generator Control Panel (UI Layer)
**Pattern**: Follow ControlPanel pattern from `AI/docs/ui/ui_patterns.json`  
**Y2K Affordances**: Glow on hover, pulse on change, state-based colors  
**Handoff**: "What pattern catalog metadata is needed?"

#### Step 8: Update AI Pattern Catalog (AI Layer)
**File**: `AI/docs/ui/ui_patterns.json`  
**Purpose**: Enable UiDesignAssistant code-shape awareness  
**Add**: Pattern metadata for refactoring suggestions

### Build Workflows

#### Windows Dev Shell Verification (VSCode Integrated Terminal)
```cmd
where cl
where msbuild
echo %VSCMD_VER%
```

Expected baseline:
- `cl` resolves under `C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\...`
- `msbuild` resolves from Visual Studio/MSBuild path
- `%VSCMD_VER%` is set (example: `18.2.1`)

If these fail in VSCode, reload the window and open a fresh terminal so `.vscode/vsdev-init.cmd` re-initializes the toolchain.

#### Standard Build (All Targets)
```powershell
# Configure
cmake -B build -S .

# Build Core
cmake --build build --target RogueCityCore --config Release

# Build Generators
cmake --build build --target RogueCityGenerators --config Release

# Build GUI
cmake --build build --target RogueCityVisualizerGui --config Release
```

#### MSBuild Diagnostics Policy (Windows)
```cmd
:: Normal/default diagnostics + binary log
msbuild build_vs\RogueCities.sln /restore /t:Build /m /v:minimal /bl:artifacts\build.binlog

:: Deep diagnostics only when investigating failures
msbuild build_vs\RogueCities.sln /restore /t:Build /m /v:diag /fl /flp:logfile=artifacts\build.diag.log;verbosity=diagnostic
```

#### Fast Core-Only Iteration
```powershell
cmake -B build_core -S . -DBUILD_CORE_ONLY=ON
cmake --build build_core --target RogueCityCore --config Release
```

#### Visual Studio Solution
```powershell
start build_vs\RogueCities.sln
# Set RogueCityVisualizerGui as startup project
# Press F5 to build and run
```

#### Run Tests
```powershell
# Build tests
cmake --build build --target test_generators --config Release

# Run via CTest
ctest --test-dir build --output-on-failure

# Or run directly
.\build\Release\test_generators.exe
```

### Diagnostics Toolchain (Agent-RC Standard)

Use this sequence whenever diagnostics drift from build reality:

```powershell
# 1) Verify environment/toolchain + workspace wiring
python tools/env_doctor.py

# 2) Enforce clang/builder contract
python tools/check_clang_builder_contract.py

# 3) Triage current VS Code Problems export
python tools/problems_triage.py --input .vscode/problems.export.json

# 4) Diff against previous snapshot and store current snapshot
python tools/problems_diff.py --current .vscode/problems.export.json --snapshot-current

# 5) One-click refresh (configure + compile db + build + diff + triage)
python tools/dev_refresh.py --configure-preset dev --build-preset gui-release
```

Notes:
- Problems export source: `.vscode/problems.export.json`
- Snapshot history: `.vscode/problems-history/`
- If export is missing, run VS Code command: `Problems Bridge: Export Diagnostics Now`

### AI Toolserver Workflows

#### Quick Start (3 Steps)
```powershell
# Step 1: Clean
.\tools\Quick_Fix.ps1 -Force

# Step 2: Start Bridge
.\tools\Start_Ai_Bridge_Fixed.ps1

# Step 3: Launch Visualizer
.\bin\RogueCityVisualizerGui.exe
```

#### Troubleshooting
```powershell
# Stop all
.\tools\Stop_Ai_Bridge_Fixed.ps1

# Check health
curl http://127.0.0.1:7077/health

# Kill everything (nuclear option)
taskkill /F /IM python.exe
taskkill /F /IM RogueCityVisualizerGui.exe
```

---

## 📋 Contracts & Compliance

### Generator-Viewport Contract

**Single Source of Truth**: `RogueCity::App::ApplyCityOutputToGlobalState`

**Invariants**:
1. No panel-local sync implementations allowed
2. All generator completion must call `ApplyCityOutputToGlobalState`
3. Live preview uses coordinator APIs only
4. Viewport index rebuild controlled by applier options
5. Axiom input stays in viewport interaction utilities

**Enforcement**: `python3 tools/check_generator_viewport_contract.py`

### Tool-Wiring Contract

**Single Entry Point**: `DispatchToolAction(action_id, context)`

**Key Types**:
- `ToolActionId`: Stable ID for every action
- `ToolExecutionPolicy`: `ActivateOnly` (safe default), `ActivateAndExecute` (opt-in), `Disabled` (visible stub)
- `ToolAvailability`: `Available` or `Disabled`

**Rules**:
1. Every tool click routes through dispatcher
2. Disabled actions include explicit reason
3. No destructive generation for `ActivateOnly` actions
4. Runtime state stored in `GlobalState::tool_runtime`

**Enforcement**: `python3 tools/check_tool_wiring_contract.py`

### UI Patterns Contract

**Panel Shell Pattern**:
```cpp
const bool open = RC_UI::Components::BeginTokenPanel(
    "Panel Name",
    RC_UI::UITokens::CyanAccent,
    nullptr,
    ImGuiWindowFlags_NoCollapse);
if (!open) {
    RC_UI::Components::EndTokenPanel();
    return;
}
// content...
RC_UI::Components::EndTokenPanel();
```

**Dockable Window Pattern**:
```cpp
static RC_UI::DockableWindowState s_window;
if (!RC_UI::BeginDockableWindow("Tools", s_window, "Bottom", ImGuiWindowFlags_NoCollapse)) {
    return;
}
// content...
RC_UI::EndDockableWindow();
```

**Enforcement**: `python3 tools/check_ui_compliance.py`

---

## 🎨 The Cockpit Doctrine (UI/UX Philosophy)

### Core Philosophy
You are building a **state-reactive control surface**, not windows.

### Design Language
> **Vignelli structure + Y2K geometry + LCARS segmentation + Blender ergonomics + ImGui reactivity**

### The Four Sacred Principles
1. **Viewport is sacred**: Never obstruct 3D view
2. **Tools are tactile**: Clicks/drags/hovers feel responsive
3. **Properties are contextual**: Show only what matters for selection/mode
4. **Data is always visible**: Critical indices (Road/District/Lot IDs) persist in UI

### Design Mandates (MUST ENFORCE)
✅ **Required**:
- Every panel responds to HFSM state changes
- Motion conveys meaning (state change, action prompt, data link)
- Tactile feedback on all user actions
- Panel organization reflects data architecture
- Y2K geometric language applied consistently

❌ **Forbidden**:
- Generic window layouts ignoring data relationships
- Static tooltips as primary instruction
- Decorative animations without meaning
- Permanent UI chrome blocking viewport
- Ignoring HFSM state in panel design

### Why This Works
- **Satisfying to operate**: Tactile, responsive, weighted
- **Rewards curiosity**: Exploration reveals new states
- **Visually responsive**: Never feels static
- **Feels like machinery**: Controlling a system, not filling forms

---

## 🧪 Testing & Quality

### Test Organization

| Test Suite | File | Purpose |
|------------|------|---------|
| **Core Tests** | `tests/test_core.cpp` | Vec2, Tensor2D, CIV, SIV, RogueWorker |
| **Generator Tests** | `tests/test_generators.cpp` | Pipeline stages, road tracing, districts |
| **HFSM Tests** | `tests/test_editor_hfsm.cpp` | State transitions, timing, determinism |
| **AI Tests** | `tests/test_ai.cpp` | Protocol, clients, toolserver integration |

### Recent Bug Fixes Validated by Tests

1. **M_PI → std::numbers::pi**: C++20 migration for MSVC compatibility
2. **Vec2::lerp**: Free function vs static method correction
3. **Viewport Index**: Single source of truth for entity IDs
4. **Property Editing**: Command-history driven undo/redo
5. **Dirty-Layer Propagation**: Explicit regeneration tracking

### Compliance Checks

Run before committing:
```powershell
python3 tools/check_ui_compliance.py
python3 tools/check_generator_viewport_contract.py
python3 tools/check_tool_wiring_contract.py
```

---

## 📖 Key Documentation Reference

### Must-Read Documents (Priority Order)

1. **Architecture & Design**
   - `ReadMe.md` - Project overview and pipeline
   - `docs/TheRogueCityDesignerSoft.md` - Design research (single source of truth for formulas)
   - `.github/copilot-instructions.md` - Agent rules and patterns
   - `.github/AGENTS.md` - Agent roles and responsibilities
   - `docs/30_architecture/architecture-map.md` - Module dependency graph

2. **Contracts & Specs**
   - `docs/20_specs/generator-viewport-contract.md` - Generation output flow
   - `docs/20_specs/tool-wiring-contract.md` - Tool dispatch rules
   - `docs/20_specs/ui-patterns.md` - UI component patterns
   - `docs/20_specs/tech-stack.md` - Dependencies and tooling

3. **Build & Testing**
   - `BUILD_INSTRUCTIONS.md` - Quick start build guide
   - `docs/20_specs/quick-start.md` - AI Bridge quick start
   - `docs/50_testing/testing-summary.md` - Test infrastructure
   - `docs/50_testing/testing-quickstart.md` - Running tests

4. **AI Integration**
   - `docs/AI_Integration_Summary.md` - Overview of 4 phases
   - `AI/docs/Phase4_QuickReference.md` - Quick reference for AI features
   - `AI/docs/ui/ui_patterns.json` - Pattern catalog for code-shape awareness

### Quick Reference Files

- **Data Types**: `core/include/RogueCity/Core/Types.hpp`
- **Global State**: `core/include/RogueCity/Core/Editor/GlobalState.hpp`
- **City Pipeline**: `generators/include/RogueCity/Generators/Pipeline/CityGenerator.hpp`
- **AESP Logic**: `generators/include/RogueCity/Generators/Districts/AESPClassifier.hpp`
- **HFSM States**: `app/include/RogueCity/App/HFSM/HFSM.hpp`

---

## 🚀 Common Tasks Checklist

### When Adding a C++ Feature
- [ ] Identify correct layer (Core/Generators/App)
- [ ] Choose correct container (FVA/SIV/CIV)
- [ ] Check if threading needed (>10ms work)
- [ ] Add to GlobalState if UI-visible
- [ ] Update HFSM if state-dependent
- [ ] Add tests to relevant test suite
- [ ] Run compliance checks
- [ ] Update documentation

### When Modifying UI
- [ ] Verify Cockpit Doctrine compliance
- [ ] Check HFSM state integration
- [ ] Add motion with purpose (not decoration)
- [ ] Ensure viewport not obstructed
- [ ] Test docking/undocking behavior
- [ ] Verify panel responds to state changes
- [ ] Run `check_ui_compliance.py`

### When Touching AESP/Formulas
- [ ] Read `TheRogueCityDesignerSoft.md` first
- [ ] Verify formula against design doc
- [ ] Update `AESPClassifier` lookup tables
- [ ] Consult Math Genius Agent
- [ ] Add/update unit tests
- [ ] Document formula source

### When Adding Generator Stage
- [ ] Follow 8-step workflow above
- [ ] Extend GlobalState with containers
- [ ] Create bridge in App layer
- [ ] Add HFSM states if needed
- [ ] Implement control panel
- [ ] Add viewport overlays
- [ ] Update AI pattern catalog
- [ ] Test with Debug Manager

---

## 🎭 Agent Communication Protocol

### Handoff Questions (Inter-Agent Workflow)

Each agent asks the next agent what they need:

```
Math Genius → Coder: "What GlobalState containers do you need?"
Coder → UI/UX Master: "What bridge API calls do you need?"
UI/UX Master → Coder: "What panel visibility rules per state?"
UI/UX Master → AI Integration: "What pattern catalog metadata is needed?"
All → Resource Manager: "What are the performance/memory constraints?"
All → Debug Manager: "What tests and validation do you need?"
```

### When to Escalate to The Architect

- Architecture layer violations
- Performance mandate conflicts
- Cross-module refactoring decisions
- Design philosophy questions
- Contract enforcement issues

---

## 🔥 Common Pitfalls (Learn from Past Mistakes)

### ❌ Don't Do This

1. **Using std::vector for UI entities** → Use FVA for stable IDs
2. **Threading micro-tasks** → Only thread when >10ms of work
3. **Adding imgui.h to Core** → Build will fail, Core must stay UI-free
4. **Guessing AESP formulas** → Always read design doc first
5. **Static tooltips** → Use motion-based affordances instead
6. **Panel-local sync logic** → Must use `ApplyCityOutputToGlobalState`
7. **Direct generator calls** → Must route through coordinator
8. **Ignoring HFSM state** → UI must respond to state changes
9. **Blocking viewport** → Keep viewport canvas sacred
10. **Skipping compliance checks** → Always run checks before commit

### ✅ Do This Instead

1. **Choose FVA/SIV/CIV** based on use case matrix
2. **Calculate complexity threshold** before threading
3. **Keep Core pure** with zero UI dependencies
4. **Cite design doc** for all formula work
5. **Animate with purpose** to teach user
6. **Use canonical applier** for all generation output
7. **Use coordinator APIs** for all generation requests
8. **Make UI state-reactive** with HFSM integration
9. **Overlay contextually** without blocking view
10. **Run compliance checks** as pre-commit validation

---

## 🎓 Onboarding Completion

You are ready to work on RogueCity when you can answer:

1. **What are the 3 layers and their dependency rules?**
2. **When do you use FVA vs SIV vs CIV?**
3. **What is the RogueWorker threading threshold?**
4. **What are the 5 AESP district formulas?**
5. **What are the 4 Sacred Principles of Cockpit Doctrine?**
6. **Where is the single source of truth for generation output?**
7. **Which agent handles UI design decisions?**
8. **What's the 8-step workflow for adding a generator?**
9. **Where do you read formulas (never guess)?**
10. **What makes motion "purposeful" vs decorative?**

### Answer Key (Check Your Understanding)

1. Core (data/math, no UI) → Generators (algorithms) → App (editor/UI)
2. FVA: UI entities with stable IDs | SIV: High-churn with validity | CIV: Internal only
3. Enable RogueWorker when work >10ms and complexity N > 100
4. Mixed-Use: 0.25(A+E+S+P) | Residential: 0.6P+0.2A+0.1S+0.1E | Commercial: 0.6E+0.2A+0.1S+0.1P | Civic: 0.5E+0.2A+0.1S+0.2P | Industrial: 0.6S+0.25A+0.1E+0.05P
5. Viewport is sacred | Tools are tactile | Properties are contextual | Data is always visible
6. `RogueCity::App::ApplyCityOutputToGlobalState`
7. UI/UX/ImGui/ImVue Master (enforces Cockpit Doctrine)
8. Pipeline → GlobalState → Bridge → HFSM → Panels → Viewport → Control → Catalog
9. `docs/TheRogueCityDesignerSoft.md` (single source of truth)
10. Motion teaches/prompts/links vs motion that's just pretty

---

## 📞 Getting Help

### When Stuck
1. **Read the docs** listed in Key Documentation Reference
2. **Consult specialist agent** using the "When to Consult" guide
3. **Check contracts** for invariants and rules
4. **Run compliance checks** to find violations
5. **Escalate to Architect** for design decisions

### Useful Commands
```powershell
# Find documentation
Get-ChildItem -Path docs -Recurse -Filter *.md

# Search for patterns
grep -r "pattern_name" .

# Check errors
python3 tools/check_ui_compliance.py
python3 tools/check_generator_viewport_contract.py
python3 tools/check_tool_wiring_contract.py

# Build and test
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

---

**Welcome to RogueCity! Generate beautiful, complex cities.**
*This document is maintained by the Documentation Keeper Agent. Last review: February 16, 2026.*
