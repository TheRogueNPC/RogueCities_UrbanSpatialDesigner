# Resize Regression Checklist

Use this checklist after any changes to dock layout, viewport input, or OS window sizing behavior.

## 1. Static checks

1. Run `python3 tools/check_imgui_contracts.py`.
2. Run `python3 tools/check_generator_viewport_contract.py`.
3. Run `python3 tools/check_ui_compliance.py`.

## 2. Startup and restore

1. Launch app normally and verify all primary panes are legible.
2. Launch minimized, restore, and verify no malformed/blank dock panes.
3. Verify `Master Panel`, `RogueVisualizer`, and `Tool Deck` remain clickable.

## 3. Continuous resize sweep

1. Drag OS window down to minimum size.
2. Drag back to full size.
3. Repeat quickly and slowly.
4. Confirm no frame de-render, no dock corruption, and no blocked tabs/buttons.

## 4. Interaction during resize

1. Select each core tool domain.
2. Click in viewport after resize.
3. Verify viewport action still triggers and status updates in Dev Shell.

## 5. Docking invariants

1. Dock/undock/popout/redock windows repeatedly after multiple resizes.
2. Verify no window becomes permanently unreachable or non-interactive.
3. Verify `Ctrl+R` (soft reset) and `Ctrl+Shift+R` (hard reset) recover layout.

## 6. Final pass

1. Validate at `1280x1024` and `1920x1080`.
2. Confirm no clipped critical controls (tabs, filters, tool buttons).
3. Confirm resize flow has no helper popup windows stealing focus/capture.
4. Verify minimum window size contract: OS window must not shrink below `1100x700` (GLFW enforced).
5. Verify dock rebuild debouncing: dock layout should not churn during active drag, only rebuild after 3 stable frames.

## 7. Key implementation constants

- Minimum window size: `max(25% display, 1100x700)` via `glfwSetWindowSizeLimits`
- Dock stability frames: `kDockBuildStableFrameCount = 3`
- Dock stability tolerance: `kDockSizeStabilityTolerancePx = 2.0f`
- Minimum center ratio: `kMinCenterRatio = 0.55f` (55% for center workspace)
