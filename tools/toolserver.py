from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
import httpx
import json
import os
from typing import Any, Dict, List, Optional

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

# =======================================================================
# UI DESIGN ASSISTANT (Phase 4 - Refactoring/Patterns with Layout Diff)
# =======================================================================

class PanelChangeModel(BaseModel):
    field: str
    from_: Optional[Any] = None
    to: Optional[Any] = None

class PanelModifiedModel(BaseModel):
    id: str
    changes: List[PanelChangeModel]

class PanelAddedModel(BaseModel):
    id: str
    role: str
    dock_area: str
    tags: List[str] = []
    summary: str = ""

class PanelRemovedModel(BaseModel):
    id: str
    reason: str = ""

class ActionAddedModel(BaseModel):
    id: str
    panel_id: str
    label: str
    context_tags: List[str] = []

class LayoutDiffModel(BaseModel):
    panels_added: List[PanelAddedModel] = []
    panels_removed: List[PanelRemovedModel] = []
    panels_modified: List[PanelModifiedModel] = []
    actions_added: List[ActionAddedModel] = []

class ComponentPatternModel(BaseModel):
    name: str
    template: str
    applies_to: List[str]
    props: List[str] = []
    description: str = ""

class RefactorOpportunityModel(BaseModel):
    name: str
    description: str = ""
    priority: str
    affected_panels: List[str]
    suggested_action: str
    rationale: str

class UiDesignRequest(BaseModel):
    snapshot: dict
    pattern_catalog: dict
    goal: str
    model: str = "qwen2.5:latest"

class UiDesignResponse(BaseModel):
    updated_layout: dict
    layout_diff: LayoutDiffModel
    component_patterns: List[ComponentPatternModel]
    refactoring_opportunities: List[RefactorOpportunityModel]
    suggested_files: List[str]
    summary: str

DESIGN_ASSISTANT_PROMPT = """You are a UI architecture assistant for RogueCity Visualizer.
Given a UI snapshot with code-shape metadata and a pattern catalog, analyze for refactoring opportunities.

Your response MUST be valid JSON matching this schema:
{
  "updated_layout": {
    "panels": [ /* updated panel list */ ]
  },
  "layout_diff": {
    "panels_added": [{
      "id": "panel_id",
      "role": "inspector|toolbox|viewport|nav|log|index",
      "dock_area": "Left|Right|Center|Bottom",
      "tags": ["tag1", "tag2"],
      "summary": "Brief description"
    }],
    "panels_removed": [{
      "id": "panel_id",
      "reason": "Why it was removed"
    }],
    "panels_modified": [{
      "id": "panel_id",
      "changes": [{
        "field": "dock_area",
        "from_": "Left",
        "to": "Right"
      }]
    }],
    "actions_added": [{
      "id": "action_id",
      "panel_id": "panel_id",
      "label": "Action Label",
      "context_tags": ["mode:AXIOM", "selection:single"]
    }]
  },
  "component_patterns": [{
    "name": "PatternName",
    "template": "RcPatternName<T>",
    "applies_to": ["panel_id1", "panel_id2"],
    "props": ["prop1", "prop2"],
    "description": "..."
  }],
  "refactoring_opportunities": [{
    "name": "Refactoring name",
    "priority": "high|medium|low",
    "affected_panels": ["panel_id1", ...],
    "suggested_action": "Create RcTemplate<T> in path/to/file.h",
    "rationale": "Why this refactoring improves the codebase"
  }],
  "suggested_files": ["path/to/file1.h", ...],
  "summary": "High-level summary of findings"
}

Focus on:
1. Code duplication - panels with similar structure
2. Data binding patterns - common data access patterns
3. Interaction patterns - reusable UI behaviors
4. Role clustering - panels with the same role

IMPORTANT: Always fill layout_diff to summarize changes. Keep IDs consistent with current_layout.
"""

@app.post("/ui_design_assistant", response_model=UiDesignResponse)
async def ui_design_assistant(req: UiDesignRequest):
    if _toolserver_mock_enabled():
        # Mock response with sample diff
        return UiDesignResponse(
            updated_layout={"panels": req.snapshot.get("panels", [])},
            layout_diff=LayoutDiffModel(
                panels_added=[
                    PanelAddedModel(
                        id="rc_axiom_metrics",
                        role="inspector",
                        dock_area="Right",
                        tags=["axiom", "metrics"],
                        summary="New metrics inspector for axioms"
                    )
                ],
                panels_removed=[],
                panels_modified=[
                    PanelModifiedModel(
                        id="rc_panel_tools",
                        changes=[
                            PanelChangeModel(field="dock_area", from_="Bottom", to="Left")
                        ]
                    )
                ],
                actions_added=[]
            ),
            component_patterns=[
                ComponentPatternModel(
                    name="DataIndexPanel<T>",
                    template="RcDataIndexPanel<T>",
                    applies_to=["rc_panel_road_index", "rc_panel_district_index"],
                    props=["entities[]", "selected_id"],
                    description="Generic sortable/filterable table"
                )
            ],
            refactoring_opportunities=[
                RefactorOpportunityModel(
                    name="Extract common DataIndexPanel pattern",
                    priority="high",
                    affected_panels=["rc_panel_road_index", "rc_panel_district_index", "rc_panel_lot_index"],
                    suggested_action="Create generic RcDataIndexPanel<T> template",
                    rationale="3+ panels with 80% code duplication"
                )
            ],
            suggested_files=[
                "visualizer/src/ui/patterns/rc_ui_data_index_panel.h",
                "visualizer/src/ui/patterns/rc_ui_inspector_panel.h"
            ],
            summary="Found high-priority refactoring: extract DataIndexPanel template to eliminate duplication."
        )
    
    prompt = f"{DESIGN_ASSISTANT_PROMPT}\n\nSnapshot:\n{json.dumps(req.snapshot, indent=2)}\n\nPattern Catalog:\n{json.dumps(req.pattern_catalog, indent=2)}\n\nGoal: {req.goal}\n\nJSON:"
    
    ollama_url = "http://127.0.0.1:11434/api/generate"
    ollama_payload = {
        "model": req.model,
        "prompt": prompt,
        "stream": False,
        "format": "json"
    }
    
    async with httpx.AsyncClient(timeout=180.0) as client:
        try:
            response = await client.post(ollama_url, json=ollama_payload)
            response.raise_for_status()
            ollama_response = response.json()
            generated_text = ollama_response.get("response", "")
            
            plan = json.loads(generated_text)
            return UiDesignResponse(**plan)
                    
        except Exception as e:
            raise HTTPException(status_code=500, detail=f"Ollama error: {str(e)}")
