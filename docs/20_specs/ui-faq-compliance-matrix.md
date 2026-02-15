# UI FAQ Compliance Matrix

Purpose: map Dear ImGui FAQ rules to concrete implementation contracts in RogueCity UI.

## Scope
- FAQ reference source: `docs/60_operations/imgui-faq-reference.md`
- Enforced paths: `visualizer/src/main_gui.cpp`, `visualizer/src/ui/`, `visualizer/src/ui/panels/`, `tools/check_ui_compliance.py`, `tools/check_imgui_contracts.py`
- Policy: hard-gate on compliance script failures.

## Matrix

| Rule Area | FAQ Reference | Contract | Implementation Owner | Gate/Verification |
|---|---|---|---|---|
| Input dispatch and capture | `imgui-faq-reference.md` section "How can I tell whether to dispatch mouse/keyboard" | Viewport actions are allowed only through centralized gate using `io.WantCaptureMouse/Keyboard/TextInput`. | `visualizer/src/ui/rc_ui_input_gate.cpp`, `visualizer/src/ui/panels/rc_panel_axiom_editor.cpp` | Runtime check in Dev Shell input diagnostics + manual regression tests |
| Avoid manual hover-only arbitration | same section as above | No standalone hover heuristics for editor actions without gate arbitration. | `visualizer/src/ui/panels/rc_panel_axiom_editor.cpp` | `tools/check_imgui_contracts.py` + manual interaction tests |
| Clip rect correctness | FAQ clipping guidance | Renderer backend clipping must remain upstream backend behavior. | `3rdparty/imgui/backends/*` (do not fork behavior) | Build + visual sanity checks |
| ID stack safety | FAQ ID stack section | Loop-generated widgets with static labels must use scoped IDs (`PushID`). | `visualizer/src/ui/panels/*.cpp` | `tools/check_imgui_contracts.py` loop ID heuristic |
| Draw layer ownership | FAQ click-react + stack correctness guidance | `DrawContent()` must be content-only. Top-level window creation is root/wrapper-only. | `visualizer/src/ui/panels/*`, `visualizer/src/ui/rc_ui_root.cpp` | `tools/check_imgui_contracts.py` (forbidden calls in `DrawContent`) |
| Dock host correctness | Docking best-practice alignment | Dock host uses `ImGuiWindowFlags_NoDocking`; dockspace rebuild only on stable viewport after restore. | `visualizer/src/ui/rc_ui_root.cpp` | Startup/minimize/restore scenario tests |
| DPI handling | FAQ DPI section | DPI behavior is user-configurable in `EditorConfig` and applied at startup (`ConfigDpiScaleFonts/Viewports`). | `core/include/RogueCity/Core/Editor/GlobalState.hpp`, `visualizer/src/main_gui.cpp`, `visualizer/src/ui/panels/rc_panel_ui_settings.cpp` | Build + run test at 1280x1024 and 1920x1080 |
| Multi-viewport policy | FAQ + project policy | Default OFF, explicit opt-in via config/env override for soak testing. | `visualizer/src/main_gui.cpp` | Config review + manual test |
| Legacy IO writes | FAQ input API guidance | No direct assignment to `io.MousePos/io.MouseDown/io.Key*`; event API only. | `visualizer/src/main_gui.cpp` | `tools/check_imgui_contracts.py` |
| AI panel resilience | Runtime safety requirement | Worker threads never call ImGui APIs; thread bodies are exception-contained and marshal results back to UI state. | `visualizer/src/ui/panels/rc_panel_ui_agent.cpp`, `visualizer/src/ui/panels/rc_panel_city_spec.cpp` | Manual AI offline/online stress test |

## Ownership Rules
- Root orchestrator (`rc_ui_root.cpp`) is the only owner of top-level library windows.
- `Panels::AxiomEditor::DrawAxiomLibraryContent()` is content-only and must not open/close windows.
- All viewport interaction paths publish gate state each frame with `PublishUiInputGateState()`.

## Required Test Pass
1. `python3 tools/check_ui_compliance.py`
2. `python3 tools/check_imgui_contracts.py`
3. Launch minimized, restore, verify no bunching and clickable tabs.
4. Dock/undock `Master Panel`, `RogueVisualizer`, `Tool Deck`, library windows.
5. Verify AI panels with bridge offline/online do not crash.
