# Tool Wiring Regression Checklist

Use this checklist after any change to tool libraries, dispatcher, or panel routing.

## 1. Static Gates

1. Run `python3 tools/check_tool_wiring_contract.py`.
2. Run `python3 tools/check_context_command_contract.py`.
3. Run `python3 tools/check_generator_viewport_contract.py`.
4. Run `python3 tools/check_imgui_contracts.py`.
5. Run `python3 tools/check_ui_compliance.py`.
6. Verify all checks pass with no contract violations.

## 2. Build

1. Build `RogueCityVisualizerGui` (Release and/or Debug).
2. Confirm no new warnings from project-owned files.

## 3. Library Click Behavior

1. Open each library: Axiom, Water, Road, District, Lot, Building.
2. Click every action icon once.
3. Verify each click updates observable runtime state (domain/subtool).
4. Verify no click is dead.

## 4. Mutation Safety

1. Click library actions and verify no unexpected generation executes.
2. Verify generation still works only from explicit control-panel buttons:
   - `Generate Zones & Buildings`
   - `Generate Lots`
   - `Place Buildings`
   - Water authoring explicit buttons

## 5. Runtime Telemetry

1. Open Dev Shell.
2. Confirm `Tool Runtime` fields update on each click:
   - Domain
   - Last Action ID/Label/Status
   - Dispatch Serial
3. Confirm Property tooling shows active tool runtime section.

## 6. Docking and Popout Invariants

1. Dock/undock library windows repeatedly.
2. Double-click popout title bar to re-dock; verify behavior is stable.
3. Confirm no input blocking overlays.

## 7. Startup / Restore

1. Launch minimized.
2. Restore window.
3. Verify tabs and library actions remain clickable.
4. Verify no panel bunching/overlap regressions.

## 8. Future Stub UX

1. Confirm stubs are visible-disabled:
   - Flow (Future)
   - Paths (Future)
   - FloorPlan (Future)
   - Furnature (Future)
2. Confirm tooltips show dependency/reason text.
3. Confirm clicking stubs does not crash.

## 9. Context Command UX

1. Right-click viewport opens configured default mode (Smart List / Pie / Global Palette).
2. Hotkeys work when viewport-focused and no text input is active:
   - `Space`
   - `/`
   - `` `~ ``
   - `P`
3. Verify `Ctrl+P` still opens Master Panel search, not viewport palette.

## 10. Generator/Viewport Ownership

1. Confirm preview output applies through `ApplyCityOutputToGlobalState`.
2. Confirm panel routes generation through `GenerationCoordinator`.
3. Confirm no panel-local output sync function reappears.
