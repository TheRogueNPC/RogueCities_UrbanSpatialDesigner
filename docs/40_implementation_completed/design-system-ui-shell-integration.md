# UIShell Integration with Design System Enforcement

**Date**: 2026-02-06  
**Purpose**: Guide for integrating UIShell while preserving Cockpit Doctrine design

---

## Design System Architecture

```
DesignSystem.h/cpp (Single Source of Truth)
    ? (tokens, helpers)
UIShell.h/cpp (Layout & Panel Management)
    ? (panel rendering)
Panel Callbacks (Content-only, use DesignSystem helpers)
```

**Key Principle**: DesignSystem owns ALL styling. UIShell and panels MUST NOT hardcode colors/spacing.

---

## Integration Checklist

### Step 1: Add DesignSystem to Build ?
```cmake
# app/CMakeLists.txt
set(APP_SOURCES
    ...
    src/UI/DesignSystem.cpp
)
```

### Step 2: Initialize Theme in main_gui.cpp
```cpp
#include "RogueCity/App/UI/DesignSystem.h"

int main() {
    // ... ImGui setup ...
    
    // Apply Cockpit Doctrine theme (BEFORE any UI rendering)
    RogueCity::UI::DesignSystem::ApplyCockpitTheme();
    
    // ... main loop ...
}
```

### Step 3: UIShell Must Use DesignSystem
```cpp
// UIShell.cpp
#include "RogueCity/App/UI/DesignSystem.h"

void UIShell::setup_dockspace() {
    // Use design tokens, NOT hardcoded values
    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
    
    // Border color from DesignSystem
    ImGui::PushStyleColor(ImGuiCol_Border, 
        DesignSystem::ToVec4(DesignTokens::YellowWarning));
    
    // ... docking setup ...
    
    ImGui::PopStyleColor();
}

void UIShell::draw_panels() {
    for (auto& panel : m_panels) {
        if (!panel->visible) continue;
        
        // Use DesignSystem panel helpers (ensures consistent styling)
        DesignSystem::BeginPanel(panel->title.c_str(), panel->flags);
        
        if (panel->renderContent) {
            panel->renderContent();  // User code, should also use DesignSystem
        }
        
        DesignSystem::EndPanel();
    }
}
```

### Step 4: Panel Callbacks Use DesignSystem Helpers
```cpp
// Example: Axiom Editor panel
shell.get_panel(PanelID::RogueVisualizer)->renderContent = []() {
    // ? CORRECT: Use DesignSystem helpers
    DesignSystem::SectionHeader("Viewport Navigation");
    
    if (DesignSystem::ButtonPrimary("GENERATE CITY", ImVec2(180, 40))) {
        // Generate city...
    }
    
    DesignSystem::StatusMessage("Generation complete!", false);
    DesignSystem::CoordinateDisplay("Mouse World:", mouse_x, mouse_y);
    
    // ? WRONG: Hardcoded colors
    // ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 150, 255, 255));
    // ImGui::Button("Generate");
    // ImGui::PopStyleColor();
};
```

---

## Enforcement Mechanisms

### 1. Code Review Checklist
Before merging any UI code:
- [ ] All colors use `DesignTokens::*`
- [ ] All spacing uses `DesignTokens::Space*`
- [ ] All animations use `DesignTokens::Anim*`
- [ ] Buttons use `DesignSystem::Button*()` helpers
- [ ] Status messages use `DesignSystem::StatusMessage()`
- [ ] No hardcoded `IM_COL32()` or `ImVec4()` calls

### 2. Static Analysis (Future)
```cpp
// Add to CI/CD pipeline
#define HARDCODED_COLOR_CHECK(x) static_assert(false, "Use DesignTokens")

// Example usage
ImU32 color = IM_COL32(255, 0, 0, 255);  // ? Fails at compile-time
HARDCODED_COLOR_CHECK(color);
```

### 3. Runtime Validation (Debug Builds)
```cpp
// In UIShell::end_frame() (debug builds only)
#ifdef _DEBUG
    DesignSystem::ValidateNoHardcodedValues();
#endif
```

---

## Migration Pattern for Existing Code

### Before (Hardcoded)
```cpp
// visualizer/src/ui/panels/rc_panel_axiom_editor.cpp
ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 150, 255, 255));
ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 180, 255, 255));
ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 120, 200, 255));

if (ImGui::Button("GENERATE CITY", ImVec2(180, 40))) {
    // ...
}

ImGui::PopStyleColor(3);
```

### After (Design System)
```cpp
#include "RogueCity/App/UI/DesignSystem.h"

if (DesignSystem::ButtonPrimary("GENERATE CITY", ImVec2(180, 40))) {
    // ...
}
```

**Benefits**:
- 1 line instead of 7
- Color consistency guaranteed
- Easy to change theme globally
- No risk of typos or drift

---

## UIShell Theme Integration

### UIShell.h Must Reference DesignSystem
```cpp
#include "RogueCity/App/UI/DesignSystem.h"

class UIShell {
public:
    // Apply Cockpit Doctrine theme (calls DesignSystem internally)
    static void initialize();
    
    // Validation: Ensure no panels hardcode styles
    void validate_design_compliance();
    
private:
    // Panels must use DesignSystem helpers
    std::vector<std::unique_ptr<Panel>> m_panels;
};
```

### UIShell::initialize() Implementation
```cpp
void UIShell::initialize() {
    // Apply Cockpit Doctrine theme globally
    DesignSystem::ApplyCockpitTheme();
    
    // Set up default dockspace layout (uses DesignTokens for spacing)
    setup_default_layout();
    
    // Register all panels (their callbacks MUST use DesignSystem)
    register_default_panels();
}
```

---

## Cockpit Doctrine Validation Workflow

### Pre-Commit Hook (Git)
```bash
# .git/hooks/pre-commit
#!/bin/bash
echo "Checking for hardcoded UI colors..."

# Search for IM_COL32 outside of DesignSystem.cpp
if grep -r "IM_COL32" app/src/ visualizer/src/ --exclude="DesignSystem.cpp" | grep -v "DesignTokens"; then
    echo "ERROR: Hardcoded colors detected. Use DesignTokens instead."
    exit 1
fi

echo "? Design system compliance OK"
```

### CI/CD Pipeline Check
```yaml
# .github/workflows/design-validation.yml
name: Design System Validation

on: [push, pull_request]

jobs:
  validate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      
      - name: Check for hardcoded colors
        run: |
          if grep -r "IM_COL32" app/src/ visualizer/src/ --exclude="DesignSystem.cpp"; then
            echo "::error::Hardcoded colors detected"
            exit 1
          fi
      
      - name: Check for hardcoded spacing
        run: |
          if grep -r "ImVec2([0-9]" app/src/ visualizer/src/ | grep -v "DesignTokens"; then
            echo "::error::Hardcoded spacing detected"
            exit 1
          fi
```

---

## Design Token Updates (How to Evolve Theme)

### ? Correct Process
1. Update `DesignTokens` in `DesignSystem.h`
2. Rebuild project
3. All UI automatically updates

```cpp
// Change accent color globally
static constexpr ImU32 CyanAccent = IM_COL32(0, 200, 255, 255);  // Lighter cyan
```

### ? Wrong Process
1. ~~Edit color in panel callback directly~~
2. ~~Copy-paste new color everywhere~~
3. ~~Result: Inconsistent UI, drift from design~~

---

## Example: Converting Existing Panel

### Before (Direct ImGui)
```cpp
void DrawAxiomEditor(float dt) {
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.12f, 1.0f));
    ImGui::Begin("Axiom Editor");
    
    ImGui::Text("Status: Ready");
    
    if (ImGui::Button("Generate")) {
        // ...
    }
    
    ImGui::End();
    ImGui::PopStyleColor();
}
```

### After (DesignSystem + UIShell)
```cpp
void SetupAxiomEditorPanel(UIShell& shell) {
    shell.get_panel(PanelID::RogueVisualizer)->renderContent = []() {
        DesignSystem::StatusMessage("Status: Ready", false);
        
        if (DesignSystem::ButtonPrimary("GENERATE CITY")) {
            // ...
        }
    };
}

// In main loop
void DrawUI(float dt) {
    shell.begin_frame();
    shell.draw_panels();  // Automatically uses DesignSystem styling
    shell.end_frame();
}
```

---

## Design System Benefits

### Consistency ?
- All panels use same colors/spacing/animations
- No accidental drift from Cockpit Doctrine
- Easy to maintain cohesive visual language

### Maintainability ?
- Change theme in ONE place (DesignSystem.h)
- No need to hunt down hardcoded values
- Safe refactoring (tokens enforce contract)

### Onboarding ?
- New developers can't accidentally break design
- Clear API: "Use `DesignSystem::Button*()`, not raw `ImGui::Button()`"
- Self-documenting code (token names explain intent)

---

## UI/UX Master Responsibilities

### 1. Token Ownership
- Only UI/UX Master can modify `DesignTokens`
- Changes must be approved + documented
- No ad-hoc color/spacing tweaks

### 2. Helper Function Design
- Create new `DesignSystem::*()` helpers for common patterns
- Ensure all helpers enforce Cockpit Doctrine
- Document usage with examples

### 3. Design Reviews
- Review all panel callback code for compliance
- Flag hardcoded values in PRs
- Enforce use of `DesignSystem::*()` helpers

---

## Final Integration Steps

1. ? Add `DesignSystem.h/cpp` to `app/` (done)
2. ? Update `app/CMakeLists.txt` to compile `DesignSystem.cpp`
3. ? Call `DesignSystem::ApplyCockpitTheme()` in `main_gui.cpp`
4. ? Refactor existing panels to use `DesignSystem::*()` helpers
5. ? Integrate UIShell with `DesignSystem::BeginPanel()`
6. ? Add validation to CI/CD pipeline

---

## Documentation References

- **Design Philosophy**: `docs/Intergration Notes/AxiomTools/AxiomToolIntegrationRoadmap.md`
  - Section: "Cockpit Doctrine Compliance Checklist"
  - Section: "Design Rationale: Motion as Instruction"

- **Implementation**: `app/include/RogueCity/App/UI/DesignSystem.h`
  - Color tokens
  - Spacing system
  - Animation timing
  - Helper functions

- **Examples**: `visualizer/src/ui/panels/rc_panel_axiom_editor.cpp`
  - Status messages
  - Button styling
  - Coordinate displays

---

**Status**: ? Design System Ready for UIShell Integration  
**Next Step**: Update `app/CMakeLists.txt` to build `DesignSystem.cpp`

---

*Document Owner: UI/UX Master*  
*Enforcement Owner: Coder Agent (validation scripts)*  
*Token Updates Require: UI/UX Master Approval*
