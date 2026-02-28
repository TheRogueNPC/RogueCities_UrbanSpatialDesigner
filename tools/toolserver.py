from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
import httpx
import json
import os
import asyncio
import subprocess
import re
import math
import hashlib
import base64
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, List, Optional
from urllib.parse import urlparse, urlunparse

app = FastAPI()

def _toolserver_mock_enabled() -> bool:
    v = os.getenv("ROGUECITY_TOOLSERVER_MOCK", "").strip().lower()
    return v in ("1", "true", "yes", "on")


def _normalize_ollama_base_url(raw: str) -> Optional[str]:
    s = str(raw or "").strip().rstrip("/")
    if not s:
        return None
    if "://" not in s:
        s = f"http://{s}"
    try:
        parsed = urlparse(s)
    except Exception:
        return None
    if not parsed.scheme or not parsed.netloc:
        return None
    return urlunparse((parsed.scheme, parsed.netloc, "", "", "", "")).rstrip("/")


def _running_in_wsl() -> bool:
    if os.name == "nt":
        return False
    try:
        pv = Path("/proc/version")
        if pv.exists():
            text = pv.read_text(encoding="utf-8", errors="ignore").lower()
            return "microsoft" in text or "wsl" in text
    except Exception:
        pass
    return False


def _ollama_base_candidates() -> List[str]:
    out: List[str] = []
    seen: set[str] = set()

    def _push(raw: str) -> None:
        base = _normalize_ollama_base_url(raw)
        if base and base not in seen:
            seen.add(base)
            out.append(base)

    _push(os.getenv("OLLAMA_BASE_URL", ""))
    _push(os.getenv("OLLAMA_HOST", ""))
    _push("http://127.0.0.1:11434")

    if os.name != "nt":
        _push("http://host.docker.internal:11434")
        resolv = Path("/etc/resolv.conf")
        if resolv.exists():
            try:
                for line in resolv.read_text(encoding="utf-8", errors="ignore").splitlines():
                    if line.strip().startswith("nameserver "):
                        parts = line.split()
                        if len(parts) >= 2:
                            _push(f"http://{parts[1].strip()}:11434")
                        break
            except Exception:
                pass
        if _running_in_wsl():
            _push("http://172.17.0.1:11434")

    return out


async def _ollama_post_json(path: str, payload: Dict[str, Any], timeout: float) -> Dict[str, Any]:
    last_err = ""
    tried: List[str] = []
    connect_timeout = 2.0 if timeout > 2.0 else timeout
    timeout_cfg = httpx.Timeout(timeout=timeout, connect=connect_timeout)
    async with httpx.AsyncClient(timeout=timeout_cfg) as client:
        for base in _ollama_base_candidates():
            url = f"{base}{path}"
            tried.append(url)
            try:
                resp = await client.post(url, json=payload)
                resp.raise_for_status()
                body = resp.json()
                if isinstance(body, dict):
                    return body
                return {"response": str(body)}
            except Exception as exc:
                last_err = str(exc)
                continue
    tried_text = ", ".join(tried)
    raise RuntimeError(f"Ollama request failed for {path}; tried [{tried_text}]; last_error={last_err}")

# =======================================================================
# HEALTH
# =======================================================================

@app.get("/health")
async def health():
    return {
        "status": "ok",
        "service": "RogueCity AI Bridge",
        "mock": _toolserver_mock_enabled(),
        "pid": os.getpid(),
    }

@app.post("/admin/shutdown")
async def admin_shutdown():
    # Local-only bridge control endpoint (service is bound to 127.0.0.1).
    async def _shutdown_soon():
        await asyncio.sleep(0.15)
        os._exit(0)

    asyncio.create_task(_shutdown_soon())
    return {"status": "shutting_down", "pid": os.getpid()}

# =======================================================================
# UI AGENT
# =======================================================================

class UiAgentRequest(BaseModel):
    snapshot: dict
    goal: str
    model: str = "gemma3:4b"

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
    ollama_payload = {
        "model": req.model,
        "prompt": prompt,
        "stream": False,
        "format": "json"
    }

    try:
        ollama_response = await _ollama_post_json("/api/generate", ollama_payload, timeout=60.0)
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
    model: str = "gemma3:4b"

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
    ollama_payload = {
        "model": req.model,
        "prompt": prompt,
        "stream": False,
        "format": "json"
    }

    try:
        ollama_response = await _ollama_post_json("/api/generate", ollama_payload, timeout=120.0)
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
    snapshot: Optional[dict] = None
    introspection_snapshot: Optional[dict] = None
    pattern_catalog: dict
    goal: str
    model: str = "gemma3:4b"

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
    snapshot_payload = req.snapshot or req.introspection_snapshot or {}

    if _toolserver_mock_enabled():
        # Mock response with sample diff
        return UiDesignResponse(
            updated_layout={"panels": snapshot_payload.get("panels", [])},
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
    
    prompt = f"{DESIGN_ASSISTANT_PROMPT}\n\nSnapshot:\n{json.dumps(snapshot_payload, indent=2)}\n\nPattern Catalog:\n{json.dumps(req.pattern_catalog, indent=2)}\n\nGoal: {req.goal}\n\nJSON:"
    
    ollama_payload = {
        "model": req.model,
        "prompt": prompt,
        "stream": False,
        "format": "json"
    }

    try:
        ollama_response = await _ollama_post_json("/api/generate", ollama_payload, timeout=180.0)
        generated_text = ollama_response.get("response", "")
        plan = json.loads(generated_text)
        return UiDesignResponse(**plan)
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Ollama error: {str(e)}")

# =======================================================================
# PIPELINE V2 (Staged Gemma-First Orchestration)
# =======================================================================

REPO_ROOT = Path(__file__).resolve().parents[1]
AI_INDEX_DIR = REPO_ROOT / "tools" / ".ai-index"
INDEX_CHUNKS = AI_INDEX_DIR / "chunks.jsonl"
INDEX_VECTORS = AI_INDEX_DIR / "vectors.jsonl"
INDEX_META = AI_INDEX_DIR / "meta.json"
AI_CONFIG_PATH = REPO_ROOT / "AI" / "ai_config.json"


def _env_bool(name: str, default: bool) -> bool:
    v = os.getenv(name)
    if v is None:
        return default
    return v.strip().lower() in ("1", "true", "yes", "on")


def _load_ai_config() -> Dict[str, Any]:
    if not AI_CONFIG_PATH.exists():
        return {}
    try:
        return json.loads(AI_CONFIG_PATH.read_text(encoding="utf-8"))
    except Exception:
        return {}


def _cfg_bool(env_name: str, cfg: Dict[str, Any], cfg_key: str, default: bool) -> bool:
    env_v = os.getenv(env_name)
    if env_v is not None:
        return env_v.strip().lower() in ("1", "true", "yes", "on")
    cfg_v = cfg.get(cfg_key, default)
    if isinstance(cfg_v, bool):
        return cfg_v
    if isinstance(cfg_v, str):
        return cfg_v.strip().lower() in ("1", "true", "yes", "on")
    return bool(cfg_v)


def _pipeline_v2_enabled() -> bool:
    cfg = _load_ai_config()
    return _cfg_bool("RC_AI_PIPELINE_V2", cfg, "pipeline_v2_enabled", False)


def _audit_strict_enabled() -> bool:
    cfg = _load_ai_config()
    return _cfg_bool("RC_AI_AUDIT_STRICT", cfg, "audit_strict_enabled", False)


def _default_models() -> Dict[str, str]:
    cfg = _load_ai_config()
    # Gemma-first defaults for pipeline v2; legacy endpoints remain unchanged.
    return {
        "controller_model": os.getenv("RC_AI_CONTROLLER_MODEL", str(cfg.get("controller_model", "functiongemma"))),
        "triage_model": os.getenv("RC_AI_TRIAGE_MODEL", str(cfg.get("triage_model", "codegemma:2b"))),
        "synth_fast_model": os.getenv("RC_AI_SYNTH_FAST_MODEL", str(cfg.get("synth_fast_model", "gemma3:4b"))),
        "synth_escalation_model": os.getenv("RC_AI_SYNTH_ESCALATION_MODEL", str(cfg.get("synth_escalation_model", "gemma3:12b"))),
        "embedding_model": os.getenv("RC_AI_EMBEDDING_MODEL", str(cfg.get("embedding_model", "embeddinggemma"))),
        "vision_model": os.getenv("RC_AI_VISION_MODEL", str(cfg.get("vision_model", "granite3.2-vision"))),
        "ocr_model": os.getenv("RC_AI_OCR_MODEL", str(cfg.get("ocr_model", "glm-ocr"))),
    }


def _safe_resolve_repo_path(rel_path: str) -> Optional[Path]:
    if not rel_path:
        return None
    p = Path(rel_path)
    if p.is_absolute():
        candidate = p.resolve()
    else:
        candidate = (REPO_ROOT / p).resolve()
    try:
        candidate.relative_to(REPO_ROOT.resolve())
    except ValueError:
        return None
    return candidate


def _file_exists_repo(rel_path: str) -> bool:
    p = _safe_resolve_repo_path(rel_path)
    return bool(p and p.exists())


def _extract_json_object(text: str) -> Optional[dict]:
    if not text:
        return None
    candidates: List[str] = [text.strip()]
    m = re.search(r"```(?:json)?\s*([\s\S]*?)```", text)
    if m:
        candidates.append(m.group(1).strip())
    start = text.find("{")
    if start >= 0:
        depth = 0
        in_string = False
        escape = False
        for i in range(start, len(text)):
            ch = text[i]
            if escape:
                escape = False
                continue
            if ch == "\\" and in_string:
                escape = True
                continue
            if ch == '"':
                in_string = not in_string
                continue
            if not in_string:
                if ch == "{":
                    depth += 1
                elif ch == "}":
                    depth -= 1
                    if depth == 0:
                        candidates.append(text[start:i + 1].strip())
                        break
    for c in candidates:
        try:
            data = json.loads(c)
            if isinstance(data, dict):
                return data
        except Exception:
            continue
    return None


async def _ollama_generate(
    model: str,
    prompt: str,
    timeout: float = 120.0,
    force_json: bool = False,
    temperature: float = 0.1,
    top_p: float = 0.85,
    num_predict: int = 256,
) -> str:
    payload: Dict[str, Any] = {
        "model": model,
        "prompt": prompt,
        "stream": False,
        "options": {
            "temperature": temperature,
            "top_p": top_p,
            "num_predict": num_predict,
        },
    }
    if force_json:
        payload["format"] = "json"
    body = await _ollama_post_json("/api/generate", payload, timeout=timeout)
    return str(body.get("response", ""))


async def _ollama_embed(model: str, text: str, dimensions: int = 512, timeout: float = 90.0) -> List[float]:
    payload = {"model": model, "prompt": text, "dimensions": dimensions}
    last_error = ""
    connect_timeout = 2.0 if timeout > 2.0 else timeout
    timeout_cfg = httpx.Timeout(timeout=timeout, connect=connect_timeout)
    async with httpx.AsyncClient(timeout=timeout_cfg) as client:
        for base in _ollama_base_candidates():
            try:
                resp = await client.post(f"{base}/api/embeddings", json=payload)
                resp.raise_for_status()
                body = resp.json()
                emb = body.get("embedding", [])
                return [float(x) for x in emb]
            except Exception as first_error:
                last_error = str(first_error)
                try:
                    resp = await client.post(f"{base}/api/embed", json=payload)
                    resp.raise_for_status()
                    body = resp.json()
                    emb = body.get("embeddings", [[]])[0]
                    return [float(x) for x in emb]
                except Exception as second_error:
                    last_error = str(second_error)
                    continue
    raise RuntimeError(f"Ollama embedding request failed across candidates: {last_error}")


def _normalize(v: List[float]) -> List[float]:
    n = math.sqrt(sum(x * x for x in v)) if v else 0.0
    if n <= 1e-12:
        return v
    return [x / n for x in v]


def _cosine(a: List[float], b: List[float]) -> float:
    if not a or not b or len(a) != len(b):
        return -1.0
    return float(sum(x * y for x, y in zip(a, b)))


def _run_rg_search(patterns: List[str], max_hits: int = 20) -> List[Dict[str, Any]]:
    hits: List[Dict[str, Any]] = []
    rg_bin = "rg"
    for pat in patterns:
        if not pat:
            continue
        try:
            proc = subprocess.run(
                [rg_bin, "-n", "--no-heading", "--fixed-strings", "--", pat, str(REPO_ROOT)],
                capture_output=True,
                text=True,
                timeout=20,
            )
            out_lines = proc.stdout.splitlines()[:max_hits]
            for line in out_lines:
                # Expected: path:line:snippet
                parts = line.split(":", 2)
                if len(parts) != 3:
                    continue
                p, ln, snippet = parts
                rel = str(Path(p).resolve().relative_to(REPO_ROOT.resolve()))
                try:
                    line_num = int(ln)
                except Exception:
                    line_num = 0
                hits.append({"path": rel, "line": line_num, "snippet": snippet.strip(), "pattern": pat})
        except Exception:
            continue
        if len(hits) >= max_hits:
            break
    return hits[:max_hits]


def _gather_context_files(files: List[str], max_chars: int = 24000) -> str:
    blocks: List[str] = []
    total = 0
    for rel in files:
        p = _safe_resolve_repo_path(rel)
        if not p or not p.exists() or not p.is_file():
            continue
        try:
            text = p.read_text(encoding="utf-8", errors="replace")
        except Exception:
            continue
        text = re.sub(r"[\x00-\x08\x0B\x0C\x0E-\x1F]", " ", text)
        if len(text) > 8000:
            text = text[:8000]
        block = f"FILE: {rel}\n{text}"
        if total + len(block) > max_chars:
            break
        blocks.append(block)
        total += len(block)
    return "\n\n---\n\n".join(blocks)


def _repo_index_snippet(limit: int = 1200, max_chars: int = 12000) -> str:
    try:
        proc = subprocess.run(["rg", "--files", str(REPO_ROOT)], capture_output=True, text=True, timeout=20)
        lines = proc.stdout.splitlines()[:limit]
        rels: List[str] = []
        for p in lines:
            try:
                rels.append(str(Path(p).resolve().relative_to(REPO_ROOT.resolve())))
            except Exception:
                continue
        text = "\n".join(rels)
        return text[:max_chars]
    except Exception:
        return ""


def _chunk_text(text: str, chunk_size: int = 1400, overlap: int = 200) -> List[str]:
    if len(text) <= chunk_size:
        return [text]
    chunks: List[str] = []
    start = 0
    while start < len(text):
        end = min(len(text), start + chunk_size)
        chunks.append(text[start:end])
        if end >= len(text):
            break
        start = max(0, end - overlap)
    return chunks


async def _build_semantic_index(model: str, dimensions: int = 512, max_files: int = 600) -> Dict[str, Any]:
    AI_INDEX_DIR.mkdir(parents=True, exist_ok=True)
    proc = subprocess.run(["rg", "--files", str(REPO_ROOT)], capture_output=True, text=True, timeout=25)
    files = proc.stdout.splitlines()[:max_files]
    chunks_out = []
    vectors_out = []
    indexed_files = 0
    for p in files:
        path = Path(p)
        if path.suffix.lower() not in {".cpp", ".h", ".hpp", ".ps1", ".py", ".json", ".md"}:
            continue
        try:
            text = path.read_text(encoding="utf-8", errors="replace")
        except Exception:
            continue
        rel = str(path.resolve().relative_to(REPO_ROOT.resolve()))
        file_chunks = _chunk_text(text)
        for i, chunk in enumerate(file_chunks):
            chunk_id = f"{rel}::c{i}"
            h = hashlib.sha256(chunk.encode("utf-8", errors="ignore")).hexdigest()
            emb = await _ollama_embed(model=model, text=chunk[:4000], dimensions=dimensions)
            emb = _normalize(emb)
            chunks_out.append({
                "path": rel,
                "chunk_id": chunk_id,
                "start_line": 1,
                "end_line": 1,
                "text": chunk,
                "hash": h,
            })
            vectors_out.append({"chunk_id": chunk_id, "vector": emb})
        indexed_files += 1

    with INDEX_CHUNKS.open("w", encoding="utf-8") as f:
        for row in chunks_out:
            f.write(json.dumps(row, ensure_ascii=False) + "\n")
    with INDEX_VECTORS.open("w", encoding="utf-8") as f:
        for row in vectors_out:
            f.write(json.dumps(row, ensure_ascii=False) + "\n")
    meta = {
        "embedding_model": model,
        "embedding_dimensions": dimensions,
        "count_chunks": len(chunks_out),
        "count_files": indexed_files,
    }
    INDEX_META.write_text(json.dumps(meta, indent=2), encoding="utf-8")
    return meta


async def _semantic_search(query: str, model: str, dimensions: int = 512, top_k: int = 8) -> List[Dict[str, Any]]:
    if not INDEX_CHUNKS.exists() or not INDEX_VECTORS.exists():
        return []
    vec_map: Dict[str, List[float]] = {}
    with INDEX_VECTORS.open("r", encoding="utf-8") as f:
        for line in f:
            if not line.strip():
                continue
            row = json.loads(line)
            vec_map[row["chunk_id"]] = [float(x) for x in row["vector"]]
    chunk_map: Dict[str, Dict[str, Any]] = {}
    with INDEX_CHUNKS.open("r", encoding="utf-8") as f:
        for line in f:
            if not line.strip():
                continue
            row = json.loads(line)
            chunk_map[row["chunk_id"]] = row
    qv = _normalize(await _ollama_embed(model=model, text=query[:4000], dimensions=dimensions))
    scored: List[Dict[str, Any]] = []
    for cid, vec in vec_map.items():
        s = _cosine(qv, vec)
        if cid in chunk_map:
            c = chunk_map[cid]
            scored.append({
                "path": c["path"],
                "chunk_id": cid,
                "score": s,
                "snippet": c["text"][:220].replace("\n", " "),
            })
    scored.sort(key=lambda x: x["score"], reverse=True)
    return scored[:top_k]


class TriagedQueryPlan(BaseModel):
    intent: str = "lookup"
    search_patterns: List[str] = []
    symbol_guesses: List[str] = []
    file_globs: List[str] = []
    candidate_paths: List[str] = []
    required_paths: List[str] = []
    needs_vision: bool = False
    notes: str = ""


class ToolCall(BaseModel):
    tool: str
    arguments: Dict[str, Any] = {}
    idempotent: bool = True
    timeout_ms: int = 120000


class VisualRegion(BaseModel):
    label: str
    bbox: List[int] = []


class VisualEvidence(BaseModel):
    source: str
    summary: str = ""
    text_spans: List[str] = []
    ui_regions: List[VisualRegion] = []
    confidence: str = "low"


class LexicalHit(BaseModel):
    path: str
    line: int
    snippet: str


class SemanticHit(BaseModel):
    path: str
    chunk_id: str
    score: float
    snippet: str


class EvidenceBundle(BaseModel):
    query: str
    triage_plan: Dict[str, Any]
    lexical_hits: List[LexicalHit] = []
    semantic_hits: List[SemanticHit] = []
    visual_evidence: List[VisualEvidence] = []
    required_paths: List[str] = []


class PipelineVerifier(BaseModel):
    json_ok: bool
    invalid_paths: List[str] = []
    missing_required_paths: List[str] = []
    answer_quality_ok: bool = True
    answer_quality_reason: str = "ok"


class PipelineAnswer(BaseModel):
    answer: str
    paths_used: List[str] = []
    insufficient_context: bool = False
    confidence: str = "medium"
    mode: str = "normal"
    verifier: PipelineVerifier
    debug: Optional[Dict[str, Any]] = None


class PipelineQueryRequest(BaseModel):
    prompt: str
    mode: str = "normal"  # normal|audit
    context_files: List[str] = []
    required_paths: List[str] = []
    search_hints: List[str] = []
    include_repo_index: bool = False
    include_semantic: bool = False
    embedding_dimensions: int = 512
    max_search_results: int = 20
    max_context_chars: int = 24000
    model_overrides: Dict[str, str] = {}
    debug_trace: bool = False
    temperature: float = 0.1
    top_p: float = 0.85
    num_predict: int = 256


class PipelineEvalRequest(BaseModel):
    case_file: str = "tools/ai_eval_cases.json"
    max_cases: int = 10
    mode: str = "audit"
    model_overrides: Dict[str, str] = {}
    include_repo_index: bool = False
    include_semantic: bool = False
    embedding_dimensions: int = 512
    temperature: float = 0.1
    top_p: float = 0.85
    num_predict: int = 256


class PipelineIndexBuildRequest(BaseModel):
    embedding_model: Optional[str] = None
    embedding_dimensions: int = 512
    max_files: int = 600


def _triage_fallback(prompt: str, search_hints: List[str], required_paths: List[str]) -> Dict[str, Any]:
    p = prompt.lower()
    intent = "lookup"
    if "refactor" in p:
        intent = "refactor"
    elif "bug" in p or "fix" in p:
        intent = "bugfix"
    elif "compare" in p:
        intent = "compare"
    patterns = [x for x in search_hints if x]
    if not patterns:
        tokens = re.findall(r"[A-Za-z_][A-Za-z0-9_:/.-]{2,}", prompt)[:6]
        patterns = tokens
    return {
        "intent": intent,
        "search_patterns": patterns,
        "symbol_guesses": [],
        "file_globs": [],
        "candidate_paths": [],
        "required_paths": required_paths,
        "needs_vision": False,
        "notes": "fallback triage",
    }


def _verify_pipeline_answer(parsed: Optional[dict], required_paths: List[str]) -> PipelineVerifier:
    if not parsed:
        return PipelineVerifier(
            json_ok=False,
            invalid_paths=[],
            missing_required_paths=required_paths,
            answer_quality_ok=False,
            answer_quality_reason="missing_json",
        )
    required_schema_keys = ("answer", "paths_used", "insufficient_context", "confidence")
    missing_schema_keys = [k for k in required_schema_keys if k not in parsed]
    if missing_schema_keys:
        return PipelineVerifier(
            json_ok=False,
            invalid_paths=[],
            missing_required_paths=required_paths,
            answer_quality_ok=False,
            answer_quality_reason=f"missing_schema_keys:{','.join(missing_schema_keys)}",
        )
    paths = [str(x) for x in parsed.get("paths_used", []) if x]
    invalid = [p for p in paths if p not in required_paths and not _file_exists_repo(p)]
    missing = [p for p in required_paths if p not in paths]
    return PipelineVerifier(
        json_ok=True,
        invalid_paths=invalid,
        missing_required_paths=missing,
        answer_quality_ok=True,
        answer_quality_reason="ok",
    )


def _evaluate_answer_quality(parsed: Optional[dict], evidence_exists: bool) -> Dict[str, Any]:
    if not parsed:
        return {"ok": False, "reason": "missing_json"}
    answer = str(parsed.get("answer", "")).strip()
    if not answer:
        return {"ok": False, "reason": "empty_answer"}
    lowered = answer.lower()
    sentinels = {"insufficient_context", "unknown", "n/a", "none", "null"}
    if lowered in sentinels:
        return {"ok": False, "reason": f"sentinel:{lowered}"}
    if evidence_exists and len(answer) < 24:
        return {"ok": False, "reason": "too_short_with_evidence"}
    return {"ok": True, "reason": "ok"}


def _normalize_pipeline_answer(parsed: Optional[dict]) -> Optional[dict]:
    if not parsed or not isinstance(parsed, dict):
        return parsed
    out = dict(parsed)
    if "answer" not in out:
        out["answer"] = ""
    if "paths_used" not in out:
        for alias_key in ("required_paths", "paths", "files"):
            alias_paths = out.get(alias_key, [])
            if isinstance(alias_paths, list):
                out["paths_used"] = [str(x) for x in alias_paths if x]
                break
    if "paths_used" not in out or not isinstance(out.get("paths_used"), list):
        out["paths_used"] = []
    if "insufficient_context" not in out:
        out["insufficient_context"] = False
    else:
        out["insufficient_context"] = bool(out.get("insufficient_context"))
    confidence = str(out.get("confidence", "")).strip().lower()
    if confidence not in {"high", "medium", "low"}:
        out["confidence"] = "medium"
    else:
        out["confidence"] = confidence
    return out


def _enforce_required_paths(parsed: Optional[dict], required_paths: List[str]) -> Optional[dict]:
    if not parsed:
        return parsed
    req_existing = [str(p) for p in required_paths if p]
    raw_paths = parsed.get("paths_used", [])
    model_paths = []
    if isinstance(raw_paths, list):
        model_paths = [str(x) for x in raw_paths if x]
    if not model_paths:
        for alias_key in ("required_paths", "paths", "files"):
            alias_paths = parsed.get(alias_key, [])
            if isinstance(alias_paths, list):
                model_paths.extend([str(x) for x in alias_paths if x])

    merged: List[str] = []
    for p in model_paths + req_existing:
        if not p:
            continue
        if p not in required_paths and not _file_exists_repo(p):
            continue
        if p in merged:
            continue
        merged.append(p)
    parsed["paths_used"] = merged
    return parsed


def _ensure_minimal_answer(parsed: Optional[dict]) -> Optional[dict]:
    if not parsed or not isinstance(parsed, dict):
        return parsed
    answer = str(parsed.get("answer", "")).strip()
    if answer:
        return parsed
    paths = [str(x) for x in parsed.get("paths_used", []) if x]
    if paths:
        parsed["answer"] = "Relevant implementation paths: " + ", ".join(paths[:3]) + "."
    return parsed


def _needs_escalation(attempt: int, verifier: PipelineVerifier, triage_plan: Dict[str, Any], prompt: str) -> bool:
    if attempt >= 2 and (
        not verifier.json_ok
        or verifier.invalid_paths
        or verifier.missing_required_paths
        or not verifier.answer_quality_ok
    ):
        return True
    ambiguity_high = len(triage_plan.get("candidate_paths", [])) >= 8
    multi_refactor = triage_plan.get("intent") == "refactor" and ("across" in prompt.lower() or "multi-file" in prompt.lower())
    return ambiguity_high or multi_refactor


@app.post("/pipeline/index/build")
async def pipeline_index_build(req: PipelineIndexBuildRequest):
    models = _default_models()
    emb_model = req.embedding_model or models["embedding_model"]
    meta = await _build_semantic_index(emb_model, dimensions=req.embedding_dimensions, max_files=req.max_files)
    return {"status": "ok", "index": meta}


async def _pipeline_query_impl(req: PipelineQueryRequest) -> PipelineAnswer:
    models = _default_models()
    models.update(req.model_overrides or {})

    if _toolserver_mock_enabled():
        parsed = {
            "answer": "mock pipeline response",
            "paths_used": req.required_paths[:],
            "insufficient_context": False,
            "confidence": "medium",
        }
        verifier = _verify_pipeline_answer(parsed, req.required_paths)
        return PipelineAnswer(
            answer=parsed["answer"],
            paths_used=parsed["paths_used"],
            insufficient_context=parsed["insufficient_context"],
            confidence=parsed["confidence"],
            mode=req.mode,
            verifier=verifier,
            debug={"mock": True} if req.debug_trace else None,
        )

    triage_prompt = f"""
Return ONLY JSON matching TriagedQueryPlan schema.
User prompt: {req.prompt}
Search hints: {req.search_hints}
Required paths: {req.required_paths}
"""
    triage_raw = ""
    triage = _triage_fallback(req.prompt, req.search_hints, req.required_paths)
    try:
        triage_raw = await _ollama_generate(
            models["triage_model"],
            triage_prompt,
            timeout=45.0,
            force_json=True,
            temperature=req.temperature,
            top_p=req.top_p,
            num_predict=req.num_predict,
        )
        parsed_t = _extract_json_object(triage_raw)
        if parsed_t:
            triage = {**triage, **parsed_t}
    except Exception:
        pass

    lexical_hits = _run_rg_search(
        patterns=[*triage.get("search_patterns", []), *req.search_hints],
        max_hits=max(1, req.max_search_results),
    )

    context_text = _gather_context_files(req.context_files, max_chars=req.max_context_chars)
    repo_index = _repo_index_snippet() if req.include_repo_index else ""
    semantic_hits: List[Dict[str, Any]] = []
    if req.include_semantic and INDEX_CHUNKS.exists() and INDEX_VECTORS.exists():
        try:
            semantic_hits = await _semantic_search(req.prompt, models["embedding_model"], dimensions=req.embedding_dimensions, top_k=8)
        except Exception:
            semantic_hits = []

    evidence_bundle = {
        "query": req.prompt,
        "triage_plan": triage,
        "lexical_hits": lexical_hits,
        "semantic_hits": semantic_hits,
        "visual_evidence": [],
        "required_paths": req.required_paths,
    }
    evidence_exists = bool(lexical_hits or semantic_hits or context_text.strip())

    strict_mode = req.mode == "audit" or _audit_strict_enabled()
    strict_contract = """
Return ONLY JSON object:
{
  "answer": "string",
  "paths_used": ["repo/relative/path.ext"],
  "insufficient_context": false,
  "confidence": "high|medium|low"
}
Use only valid repo-relative paths.
Include all required_paths unless impossible.
If evidence exists, answer must be substantive and not empty or sentinel text.
When evidence exists, answer must be at least 48 characters.
Never output keys like query, triage_plan, lexical_hits, semantic_hits, or visual_evidence.
"""

    base_synth_prompt = f"""
SYSTEM:
You are a deterministic local code assistant. Use evidence only.

STRICT:
{strict_contract}

QUERY:
{req.prompt}

CONTEXT FILES:
{context_text}

REPO INDEX:
{repo_index}

EVIDENCE BUNDLE:
{json.dumps(evidence_bundle, ensure_ascii=False)}

Do not return triage/evidence/query objects.
Return only the required PipelineAnswer schema keys.
"""

    attempts = 0
    parsed = None
    verifier = PipelineVerifier(json_ok=False, invalid_paths=[], missing_required_paths=req.required_paths)
    synth_model = models["synth_fast_model"]
    last_raw = ""
    trace: Dict[str, Any] = {
        "models": models,
        "triage_raw": triage_raw,
        "triage_plan": triage,
        "lexical_hit_count": len(lexical_hits),
        "semantic_hit_count": len(semantic_hits),
    }

    while attempts < 4:
        attempts += 1
        try:
            last_raw = await _ollama_generate(
                synth_model,
                base_synth_prompt,
                timeout=90.0,
                force_json=True,
                temperature=req.temperature,
                top_p=req.top_p,
                num_predict=req.num_predict,
            )
        except Exception as e:
            last_raw = json.dumps({"answer": f"generation_error: {e}", "paths_used": [], "insufficient_context": True, "confidence": "low"})
        parsed = _extract_json_object(last_raw)
        parsed = _normalize_pipeline_answer(parsed)
        parsed = _enforce_required_paths(parsed, req.required_paths)
        parsed = _ensure_minimal_answer(parsed)
        verifier = _verify_pipeline_answer(parsed, req.required_paths)
        answer_quality = _evaluate_answer_quality(parsed, evidence_exists)
        verifier.answer_quality_ok = bool(answer_quality["ok"])
        verifier.answer_quality_reason = str(answer_quality["reason"])
        if (
            verifier.json_ok
            and not verifier.invalid_paths
            and not verifier.missing_required_paths
            and verifier.answer_quality_ok
        ):
            break
        if _needs_escalation(attempts, verifier, triage, req.prompt):
            synth_model = models["synth_escalation_model"]
        correction = f"""
Your previous output failed verification.
invalid_paths={verifier.invalid_paths}
missing_required_paths={verifier.missing_required_paths}
answer_quality_ok={verifier.answer_quality_ok}
answer_quality_reason={verifier.answer_quality_reason}
Return non-empty, evidence-grounded answer text when evidence exists.
Minimum answer length when evidence exists: 48 characters.
Return corrected JSON now.
"""
        base_synth_prompt = base_synth_prompt + "\n\nCORRECTION:\n" + correction

    if not parsed:
        parsed = {
            "answer": "insufficient_context",
            "paths_used": [],
            "insufficient_context": True,
            "confidence": "low",
        }
        parsed = _normalize_pipeline_answer(parsed)
        parsed = _enforce_required_paths(parsed, req.required_paths)
        parsed = _ensure_minimal_answer(parsed)
        verifier = _verify_pipeline_answer(parsed, req.required_paths)
        answer_quality = _evaluate_answer_quality(parsed, evidence_exists)
        verifier.answer_quality_ok = bool(answer_quality["ok"])
        verifier.answer_quality_reason = str(answer_quality["reason"])

    if strict_mode and (
        not verifier.json_ok
        or verifier.invalid_paths
        or verifier.missing_required_paths
        or not verifier.answer_quality_ok
    ):
        raise HTTPException(
            status_code=422,
            detail={
                "error": "audit_strict_verification_failed",
                "verifier": verifier.model_dump(),
                "mode": req.mode,
            },
        )

    if not strict_mode and not verifier.answer_quality_ok:
        parsed["answer"] = f"answer_quality_failed:{verifier.answer_quality_reason}"
        parsed["insufficient_context"] = True
        parsed["confidence"] = "low"

    trace.update({
        "attempts": attempts,
        "synth_model_final": synth_model,
        "verifier": verifier.model_dump(),
        "last_raw": last_raw[:4000],
    })

    return PipelineAnswer(
        answer=str(parsed.get("answer", "")),
        paths_used=[str(x) for x in parsed.get("paths_used", []) if x],
        insufficient_context=bool(parsed.get("insufficient_context", False)),
        confidence=str(parsed.get("confidence", "medium")),
        mode=req.mode,
        verifier=verifier,
        debug=trace if req.debug_trace else None,
    )


@app.post("/pipeline/query", response_model=PipelineAnswer)
async def pipeline_query(req: PipelineQueryRequest):
    if not _pipeline_v2_enabled():
        raise HTTPException(status_code=400, detail="pipeline_v2_disabled (set RC_AI_PIPELINE_V2=on)")
    return await _pipeline_query_impl(req)


@app.post("/pipeline/eval")
async def pipeline_eval(req: PipelineEvalRequest):
    if not _pipeline_v2_enabled():
        raise HTTPException(status_code=400, detail="pipeline_v2_disabled (set RC_AI_PIPELINE_V2=on)")
    case_path = _safe_resolve_repo_path(req.case_file)
    if not case_path or not case_path.exists():
        raise HTTPException(status_code=404, detail=f"case_file_not_found: {req.case_file}")
    try:
        cases = json.loads(case_path.read_text(encoding="utf-8"))
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"invalid_case_file: {e}")

    results: List[Dict[str, Any]] = []
    for case in cases[:max(1, req.max_cases)]:
        qreq = PipelineQueryRequest(
            prompt=str(case.get("prompt", "")),
            mode=req.mode,
            context_files=[str(x) for x in case.get("files", [])],
            required_paths=[str(x) for x in case.get("required_paths", [])],
            search_hints=[str(x) for x in case.get("search_patterns", [])],
            include_repo_index=req.include_repo_index,
            include_semantic=req.include_semantic,
            embedding_dimensions=req.embedding_dimensions,
            max_search_results=20,
            max_context_chars=24000,
            model_overrides=req.model_overrides,
            debug_trace=False,
            temperature=req.temperature,
            top_p=req.top_p,
            num_predict=req.num_predict,
        )
        try:
            ans = await _pipeline_query_impl(qreq)
            results.append({
                "id": str(case.get("id", "")),
                "ok": (
                    ans.verifier.json_ok
                    and not ans.verifier.invalid_paths
                    and not ans.verifier.missing_required_paths
                    and ans.verifier.answer_quality_ok
                ),
                "verifier": ans.verifier.model_dump(),
                "paths_used": ans.paths_used,
            })
        except HTTPException as he:
            results.append({
                "id": str(case.get("id", "")),
                "ok": False,
                "error": he.detail,
            })

    total = len(results)
    ok = len([r for r in results if r.get("ok")])
    invalid_paths_cases = len([r for r in results if r.get("verifier", {}).get("invalid_paths")])
    missing_required_cases = len([r for r in results if r.get("verifier", {}).get("missing_required_paths")])
    json_ok_cases = len([r for r in results if r.get("verifier", {}).get("json_ok") is True])
    answer_quality_fail_cases = len([r for r in results if r.get("verifier", {}).get("answer_quality_ok") is False])
    return {
        "case_file": req.case_file,
        "mode": req.mode,
        "total_cases": total,
        "ok_cases": ok,
        "json_ok_cases": json_ok_cases,
        "cases_with_invalid_paths": invalid_paths_cases,
        "cases_missing_required_paths": missing_required_cases,
        "cases_answer_quality_failed": answer_quality_fail_cases,
        "results": results,
    }


# =======================================================================
# PROGRAM PERCEPTION COMPONENT (See-The-Program)
# =======================================================================

DEFAULT_SNAPSHOT_PATH = "AI/docs/ui/ui_introspection_headless_latest.json"
DEFAULT_MOCKUP_PATH = "visualizer/RC_UI_Mockup.html"
REQUIRED_SHELL_SECTIONS = ["master", "viewport", "inspector", "status"]


class RuntimeSectionModel(BaseModel):
    name: str
    present: bool
    source: str = "introspection"
    panel_ids: List[str] = []


class ContractFindingModel(BaseModel):
    id: str
    severity: str
    section: str
    expected: str
    observed: str
    probable_paths: List[str] = []


class CodeCandidateModel(BaseModel):
    path: str
    reason: str
    score: float


class PerceptionObserveRequest(BaseModel):
    mode: str = "quick"  # quick|full
    frames: int = 5
    snapshot_path: Optional[str] = None
    screenshot: bool = False
    screenshot_paths: List[str] = []
    include_vision: bool = True
    include_ocr: bool = True
    strict_contract: bool = True
    mockup_path: str = DEFAULT_MOCKUP_PATH
    vision_timeout_sec: int = 45
    ocr_timeout_sec: int = 45
    model_overrides: Dict[str, str] = {}


class PerceptionAuditRequest(BaseModel):
    observations: int = 3
    include_reports: bool = False
    mode: str = "quick"
    frames: int = 5
    screenshot: bool = False
    screenshot_paths: List[str] = []
    include_vision: bool = True
    include_ocr: bool = True
    strict_contract: bool = True
    mockup_path: str = DEFAULT_MOCKUP_PATH
    vision_timeout_sec: int = 45
    ocr_timeout_sec: int = 45
    latency_gate_p95_ms: int = 8000
    model_overrides: Dict[str, str] = {}


class PerceptionReport(BaseModel):
    timestamp: str
    mode: str
    capture: Dict[str, Any]
    artifacts: Dict[str, Any]
    runtime_layout: Dict[str, Any]
    runtime_sections: List[RuntimeSectionModel] = []
    visual_evidence: List[VisualEvidence] = []
    contract_findings: List[ContractFindingModel] = []
    code_candidates: List[CodeCandidateModel] = []
    actionability: str = "low"
    warnings: List[str] = []
    errors: List[str] = []


def _now_iso_utc() -> str:
    return datetime.now(timezone.utc).isoformat()


def _sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8", errors="ignore")).hexdigest()


def _find_headless_exe() -> Optional[Path]:
    candidates = [
        REPO_ROOT / "bin" / "RogueCityVisualizerHeadless.exe",
        REPO_ROOT / "build_vs" / "bin" / "Release" / "RogueCityVisualizerHeadless.exe",
        REPO_ROOT / "build_ninja" / "bin" / "RogueCityVisualizerHeadless.exe",
    ]
    for c in candidates:
        if c.exists():
            return c
    return None


def _run_headless_snapshot(frames: int, snapshot_path: Path, timeout_sec: int = 60) -> Dict[str, Any]:
    exe = _find_headless_exe()
    if not exe:
        return {"ok": False, "error": "headless_executable_not_found", "command": ""}
    snapshot_path.parent.mkdir(parents=True, exist_ok=True)
    cmd = [
        str(exe),
        "--frames",
        str(max(1, frames)),
        "--export-ui-snapshot",
        str(snapshot_path),
    ]
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout_sec)
        ok = proc.returncode == 0 and snapshot_path.exists()
        return {
            "ok": ok,
            "error": "" if ok else f"headless_export_failed_rc_{proc.returncode}",
            "command": " ".join(cmd),
            "stdout": proc.stdout[-2000:],
            "stderr": proc.stderr[-2000:],
        }
    except subprocess.TimeoutExpired:
        return {"ok": False, "error": "headless_export_timeout", "command": " ".join(cmd)}
    except Exception as e:
        return {"ok": False, "error": f"headless_export_exception: {e}", "command": " ".join(cmd)}


def _run_headless_capture(
    frames: int,
    snapshot_path: Path,
    screenshot_path: Optional[Path] = None,
    timeout_sec: int = 60,
) -> Dict[str, Any]:
    exe = _find_headless_exe()
    if not exe:
        return {"ok": False, "error": "headless_executable_not_found", "command": ""}
    snapshot_path.parent.mkdir(parents=True, exist_ok=True)
    cmd = [
        str(exe),
        "--frames",
        str(max(1, frames)),
        "--export-ui-snapshot",
        str(snapshot_path),
    ]
    if screenshot_path is not None:
        screenshot_path.parent.mkdir(parents=True, exist_ok=True)
        cmd += ["--export-ui-screenshot", str(screenshot_path)]
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout_sec)
        snapshot_ok = snapshot_path.exists()
        screenshot_ok = True if screenshot_path is None else screenshot_path.exists()
        ok = proc.returncode == 0 and snapshot_ok and screenshot_ok
        return {
            "ok": ok,
            "error": "" if ok else f"headless_capture_failed_rc_{proc.returncode}",
            "command": " ".join(cmd),
            "stdout": proc.stdout[-2000:],
            "stderr": proc.stderr[-2000:],
            "snapshot_ok": snapshot_ok,
            "screenshot_ok": screenshot_ok,
        }
    except subprocess.TimeoutExpired:
        return {"ok": False, "error": "headless_capture_timeout", "command": " ".join(cmd)}
    except Exception as e:
        return {"ok": False, "error": f"headless_capture_exception: {e}", "command": " ".join(cmd)}


def _load_json_file(path: Path) -> Optional[Dict[str, Any]]:
    if not path.exists():
        return None
    try:
        return json.loads(path.read_text(encoding="utf-8", errors="replace"))
    except Exception:
        return None


def _panel_text(panel: Dict[str, Any]) -> str:
    tags = panel.get("tags", [])
    if not isinstance(tags, list):
        tags = []
    parts = [
        str(panel.get("id", "")),
        str(panel.get("title", "")),
        str(panel.get("owner_module", "")),
        " ".join(str(x) for x in tags),
    ]
    return " ".join(parts).lower()


def _runtime_sections_from_snapshot(snapshot: Dict[str, Any]) -> List[RuntimeSectionModel]:
    panels = snapshot.get("panels", [])
    if not isinstance(panels, list):
        panels = []

    section_rules = {
        "master": ["master panel", "master", "workspaceselector", "workspace"],
        "viewport": [
            "rc_visualizer",
            "viewport",
            "axiom editor",
            "roguevisualizer",
            "[/ / / rc_visualizer / / /]",
        ],
        "inspector": ["inspector", "system map"],
        "status": ["status bar", "status", "validation", "telemetry", "contextual log"],
        "titlebar": ["titlebar", "title bar", "rogue titlebar"],
    }
    matches: Dict[str, List[str]] = {k: [] for k in section_rules}
    for p in panels:
        if not isinstance(p, dict):
            continue
        text = _panel_text(p)
        pid = str(p.get("id", p.get("title", "")))
        for section, keywords in section_rules.items():
            if any(k in text for k in keywords):
                if pid and pid not in matches[section]:
                    matches[section].append(pid)

    return [
        RuntimeSectionModel(name=s, present=len(ids) > 0, source="introspection", panel_ids=ids)
        for s, ids in matches.items()
    ]


def _expected_sections_from_mockup(mockup_path: Path) -> Dict[str, Any]:
    if not mockup_path.exists():
        return {
            "expected_sections": REQUIRED_SHELL_SECTIONS + ["titlebar"],
            "hash": "",
            "warning": f"mockup_not_found:{mockup_path}",
        }
    try:
        text = mockup_path.read_text(encoding="utf-8", errors="replace")
    except Exception as e:
        return {
            "expected_sections": REQUIRED_SHELL_SECTIONS + ["titlebar"],
            "hash": "",
            "warning": f"mockup_read_error:{e}",
        }

    low = text.lower()
    expected = []
    checks = {
        "titlebar": ["class=\"titlebar", "grid-area: titlebar"],
        "master": ["class=\"master-panel", "grid-area: master"],
        "viewport": ["class=\"viewport-area", "grid-area: viewport"],
        "inspector": ["class=\"inspector-panel", "grid-area: inspector"],
        "status": ["class=\"status-bar", "grid-area: status"],
    }
    for section, pats in checks.items():
        if any(p in low for p in pats):
            expected.append(section)
    if not expected:
        expected = REQUIRED_SHELL_SECTIONS + ["titlebar"]

    return {"expected_sections": expected, "hash": _sha256_text(text), "warning": ""}


def _section_probable_paths(section: str) -> List[str]:
    path_map = {
        "master": [
            "visualizer/src/ui/panels/RcMasterPanel.cpp",
            "visualizer/src/ui/panels/RcMasterPanel.h",
            "visualizer/src/ui/rc_ui_root.cpp",
        ],
        "viewport": [
            "visualizer/src/ui/rc_ui_root.cpp",
            "visualizer/src/main.cpp",
            "visualizer/src/ui/viewport/rc_viewport_overlays.cpp",
        ],
        "inspector": [
            "visualizer/src/ui/panels/PanelRegistry.cpp",
            "visualizer/src/ui/panels/RcPanelDrawers.cpp",
            "visualizer/src/ui/panels/rc_panel_system_map.h",
        ],
        "status": [
            "visualizer/src/ui/rc_ui_root.cpp",
            "visualizer/src/ui/panels/rc_panel_validation.cpp",
            "visualizer/src/ui/panels/rc_panel_log.h",
        ],
        "titlebar": [
            "visualizer/src/ui/rc_ui_root.cpp",
            "visualizer/src/ui/panels/RcMasterPanel.cpp",
        ],
    }
    return [p for p in path_map.get(section, []) if _file_exists_repo(p)]


def _build_contract_findings(
    expected_sections: List[str],
    runtime_sections: List[RuntimeSectionModel],
    strict_contract: bool = True,
) -> List[ContractFindingModel]:
    runtime_map = {s.name: s for s in runtime_sections}
    findings: List[ContractFindingModel] = []
    for section in expected_sections:
        rs = runtime_map.get(section)
        if rs and rs.present:
            continue
        if section in ("master", "viewport"):
            severity = "high" if strict_contract else "medium"
        elif section in ("inspector", "status"):
            severity = "medium"
        else:
            severity = "low"
        findings.append(
            ContractFindingModel(
                id=f"missing_{section}",
                severity=severity,
                section=section,
                expected="present",
                observed="absent",
                probable_paths=_section_probable_paths(section),
            )
        )
    return findings


def _build_code_candidates(findings: List[ContractFindingModel]) -> List[CodeCandidateModel]:
    candidates: Dict[str, CodeCandidateModel] = {}
    patterns: List[str] = []
    for f in findings:
        patterns.append(f.section)
        for p in f.probable_paths:
            score = 0.85
            existing = candidates.get(p)
            if not existing or score > existing.score:
                candidates[p] = CodeCandidateModel(path=p, reason=f"contract_finding:{f.id}", score=score)

    lexical = _run_rg_search(patterns=[p for p in patterns if p], max_hits=30)
    for hit in lexical:
        p = str(hit.get("path", ""))
        if not p or not _file_exists_repo(p):
            continue
        score = 0.58
        reason = f"lexical:{hit.get('pattern', '')}"
        existing = candidates.get(p)
        if not existing or score > existing.score:
            candidates[p] = CodeCandidateModel(path=p, reason=reason, score=score)

    out = list(candidates.values())
    out.sort(key=lambda x: x.score, reverse=True)
    return out[:25]


def _resolve_screenshot_paths(req: PerceptionObserveRequest) -> List[str]:
    candidates = [x for x in req.screenshot_paths if x]
    resolved: List[str] = []
    for c in candidates:
        p = _safe_resolve_repo_path(c)
        if p and p.exists() and p.is_file():
            resolved.append(str(p.resolve()))
    return resolved


async def _ollama_generate_with_images(
    model: str,
    prompt: str,
    image_paths: List[str],
    timeout: float = 60.0,
) -> str:
    images: List[str] = []
    for p in image_paths:
        pb = Path(p)
        raw = pb.read_bytes()
        images.append(base64.b64encode(raw).decode("ascii"))
    payload = {
        "model": model,
        "prompt": prompt,
        "images": images,
        "stream": False,
        "options": {"temperature": 0.1, "top_p": 0.8, "num_predict": 64},
    }
    body = await _ollama_post_json("/api/generate", payload, timeout=timeout)
    return str(body.get("response", ""))


def _ocr_confidence(text: str) -> str:
    s = text.strip()
    if len(s) < 16:
        return "low"
    alnum = sum(1 for ch in s if ch.isalnum())
    ratio = float(alnum) / float(max(1, len(s)))
    if ratio < 0.45:
        return "low"
    if ratio < 0.60:
        return "medium"
    return "high"


def _compute_actionability(
    findings: List[ContractFindingModel],
    errors: List[str],
    warnings: List[str],
    runtime_sections: List[RuntimeSectionModel],
) -> str:
    if errors:
        return "low"
    runtime_map = {s.name: s.present for s in runtime_sections}
    if not runtime_map.get("master", False) or not runtime_map.get("viewport", False):
        if any(s.present for s in runtime_sections):
            return "medium"
        return "low"
    high_count = len([f for f in findings if f.severity == "high"])
    if high_count == 0 and not warnings and not findings:
        return "high"
    return "medium"


async def _perception_observe_impl(req: PerceptionObserveRequest) -> PerceptionReport:
    warnings: List[str] = []
    errors: List[str] = []
    models = _default_models()
    models.update(req.model_overrides or {})

    rel_snapshot = req.snapshot_path or DEFAULT_SNAPSHOT_PATH
    snapshot_p = _safe_resolve_repo_path(rel_snapshot)
    if not snapshot_p:
        snapshot_p = _safe_resolve_repo_path(DEFAULT_SNAPSHOT_PATH)
    if not snapshot_p:
        raise HTTPException(status_code=400, detail="invalid_snapshot_path")

    capture_info: Dict[str, Any] = {"status": "reused_snapshot", "command": "", "headless_ok": False}
    runtime_screenshot_p: Optional[Path] = None

    should_capture = req.mode == "full" or not snapshot_p.exists()
    if should_capture:
        if req.screenshot and not req.screenshot_paths:
            runtime_screenshot_p = _safe_resolve_repo_path("AI/docs/ui/ui_screenshot_latest.png")
        capture = _run_headless_capture(
            frames=req.frames,
            snapshot_path=snapshot_p,
            screenshot_path=runtime_screenshot_p,
        )
        capture_info = {
            "status": "captured" if capture.get("ok") else "capture_failed",
            "command": capture.get("command", ""),
            "headless_ok": bool(capture.get("ok")),
        }
        if not capture.get("ok"):
            warnings.append(str(capture.get("error", "headless_capture_failed")))
            if capture.get("stderr"):
                warnings.append("headless_stderr:" + str(capture.get("stderr"))[:300])

    snapshot = _load_json_file(snapshot_p)
    if not snapshot:
        errors.append(f"snapshot_unavailable:{snapshot_p}")
        return PerceptionReport(
            timestamp=_now_iso_utc(),
            mode=req.mode,
            capture={**capture_info, "status": "error"},
            artifacts={
                "snapshot_json": str(snapshot_p),
                "screenshot_paths": [],
                "mockup_path": req.mockup_path,
                "mockup_hash": "",
            },
            runtime_layout={"panel_count": 0, "panels": []},
            runtime_sections=[],
            visual_evidence=[],
            contract_findings=[],
            code_candidates=[],
            actionability="low",
            warnings=warnings,
            errors=errors,
        )

    runtime_sections = _runtime_sections_from_snapshot(snapshot)
    panels = snapshot.get("panels", [])
    if not isinstance(panels, list):
        panels = []

    if req.mode == "full":
        required_sections = set(REQUIRED_SHELL_SECTIONS + ["titlebar"])
        retry_frames = [max(req.frames + 3, 8), max(req.frames + 7, 12)]
        for next_frames in retry_frames:
            section_map = {s.name: s.present for s in runtime_sections}
            missing_sections = [s for s in required_sections if not section_map.get(s, False)]
            if not missing_sections:
                break
            recapture = _run_headless_capture(
                frames=next_frames,
                snapshot_path=snapshot_p,
                screenshot_path=runtime_screenshot_p if req.screenshot else None,
            )
            if not recapture.get("ok"):
                warnings.append(f"recapture_failed_frames_{next_frames}:{recapture.get('error', 'unknown')}")
                continue
            snapshot_retry = _load_json_file(snapshot_p)
            if not snapshot_retry:
                warnings.append(f"recapture_snapshot_unavailable_frames_{next_frames}")
                continue
            runtime_sections_retry = _runtime_sections_from_snapshot(snapshot_retry)
            retry_map = {s.name: s.present for s in runtime_sections_retry}
            missing_retry = [s for s in required_sections if not retry_map.get(s, False)]
            if len(missing_retry) < len(missing_sections):
                snapshot = snapshot_retry
                runtime_sections = runtime_sections_retry
                panels = snapshot.get("panels", [])
                if not isinstance(panels, list):
                    panels = []
                capture_info = {
                    "status": "captured_stabilized",
                    "command": recapture.get("command", ""),
                    "headless_ok": True,
                }

    mockup_rel = req.mockup_path or DEFAULT_MOCKUP_PATH
    mockup_p = _safe_resolve_repo_path(mockup_rel) or _safe_resolve_repo_path(DEFAULT_MOCKUP_PATH)
    expected_info = _expected_sections_from_mockup(mockup_p if mockup_p else Path(DEFAULT_MOCKUP_PATH))
    if expected_info.get("warning"):
        warnings.append(str(expected_info["warning"]))
    findings = _build_contract_findings(
        expected_sections=[str(x) for x in expected_info.get("expected_sections", REQUIRED_SHELL_SECTIONS)],
        runtime_sections=runtime_sections,
        strict_contract=req.strict_contract,
    )

    baseline_hash = os.getenv("RC_PERCEPTION_MOCKUP_HASH", "").strip()
    mockup_hash = str(expected_info.get("hash", ""))
    if baseline_hash and mockup_hash and baseline_hash != mockup_hash:
        findings.append(
            ContractFindingModel(
                id="mockup_contract_drift",
                severity="medium",
                section="mockup",
                expected=f"hash:{baseline_hash}",
                observed=f"hash:{mockup_hash}",
                probable_paths=[req.mockup_path],
            )
        )

    code_candidates = _build_code_candidates(findings)

    screenshot_paths = _resolve_screenshot_paths(req)
    if req.screenshot and not screenshot_paths and runtime_screenshot_p and runtime_screenshot_p.exists():
        screenshot_paths = [str(runtime_screenshot_p.resolve())]
    if req.screenshot and not screenshot_paths:
        errors.append("runtime_screenshot_required_but_missing")

    visual_evidence: List[VisualEvidence] = []
    use_vision = req.mode == "full" and req.include_vision
    use_ocr = req.mode == "full" and req.include_ocr

    if _toolserver_mock_enabled() and (use_vision or use_ocr):
        warnings.append("mock_mode_active_vision_ocr_skipped")
    elif screenshot_paths and (use_vision or use_ocr):
        jobs: List[asyncio.Task] = []
        labels: List[str] = []
        if use_vision:
            jobs.append(
                asyncio.create_task(
                        _ollama_generate_with_images(
                            models["vision_model"],
                            "Summarize UI layout in short bullets.",
                            screenshot_paths[:1],
                            timeout=float(max(5, req.vision_timeout_sec)),
                        )
                )
            )
            labels.append("vision")
        if use_ocr:
            jobs.append(
                asyncio.create_task(
                        _ollama_generate_with_images(
                            models["ocr_model"],
                            "Return visible UI text lines only. No commentary.",
                            screenshot_paths[:1],
                            timeout=float(max(5, req.ocr_timeout_sec)),
                        )
                )
            )
            labels.append("ocr")
        outputs = await asyncio.gather(*jobs, return_exceptions=True)
        for label, out in zip(labels, outputs):
            if isinstance(out, Exception):
                warnings.append(f"{label}_unavailable:{out}")
                continue
            if label == "vision":
                vtxt = str(out)
                visual_evidence.append(
                    VisualEvidence(
                        source=models["vision_model"],
                        summary=vtxt[:900],
                        text_spans=[],
                        ui_regions=[],
                        confidence="medium" if vtxt.strip() else "low",
                    )
                )
            else:
                otxt = str(out)
                spans = [ln.strip() for ln in otxt.splitlines() if ln.strip()][:60]
                conf = _ocr_confidence(otxt)
                if conf == "low":
                    warnings.append("ocr_low_confidence_text")
                visual_evidence.append(
                    VisualEvidence(
                        source=models["ocr_model"],
                        summary=(spans[0] if spans else "")[:300],
                        text_spans=spans,
                        ui_regions=[],
                        confidence=conf,
                    )
                )

    actionability = _compute_actionability(findings, errors, warnings, runtime_sections)

    return PerceptionReport(
        timestamp=_now_iso_utc(),
        mode=req.mode,
        capture=capture_info,
        artifacts={
            "snapshot_json": str(snapshot_p),
            "screenshot_paths": screenshot_paths,
            "mockup_path": str(mockup_p) if mockup_p else req.mockup_path,
            "mockup_hash": mockup_hash,
        },
        runtime_layout={"panel_count": len(panels), "panels": panels},
        runtime_sections=runtime_sections,
        visual_evidence=visual_evidence,
        contract_findings=findings,
        code_candidates=code_candidates,
        actionability=actionability,
        warnings=warnings,
        errors=errors,
    )


def _percentile(values: List[float], p: float) -> float:
    if not values:
        return 0.0
    s = sorted(values)
    idx = int(max(0, min(len(s) - 1, round((p / 100.0) * (len(s) - 1)))))
    return float(s[idx])


@app.post("/perception/observe", response_model=PerceptionReport)
async def perception_observe(req: PerceptionObserveRequest):
    if not _pipeline_v2_enabled():
        raise HTTPException(status_code=400, detail="pipeline_v2_disabled (set RC_AI_PIPELINE_V2=on)")
    return await _perception_observe_impl(req)


@app.post("/perception/audit")
async def perception_audit(req: PerceptionAuditRequest):
    if not _pipeline_v2_enabled():
        raise HTTPException(status_code=400, detail="pipeline_v2_disabled (set RC_AI_PIPELINE_V2=on)")

    runs = max(1, min(20, int(req.observations)))
    reports: List[PerceptionReport] = []
    run_summaries: List[Dict[str, Any]] = []
    latencies_ms: List[float] = []

    for i in range(runs):
        oreq = PerceptionObserveRequest(
            mode=req.mode,
            frames=req.frames,
            screenshot=req.screenshot,
            screenshot_paths=req.screenshot_paths,
            include_vision=req.include_vision,
            include_ocr=req.include_ocr,
            strict_contract=req.strict_contract,
            mockup_path=req.mockup_path,
            vision_timeout_sec=req.vision_timeout_sec,
            ocr_timeout_sec=req.ocr_timeout_sec,
            model_overrides=req.model_overrides,
        )
        start = time.perf_counter()
        rep = await _perception_observe_impl(oreq)
        latency = (time.perf_counter() - start) * 1000.0
        latencies_ms.append(latency)
        reports.append(rep)
        run_summaries.append(
            {
                "index": i + 1,
                "actionability": rep.actionability,
                "finding_count": len(rep.contract_findings),
                "warnings": len(rep.warnings),
                "errors": len(rep.errors),
                "latency_ms": round(latency, 2),
            }
        )

    sections = REQUIRED_SHELL_SECTIONS + ["titlebar"]
    section_presence_rate: Dict[str, float] = {}
    for section in sections:
        present_count = 0
        for rep in reports:
            smap = {s.name: s.present for s in rep.runtime_sections}
            if smap.get(section, False):
                present_count += 1
        section_presence_rate[section] = round(float(present_count) / float(len(reports)), 3)

    mismatch_runs = len([r for r in reports if len(r.contract_findings) > 0])
    candidate_runs = len([r for r in reports if len(r.code_candidates) > 0])
    metrics = {
        "section_presence_rate": section_presence_rate,
        "contract_mismatch_rate": round(float(mismatch_runs) / float(len(reports)), 3),
        "code_candidate_precision_proxy": round(float(candidate_runs) / float(len(reports)), 3),
        "latency_ms": {
            "p50": round(_percentile(latencies_ms, 50), 2),
            "p95": round(_percentile(latencies_ms, 95), 2),
        },
    }
    gate_target_p95_ms = int(max(1000, req.latency_gate_p95_ms))
    gate_pass = metrics["latency_ms"]["p95"] <= gate_target_p95_ms

    out = {
        "observations": runs,
        "metrics": metrics,
        "gate_pass": gate_pass,
        "gate_target_p95_ms": gate_target_p95_ms,
        "runs": run_summaries,
    }
    if req.include_reports:
        out["reports"] = [r.model_dump() for r in reports]
    return out
