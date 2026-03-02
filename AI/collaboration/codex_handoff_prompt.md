# RogueCities AI Collaboration Handoff: Welcome, Codex

**To:** Codex (GitHub Copilot / OpenAI)
**From:** Claude (Anthropic)
**Via:** RogueCities Unified Collaboration Space
**Date:** 2026-02-28

---

## 1. What You're Walking Into

You are joining a multi-agent engineering organization. Gemini Base Command built the foundation and briefed me; I'm now briefing you. We are not isolated volunteers — we share a workspace and a contract.

The core application is **RogueCities Urban Spatial Designer**: a C++/ImGui procedural city generator with a staged pipeline. It is well-architected, actively developed, and has zero tolerance for silent failures.

**First things first — read these two files before touching anything:**

- `AI/Agent-Contract.md` — your binding operational contract
- `.gemini/GEMINI.md` — canonical layer ownership and engineering mandates

---

## 2. The Three-Layer Architecture (Do Not Cross These)

| Layer | Directory | Owns | Forbidden |
|-------|-----------|------|-----------|
| 0 | `core/` | Domain data, `Vec2`, city entities, `GlobalState` | UI code, generation algorithms |
| 1 | `generators/` | Procedural algorithms, tensor/road logic, staged cache | Any ImGui/UI |
| 2 | `app/`, `visualizer/` | UI panels, HFSM state, bridge, themes | Generation policy |

If you're writing ImGui code, you're in `visualizer/`. If you touch a generator, stay out of ImGui. This is enforced.

---

## 3. What Gemini and I Have Already Built

Don't duplicate this work. Read, then extend.

### Stabilized Infrastructure
- **AiTelemetry Singleton** — crashes auto-serialize to `AI/diagnostics/runtime_crash.json`. If the app crashes, read that file. Do not ask the user to paste logs.
- **Headless Execution** — `RogueCityVisualizerHeadless.exe` runs without a display and dumps `AI/docs/ui/ui_introspection_headless_latest.json` (widget tree) and `AI/docs/ui/ui_screenshot_latest.png`. This is your ground truth for UI verification.
- **AI Bridge (port 7222)** — local Ollama stack with Gemma3:4b (synthesis), CodeGemma:2b (triage), Granite3.2-vision + GLM-OCR (visual perception). Verified operational as of 2026-02-28.

### UI System (Visualizer Layer)
- **WorkspaceProfile System** — 5 built-in personas (Rogue, Enterprise, Hobbyist, Planner, Artist) each with theme + layout presets. Files: `visualizer/include/RogueCity/Visualizer/WorkspaceProfile.hpp`, `visualizer/src/ui/WorkspaceProfile.cpp`, `visualizer/src/ui/panels/rc_panel_workspace.h/.cpp`
- **ThemeManager** — 8 built-in themes in `app/src/UI/ThemeManager.cpp`. Token source of truth: `visualizer/src/ui/rc_ui_tokens.h` → `UITokens::`. Use `CyanAccent` for active state. There is no `PrimaryAccent` token.
- **Mockup Live Link** — CSS `:root {}` vars in `visualizer/RC_UI_Mockup.html` hot-reload into the running app. Parser: `visualizer/src/ui/MockupTokenParser.cpp`.
- **Schema-2 Workspace Presets** — `AI/docs/ui/workspace_presets.json` stores `{ "schema": 2, "presets": { "name": { "ini": "...", "theme": {...} } } }`. Schema-1 files load gracefully.
- **RC_FEATURE_AI_BRIDGE** — feature flag (default OFF) in `AI/CMakeLists.txt`. Gates HTTP PostJson calls only. UiIntrospector is always on.

---

## 4. The Dev Shell is Mandatory

Do not run naked `cmake` or `ctest`. Use the PowerShell dev shell:

```powershell
# Source it first
. tools/dev-shell.ps1

# Then use these aliases
rc-bld-core          # build core
rc-cfg               # configure cmake
rc-perceive-ui -Mode quick -Screenshot   # UI validation after any panel change
rc-full-smoke -Port 7222 -Runs 1         # full pipeline smoke test
```

**If you modify any UI panel, you must run `rc-perceive-ui` and verify the widget tree.** We do not guess screen bounds.

---

## 5. Panel Implementation Pattern

Every panel follows this exact structure. Do not deviate:

```cpp
// rc_panel_<name>.h
namespace RC::Panel::Name {
    bool IsOpen();
    void Toggle();
    void Draw(float dt);
}

// rc_panel_<name>.cpp
static bool s_open = false;
bool IsOpen() { return s_open; }
void Toggle() { s_open = !s_open; }
static void DrawContent(float dt) { /* imgui widgets */ }
void Draw(float dt) {
    if (!s_open) return;
    auto& uiint = RogueCity::Visualizer::UiIntrospector::Get();
    uiint.BeginPanel("PanelId", "Title", "owner_module.cpp");
    DrawContent(dt);
    uiint.EndPanel();
}
```

Panels live in `visualizer/src/ui/panels/`. Register new ones in `visualizer/CMakeLists.txt` (both executables).

---

## 6. Your Collaboration Obligations

Per `AI/Agent-Contract.md` Section 6 — this directory (`AI/collaboration/`) is the shared brain:

- **Before starting work**: Read existing manifests here to know what's in flight.
- **After completing a feature**: Write a brief markdown file here documenting what you built, what files you touched, and any gotchas.
- **Layer ownership handoffs**: Note in your brief which layer you worked in so other agents don't collide.
- **Changelog is mandatory**: For any tangible change, append a concise atomic entry to `CHANGELOG.md` in the same session.
- **Mandatory reference**: Follow `AI/collaboration/CHANGELOG_MANDATE.md`.

Suggested naming: `codex_<feature>_brief.md` or `codex_<date>_session.md`.

---

## 7. Where to Focus Your Strengths

Based on what Gemini and I have built, here's where Codex can add the most value:

1. **`generators/` layer** — Procedural algorithms, staged pipeline, RK4 streamline integration, AESP zone formulas, Boost::Geometry predicates. This is pure C++ math work — no UI entanglement. Your code completion and algorithm work can shine here without layer compliance risk.
2. **Test Coverage** — `test_generators`, `test_editor_hfsm`, `test_full_pipeline`. The codebase needs more unit coverage on generator stages.
3. **`.agents/` skills** — We need Markdown skill files for: pulling headless screenshots, parsing `ui_introspection_headless_latest.json` to verify panel presence, and using `rc-full-smoke` as a commit gate. These are tool-agnostic markdown scripts any agent can execute.

---

## 8. Quick Reference: Key Paths

| What | Where |
|------|-------|
| Agent contract | `AI/Agent-Contract.md` |
| Layer mandates | `.gemini/GEMINI.md` |
| Crash telemetry | `AI/diagnostics/runtime_crash.json` |
| UI widget tree | `AI/docs/ui/ui_introspection_headless_latest.json` |
| UI screenshot | `AI/docs/ui/ui_screenshot_latest.png` |
| Workspace presets | `AI/docs/ui/workspace_presets.json` |
| Theme configs | `AI/config/themes/` |
| UI token constants | `visualizer/src/ui/rc_ui_tokens.h` |
| Collaboration space | `AI/collaboration/` (this directory) |

---

## 9. The Smoke Test Passed

As of 2026-02-28 01:49, `rc-full-smoke` confirms:
- AI Bridge health: **OK** (pid 266316, no mock)
- Ollama reachable: **true**
- Missing models: **none**
- Pipeline + perception endpoints: **present**
- `ok_core`: **true**, `ok_full`: **true**

The environment is stable. The contract is clear. Build something good.

---

*— Claude (Anthropic)*
*Collaboration brief filed: `AI/collaboration/codex_handoff_prompt.md`*
