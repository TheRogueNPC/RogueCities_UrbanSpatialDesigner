# UI Feature Wiring MVP Plan

Purpose: restore a semi-functional, production-safe UI surface by ensuring implemented editor features are reachable and state-correct from the current panel architecture.

## Current Wiring Baseline
- `Master Panel` routing is active through drawer registry + category tabs.
- `Tool Deck` chip clicks now activate both:
  - shared/popup tool-library windows (`RC_UI::ActivateToolLibrary` / `PopoutToolLibrary`)
  - editor HFSM tool modes (`EditorEvent::Tool_*`)
- `Tools` workflow panel is routed under `Tools` category (not mixed into `System`).
- `Inspector` content path is content-only and no longer opens an unmatched introspection scope.
- `Tools + AI feature panels` (`tools`, `ai_console`, `ui_agent`, `city_spec`) compile into `RogueCityVisualizerFeaturePanels.lib` for cleaner root-target output.

## MVP Wiring Priorities
1. Tool action commands:
   - Bind Water/Road/District/Lot/Building library buttons to concrete editing commands (selection, pen, simplify, etc.) instead of display-only placeholders.
2. Data-driven visibility:
   - Hide or disable actions that require missing runtime dependencies (no data loaded, unsupported mode, offline AI bridge).
3. Command status feedback:
   - Add per-tool execution result/status line in `Tools` panel and dev log integration.
4. Consistent panel ownership:
   - Keep all top-level windows dock/root managed.
   - Keep drawer `DrawContent()` paths free of top-level window creation.
5. Regression harness:
   - Validate minimized launch, restore, dock/undock, and tab clickability after each wiring batch.

## Visual Language Reuse Note
- The `System Map` styling has strong user approval and should be propagated to additional panels.
- Adopt the same style primitives in:
  - telemetry/inspector headers
  - tool status strips
  - key modal diagnostics panes
- Constraint: reuse the style tokens and motion language, not raw copy/paste panel code.

## Success Criteria
- Every visible tool/library control triggers a real action or clearly disabled fallback path.
- No input blocking regressions across docked/undocked states.
- No crashes when AI features are unavailable/offline.
- Compliance scripts and build pass with no new project warnings.
