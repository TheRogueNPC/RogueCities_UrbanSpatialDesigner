# AI Toolserver Integration - FastAPI Endpoint

## Add to your existing toolserver.py (used by Start_Ai_Bridge.ps1)

### 1. Add imports at the top:

```python
from pydantic import BaseModel
from typing import Any, Dict
import json
import httpx
```

### 2. Add request model:

```python
class UiAgentIn(BaseModel):
    snapshot: Dict[str, Any]
    goal: str
```

### 3. Add /ui_agent endpoint:

```python
@app.post("/ui_agent")
async def ui_agent(payload: UiAgentIn):
    """
    UI agent endpoint:
    - Receives snapshot + goal
    - Calls local model via OpenAI-compatible /v1/chat/completions
    - Returns JSON array of commands
    """
    system_msg = (
        "You are assisting a C++/ImGui city generator editor.\n"
        "You NEVER assume UI state. You ONLY use the provided UI Snapshot JSON.\n"
        "You respond with a JSON array of commands only (no extra text).\n"
        "If information is missing, emit a single command: "
        '{"cmd":"Request","fields":[...]}.\n'
        "Constraints:\n"
        "- Commands must be safe and reversible.\n"
        "- Do not invent panels, ids, or fields that are not in the snapshot.\n"
        "- Prefer minimal changes.\n"
    )

    snapshot_str = json.dumps(payload.snapshot, indent=2)
    user_msg = (
        f"Snapshot:\n{snapshot_str}\n\n"
        f"Goal: {payload.goal}\n\n"
        "Respond with ONLY a JSON array of commands, no explanation."
    )

    # Replace with your actual model name and Ollama endpoint
    req = {
        "model": "qwen2.5:latest",  # or whatever model you're using
        "messages": [
            {"role": "system", "content": system_msg},
            {"role": "user",   "content": user_msg},
        ],
        "temperature": 0.2,
        "stream": False,
    }

    # Call Ollama's OpenAI-compatible endpoint
    async with httpx.AsyncClient(timeout=300.0) as client:
        r = await client.post("http://127.0.0.1:11434/v1/chat/completions", json=req)
        r.raise_for_status()
        data = r.json()

    content = data["choices"][0]["message"]["content"]

    try:
        commands = json.loads(content)
        if isinstance(commands, list):
            return commands
    except Exception:
        return [{"cmd": "Request", "fields": ["invalid_response_format"]}]
```

### 4. Example Commands the AI might return:

```json
[
    {"cmd": "SetHeader", "mode": "REACTIVE", "filter": "CAUTION"},
    {"cmd": "DockPanel", "panel": "Inspector", "targetDock": "Left", "ownDockNode": true},
    {"cmd": "SetState", "key": "flowRate", "valueNumber": 1.5},
    {"cmd": "Request", "fields": ["current_axiom_positions", "district_count"]}
]
```

### 5. Testing the endpoint:

```bash
# Test with curl
curl -X POST http://127.0.0.1:7077/ui_agent \
  -H "Content-Type: application/json" \
  -d '{
    "snapshot": {
      "app": "RogueCity Visualizer",
      "header": {"left": "ROGUENAV", "mode": "SOLITON", "filter": "NORMAL"},
      "layout": {"dockingEnabled": true, "multiViewportEnabled": false, "panels": []},
      "state": {"flowRate": 1.0, "livePreview": true, "debounceSec": 0.2},
      "logTail": []
    },
    "goal": "Change minimap mode to REACTIVE"
  }'
```

Expected response:
```json
[
    {"cmd": "SetHeader", "mode": "REACTIVE"}
]
```

## Implementation Notes

1. **HTTP Client in C++**: The current implementation uses a stub. Replace `AI/tools/HttpClient.cpp` with:
   - **cpr** (recommended): Modern C++ wrapper for libcurl
   - **cpp-httplib**: Header-only HTTP library
   - **WinHTTP**: Windows-only, built-in

2. **Error Handling**: The AI endpoint returns an empty array or Request command on errors. The C++ side handles this gracefully.

3. **Security**: This endpoint should only be accessible from localhost (127.0.0.1) to prevent unauthorized AI control.

4. **Performance**: AI responses typically take 1-10 seconds depending on model size. UI should show "Processing..." state.

## Future Enhancements

- Add authentication/API key
- Support streaming responses for long operations
- Add command validation before applying
- Log all AI interactions for debugging
- Support undo/redo for AI commands
