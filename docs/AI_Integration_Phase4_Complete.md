# AI Integration - Phase 4: Code-Shape Aware AI ?

**Date**: 2026-02-06  
**Status**: ? **Phase 4 COMPLETE** - Code-Shape Aware Refactoring Assistant  
**Build**: Successful (RogueCityVisualizerGui.exe)

---

## Phase 4: Code-Shape Awareness & Design Assistant

### What Was Implemented

This phase transforms the AI from **simple layout automation** to **architecture-aware refactoring assistant** that can understand and suggest improvements to your code structure.

#### 1. Enhanced UiSnapshot with Code Metadata ?

**Modified**: `AI/protocol/UiAgentProtocol.h`

**New Fields in `UiPanelInfo`**:
```cpp
std::string role;               // "inspector" | "toolbox" | "viewport" | "nav" | "log"
std::string owner_module;       // e.g. "rc_panel_axiom_editor"
std::vector<std::string> data_bindings;       // ["axiom.selected_id", "road.brush_radius"]
std::vector<std::string> interaction_patterns; // ["list+detail", "toolbar+canvas"]
```

**New Fields in `UiStateInfo`**:
```cpp
std::map<std::string, std::string> state_model;
// e.g. {"axiom.selected_id": "A123", "road.brush_radius": "15.0"}
```

**Purpose**: The AI can now see:
- **What role each panel plays** (inspector vs toolbox vs viewport)
- **Which module owns it** (for understanding code structure)
- **What data it's bound to** (for identifying duplication)
- **What interaction patterns it uses** (for recognizing common patterns)

---

#### 2. UI Pattern Catalog System ?

**Created**: `AI/docs/ui/ui_patterns.json`

**Contents**:
- **Role definitions**: Describes canonical panel roles (inspector, toolbox, viewport, nav, log, index)
- **Pattern templates**: Defines reusable patterns like `InspectorPanel<T>`, `ToolStrip`, `DataIndexPanel<T>`
- **Current panels**: Maps existing panels to suggested patterns
- **Refactoring opportunities**: Lists specific refactorings with priorities

**Example Pattern**:
```json
{
  "InspectorPanel": {
    "description": "Generic inspector panel with property editing",
    "template": "RcInspectorPanel<T>",
    "props": ["selected_id", "properties"],
    "example_instances": [
      "axiom_inspector",
      "road_inspector",
      "district_inspector"
    ]
  }
}
```

---

#### 3. AI Design Assistant Client ?

**Created**:
- `AI/client/UiDesignAssistant.h`
- `AI/client/UiDesignAssistant.cpp`

**Features**:
- `GenerateDesignPlan()` - Query AI for refactoring suggestions
- `SaveDesignPlan()` - Save JSON plan to timestamped file
- `LoadPatternCatalog()` - Load canonical patterns for AI context

**Data Structures**:
```cpp
struct UiComponentPattern {
    std::string name;
    std::string template_name;
    std::vector<std::string> applies_to;
    std::vector<std::string> props;
};

struct UiDesignSuggestion {
    std::string name;
    std::string priority;  // "high", "medium", "low"
    std::vector<std::string> affected_panels;
    std::string suggested_action;
    std::string rationale;
};

struct UiDesignPlan {
    std::vector<UiComponentPattern> component_patterns;
    std::vector<UiDesignSuggestion> refactoring_opportunities;
    std::vector<std::string> suggested_files;
    std::string summary;
};
```

---

#### 4. Dual-Mode UI Agent Panel ?

**Enhanced**: `visualizer/src/ui/panels/rc_panel_ui_agent.cpp`

**Two Modes**:

##### Mode 1: Layout Commands (Original)
- **Button**: "Apply AI Layout"
- **Purpose**: Immediate layout adjustments
- **Output**: JSON commands applied directly to docking
- **Example**: "Move inspector to left dock"

##### Mode 2: Design/Refactor Planning (NEW)
- **Button**: "Generate Refactor Plan"
- **Purpose**: Architecture analysis and suggestions
- **Output**: JSON file saved to `AI/docs/ui/ui_refactor_<timestamp>.json`
- **Example**: "Extract common DataIndexPanel pattern from 4 panels"

**Enhanced Snapshot Builder**:
```cpp
static AI::UiSnapshot BuildEnhancedSnapshot() {
    // ... basic fields ...
    
    // Add code-shape metadata
    snapshot.panels.push_back({
        "road_index", "Bottom", true,
        "index", "rc_panel_road_index",
        {"roads[]", "roads.selected_id"},
        {"table+selection"}
    });
    
    // Add state model
    snapshot.state.state_model["axiom.selected_id"] = "A123";
    snapshot.state.state_model["road.brush_radius"] = "15.0";
}
```

---

## Architecture Diagram (Phase 4)

```
???????????????????????????????????????????????????????????
?         VISUALIZER (Enhanced UI Agent Panel)            ?
?  ??????????????????????????????????????????????????    ?
?  ?   MODE 1: Layout Commands                      ?    ?
?  ?   [Apply AI Layout]                            ?    ?
?  ?   ? Snapshot ? /ui_agent ? Commands ? Apply   ?    ?
?  ??????????????????????????????????????????????????    ?
?  ?   MODE 2: Design/Refactor Planning (NEW)      ?    ?
?  ?   [Generate Refactor Plan]                     ?    ?
?  ?   ? Enhanced Snapshot ? /ui_design_assistant  ?    ?
?  ?   ? Design Plan ? Save JSON                    ?    ?
?  ??????????????????????????????????????????????????    ?
???????????????????????????????????????????????????????????
                           ?
    ????????????????????????????????????????????????
    ?                      ?                       ?
    ?                      ?                       ?
???????????????    ?????????????????    ???????????????????
? UI Snapshot ?    ? UI Pattern    ?    ? Design Plan     ?
? (enhanced)  ?????? Catalog       ?    ? (timestamped)   ?
?             ?    ? (canonical)   ?    ? JSON files      ?
???????????????    ?????????????????    ???????????????????
     ?                     ?                       ?
     ?                     ?                       ?
     ???????????????????????????????????????????????
                           ?
                           ?
               ??????????????????????????
               ?  TOOLSERVER (FastAPI)  ?
               ?  /ui_agent (commands)  ?
               ?  /ui_design_assistant  ?  ? NEW
               ?    (refactor plans)    ?
               ??????????????????????????
```

---

## Usage

### Generate Design/Refactor Plan

1. **Launch Visualizer**
   ```bash
   .\bin\RogueCityVisualizerGui.exe
   ```

2. **Start AI Bridge**
   - Open "AI Console" panel
   - Click "Start AI Bridge"
   - Wait for "Online" status

3. **Generate Plan**
   - Open "UI Agent Assistant" panel
   - Scroll to "Design & Refactoring" section
   - Enter goal: `"Analyze UI for refactoring opportunities"`
   - Click **"Generate Refactor Plan"**
   - Wait for completion
   - Check output path: `AI/docs/ui/ui_refactor_YYYYMMDD_HHMMSS.json`

### Example Output

**File**: `AI/docs/ui/ui_refactor_20260206_143022.json`

```json
{
  "component_patterns": [
    {
      "name": "DataIndexPanel<T>",
      "template": "RcDataIndexPanel<T>",
      "applies_to": ["road_index", "district_index", "lot_index", "river_index"],
      "props": ["entities[]", "columns", "selected_id"],
      "description": "Generic sortable/filterable table of entities"
    }
  ],
  "refactoring_opportunities": [
    {
      "name": "Extract common DataIndexPanel pattern",
      "priority": "high",
      "affected_panels": ["road_index", "district_index", "lot_index", "river_index"],
      "suggested_action": "Create generic RcDataIndexPanel<T> template in visualizer/src/ui/patterns/",
      "rationale": "4 panels with nearly identical code structure - 80% code duplication"
    }
  ],
  "suggested_files": [
    "visualizer/src/ui/patterns/rc_ui_data_index_panel.h",
    "visualizer/src/ui/patterns/rc_ui_inspector_panel.h",
    "visualizer/src/ui/patterns/rc_ui_tool_strip.h"
  ],
  "summary": "Found 4 panels that can be refactored into a single DataIndexPanel<T> template...",
  "generated_at": "2026-02-06 14:30:22"
}
```

---

## Toolserver Integration (Phase 4)

### New Endpoint Required

#### `/ui_design_assistant`

**Request**:
```json
{
  "snapshot": {
    "panels": [
      {
        "id": "road_index",
        "role": "index",
        "owner_module": "rc_panel_road_index",
        "data_bindings": ["roads[]", "roads.selected_id"],
        "interaction_patterns": ["table+selection"]
      },
      { /* more panels... */ }
    ],
    "state_model": {
      "axiom.selected_id": "A123",
      "road.brush_radius": "15.0"
    }
  },
  "pattern_catalog": { /* loaded from ui_patterns.json */ },
  "goal": "Analyze UI for refactoring opportunities",
  "model": "qwen2.5:latest"
}
```

**Response**:
```json
{
  "component_patterns": [ /* ... */ ],
  "refactoring_opportunities": [ /* ... */ ],
  "suggested_files": [ /* ... */ ],
  "summary": "Found 4 panels that can be refactored..."
}
```

### System Prompt (Suggested)

```python
DESIGN_ASSISTANT_PROMPT = """You are a UI architecture assistant for RogueCity Visualizer.
Given a UI snapshot with code-shape metadata and a pattern catalog, analyze for refactoring opportunities.

Your response MUST be valid JSON matching this schema:
{
  "component_patterns": [{
    "name": "PatternName",
    "template": "RcPatternName<T>",
    "applies_to": ["panel_id1", "panel_id2"],
    "props": ["prop1", "prop2"],
    "description": "..."
  }],
  "refactoring_opportunities": [{
    "name": "Refactoring name",
    "priority": "high|medium|low",
    "affected_panels": ["panel_id1", ...],
    "suggested_action": "Create RcTemplate<T> in path/to/file.h",
    "rationale": "Why this refactoring improves the codebase"
  }],
  "suggested_files": ["path/to/file1.h", ...],
  "summary": "High-level summary of findings"
}

Focus on:
1. **Code duplication** - panels with similar structure
2. **Data binding patterns** - common data access patterns
3. **Interaction patterns** - reusable UI behaviors
4. **Role clustering** - panels with the same role

DO NOT suggest refactorings that:
- Break existing functionality
- Are purely cosmetic
- Require major architectural changes

Prioritize refactorings that:
- Reduce code duplication significantly (>70%)
- Make the codebase more maintainable
- Enable future feature development
"""
```

---

## Benefits

### Before Phase 4

**AI could only do**:
- Move panels
- Change modes
- Adjust settings

**Problems**:
- No understanding of code structure
- Can't help with refactoring
- Generates ad-hoc ImGui code

### After Phase 4

**AI can now**:
- **Analyze code structure** via metadata
- **Identify duplication** across panels
- **Suggest patterns** that reduce complexity
- **Propose refactorings** with priorities and rationale
- **Generate design documents** not just commands

**Benefits**:
- **Architectural guidance** from AI
- **Refactoring assistance** with concrete suggestions
- **Pattern library** that evolves with codebase
- **Documentation** of design decisions

---

## Next Steps

### Short Term (Do Now)

1. **Implement `/ui_design_assistant` endpoint** in toolserver
   - Add system prompt
   - Test with sample snapshot
   - Verify JSON output format

2. **Try a refactoring**:
   - Generate plan
   - Pick highest priority suggestion
   - Extract `RcDataIndexPanel<T>` template manually
   - Validate with AI feedback

### Medium Term

3. **Build pattern library**:
   - Create `visualizer/src/ui/patterns/` directory
   - Implement `RcDataIndexPanel<T>`
   - Implement `RcInspectorPanel<T>`
   - Implement `RcToolStrip`

4. **Iterate on catalog**:
   - Add more patterns as discovered
   - Update `ui_patterns.json` based on refactorings
   - Let AI learn from your pattern evolution

### Long Term

5. **AI-assisted pattern generation**:
   - Have AI generate C++ template stubs
   - Review and integrate into codebase
   - Build feedback loop: code ? AI ? better patterns

6. **Pattern DSL**:
   - Define simple DSL for UI composition
   - Have AI emit DSL instances
   - Your code interprets DSL into ImGui calls

---

## Testing Checklist

- [x] Enhanced UiSnapshot compiles
- [x] JSON serialization includes metadata
- [x] UI Pattern Catalog loads
- [x] Design Assistant client compiles
- [x] UI Agent Panel has both modes
- [x] Build successful
- [ ] Toolserver `/ui_design_assistant` endpoint implemented
- [ ] Generate actual design plan from running app
- [ ] Follow one refactoring suggestion manually
- [ ] Validate AI's analysis accuracy

---

## File Summary

### New Files (Phase 4)
```
AI/docs/ui/ui_patterns.json          (Pattern catalog - canonical UI patterns)
AI/client/UiDesignAssistant.h        (Design assistant client header)
AI/client/UiDesignAssistant.cpp      (Design assistant implementation)
AI/docs/ui/ui_refactor_*.json        (Generated refactor plans - timestamped)
```

### Modified Files (Phase 4)
```
AI/protocol/UiAgentProtocol.h        (Added code-shape metadata fields)
AI/protocol/UiAgentProtocol.cpp      (Updated JSON serialization)
AI/CMakeLists.txt                    (Added UiDesignAssistant sources)
visualizer/src/ui/panels/rc_panel_ui_agent.h    (Added design mode)
visualizer/src/ui/panels/rc_panel_ui_agent.cpp  (Dual-mode UI with refactor planning)
```

---

## Documentation Links

- **Phase 1**: `docs/AI_Integration_Phase1_Complete.md`
- **Phases 2 & 3**: `docs/AI_Integration_Phase2_3_Complete.md`
- **Phase 4**: This document
- **Summary**: `docs/AI_Integration_Summary.md`

---

**Status**: ? **Phase 4 Complete and Built**  
**Next**: Implement `/ui_design_assistant` endpoint and test refactoring workflow  
**Owner**: Coder Agent + UI/UX/ImGui Master

---

*Implementation Date: 2026-02-06*  
*Build: Successful*  
*Ready for Architecture-Aware AI Assistance*
