---
tags: [roguecity, ui-agent, city-spec, ai-endpoints, toolserver]
type: reference
created: 2026-02-15
---

# UI Agent and CitySpec AI Endpoints

The AI docs define endpoint contracts where snapshot-and-goal payloads are exchanged with a local OpenAI-compatible model server, returning machine-readable commands (UI Agent) or structured city specs (CitySpec).

## Endpoint Concepts
- `/ui_agent`: consumes UI snapshot + goal, returns command list JSON
- `/city_spec`: consumes city intent text + constraints, returns structured spec JSON
- Recommended local host target: `http://127.0.0.1:7077`

## Command Safety Themes
- Prefer minimal reversible operations
- Reject invented fields not present in snapshot
- Return request-for-more-info commands when context is missing

## Source Files
- `AI/docs/ToolserverIntegration.md`
- `AI/client/UiAgentClient.cpp`
- `AI/client/CitySpecClient.cpp`

## Related
- [[topics/ai-bridge-and-assistants]]
- [[notes/ai-bridge-runtime-config-and-health-checks]]
- [[notes/phase-4-code-shape-design-assistant]]
