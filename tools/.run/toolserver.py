# File: .run/toolserver.py
from fastapi import FastAPI
from pydantic import BaseModel
import httpx
import uvicorn

OLLAMA_URL = "http://127.0.0.1:11434"
DEFAULT_MODEL = "qwen3-coder-optimized:latest"

app = FastAPI()

class ChatIn(BaseModel):
    prompt: str
    model: str | None = None
    system: str | None = None

@app.get("/health")
async def health():
    try:
        async with httpx.AsyncClient(timeout=5.0) as client:
            r = await client.get(f"{OLLAMA_URL}/api/tags")
            r.raise_for_status()
            data = r.json()
            models = data.get("models", [])
            model_names = [m.get("name", "unknown") for m in models]
            return {
                "ok": True,
                "ollama": "connected",
                "models_available": len(models),
                "models": model_names,
                "default_model": DEFAULT_MODEL
            }
    except httpx.ConnectError:
        return {"ok": False, "ollama": "disconnected", "error": f"Cannot connect to Ollama at {OLLAMA_URL}"}
    except Exception as e:
        return {"ok": False, "ollama": "error", "error": str(e)}

@app.get("/models")
async def list_models():
    async with httpx.AsyncClient(timeout=5.0) as client:
        r = await client.get(f"{OLLAMA_URL}/api/tags")
        r.raise_for_status()
        return r.json()

@app.post("/chat")
async def chat(payload: ChatIn):
    model = payload.model or DEFAULT_MODEL
    prompt = payload.prompt if not payload.system else f"{payload.system}\n\n{payload.prompt}"
    req = {"model": model, "prompt": prompt, "stream": False}
    async with httpx.AsyncClient(timeout=300.0) as client:
        r = await client.post(f"{OLLAMA_URL}/api/generate", json=req)
        r.raise_for_status()
        data = r.json()
        return {"model": model, "response": data.get("response", "")}

if __name__ == "__main__":
    uvicorn.run(app, host="127.0.0.1", port=7077, log_level="warning")
