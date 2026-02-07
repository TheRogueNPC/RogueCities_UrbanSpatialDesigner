from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
import httpx
import json
import os

app = FastAPI()

def _toolserver_mock_enabled() -> bool:
    v = os.getenv("ROGUECITY_TOOLSERVER_MOCK", "").strip().lower()
    return v in ("1", "true", "yes", "on")

# =======================================================================
# HEALTH
# =======================================================================

@app.get("/health")
async def health():
    return {"status": "ok", "service": "RogueCity AI Bridge"}

# =======================================================================
# UI AGENT
# =======================================================================

class UiAgentRequest(BaseModel):
    snapshot: dict
    goal: str
    model: str = "qwen2.5:latest"

SYSTEM_PROMPT_UI = """You are a UI layout assistant for RogueCity Visualizer.
Given a UI snapshot and a user goal, respond with JSON commands to adjust the layout.

Available commands:
- {"cmd": "SetHeader", "mode": "ROAD|DISTRICT|AXIOM"}
- {"cmd": "DockPanel", "panel": "panel_id", "targetDock": "Left|Right|Center|Bottom"}
- {"cmd": "SetState", "key": "state_key", "value": "new_value"}

Respond ONLY with a JSON array of commands.

Example:
[
  {"cmd": "DockPanel", "panel": "Tools", "targetDock": "Left"},
  {"cmd": "SetHeader", "mode": "ROAD"}
]
"""

@app.post("/ui_agent")
async def ui_agent(req: UiAgentRequest):
    if _toolserver_mock_enabled():
        return {
            "commands": [
                {"cmd": "SetHeader", "mode": "ROAD"},
                {"cmd": "DockPanel", "panel": "Tools", "targetDock": "Left"}
            ],
            "mock": True
        }

    prompt = f"{SYSTEM_PROMPT_UI}\n\nSnapshot:\n{json.dumps(req.snapshot, indent=2)}\n\nGoal: {req.goal}\n\nCommands:"
    ollama_url = "http://127.0.0.1:11434/api/generate"
    ollama_payload = {
        "model": req.model,
        "prompt": prompt,
        "stream": False,
        "format": "json"
    }

    async with httpx.AsyncClient(timeout=60.0) as client:
        try:
            response = await client.post(ollama_url, json=ollama_payload)
            response.raise_for_status()
            ollama_response = response.json()
            generated_text = ollama_response.get("response", "")
            commands = json.loads(generated_text)
            return {"commands": commands if isinstance(commands, list) else []}
        except Exception as e:
            return {"commands": [], "error": str(e)}

# =======================================================================
# CITY SPEC
# =======================================================================

class CitySpecRequest(BaseModel):
    description: str
    constraints: dict = {}
    model: str = "qwen2.5:latest"

CITYSPEC_SYSTEM_PROMPT = """You are a city design assistant.
Given a description, generate a structured city specification in JSON format.

Required schema:
{
  "intent": {
    "description": "...",
    "scale": "hamlet|town|city|metro",
    "climate": "temperate|tropical|arid|cold",
    "style_tags": ["modern", "industrial", ...]
  },
  "districts": [
    {"type": "residential|commercial|industrial|downtown", "density": 0.0-1.0}
  ],
  "seed": 0,
  "road_density": 0.5
}

Respond ONLY with valid JSON matching this schema.
"""

@app.post("/city_spec")
async def city_spec(req: CitySpecRequest):
    if _toolserver_mock_enabled():
        scale = req.constraints.get("scale", "town")
        return {
            "spec": {
                "intent": {
                    "description": req.description,
                    "scale": scale,
                    "climate": "temperate",
                    "style_tags": ["modern"]
                },
                "districts": [{"type": "downtown", "density": 0.9}],
                "seed": 0,
                "road_density": 0.5
            },
            "mock": True
        }

    prompt = f"{CITYSPEC_SYSTEM_PROMPT}\n\nDescription: {req.description}\nConstraints: {json.dumps(req.constraints)}\n\nJSON:"
    ollama_url = "http://127.0.0.1:11434/api/generate"
    ollama_payload = {
        "model": req.model,
        "prompt": prompt,
        "stream": False,
        "format": "json"
    }

    async with httpx.AsyncClient(timeout=120.0) as client:
        try:
            response = await client.post(ollama_url, json=ollama_payload)
            response.raise_for_status()
            ollama_response = response.json()
            generated_text = ollama_response.get("response", "")
            spec = json.loads(generated_text)
            return {"spec": spec}
        except Exception as e:
            raise HTTPException(status_code=500, detail=f"Ollama error: {str(e)}")
