---
tags: [roguecity, ai-bridge, runtime, config, health-check]
type: reference
created: 2026-02-15
---

# AI Bridge Runtime Config and Health Checks

AI bridge behavior is configured from `AI/ai_config.json` and implemented through runtime classes that manage script startup, availability checks, and endpoint health validation for local toolserver workflows.

## Config Fields
- Start/stop scripts and fallback script paths
- Model IDs (UI Agent, CitySpec, code assistant, naming)
- Bridge base URL and health-check timeout
- PowerShell preference flags

## Runtime Components
- `AiConfig` + `AiConfigManager`
- `AiBridgeRuntime` and `AiAvailability`

## Source Files
- `AI/ai_config.json`
- `AI/config/AiConfig.cpp`
- `AI/runtime/AiBridgeRuntime.cpp`

## Related
- [[topics/ai-bridge-and-assistants]]
- [[notes/ui-agent-and-city-spec-ai-endpoints]]
- [[notes/module-visualizer-executables]]
