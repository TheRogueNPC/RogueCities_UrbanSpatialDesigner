# AI/App Agent Assignment Prompt - Runtime + Integration (2026-03-01)

## Lane
Own `AI + app` runtime integration and command-path hardening.

## Primary Goal
Remove AI transport/runtime stubs and ensure app/runtime messaging reflects real behavior and supports new multi-lane features.

## Scope
1. Replace non-Windows HTTP stub path with working implementation or explicit guarded fallback with clear diagnostics.
2. Remove user-facing "HTTP stub" messaging and replace with actionable status/errors.
3. Extend command application wiring for behaviors introduced by current lanes.
4. Align app-level viewport/runtime descriptions with actual implemented behavior.
5. Ensure runtime health checks and failures are observable and debuggable.

## High-Signal Source Targets
1. `AI/tools/HttpClient.cpp`
2. `AI/client/UiAgentClient.cpp`
3. `AI/client/CitySpecClient.cpp`
4. `AI/integration/AiAssist.cpp`
5. `AI/runtime/AiBridgeRuntime.cpp`
6. `visualizer/src/ui/panels/rc_panel_ui_agent.cpp`
7. `app/include/RogueCity/App/Viewports/ViewportManager.hpp`
8. `app/src/Viewports/PrimaryViewport.cpp`

## Constraints
1. No silent failure paths for network/runtime operations.
2. Preserve existing working Windows behavior while adding missing platform behavior.
3. Keep command-application idempotent and safe.

## Acceptance Criteria
1. Non-Windows transport gap is closed or explicitly guarded with actionable diagnostics.
2. AI assist status text no longer references known stubs as expected behavior.
3. Command routing supports newly introduced lane behaviors.
4. Tests or scripted validation paths provided.
5. Collaboration brief + changelog entry completed.

## Required Report Back
1. Transport/runtime changes and platform behavior.
2. Command-path coverage improvements.
3. Validation scripts/tests run.
4. Remaining limitations (if any) with concrete follow-ups.

