# Gemini Pro Assignment Prompt - Visualizer Rendering (2026-03-01)

## Lane
Own `visualizer` rendering modernization for 3D-readiness.

## Primary Goal
Move rendering behavior from marker-only/2D assumptions toward contract-driven 3D-ready presentation while preserving current usability.

## Scope
1. Consume new core/generator metadata introduced by the core/generator lane.
2. Upgrade building rendering from simple marker-only representation when footprint/height-capable data exists.
3. Render road vertical/layer distinctions from preserved generator metadata.
4. Keep graceful fallback when new metadata is absent.
5. Remove or replace rendering-side "future" placeholder behavior where implementation is now possible.

## High-Signal Source Targets
1. `visualizer/src/ui/viewport/rc_viewport_overlays.cpp`
2. `visualizer/src/ui/viewport/rc_viewport_overlays.h`
3. `visualizer/src/ui/viewport/rc_scene_frame.h`
4. `visualizer/src/ui/viewport/rc_scene_frame.cpp`
5. `visualizer/src/ui/viewport/rc_viewport_scene_controller.cpp`
6. `app/src/Viewports/PrimaryViewport.cpp`
7. `app/include/RogueCity/App/Viewports/PrimaryViewport.hpp`

## Constraints
1. No visual regressions for existing road/district/lot visibility.
2. Maintain performance with large scenes.
3. Preserve deterministic LOD behavior.

## Acceptance Criteria
1. Rendering reflects new metadata when present.
2. Existing scenes still render correctly without new metadata.
3. Removed/reduced known placeholder render comments/paths where implemented.
4. Deterministic validation path or tests included.
5. Collaboration brief + changelog entry completed.

## Required Report Back
1. New render behaviors by entity type.
2. Fallback behavior matrix.
3. Performance notes and any observed hotspots.
4. Tests/validation evidence.

