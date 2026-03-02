# Codex Assignment Prompt - Visualizer Interaction + Tooling (2026-03-01)

## Lane
Own `visualizer` interaction/tooling behavior and selection-path hardening.

## Primary Goal
Eliminate interaction stubs and activate accelerated viewport selection/query paths for high-entity-count editing.

## Scope
1. Implement spatial-grid-backed pick/query functions currently stubbed to false.
2. Ensure click, box, and lasso selection paths consume accelerated queries where available.
3. Preserve fallback correctness when accelerated paths are unavailable.
4. Replace actionable planned/placeholder visualizer controls with real routed behavior where contracts already exist.
5. Keep undo-safe and deterministic selection/edit transitions.

## High-Signal Source Targets
1. `visualizer/src/ui/viewport/handlers/rc_viewport_handler_common.cpp`
2. `visualizer/src/ui/viewport/handlers/rc_viewport_non_axiom_pipeline.cpp`
3. `visualizer/src/ui/viewport/rc_viewport_interaction.h`
4. `visualizer/src/ui/viewport/rc_viewport_interaction.cpp`
5. `visualizer/src/ui/panels/rc_panel_axiom_editor.cpp`
6. `visualizer/src/ui/tools/rc_tool_contract.cpp`
7. `visualizer/src/ui/tools/rc_tool_dispatcher.cpp`

## Constraints
1. Keep current selection semantics: click select, shift add, ctrl toggle, drag box/lasso.
2. Do not regress domain handlers for road/district/lot/building/water.
3. Do not introduce non-deterministic ordering in multi-select results.

## Acceptance Criteria
1. Spatial pick/query stubs replaced with active behavior.
2. Region selection no longer full-scan-only when spatial path is valid.
3. Placeholder action paths reduced/wired where behavior exists.
4. Tests/validation cover selection correctness and no-regression.
5. Collaboration brief + changelog entry completed.

## Required Report Back
1. Interaction paths changed and why.
2. Accuracy/performance comparison before vs after.
3. Known edge cases.
4. Tests/validation evidence.

