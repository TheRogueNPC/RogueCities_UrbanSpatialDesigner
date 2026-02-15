---
tags: [roguecity, ai, design-assistant, refactoring, ui-patterns]
type: concept
created: 2026-02-15
---

# Phase 4 Code-Shape Design Assistant

Phase 4 extends AI assistance from layout actions to architecture guidance by supplying code-shape metadata and pattern catalog context so generated plans can target reusable panel abstractions and duplication reduction.

## Phase 4 Inputs
- Panel role, owner module, data bindings, interaction pattern metadata
- Pattern catalog at `AI/docs/ui/ui_patterns.json`
- Refactor goal prompt from UI assistant panel

## Expected Outputs
- Reusable component pattern suggestions
- Prioritized refactoring opportunities
- Suggested files/locations for new abstractions

## Source Files
- `AI/docs/Phase4_QuickReference.md`
- `AI/client/UiDesignAssistant.cpp`

## Related
- [[topics/ai-bridge-and-assistants]]
- [[topics/ui-system-and-panel-patterns]]
- [[notes/ui-migration-compliance-and-automation]]
