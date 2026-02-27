# RogueCities — AI Integration

## AI Layer Structure (AI/)
```
AI/
  config/
    AiConfig.h/.cpp             ← runtime config (model, endpoint, feature flags)
    themes/                     ← custom theme JSON files
  client/
    UiAgentClient.h/.cpp        ← sends UiSnapshot → /ui_agent endpoint
    CitySpecClient.h/.cpp       ← sends CityIntent → /city_spec endpoint
    UiDesignAssistant.h/.cpp    ← code-shape analysis / refactoring suggestions
  protocol/
    UiAgentProtocol.h/.cpp      ← UiSnapshot, UiCommand types
  integration/
    AiAssist.h/.cpp             ← main bridge to editor
  runtime/
    AiAvailability.h/.cpp       ← health polling
    AiBridgeRuntime.h/.cpp      ← PowerShell-managed Ollama startup/shutdown
  tools/
    HttpClient.h/.cpp           ← WinHTTP client wrapper
    Url.h/.cpp                  ← URL helpers
  docs/
    Phase4_QuickReference.md    ← code-shape AI guide
    ToolserverIntegration.md    ← FastAPI endpoint spec
    ui/ui_patterns.json         ← canonical UI pattern catalog
    ui/ui_introspection_*.json  ← captured introspection snapshots
```

## Toolserver Endpoints (local Ollama, port 7077)

### POST /ui_agent
```json
Request: { "snapshot": {...UiSnapshot...}, "goal": "string" }
Response: [ {"cmd": "SetHeader", ...}, {"cmd": "DockPanel", ...}, ... ]
```
Commands: SetHeader, DockPanel, SetState, Request

### POST /city_spec
```json
Request: { "intent": CityIntent }
Response: CitySpec JSON
```

## UiSnapshot Structure (protocol/UiAgentProtocol.h)
Captures: panel state, HFSM mode, tool status, active selections

## AI Phases
- Phase 1: Bridge runtime control (PowerShell → Ollama health polling, start/stop from menu)
- Phase 2: UI Agent Protocol (natural language → JSON commands via WinHTTP)
- Phase 3: CitySpec MVP (description → structured city spec)
- Phase 4 (current): Code-shape aware refactoring
  - ui_patterns.json defines canonical patterns
  - UiDesignAssistant analyzes duplication, suggests refactors
  - Output: JSON plan saved to AI/docs/ui/ui_refactor_*.json for manual review

## Pattern Catalog (AI/docs/ui/ui_patterns.json)
Maps panels to roles, data bindings, interaction patterns.
Update when adding new panels or refactoring existing ones.
AI uses this for architectural suggestions.

## Starting AI Bridge
From startup scripts (Windows):
1. `tools/preflight_startup.ps1 -VerboseChecks`
2. Fallback: `StartupBuild.bat`
3. Fallback: `build_and_run.bat`
4. GUI path: `build_and_run_gui.ps1`

## Feature Gates
AI panels (AiConsole, UiAgent, CitySpec) are feature-gated.
Check `AiConfig` for enable/disable flags before touching AI panel code.
