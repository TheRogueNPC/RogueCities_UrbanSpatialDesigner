# File: .run/toolserver.py
# Local API for n8n Cloud -> your machine -> Ollama
# Runs on localhost only: http://127.0.0.1:7077
# Endpoints:
#   GET  /health
#   POST /chat  {prompt, model?, system?}
#
# Security:
#   Requires header: X-API-KEY: <key>

from fastapi import FastAPI, Header, HTTPException
from pydantic import BaseModel
import httpx
import uvicorn

OLLAMA_URL = "http://127.0.0.1:11434"
DEFAULT_MODEL = "qwen3-coder-optimized:latest"
API_KEY = "dba23a12396d4184be2839d731ea4b93"

app = FastAPI()

class ChatIn(BaseModel):
    prompt: str
    model: str | None = None
    system: str | None = None

def require_key(x_api_key: str | None):
    if x_api_key != API_KEY:
        raise HTTPException(status_code=401, detail="Unauthorized")

@app.get("/health")
def health(x_api_key: str | None = Header(default=None)):
    require_key(x_api_key)
    return {"ok": True}

@app.post("/chat")
async def chat(payload: ChatIn, x_api_key: str | None = Header(default=None)):
    require_key(x_api_key)

    model = payload.model or DEFAULT_MODEL
    prompt = payload.prompt if not payload.system else f"{payload.system}\n\n{payload.prompt}"
    req = {"model": model, "prompt": prompt, "stream": False}

    async with httpx.AsyncClient(timeout=300.0) as client:
        r = await client.post(f"{OLLAMA_URL}/api/generate", json=req)
        r.raise_for_status()
        data = r.json()

    return {"model": model, "response": data.get("response", "")}

if __name__ == "__main__":
    uvicorn.run(app, host="127.0.0.1", port=7077)
