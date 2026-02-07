# Phase 4: Code-Shape Aware AI - Quick Reference

## What's New

**The AI can now understand your code structure** and suggest architectural refactorings, not just layout tweaks.

## Key Concepts

### 1. Code-Shape Metadata
Every panel now tells the AI:
- **Role**: What it does (inspector, toolbox, viewport, nav, log, index)
- **Owner Module**: Which file implements it
- **Data Bindings**: What data it displays/edits
- **Interaction Patterns**: How users interact with it

### 2. Pattern Catalog
`AI/docs/ui/ui_patterns.json` defines:
- Canonical patterns (InspectorPanel<T>, ToolStrip, DataIndexPanel<T>)
- Current panel-to-pattern mappings
- Known refactoring opportunities

### 3. Two AI Modes
**Mode 1: Layout Commands** ? Apply immediately  
**Mode 2: Design/Refactor Planning** ? Save JSON, review manually

---

## Usage

### Generate Refactor Plan

1. **Start AI Bridge** (AI Console panel)
2. **Open UI Agent Assistant** panel
3. **Scroll to "Design & Refactoring"** section
4. **Enter goal** (or use default "Analyze UI for refactoring opportunities")
5. **Click "Generate Refactor Plan"**
6. **Check output**: `AI/docs/ui/ui_refactor_YYYYMMDD_HHMMSS.json`

### Review Plan

Open the generated JSON file. Look for:
- **component_patterns**: Suggested reusable templates
- **refactoring_opportunities**: Specific refactorings with priorities
- **suggested_files**: Where to create pattern implementations
- **summary**: High-level overview

### Follow a Suggestion

Example: "Extract common DataIndexPanel pattern"

**Manual steps**:
1. Create `visualizer/src/ui/patterns/rc_ui_data_index_panel.h`
2. Implement generic `RcDataIndexPanel<T>` template
3. Replace `road_index`, `district_index`, `lot_index`, `river_index` implementations
4. Test and validate

---

## Toolserver Integration

### New Endpoint: `/ui_design_assistant`

**Input**:
- Enhanced UiSnapshot (with code metadata)
- Pattern catalog (from `ui_patterns.json`)
- Goal string

**Output**:
- Component patterns
- Refactoring opportunities (with priorities)
- Suggested file paths
- Summary

**System Prompt Guidelines**:
- Focus on code duplication (>70%)
- Recognize common data binding patterns
- Cluster panels by role
- Prioritize high-impact, low-risk refactorings

---

## Example Workflow

### Scenario: Too much duplicate code in index panels

1. **Generate plan**: "Extract common index panel pattern"
2. **AI analyzes**: Finds 4 panels (`road_index`, `district_index`, `lot_index`, `river_index`) with 80% code overlap
3. **AI suggests**: Create `RcDataIndexPanel<T>` template
4. **AI provides**:
   - Pattern definition
   - Props: `["entities[]", "columns", "selected_id"]`
   - Priority: High
   - Rationale: "Eliminate 80% code duplication"
5. **You implement**: Extract template manually, guided by AI's analysis
6. **Update catalog**: Add new pattern to `ui_patterns.json`
7. **Iterate**: Run again, AI learns from your changes

---

## Benefits

### Before Phase 4
- AI proposes: "Move panel to left dock"
- You implement: Change dock setting
- Result: Layout improved

### After Phase 4
- AI proposes: "Extract DataIndexPanel<T> from 4 panels"
- You implement: Create reusable template
- Result: **Codebase structure improved**

**Phase 4 moves AI from "layout helper" to "architecture assistant"**

---

## Files to Know

```
AI/docs/ui/ui_patterns.json              ? Edit to define new patterns
AI/docs/ui/ui_refactor_*.json            ? Review generated plans
AI/client/UiDesignAssistant.h/.cpp       ? Design assistant implementation
visualizer/src/ui/patterns/ (future)     ? Create pattern templates here
```

---

## Next Steps

1. **Implement toolserver endpoint** (`/ui_design_assistant`)
2. **Generate first plan** from running visualizer
3. **Pick one refactoring** (highest priority)
4. **Implement manually** using AI's guidance
5. **Update catalog** with new pattern
6. **Iterate**: Generate plan again, see if AI recognizes your new pattern

---

**Phase 4 is about making the AI code-aware, so it can help you build better software, not just better layouts.**
