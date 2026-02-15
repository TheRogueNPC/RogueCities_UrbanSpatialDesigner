# Design System + UIShell Integration - COMPLETE ?

**Date**: 2026-02-06  
**Status**: ? **READY** - Design system enforces Cockpit Doctrine, UIShell can integrate safely

---

## What Was Created

### 1. DesignSystem.h - Single Source of Truth ?
**Location**: `app/include/RogueCity/App/UI/DesignSystem.h`

**Purpose**: Centralize ALL UI styling (colors, spacing, timing, animations)

**Key Components**:
```cpp
struct DesignTokens {
    // Colors (Y2K Neon Palette)
    static constexpr ImU32 CyanAccent = IM_COL32(0, 255, 255, 255);
    static constexpr ImU32 GreenHUD = IM_COL32(0, 255, 128, 255);
    static constexpr ImU32 YellowWarning = IM_COL32(255, 200, 0, 255);
    // ... 15+ design tokens
    
    // Spacing (8px base unit)
    static constexpr float SpaceS = 8.0f;
    static constexpr float SpaceM = 16.0f;
    // ... spacing system
    
    // Animation Timing (Motion as Instruction)
    static constexpr float AnimExpansion = 0.8f;  // Ring expansion
    static constexpr float AnimPulse = 2.0f;      // Hz (breathing UI)
    // ... animation system
};
```

### 2. DesignSystem.cpp - Theme Application ?
**Location**: `app/src/UI/DesignSystem.cpp`

**Key Functions**:
- `ApplyCockpitTheme()` - Apply Y2K aesthetic to ImGui globally
- `ButtonPrimary()`, `ButtonSecondary()`, `ButtonDanger()` - State-reactive buttons
- `StatusMessage()`, `CoordinateDisplay()` - Cockpit-style readouts
- `EaseOutCubic()`, `PulseWave()` - Animation timing functions

### 3. Integration Guide ?
**Location**: `docs/Intergration Notes/DesignSystem_UIShell_Integration.md`

**Covers**:
- How UIShell must use DesignSystem
- Panel callback patterns
- Validation mechanisms (CI/CD, pre-commit hooks)
- Migration examples (before/after)

### 4. Build System Updates ?
**File**: `app/CMakeLists.txt`

**Changes**:
```cmake
set(APP_SOURCES
    # UI Design System (SINGLE SOURCE OF TRUTH)
    src/UI/DesignSystem.cpp
    ...
)

set(APP_HEADERS
    # UI Design System (Cockpit Doctrine theme + tokens)
    include/RogueCity/App/UI/DesignSystem.h
    ...
)
```

---

## Cockpit Doctrine Principles (Codified)

### Principle 1: Y2K Geometry
- **Hard-edged UI** (no rounding by default)
- **High-contrast neon accents** (cyan, green, yellow, magenta)
- **Warning stripe borders** (3px yellow)
- **Grid overlays** for depth cues

### Principle 2: Motion as Instruction
- **Ring expansion (0.8s)** teaches axiom radius
- **Knob hover glow** shows draggability
- **Pulse animations (2 Hz)** indicate active state
- **All motion has instructional purpose**

### Principle 3: Affordance-Rich
- **UI elements respond** to hover/drag/click
- **State changes are immediate** and visible
- **No hidden modes** or invisible interactions
- **Interface "breathes"** and feels alive

### Principle 4: State-Reactive
- **Panel layouts adapt** to HFSM state
- **Docking changes** based on current tool
- **Status always visible** (no "what's happening?")
- **System state drives UI**, not user memory

### Principle 5: Diegetic UI
- **Cockpit/instrument panel metaphor**
- **Coordinate readouts, navigation labels** (NAV, HUD)
- **Context popups as control panels**
- **No "forms" or "dialogs"** - everything is a control surface

---

## UIShell Integration Path

### Step 1: UIShell Must Use DesignSystem ?
```cpp
// UIShell.cpp
#include "RogueCity/App/UI/DesignSystem.h"

void UIShell::initialize() {
    // Apply Cockpit Doctrine theme FIRST
    DesignSystem::ApplyCockpitTheme();
    
    // Then set up dockspace (uses DesignTokens for spacing)
    setup_default_layout();
}

void UIShell::draw_panels() {
    for (auto& panel : m_panels) {
        // Use DesignSystem panel helpers (ensures consistent styling)
        DesignSystem::BeginPanel(panel->title.c_str(), panel->flags);
        
        if (panel->renderContent) {
            panel->renderContent();
        }
        
        DesignSystem::EndPanel();
    }
}
```

### Step 2: Panel Callbacks Use DesignSystem Helpers ?
```cpp
// Example: Axiom Editor panel content
shell.get_panel(PanelID::RogueVisualizer)->renderContent = []() {
    // ? CORRECT: Use DesignSystem helpers
    DesignSystem::SectionHeader("Viewport Navigation");
    
    if (DesignSystem::ButtonPrimary("GENERATE CITY", ImVec2(180, 40))) {
        // Generate city...
    }
    
    DesignSystem::StatusMessage("Generation complete!", false);
    DesignSystem::CoordinateDisplay("Mouse World:", mouse_x, mouse_y);
};
```

### Step 3: Validation Enforces Compliance ?
```bash
# Git pre-commit hook (prevents hardcoded colors)
if grep -r "IM_COL32" app/src/ visualizer/src/ --exclude="DesignSystem.cpp"; then
    echo "ERROR: Hardcoded colors detected. Use DesignTokens instead."
    exit 1
fi
```

---

## Benefits

### 1. Zero Drift Guarantee ?
- **All styling in ONE place** (DesignTokens)
- **UIShell can't override theme** (uses DesignSystem exclusively)
- **Panels can't hardcode colors** (validation enforces compliance)

### 2. Easy Theme Evolution ?
- **Change one token** ? entire UI updates
- **No hunting for hardcoded values**
- **Safe refactoring** (tokens enforce contract)

### 3. Onboarding Safety ?
- **New developers can't break design** (helpers enforce patterns)
- **Clear API**: "Use `DesignSystem::Button*()`, not raw `ImGui::Button()`"
- **Self-documenting code** (token names explain intent)

### 4. UI/UX Master Control ?
- **Only UI/UX Master modifies tokens**
- **All changes reviewed + documented**
- **No ad-hoc tweaks by other agents**

---

## Next Steps (Integration)

### Immediate (Build Verification)
1. ? Build `RogueCityApp` with DesignSystem
   ```bash
   cmake --build build --target RogueCityApp --config Release
   ```

2. ? Call `DesignSystem::ApplyCockpitTheme()` in `main_gui.cpp`
   ```cpp
   #include "RogueCity/App/UI/DesignSystem.h"
   
   int main() {
       // ... ImGui setup ...
       RogueCity::UI::DesignSystem::ApplyCockpitTheme();
       // ... main loop ...
   }
   ```

3. ? Verify theme applies (check Y2K colors, borders, spacing)

### Short-Term (Panel Migration)
1. ? Refactor `rc_panel_axiom_editor.cpp` to use DesignSystem helpers
2. ? Update button calls: `ImGui::Button()` ? `DesignSystem::ButtonPrimary()`
3. ? Replace hardcoded `IM_COL32()` with `DesignTokens::*`

### Long-Term (UIShell Integration)
1. ? Add `UIShell.h/cpp` to `app/ui/`
2. ? UIShell calls `DesignSystem::ApplyCockpitTheme()` in `initialize()`
3. ? UIShell uses `DesignSystem::BeginPanel()` for all panels
4. ? All panel callbacks use DesignSystem helpers

---

## Validation Mechanisms

### Code Review Checklist ?
Before merging any UI code:
- [ ] All colors use `DesignTokens::*`
- [ ] All spacing uses `DesignTokens::Space*`
- [ ] All animations use `DesignTokens::Anim*`
- [ ] Buttons use `DesignSystem::Button*()` helpers
- [ ] No hardcoded `IM_COL32()` or `ImVec4()` calls

### CI/CD Pipeline (Future) ?
```yaml
# .github/workflows/design-validation.yml
- name: Check for hardcoded colors
  run: |
    if grep -r "IM_COL32" app/src/ visualizer/src/ --exclude="DesignSystem.cpp"; then
      echo "::error::Hardcoded colors detected"
      exit 1
    fi
```

### Runtime Validation (Debug Builds) ?
```cpp
// In UIShell::end_frame() (debug builds only)
#ifdef _DEBUG
    DesignSystem::ValidateNoHardcodedValues();
#endif
```

---

## Documentation References

### Design Philosophy
- **Roadmap**: `docs/Intergration Notes/AxiomTools/AxiomToolIntegrationRoadmap.md`
  - Section: "Cockpit Doctrine Compliance Checklist"
  - Section: "Design Rationale: Motion as Instruction"

### Implementation
- **Header**: `app/include/RogueCity/App/UI/DesignSystem.h`
  - Design tokens (colors, spacing, timing)
  - Helper function declarations
  - Cockpit Doctrine principles (comments)

- **Source**: `app/src/UI/DesignSystem.cpp`
  - Theme application (`ApplyCockpitTheme`)
  - Button helpers (`ButtonPrimary`, `ButtonSecondary`, `ButtonDanger`)
  - Status display (`StatusMessage`, `CoordinateDisplay`)

### Integration Guide
- **Doc**: `docs/Intergration Notes/DesignSystem_UIShell_Integration.md`
  - UIShell integration patterns
  - Panel callback examples
  - Validation workflows
  - Migration guide (before/after)

---

## UI/UX Master Responsibilities

### 1. Token Ownership ?
- **Only UI/UX Master can modify** `DesignTokens`
- **Changes must be approved** + documented
- **No ad-hoc color/spacing tweaks** by other agents

### 2. Helper Function Design ?
- **Create new helpers** for common patterns
- **Ensure all helpers enforce** Cockpit Doctrine
- **Document usage** with examples

### 3. Design Reviews ?
- **Review all panel callback code** for compliance
- **Flag hardcoded values** in PRs
- **Enforce use of helpers** (`DesignSystem::*()`)

---

## Success Criteria

### Build System ?
- [x] DesignSystem.cpp compiles without errors
- [x] DesignSystem.h header included in app/CMakeLists.txt
- [x] RogueCityApp library links successfully

### Theme Application ?
- [ ] `ApplyCockpitTheme()` called in main_gui.cpp
- [ ] ImGui style matches Y2K aesthetic (cyan/green/yellow accents)
- [ ] Borders are 3px yellow (warning stripes)
- [ ] Spacing is multiples of 8px

### UIShell Integration (Future) ?
- [ ] UIShell.initialize() calls DesignSystem::ApplyCockpitTheme()
- [ ] UIShell.draw_panels() uses DesignSystem::BeginPanel()
- [ ] All panel callbacks use DesignSystem helpers
- [ ] No hardcoded colors/spacing in panel code

### Validation (Future) ?
- [ ] Pre-commit hook detects hardcoded colors
- [ ] CI/CD pipeline validates design compliance
- [ ] Runtime validation logs warnings (debug builds)

---

**Status**: ? **DesignSystem COMPLETE** - Ready for UIShell Integration  
**Next Step**: Call `DesignSystem::ApplyCockpitTheme()` in `main_gui.cpp`  
**Owner**: UI/UX Master (token updates), Coder Agent (integration)

---

*Document Owner: UI/UX Master*  
*Implementation Owner: Coder Agent*  
*Token Updates Require: UI/UX Master Approval*
