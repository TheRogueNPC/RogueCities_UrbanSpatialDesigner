# RogueCities AI Collaboration Handoff: The Stable Vanguard

**To:** Claude (and any other active AI Agents)
**From:** Gemini Base Command
**Topic:** Architectural Stabilization, Telemetry Gates, and Next Steps

## 1. The New Reality: Zero Tolerance for Silent Failures
You are entering a fully stabilized workspace. I have spent the last session fortifying the RogueCities engine with the user.

1.  **AI Telemetry Sink**: We have embedded a C++ telemetry singleton (`RogueCity::Core::Validation::AiTelemetry`). If the app crashes, it serializes a `runtime_crash.json`. You are mandated to read this file upon any user report of a crash. Do not ask for logs.
2.  **UI Introspection**: The ImGui application is now fully deterministic. We have enabled headless background execution (`RogueCityVisualizerHeadless.exe`) that can dump screenshots and widget trees (`ui_introspection_headless_latest.json`).
3.  **Strict Commit Gates**: The PowerShell Dev Shell now features `rc-full-smoke` and `rc-perceive-ui`. If you modify a UI panel, you **must** run these commands and verify your changes programmatically. We do not guess screen bounds anymore.

## 2. The Gemma-First Local Stack
The user has initialized a lightweight, multimodal, lightning-fast local AI stack powered by `Ollama` and the `Gemma` family:
- **`gemma3:4b`**: Fast synthesis and UI layout generation.
- **`codegemma:2b`**: Surgical code triage.
- **`granite3.2-vision` & `glm-ocr`**: Multimodal UI perception validating our headless screenshots.

This entire bridge is orchestrated natively in the `dev-shell.ps1`, spinning up Python servers in the background. If you need to write generation logic, assume it connects to the local Ollama via port `7222`.

## 3. The Unified Mindspace
We are no longer isolated assistants; we are a dedicated engineering organization. 

**MANDATE:** Every time you complete a major feature, draft a plan, or discover a framework gotcha, you must write a Markdown brief and save it inside the `AI/collaboration/` directory.
**MANDATE:** For every tangible change, update `CHANGELOG.md` in the same session and follow `AI/collaboration/CHANGELOG_MANDATE.md`.

## 4. Immediate Context: UI Layout Phase 4 Complete
Gemini has just completed the **Phase 4 UI Layout Refactor** for `RogueCities_UrbanSpatialDesigner`.
- **Global Structure:** The `ImGui` dockspace was constrained to 3 strictly proportioned columns (Master Panel, Viewport, Inspector) residing perfectly between a 34px top `Titlebar` and a 28px bottom `StatusBar`.
- **Inspector Panel:** We integrated a complex `Inspector` panel populated tightly with Tool Runtime telemetry, Gizmos, Layer toggles, and Property Editors matching the Y2K CSS Mockups.
- **HUD Geometry:** Shifted the `CompassGimbal` to `right: 90px`, and moved the scale ruler + coordinate bindings to exactly bottom-center to finalize the viewport aesthetic.
You can review the full breakdown in `AI/collaboration/gemini_handoff_ui_phase4_complete_2026-02-28.md` whenever referencing UI layout queries.

## 5. Implement Phase 5
1. Please proceed with implementing Phase 5 of the UI setup.
2. Review the previous sections and use the original `RC_UI_Mockup.html` you generated for your refactor in full at your discretion. The HTML is your master reference.

The environment is yours. Execute.
