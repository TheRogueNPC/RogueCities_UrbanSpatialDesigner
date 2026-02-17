# Tool→Viewport/Generation Contract Completion + Smooth Resize Redraw Plan (S0–S6)

## Summary
This plan closes the remaining gaps in your tool interaction pipeline and resize behavior with one consistent contract:
1. Tool clicks must always produce visible viewport state changes.
2. Axiom generation remains live/debounced; other domains are explicit-generation by policy.
3. Resize must redraw smoothly without malformed dock rebuild states, clipping, or hidden/collapsed content.
4. Docking remains mission-critical and unchanged in capability.

## Locked Decisions
1. Input gate policy: **Viewport-local** (chosen by me per your defer).
2. Resize policy: **Debounced rebuild** (chosen).
3. Min window policy: **`max(25% display, 1100x700)`** (chosen).
4. Domain generation policy: **Hybrid by domain**, locked as **Axiom live + others explicit** (chosen).

## S0 Intake / Router
1. Lane A: Fix viewport click ingress so selected tools can act immediately.
2. Lane B: Finish tool→viewport mutation contract with deterministic behavior by domain/subtool.
3. Lane C: Finalize generator application contract so only approved paths mutate generation state.
4. Lane D: Redesign resize/dock redraw behavior for smooth OS-window resizing.
5. Lane E: Add guardrails (checks + docs) so this cannot regress.

## S1 Research (Ground Truth)
1. Current click blocker is the input gate: `visualizer/src/ui/rc_ui_input_gate.cpp:20` requires `!io.WantCaptureMouse`, which blocks viewport-local editing in ImGui windows.
2. Non-axiom interaction immediately exits when gate is false at `visualizer/src/ui/viewport/rc_viewport_interaction.cpp:1255`.
3. Tool dispatch is centralized and working (`visualizer/src/ui/tools/rc_tool_dispatcher.cpp`), so issue is not action cataloging.
4. Generation coordinator/applier modules exist and are wired (`app/src/Integration/GenerationCoordinator.cpp`, `app/src/Integration/CityOutputApplier.cpp`), so remaining work is contract completion and policy enforcement.
5. Resize/docking still includes auto-dirty on significant viewport size change (`visualizer/src/ui/rc_ui_root.cpp:1000`) and warning-window behavior (`visualizer/src/ui/rc_ui_root.cpp:786`) that can reintroduce malformed resize states.
6. Main loop has manual window clamp/continue path (`visualizer/src/main_gui.cpp:289`) in addition to GLFW size limits, which can cause resize churn.
7. Compliance scripts are passing now, but they do not yet enforce the specific anti-pattern that caused the current viewport click regression.

## S2 Plan (Implementation Packets + Acceptance Criteria)

### WP1: Input Gate Contract Rewrite (P0 blocker)
1. Refactor `BuildUiInputGateState` in `visualizer/src/ui/rc_ui_input_gate.h` and `visualizer/src/ui/rc_ui_input_gate.cpp`.
2. Remove `!imgui_wants_mouse` as a hard blocker for viewport-local interactions.
3. Gate mouse actions by viewport canvas hover + no popup/modal + no active text edit + not blocked by overlay.
4. Keep keyboard gate strict with `!WantTextInput` and popup checks.
5. Acceptance: selecting any core tool and left-clicking viewport produces interaction on first eligible click.

### WP2: Canonical Viewport Interaction Surface
1. In `visualizer/src/ui/panels/rc_panel_axiom_editor.cpp`, define a canonical viewport canvas item (`InvisibleButton` region) and derive interaction rect/hover from that item.
2. Route all viewport hover and click eligibility through this canvas state.
3. Keep minimap overlay exclusion explicit and local to this surface.
4. Acceptance: no ghost frames intercept input; clickability is stable across docked/floating states.

### WP3: Tool→Viewport Deterministic Mutation Contract
1. In `visualizer/src/ui/viewport/rc_viewport_interaction.cpp`, enforce one explicit left-click path per active domain/subtool.
2. Add status emission when a selected action is "activate-only" and has no mutation path, so "nothing happened" becomes explicit feedback.
3. Ensure dispatcher-selected subtool is visibly reflected in properties/dev shell each frame.
4. Acceptance: every enabled tool action leads to one of mutation, selection, gizmo interaction, or explicit non-mutating status.

### WP4: Domain Generation Policy Enforcement
1. Add a central policy map in app/UI integration layer: `Axiom=LiveDebounced`, `Water/Road/District/Zone/Lot/Building=Explicit`.
2. Keep live requests via `GenerationCoordinator::RequestRegeneration` for Axiom edits.
3. Non-axiom edits mark dirty layers and queue explicit generate availability (no accidental auto-destructive generation).
4. Keep `ApplyCityOutputToGlobalState` as the single output-application path.
5. Acceptance: generation is predictable, policy-driven, and no mutation bypasses coordinator/applier contract.

### WP5: Smooth Resize + Redraw Contract
1. Remove manual per-frame resize correction loop in `visualizer/src/main_gui.cpp` and rely on `glfwSetWindowSizeLimits`.
2. In `visualizer/src/ui/rc_ui_root.cpp`, keep dock rebuild debounced and stable-only; do not churn rebuild during active drag.
3. Remove runtime warning popup window from `UpdateDynamicPanelSizes`; replace with non-blocking status text if needed.
4. Enforce no-collapse/no-hide behavior for small sizes: use wrapping + scroll containers instead of collapsed/cull modes.
5. Acceptance: resizing OS window continuously keeps UI legible and responsive; no de-rendered pane states.

### WP6: Responsive Content Rules (No Clipping)
1. Normalize panel internals to metric-driven sizing (`GetFrameHeight`, `GetFontSize`) and wrapping.
2. Ensure tab bars use fitting/scroll policies where needed and content uses child scrolling for overflow.
3. Remove any remaining "collapsed mode returns" in key production panels/tool libraries.
4. Acceptance: at small but valid window sizes, text/buttons remain visible or scrollable, never clipped or hidden unexpectedly.

### WP7: Guardrail Checks
1. Extend `tools/check_imgui_contracts.py` with a rule that flags viewport gate patterns that hard-block on `WantCaptureMouse`.
2. Extend `tools/check_generator_viewport_contract.py` to validate domain generation policy mapping and explicit-generation routing.
3. Add a check ensuring viewport interaction derives from canonical canvas state (not ad-hoc hover checks).
4. Acceptance: CI fails if old input-gate anti-pattern or bypass paths reappear.

### WP8: Documentation Updates
1. Update `docs/20_specs/ui-loop-state-model.md` with the new gate state machine and resize redraw lifecycle.
2. Add `docs/20_specs/tool-viewport-generation-contract.md` with domain policy matrix and click outcomes.
3. Add `docs/30_runbooks/resize-regression-checklist.md` with exact reproduction/verification steps.
4. Acceptance: docs match runtime behavior and are sufficient for future contributors to avoid this regression class.

## S3 Duality (Risks + Preconditions)
1. Risk: loosening mouse gate can cause accidental viewport edits while interacting with UI.
2. Mitigation: canvas-local gating + popup/modal/text-input blocks + overlay exclusion.
3. Risk: resize debouncing can delay desired reflow on large resizes.
4. Mitigation: thresholded dirtying + stable-frame debounce + manual reset hotkeys retained.
5. Risk: users perceive non-axiom no-auto-generate as broken.
6. Mitigation: explicit status and command affordance for generation trigger.

## S4 Execute (Order)
1. WP1 input gate hotfix.
2. WP2 canonical viewport surface.
3. WP3 deterministic mutation contract.
4. WP4 generation policy enforcement.
5. WP5 resize/redraw rework.
6. WP6 responsive no-clipping pass.
7. WP7 contract check extensions.
8. WP8 docs and runbooks.
9. Run static checks + build + manual sweeps.

## S5 Check (Approve/Revise Gates)
1. Tool-click gate: no "click does nothing" on eligible viewport clicks.
2. Resize gate: no malformed panels during live resize at valid min sizes.
3. Docking gate: dock/undock/popout/redock unchanged in capability.
4. Generation gate: domain policy behavior matches matrix.
5. Compliance gate: all contract scripts pass.

## S6 Verify (Coverage + Exit Criteria)
1. Core tools all produce deterministic viewport outcomes.
2. Axiom live generation works; other domains explicit by policy.
3. Smooth redraw during resize at 1280x1024 and 1920x1080.
4. No clipped controls under contract min size.
5. All checks pass and docs are updated.

## Important API / Interface Changes
1. `visualizer/src/ui/rc_ui_input_gate.h`
   - `UiInputGateState` gains explicit block-reason semantics.
   - `BuildUiInputGateState(...)` inputs align to canvas-local context.
2. `core/include/RogueCity/Core/Editor/GlobalState.hpp`
   - Add `GenerationMutationPolicy` mapping/config entries for domains.
3. `visualizer/src/ui/viewport/rc_viewport_interaction.h`
   - Add explicit interaction outcome/status enum for non-mutating actions.
4. `visualizer/src/ui/panels/rc_panel_axiom_editor.cpp`
   - Canonical viewport canvas state becomes authoritative input source.

## Test Cases and Scenarios
1. Tool interaction: select each core tool/subtool, click viewport, verify deterministic result.
2. Input arbitration: text field focused, ensure viewport hotkeys/actions do not fire.
3. Resize sweep: drag OS window continuously down to min, then up; verify no malformed layout.
4. Startup/minimized restore: verify fully legible layout and interactive controls.
5. Docking invariants: repeated dock/undock/popout cycles remain stable.
6. Generation policy: Axiom live updates; non-axiom edits require explicit generation trigger.
7. Contract scripts: `python3 tools/check_ui_compliance.py` and all sub-checks pass.

## Assumptions and Defaults
1. 2.5D tool/viewport work remains the priority; 3D attachment is deferred.
2. Docking is non-negotiable and must not regress.
3. Multi-viewport remains opt-in and unchanged by this pass.
4. Explicit generation for non-axiom domains is acceptable for this phase.
5. Minimum size contract stays `max(25% display, 1100x700)`.

# Phase 2: Architecture & Polish (RC-0.10)

## Summary
Focus shifts from core stability (contracts) to application architecture, visual polish, and performance scalability.

## P0: Architecture Refactor
1. **Application Class Extraction** (`main_gui.cpp` -> `RogueCityApp`)
   - Decouple generic GLFW/ImGui loop from Rogue Cities logic.
   - Allow headless runs for CI/testing without graphics context.
2. **Dynamic Panel System** (`rc_ui_root.h`)
   - Replace static `Draw*` calls with a `PanelManager`.
   - Support dynamic registration of tool windows (plugins).

## P1: UI/UX Polish
1. **Theming Engine** (`rc_ui_theme.h`)
   - Centralize colors into `UITokens`.
   - Support "Day/Night" or "Cyberpunk/Clean" theme switching.
2. **Animation Framework**
   - Standardize `ContextWindowPopup` ease-in-out (0.3s).
   - Add visual feedback for "Generation" events (pulse effects).

## P2: Performance
1. **Async Task Queue** (`CityGenerator.hpp`)
   - Move heavy generation steps (Roads, Zoning) off the main UI thread.
   - Add a "Job Manager" UI panel to show progress.

## P3: AI & Features
1. **HFSM Simulation**
   - Implement `EditorState` simulation steps for traffic/pedestrians.
2. **Mini-Map Detachment**
   - Allow the minimap to be a floating window (multi-monitor support).
Execution Checklist
[ ] Edit visualizer/src/ui/patterns/rc_ui_data_index_panel.h to remove DrawLegacy.

[ ] Update Index Panels to generic Drawer pattern.

[ ] Prune rc_ui_responsive.h.

[ ] mv .github/prompts/PLAN.md docs/30_runbooks/completed/PHASE_1_PLAN.md.

[ ] Create docs/plans/PHASE_2_NEXT_ITERATION.md.
