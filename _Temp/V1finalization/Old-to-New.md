This agent-assigned plan follows your established hierarchy, enforces the Rogue Protocol performance mandates, and applies the Cockpit Doctrine for UI integration. Each agent has clear handoff questions to facilitate collaboration, and The Architect maintains oversight throughout.
Search-in-file: jump to phase1, phase2, agent_assignments, validation_gates, critical_paths keys inside the JSON block.
​

Outline/headings scan: quickly hop between “Phase 1”, “Phase 2”, and “Master JSON Execution Plan” sections.
​

Extract-code-block / JSON parser: lift the JSON into a standalone object so the agent can iterate tasks deterministically (and fail fast if malformed).
​

Diff/consistency checklist: compare extracted tasks against “layer separation”, “HFSM rules”, “RogueWorker/FVA/SIV/CIV mandates”, and “Core is UI-free” constraints in your Copilot instructions and agent mandates.

Keyword grep across docs: search RogueWorker, FVA, HFSM, Core stays UI-free, CityGenerator to ensure the plan’s actions don’t violate the house rules.



# **Assigned MVP Integration Plan**

## **Phase 1: Core Systems & Generators Integration**

### **Task 1.1: Architecture Planning & Validation**

**Owner:** **The Architect** (Master AI Director)

```json
{
  "agent": "The Architect",
  "task": "validate_integration_architecture",
  "priority": "CRITICAL",
  "objectives": [
    "Review _Temp MVP structure against Rogue Protocol mandates",
    "Verify layer separation compliance (Core/Generators/App)",
    "Identify container type violations (std::vector vs FVA/SIV/CIV)",
    "Create integration roadmap with dependency graph"
  ],
  "tools": ["read_file", "grep_search", "list_files"],
  "deliverables": [
    "Layer compliance report for MVP code",
    "Container type audit (flag all std::vector for Road/District data)",
    "Approved integration dependency graph"
  ],
  "handoff_to": "Documentation Keeper Agent"
}
```

### **Task 1.2: Document Integration Strategy**

**Owner:** **Documentation Keeper Agent**

```json
{
  "agent": "Documentation Keeper Agent",
  "task": "create_integration_documentation",
  "priority": "HIGH",
  "dependencies": ["Task 1.1"],
  "objectives": [
    "Create docs/MVP_Integration_Phase1.md with architecture decisions",
    "Document namespace strategy (RogueCities::Generators::*)",
    "Update README.md with new generator structure",
    "Create data contract docs for CityModel, CityParams"
  ],
  "files_to_create": [
    "docs/MVP_Integration_Phase1.md",
    "docs/generators/DataContracts.md"
  ],
  "handoff_to": "Math Genius Agent"
}
```

### **Task 1.3: Validate & Integrate Spatial Math Systems**

**Owner:** **Math Genius Agent**

```json
{
  "agent": "Math Genius Agent",
  "task": "integrate_spatial_systems",
  "priority": "CRITICAL",
  "dependencies": ["Task 1.2"],
  "scope": [
    "TensorField.cpp/h (12KB math)",
    "Streamlines.cpp/h (RK4 integration)",
    "Integrator.cpp/h (field sampling)",
    "GridStorage.cpp/h (spatial indexing)"
  ],
  "objectives": [
    "Verify all formulas against docs/TheRogueCityDesignerSoft.md",
    "Ensure numerical stability (no NaN/inf in field sampling)",
    "Validate RK4 step sizes match research paper",
    "Confirm grid index calculations are deterministic"
  ],
  "destination": "generators/src/spatial/",
  "modifications": [
    "Add namespace RogueCities::Generators::Spatial",
    "Update include paths to new structure",
    "Add assertion coverage for edge cases",
    "Document all magic numbers with paper references"
  ],
  "handoff_to": "Coder Agent",
  "handoff_question": "What GlobalState containers do you need for TensorField and Streamline data?"
}
```

### **Task 1.4: Implement Core Data Models**

**Owner:** **Coder Agent**

```json
{
  "agent": "Coder Agent",
  "task": "integrate_data_models",
  "priority": "CRITICAL",
  "dependencies": ["Task 1.3"],
  "files": [
    {
      "source": "_Temp/.../CityModel.h/cpp",
      "destination": "generators/include/generators/CityModel.h",
      "container_audit": "MUST use FVA for Road/District collections",
      "actions": [
        "Replace std::vector<RoadSegment> with FVA<RoadSegment>",
        "Replace std::vector<District> with FVA<District>",
        "Add namespace RogueCities::Generators",
        "Ensure C++20 compliance",
        "Add move semantics for performance"
      ]
    },
    {
      "source": "_Temp/.../CityParams.h/cpp",
      "destination": "generators/include/generators/CityParams.h",
      "actions": [
        "Merge with any existing CityParams",
        "Add default constructors",
        "Implement serialization (JSON support for AI CitySpec)"
      ]
    }
  ],
  "rogue_protocol_checks": [
    "NO std::vector for UI-exposed data",
    "Use FVA for Road/District (stable handles)",
    "Use SIV for Buildings (high-churn)",
    "Use CIV for internal scratch buffers only"
  ],
  "handoff_to": "Resource Manager Agent",
  "handoff_question": "What memory budgets and caps do we need for road/district counts?"
}
```

### **Task 1.5: Define Resource Budgets**

**Owner:** **Resource Manager Agent**

```json
{
  "agent": "Resource Manager Agent",
  "task": "establish_generator_budgets",
  "priority": "HIGH",
  "dependencies": ["Task 1.4"],
  "objectives": [
    "Set maximum counts per entity type",
    "Define memory caps for spatial data structures",
    "Establish performance thresholds for RogueWorker activation"
  ],
  "budget_definitions": {
    "max_road_segments": 10000,
    "max_districts": 500,
    "max_lots": 50000,
    "max_buildings": 100000,
    "tensor_field_resolution": "configurable 64-512",
    "rogueworker_threshold": 100
  },
  "handoff_to": "Coder Agent",
  "handoff_question": "Where should we add these caps in CityParams and config validation?"
}
```

### **Task 1.6: Integrate Core Generators with Performance**

**Owner:** **Coder Agent**

```json
{
  "agent": "Coder Agent",
  "task": "integrate_core_generators",
  "priority": "CRITICAL",
  "dependencies": ["Task 1.5"],
  "generators": [
    {
      "name": "RoadGenerator",
      "source": "_Temp/.../RoadGenerator.cpp (23KB)",
      "destination": "generators/src/generators/RoadGenerator.cpp",
      "performance_mandate": "MUST use RogueWorker for road tracing >10ms",
      "threading_condition": "if (seed_count * iterations > 1000) use_threading",
      "dependencies": ["TensorField", "Streamlines", "Graph"]
    },
    {
      "name": "DistrictGenerator",
      "source": "_Temp/.../DistrictGenerator.cpp (39KB)",
      "destination": "generators/src/generators/DistrictGenerator.cpp",
      "performance_mandate": "MUST use RogueWorker for polygon operations",
      "depends_on": "RoadGenerator output"
    },
    {
      "name": "BlockGenerator",
      "source": "_Temp/.../BlockGenerator.cpp (82KB)",
      "destination": "generators/src/generators/BlockGenerator.cpp",
      "performance_mandate": "CRITICAL - largest file, MUST thread subdivision",
      "geos_integration": "verify GEOS linkage in CMakeLists"
    },
    {
      "name": "LotGenerator",
      "source": "_Temp/.../LotGenerator.cpp (47KB)",
      "destination": "generators/src/generators/LotGenerator.cpp",
      "performance_mandate": "MUST use RogueWorker for lot subdivision"
    }
  ],
  "container_enforcement": [
    "Roads: FVA<RoadSegment> (UI needs stable handles)",
    "Districts: FVA<District> (UI selection)",
    "Lots: FVA<Lot> (UI selection)",
    "Buildings: SIV<Building> (high-churn, safety)"
  ],
  "handoff_to": "Debug Manager Agent",
  "handoff_question": "What test fixtures and profiling harnesses do you need?"
}
```

### **Task 1.7: Create Test Infrastructure**

**Owner:** **Debug Manager Agent** + **Resource Manager Agent**

```json
{
  "agent": "Debug Manager Agent",
  "task": "build_phase1_test_suite",
  "priority": "CRITICAL",
  "dependencies": ["Task 1.6"],
  "collaboration": "Resource Manager Agent provides test case generators",
  "objectives": [
    "Create deterministic test fixtures with seeded RNG",
    "Add performance regression tests",
    "Build profiling harness for generator timing",
    "Verify resource budgets aren't exceeded"
  ],
  "test_files": [
    {
      "path": "tests/test_generator_integration.cpp",
      "tests": [
        "test_tensor_field_generation",
        "test_road_tracing_deterministic",
        "test_district_subdivision",
        "test_lot_generation_budgets"
      ]
    },
    {
      "path": "tests/test_performance_budgets.cpp",
      "tests": [
        "test_road_count_cap",
        "test_district_memory_budget",
        "test_rogueworker_activation_threshold"
      ]
    }
  ],
  "profiling_requirements": [
    "Measure TensorField generation time",
    "Profile RoadGenerator with/without RogueWorker",
    "Track memory usage per generator stage"
  ],
  "handoff_to": "City Planner Agent",
  "handoff_question": "Do the generated districts match AESP zoning semantics?"
}
```

### **Task 1.8: Validate AESP Zoning Logic**

**Owner:** **City Planner Agent**

```json
{
  "agent": "City Planner Agent",
  "task": "validate_aesp_integration",
  "priority": "HIGH",
  "dependencies": ["Task 1.7"],
  "objectives": [
    "Verify AESP weights in DistrictGenerator match docs/TheRogueCityDesignerSoft.md",
    "Validate district archetypes (Downtown, Industrial, Residential, Slums)",
    "Ensure road AESP profiles drive correct zoning outcomes",
    "Check axiom influence on district formation"
  ],
  "verification_steps": [
    "Read AESP section from design doc",
    "Audit AESPClassifier lookup tables",
    "Test edge cases (high privacy + low access → Residential)",
    "Validate emergent zoning feels playable"
  ],
  "handoff_to": "Documentation Keeper Agent",
  "handoff_question": "What documentation updates are needed for Phase 1 completion?"
}
```

### **Task 1.9: Update Build System**

**Owner:** **Coder Agent**

```json
{
  "agent": "Coder Agent",
  "task": "update_cmake_generators",
  "priority": "CRITICAL",
  "dependencies": ["Task 1.6"],
  "file": "generators/CMakeLists.txt",
  "actions": [
    "Add all new source files to target_sources",
    "Verify GEOS linkage for BlockGenerator",
    "Add include directories for spatial/ and utils/ subdirs",
    "Ensure C++20 features enabled",
    "Link GLM for math types"
  ],
  "build_validation": [
    "cmake -B build -S .",
    "cmake --build build --target RogueCityGenerators --config Release",
    "Run test_generators executable"
  ],
  "handoff_to": "The Architect",
  "handoff_question": "Phase 1 generators complete. Ready for Phase 2 UI integration?"
}
```

---

## **Phase 2: Application & UI Integration**

### **Task 2.1: UI Architecture Planning**

**Owner:** **The Architect** + **UI/UX Master**

```json
{
  "agent": "The Architect + UI/UX Master",
  "task": "design_cockpit_ui_integration",
  "priority": "CRITICAL",
  "dependencies": ["Phase 1 Complete"],
  "objectives": [
    "Apply Cockpit Doctrine to MVP UI components",
    "Define HFSM states for generator workflows",
    "Plan state-reactive panel behaviors",
    "Design affordance systems (glow, pulse, wiggle)"
  ],
  "design_principles": [
    "Viewport is sacred (no obstruction)",
    "Tools are tactile (responsive feedback)",
    "Properties are contextual (selection-driven)",
    "Data always visible (Road/District/Lot indices)",
    "Motion teaches (animated entry points)"
  ],
  "hfsm_states_needed": [
    "AxiomPlacement",
    "TensorFieldEditing",
    "RoadTracing",
    "DistrictViewing",
    "LotSelection",
    "BuildingPlacement"
  ],
  "handoff_to": "Coder Agent",
  "handoff_question": "What HFSM infrastructure exists? Do we need new state types?"
}
```

### **Task 2.2: Extend HFSM State System**

**Owner:** **Coder Agent**

```json
{
  "agent": "Coder Agent",
  "task": "add_generator_hfsm_states",
  "priority": "CRITICAL",
  "dependencies": ["Task 2.1"],
  "files": [
    "app/include/RogueCity/App/HFSM/HFSM.hpp",
    "app/src/HFSM/HFSMStates.cpp"
  ],
  "new_states": [
    "AxiomPlacementState (axiom tool active)",
    "RoadGenerationState (generating roads)",
    "DistrictInspectionState (viewing districts)",
    "LotSubdivisionState (subdividing blocks)"
  ],
  "performance_rule": "State transitions MUST complete <10ms, delegate heavy work to RogueWorker",
  "handoff_to": "UI/UX Master",
  "handoff_question": "What panel visibility and animation behaviors per state?"
}
```

### **Task 2.3: Design State-Reactive Panel System**

**Owner:** **UI/UX Master**

```json
{
  "agent": "UI/UX Master",
  "task": "design_state_reactive_panels",
  "priority": "CRITICAL",
  "dependencies": ["Task 2.2"],
  "cockpit_doctrine_requirements": [
    "Panels morph with HFSM state changes",
    "Motion-based affordances (glow on hover, pulse on activation)",
    "Y2K geometric language (capsules, segments, warning stripes)",
    "Vignelli data structure (panels reflect data relationships)",
    "Guided affordance (wiggle on first use, highlight on relevance)"
  ],
  "panel_designs": [
    {
      "name": "AxiomControlPanel",
      "state_reactive": "Glows when AxiomPlacementState active",
      "affordance": "Wiggles on first launch to teach entry point",
      "y2k_elements": "Capsule buttons with CODE-13 labels"
    },
    {
      "name": "RoadIndexPanel",
      "state_reactive": "Highlights selected road when clicked in viewport",
      "context_menu": "Right-click shows 'View Properties' / 'Delete Road'",
      "template": "Use RcDataIndexPanel<RoadSegment> pattern"
    },
    {
      "name": "DistrictIndexPanel",
      "state_reactive": "Pulses during DistrictInspectionState",
      "visualization": "Color-coded by AESP archetype"
    },
    {
      "name": "GeneratorControlPanel",
      "state_reactive": "Shows different params per current generator stage",
      "motion": "Progress pulse (not bar) for generation status"
    }
  ],
  "handoff_to": "Coder Agent",
  "handoff_question": "What bridge APIs do you need to connect panels to generators?"
}
```

### **Task 2.4: Create Generator Bridge Layer**

**Owner:** **Coder Agent**

```json
{
  "agent": "Coder Agent",
  "task": "build_generator_bridge",
  "priority": "CRITICAL",
  "dependencies": ["Task 2.3"],
  "file": "app/include/RogueCity/App/Integration/GeneratorBridge.hpp",
  "purpose": "Translate UI parameters → Generator inputs → GlobalState population",
  "bridge_methods": [
    "generateTensorField(TensorFieldParams) -> void",
    "traceRoads(RoadGenParams) -> void",
    "subdivideDistricts(DistrictParams) -> void",
    "generateLots(LotParams) -> void"
  ],
  "integration_with_globalstate": [
    "Populate GlobalState::roads (FVA)",
    "Populate GlobalState::districts (FVA)",
    "Populate GlobalState::lots (FVA)",
    "Trigger UI refresh via state signals"
  ],
  "handoff_to": "AI Integration Agent",
  "handoff_question": "What protocol extensions needed for CitySpec to drive generators?"
}
```

### **Task 2.5: Extend AI Integration for Generator Control**

**Owner:** **AI Integration Agent**

```json
{
  "agent": "AI Integration Agent",
  "task": "extend_cityspec_for_generators",
  "priority": "HIGH",
  "dependencies": ["Task 2.4"],
  "objectives": [
    "Extend CitySpec protocol to include district/lot parameters",
    "Add toolserver endpoints for generator control",
    "Update UiSnapshot to include generator state",
    "Maintain backward compatibility with existing AI clients"
  ],
  "protocol_changes": {
    "file": "AI/protocol/UiAgentProtocol.h",
    "additions": [
      "GeneratorState enum (Idle, Generating, Complete)",
      "DistrictParams in CitySpec",
      "LotGenParams in CitySpec"
    ]
  },
  "toolserver_endpoints": [
    "/generate_city (POST) - full pipeline",
    "/generate_districts (POST) - district stage only",
    "/preview_lots (POST) - preview without commit"
  ],
  "pattern_catalog_update": {
    "file": "AI/docs/ui/ui_patterns.json",
    "add_patterns": [
      "DistrictIndexPanel",
      "LotIndexPanel",
      "GeneratorControlPanel"
    ]
  },
  "handoff_to": "UI/UX Master",
  "handoff_question": "What viewport overlays and visualizations do you need?"
}
```

### **Task 2.6: Implement Viewport Visualization**

**Owner:** **UI/UX Master** + **Coder Agent**

```json
{
  "agent": "UI/UX Master + Coder Agent",
  "task": "implement_viewport_overlays",
  "priority": "HIGH",
  "dependencies": ["Task 2.5"],
  "file": "visualizer/src/ui/rc_viewport_renderer.cpp",
  "overlays": [
    {
      "name": "TensorFieldVisualization",
      "rendering": "Major/minor eigenvector lines",
      "color_coding": "Hue by orientation, saturation by magnitude"
    },
    {
      "name": "RoadNetworkOverlay",
      "rendering": "Lines with width by road type",
      "labels": "Road IDs on hover",
      "selection": "Highlight selected road in RoadIndexPanel"
    },
    {
      "name": "DistrictAESPHeatmap",
      "rendering": "Polygon fill with AESP gradient",
      "color_scheme": "Red (Industrial), Blue (Residential), Yellow (Commercial)"
    },
    {
      "name": "LotBoundaries",
      "rendering": "Polygon outlines, highlight on selection"
    }
  ],
  "performance": "Use instanced rendering for >1000 lots",
  "handoff_to": "Coder Agent",
  "handoff_question": "What's the final integration for Application.cpp?"
}
```

### **Task 2.7: Integrate Main Application**

**Owner:** **Coder Agent**

```json
{
  "agent": "Coder Agent",
  "task": "integrate_main_application",
  "priority": "CRITICAL",
  "dependencies": ["Task 2.6"],
  "strategy": "MERGE with existing app/, do not replace",
  "files": [
    {
      "source": "_Temp/.../Application.cpp (104KB)",
      "destination": "app/src/Application.cpp",
      "action": "MERGE",
      "preserve": "Existing HFSM integration, AI bridge runtime",
      "integrate": "MVP's generator initialization and UI setup",
      "caution": "This is the critical integration point - high risk"
    },
    {
      "source": "_Temp/.../ViewportWindow.cpp (176KB)",
      "destination": "visualizer/src/ui/ViewportWindow.cpp",
      "action": "MERGE",
      "preserve": "Existing viewport rendering pipeline",
      "integrate": "MVP's 8 tool modes and selection system"
    },
    {
      "source": "_Temp/.../Windows.cpp (155KB)",
      "destination": "visualizer/src/ui/Windows.cpp",
      "action": "REFACTOR then MERGE",
      "note": "This file is massive - consider breaking into smaller panels"
    }
  ],
  "testing_after_merge": [
    "Verify application launches",
    "Test HFSM state transitions",
    "Validate generator pipeline execution",
    "Check UI responsiveness"
  ],
  "handoff_to": "Debug Manager Agent",
  "handoff_question": "What integration tests and profiling do you need?"
}
```

### **Task 2.8: Integration Testing & Validation**

**Owner:** **Debug Manager Agent** + **Resource Manager Agent**

```json
{
  "agent": "Debug Manager Agent + Resource Manager Agent",
  "task": "phase2_integration_testing",
  "priority": "CRITICAL",
  "dependencies": ["Task 2.7"],
  "test_categories": [
    {
      "name": "Build Verification",
      "tests": ["Full solution builds", "No missing dependencies", "CMake generates correctly"]
    },
    {
      "name": "Runtime Stability",
      "tests": ["Application launches", "No crashes in 10min session", "Memory leaks check"]
    },
    {
      "name": "Generator Pipeline",
      "tests": ["Tensor generation works", "Roads trace correctly", "Districts subdivide", "Lots generate"]
    },
    {
      "name": "UI Integration",
      "tests": ["HFSM state transitions work", "Panels respond to state changes", "Viewport renders correctly", "Selection system functions"]
    },
    {
      "name": "Performance",
      "tests": ["RogueWorker activates when expected", "Frame rate >30fps with 1000 roads", "Memory budgets respected"]
    }
  ],
  "profiling_requirements": [
    "Profile Application startup time",
    "Measure generator stage timings",
    "Track UI render overhead per panel",
    "Identify any blocking operations"
  ],
  "handoff_to": "Documentation Keeper Agent",
  "handoff_question": "What documentation updates for Phase 2 completion?"
}
```

### **Task 2.9: Final Documentation & Release Notes**

**Owner:** **Documentation Keeper Agent**

```json
{
  "agent": "Documentation Keeper Agent",
  "task": "finalize_integration_docs",
  "priority": "HIGH",
  "dependencies": ["Task 2.8"],
  "deliverables": [
    {
      "file": "docs/MVP_Integration_Complete.md",
      "sections": [
        "Architecture overview",
        "Generator pipeline stages",
        "HFSM state diagram",
        "UI panel descriptions",
        "Performance benchmarks",
        "Known issues and workarounds"
      ]
    },
    {
      "file": "README.md",
      "updates": [
        "Add generator usage examples",
        "Update build instructions",
        "Document new HFSM states",
        "Add UI screenshots"
      ]
    },
    {
      "file": "docs/TheRogueCityDesignerSoft.md",
      "updates": [
        "Verify implementation matches design doc",
        "Update any deviations with rationale"
      ]
    }
  ],
  "handoff_to": "The Architect",
  "handoff_question": "Final approval for MVP integration completion?"
}
```

---

## **Master JSON Execution Plan for AI Assistant**

```json
{
  "project": "RogueCities MVP Integration",
  "phases": 2,
  "agent_collaboration": true,
  "rogue_protocol_enforcement": "MANDATORY",
  "cockpit_doctrine_compliance": "MANDATORY",
  
  "phase1": {
    "name": "Core Systems & Generators Integration",
    "duration_estimate": "3-5 days",
    "tasks": [
      "1.1_architecture_validation",
      "1.2_documentation_creation",
      "1.3_spatial_math_integration",
      "1.4_data_model_implementation",
      "1.5_resource_budgets",
      "1.6_generator_integration",
      "1.7_test_infrastructure",
      "1.8_aesp_validation",
      "1.9_cmake_updates"
    ],
    "critical_paths": ["1.3→1.4→1.6→1.9"],
    "success_criteria": [
      "All generators build successfully",
      "Tests pass with deterministic results",
      "No container type violations (FVA/SIV mandates)",
      "AESP logic matches design doc",
      "Performance budgets respected"
    ]
  },
  
  "phase2": {
    "name": "Application & UI Integration",
    "duration_estimate": "4-6 days",
    "tasks": [
      "2.1_ui_architecture_planning",
      "2.2_hfsm_extension",
      "2.3_panel_design",
      "2.4_generator_bridge",
      "2.5_ai_integration",
      "2.6_viewport_visualization",
      "2.7_application_merge",
      "2.8_integration_testing",
      "2.9_final_documentation"
    ],
    "critical_paths": ["2.1→2.2→2.3→2.7→2.8"],
    "success_criteria": [
      "Application launches without crashes",
      "HFSM state transitions work correctly",
      "UI panels respond to state changes",
      "Generator pipeline executes end-to-end",
      "Cockpit Doctrine compliance verified",
      "Performance targets met (>30fps)"
    ]
  },
  
  "agent_assignments": {
    "The_Architect": ["1.1", "2.1", "final_approval"],
    "Documentation_Keeper": ["1.2", "1.8", "2.9"],
    "Math_Genius": ["1.3"],
    "Coder_Agent": ["1.4", "1.6", "1.9", "2.2", "2.4", "2.6", "2.7"],
    "Resource_Manager": ["1.5", "1.7", "2.8"],
    "Debug_Manager": ["1.7", "2.8"],
    "City_Planner": ["1.8"],
    "UI_UX_Master": ["2.1", "2.3", "2.6"],
    "AI_Integration": ["2.5"]
  },
  
  "validation_gates": [
    {
      "after_task": "1.9",
      "validator": "The_Architect",
      "criteria": "Phase 1 generators compile, link, and execute"
    },
    {
      "after_task": "2.7",
      "validator": "The_Architect + Debug_Manager",
      "criteria": "Integrated application launches and basic workflow functions"
    },
    {
      "after_task": "2.8",
      "validator": "All_Agents",
      "criteria": "Full integration test suite passes"
    }
  ]
}
```

