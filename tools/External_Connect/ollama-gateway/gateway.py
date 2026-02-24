# File: gateway.py
# Gateway for Ollama LLM and codebase file operations (read + write)
#
# Features:
# - Accepts BOTH auth styles: X-API-Key header or Authorization: Bearer
# - Read operations: listFiles, readFile, searchCode
# - Write operations: ensureDir, writeFile, appendFile, deletePath, movePath, stat
# - Ollama proxy: tags, generate, chat
# - Safety: path sandboxing, size limits, atomic writes, audit logging
# - Robustness: timeouts, non-blocking I/O, caching, rate limiting

import os
import re
import json
import base64
import shutil
import hashlib
import fnmatch
import asyncio
import time
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from functools import lru_cache
from pathlib import Path
from typing import Optional, Dict, Any, List

import httpx
from fastapi import FastAPI, Header, HTTPException, Query, Request, Response
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel, Field

APP_VERSION = "1.3.0"

# ---------------------------
# Environment config
# ---------------------------

CODE_ROOT = Path(
    os.getenv("CODE_ROOT", r"D:\Projects\RogueCities\RogueCities_UrbanSpatialDesigner")
).resolve()

API_KEY = os.getenv(
    "API_KEY",
    "Rogue_Ollama_Says_Hi_R46U8FJ48fjhKAUhfSSFEs64fs6f4SFF4SDSDFWETHARGSADFGEdfwfge4gasdgadsfgasdf4a7744354s9777645",
)

OLLAMA_BASE = os.getenv("OLLAMA_BASE_URL") or os.getenv(
    "OLLAMA_BASE", "http://127.0.0.1:11434"
)
OLLAMA_BASE = OLLAMA_BASE.rstrip("/")

# Write operation config
WRITE_ROOT = Path(os.getenv("WRITE_ROOT", str(CODE_ROOT))).resolve()
WRITE_ALLOWLIST = [
    p.strip() for p in os.getenv("WRITE_ALLOWLIST", "").split(",") if p.strip()
]
MAX_WRITE_SIZE = int(os.getenv("MAX_WRITE_SIZE", "2097152"))  # 2MB default
MAX_WRITE_SIZE_LIMIT = 10485760  # 10MB hard cap
AUDIT_DIR = Path(os.getenv("AUDIT_DIR", str(CODE_ROOT / ".gateway-audit")))

# Thread pool for blocking I/O
IO_EXECUTOR = ThreadPoolExecutor(max_workers=4)

# Request tracking for rate limiting
_active_requests = 0
_max_concurrent_requests = 10
_request_timestamps: List[float] = []
_rate_limit_window = 60
_rate_limit_max = 100

app = FastAPI(title="Local Ollama + Codebase Gateway", version=APP_VERSION)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


# ---------------------------
# Middleware: Timeout & Rate Limiting
# ---------------------------


@app.middleware("http")
async def timeout_and_rate_limit(request: Request, call_next):
    global _active_requests, _request_timestamps

    # Rate limiting check
    now = time.time()
    _request_timestamps = [
        t for t in _request_timestamps if now - t < _rate_limit_window
    ]
    if len(_request_timestamps) >= _rate_limit_max:
        return Response(
            content=json.dumps(
                {"error": "Rate limit exceeded", "retry_after": _rate_limit_window}
            ),
            status_code=429,
            media_type="application/json",
        )
    _request_timestamps.append(now)

    # Concurrent request limit
    if _active_requests >= _max_concurrent_requests:
        return Response(
            content=json.dumps({"error": "Too many concurrent requests"}),
            status_code=503,
            media_type="application/json",
        )

    _active_requests += 1
    try:
        # 25 second timeout (leaving margin for zrok's typical 30s timeout)
        result = await asyncio.wait_for(call_next(request), timeout=25.0)
        return result
    except asyncio.TimeoutError:
        return Response(
            content=json.dumps({"error": "Request timeout", "timeout_seconds": 25}),
            status_code=504,
            media_type="application/json",
        )
    finally:
        _active_requests -= 1


@app.on_event("startup")
async def startup_check():
    if not CODE_ROOT.exists():
        raise RuntimeError(f"CODE_ROOT does not exist: {CODE_ROOT}")
    if not (CODE_ROOT / ".git").exists():
        print(f"WARNING: CODE_ROOT is not a git repo: {CODE_ROOT}")
    print(f"Gateway v{APP_VERSION} starting...")
    print(f"  CODE_ROOT: {CODE_ROOT}")
    print(f"  WRITE_ROOT: {WRITE_ROOT}")
    print(f"  OLLAMA_BASE: {OLLAMA_BASE}")


# ---------------------------
# Audit logging
# ---------------------------


def _get_audit_log_path() -> Path:
    """Get the audit log file path for today."""
    AUDIT_DIR.mkdir(parents=True, exist_ok=True)
    date_str = datetime.now(timezone.utc).strftime("%Y-%m-%d")
    return AUDIT_DIR / f"{date_str}.jsonl"


def _audit_log(
    operation: str,
    path: str,
    success: bool,
    size: int = 0,
    error: Optional[str] = None,
    extra: Optional[Dict[str, Any]] = None,
):
    """Append an audit entry to today's log file."""
    entry = {
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "operation": operation,
        "path": path,
        "size": size,
        "success": success,
        "error": error,
    }
    if extra:
        entry.update(extra)

    try:
        log_path = _get_audit_log_path()
        with open(log_path, "a", encoding="utf-8") as f:
            f.write(json.dumps(entry) + "\n")
    except Exception:
        pass  # Don't fail operations if audit logging fails


# ---------------------------
# Auth helpers
# ---------------------------


def _bearer_token(authorization: Optional[str]) -> Optional[str]:
    """Extract token from 'Authorization: Bearer <token>'."""
    if not authorization:
        return None
    parts = authorization.strip().split(" ", 1)
    if len(parts) == 2 and parts[0].lower() == "bearer":
        return parts[1].strip()
    return None


def _auth(x_api_key: Optional[str], authorization: Optional[str] = None):
    """
    Accept either:
      - X-API-Key: <key>
      - Authorization: Bearer <key>
    """
    candidate = x_api_key or _bearer_token(authorization)
    if not candidate or candidate != API_KEY:
        raise HTTPException(status_code=401, detail="Unauthorized")


# ---------------------------
# Path + file helpers
# ---------------------------


def _safe_path(rel: str) -> Path:
    """Prevent path traversal outside CODE_ROOT."""
    rel = rel.replace("\\", "/").strip("/")
    p = (CODE_ROOT / rel).resolve()
    try:
        p.relative_to(CODE_ROOT)
    except Exception:
        raise HTTPException(status_code=400, detail="Path escapes code root")
    return p


def _safe_write_path(rel: str) -> Path:
    """Validate path for write operations. Returns resolved path."""
    rel = rel.replace("\\", "/").strip("/")
    p = (WRITE_ROOT / rel).resolve()

    # Must be within WRITE_ROOT
    try:
        p.relative_to(WRITE_ROOT)
    except Exception:
        raise HTTPException(status_code=400, detail="Path escapes write root")

    # If allowlist is configured, check against it
    if WRITE_ALLOWLIST:
        rel_path = rel.lower()
        allowed = any(
            rel_path.startswith(allowed_path.lower())
            for allowed_path in WRITE_ALLOWLIST
        )
        if not allowed:
            raise HTTPException(
                status_code=403,
                detail=f"Path not in write allowlist. Allowed prefixes: {WRITE_ALLOWLIST}",
            )

    return p


def _is_hidden(path: Path) -> bool:
    """Check if path is hidden (dotfiles)."""
    try:
        rel = path.relative_to(CODE_ROOT)
    except Exception:
        return False
    return any(part.startswith(".") for part in rel.parts)


def _atomic_write(path: Path, content: bytes) -> None:
    """Write content to path atomically (write to temp, then rename)."""
    tmp_path = path.with_suffix(path.suffix + ".tmp")
    try:
        tmp_path.write_bytes(content)
        tmp_path.rename(path)
    except Exception as e:
        try:
            tmp_path.unlink()
        except Exception:
            pass
        raise e


# ---------------------------
# Async I/O helpers
# ---------------------------


async def run_io(func, *args, **kwargs):
    """Run blocking I/O in thread pool."""
    loop = asyncio.get_event_loop()
    return await loop.run_in_executor(IO_EXECUTOR, lambda: func(*args, **kwargs))


_file_list_cache: Dict[str, tuple] = {}
_cache_ttl = 5.0


async def get_cached_file_list(cache_key: str, ttl: float = _cache_ttl):
    """Check if we have a cached file list."""
    if cache_key in _file_list_cache:
        data, timestamp = _file_list_cache[cache_key]
        if time.time() - timestamp < ttl:
            return data
    return None


def set_cached_file_list(cache_key: str, data: Any):
    """Cache file list data."""
    _file_list_cache[cache_key] = (data, time.time())


# ---------------------------
# Models
# ---------------------------


class SearchRequest(BaseModel):
    query: str
    path: str = ""
    glob: Optional[str] = None
    maxResults: int = Field(default=100, ge=1, le=500)
    contextLines: int = Field(default=2, ge=0, le=20)
    caseSensitive: bool = False
    useRegex: bool = False


class EnsureDirRequest(BaseModel):
    path: str


class WriteFileRequest(BaseModel):
    path: str
    content: str
    encoding: str = Field(default="text", pattern="^(text|base64)$")
    overwrite: bool = False
    createDirs: bool = True


class AppendFileRequest(BaseModel):
    path: str
    content: str
    encoding: str = Field(default="text", pattern="^(text|base64)$")
    createIfMissing: bool = True


class DeletePathRequest(BaseModel):
    path: str
    recursive: bool = False


class MovePathRequest(BaseModel):
    source: str
    destination: str
    overwrite: bool = False


# ---------------------------
# Response models
# ---------------------------


class OperationResult(BaseModel):
    ok: bool
    path: str
    message: Optional[str] = None
    error: Optional[str] = None


class StatResult(BaseModel):
    path: str
    exists: bool
    type: Optional[str] = None
    size: Optional[int] = None
    modifiedTime: Optional[str] = None
    createdTime: Optional[str] = None
    isWritable: Optional[bool] = None


# ---------------------------
# Basic routes
# ---------------------------


@app.get("/")
async def root():
    return {
        "name": "Local Ollama + Codebase Gateway",
        "version": APP_VERSION,
        "status": "ok",
        "codeRoot": str(CODE_ROOT),
        "writeRoot": str(WRITE_ROOT),
        "ollamaBase": OLLAMA_BASE,
        "routes": {
            "read": ["/health", "/files", "/file", "/search", "/fs/stat"],
            "write": [
                "/fs/ensureDir",
                "/fs/writeFile",
                "/fs/appendFile",
                "/fs/deletePath",
                "/fs/movePath",
            ],
            "ollama": ["/ollama/tags", "/ollama/generate", "/ollama/chat"],
        },
    }


@app.get("/health")
async def health():
    reachable = False
    try:
        async with httpx.AsyncClient(timeout=3.0) as client:
            r = await client.get(f"{OLLAMA_BASE}/api/tags")
            reachable = r.status_code == 200
    except Exception:
        reachable = False

    return {
        "status": "ok",
        "gatewayVersion": APP_VERSION,
        "ollama": {"baseUrl": OLLAMA_BASE, "reachable": reachable},
        "active_requests": _active_requests,
        "code_root_exists": CODE_ROOT.exists(),
    }


@app.get("/ping")
async def ping():
    return {"pong": True, "ts": time.time()}


# ---------------------------
# Read operations
# ---------------------------


@app.get("/files")
async def list_files(
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
    path: str = "",
    recursive: bool = False,
    includeHidden: bool = False,
    maxEntries: int = Query(default=5000, ge=1, le=20000),
):
    _auth(x_api_key, authorization)

    target = _safe_path(path)
    if not target.exists() or not target.is_dir():
        raise HTTPException(status_code=400, detail="Not a directory")

    cache_key = f"files:{path}:{recursive}:{includeHidden}:{maxEntries}"

    if not recursive:
        cached = await get_cached_file_list(cache_key)
        if cached:
            return {**cached, "cached": True}

    def _list_dir():
        entries = []
        iterator = target.rglob("*") if recursive else target.iterdir()
        count = 0
        truncated = False

        for p in iterator:
            if count >= maxEntries:
                truncated = True
                break

            if (not includeHidden) and _is_hidden(p):
                continue

            rel = str(p.relative_to(CODE_ROOT)).replace("\\", "/")

            if p.is_dir():
                entries.append({"path": rel, "type": "dir"})
            else:
                try:
                    st = p.stat()
                    entries.append(
                        {
                            "path": rel,
                            "type": "file",
                            "size": st.st_size,
                            "modifiedTime": datetime.fromtimestamp(
                                st.st_mtime
                            ).isoformat(),
                        }
                    )
                except Exception:
                    continue

            count += 1

        return {"root": str(CODE_ROOT), "entries": entries, "truncated": truncated}

    result = await run_io(_list_dir)

    if not recursive:
        set_cached_file_list(cache_key, result)

    return result


@app.get("/file")
async def read_file(
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
    path: str = Query(...),
    maxBytes: int = Query(default=262144, ge=1, le=5242880),
):
    _auth(x_api_key, authorization)

    p = _safe_path(path)
    if not p.exists() or not p.is_file():
        raise HTTPException(status_code=404, detail="File not found")

    try:
        data = p.read_bytes()
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"Could not read file: {e}")

    truncated = False
    if len(data) > maxBytes:
        data = data[:maxBytes]
        truncated = True

    try:
        text = data.decode("utf-8")
        return {
            "path": path,
            "encoding": "text",
            "content": text,
            "truncated": truncated,
        }
    except Exception:
        b64 = base64.b64encode(data).decode("ascii")
        return {
            "path": path,
            "encoding": "base64",
            "content": b64,
            "truncated": truncated,
        }


@app.post("/search")
async def search_code(
    req: SearchRequest,
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    _auth(x_api_key, authorization)

    base = _safe_path(req.path or "")
    if not base.exists() or not base.is_dir():
        raise HTTPException(status_code=400, detail="Search path is not a directory")

    def _do_search():
        flags = 0 if req.caseSensitive else re.IGNORECASE
        pattern = re.compile(req.query if req.useRegex else re.escape(req.query), flags)

        matches = []
        truncated = False
        files_searched = 0
        max_files = 1000

        glob_patterns = None
        if req.glob:
            glob_patterns = [g.strip() for g in req.glob.split(",") if g.strip()]

        for fp in base.rglob("*"):
            if fp.is_dir():
                continue
            if _is_hidden(fp):
                continue

            if glob_patterns:
                matches_glob = False
                for g in glob_patterns:
                    if fnmatch.fnmatch(fp.name, g):
                        matches_glob = True
                        break
                if not matches_glob:
                    continue

            files_searched += 1
            if files_searched > max_files:
                truncated = True
                break

            try:
                if fp.stat().st_size > 500_000:
                    continue
            except Exception:
                continue

            rel = str(fp.relative_to(CODE_ROOT)).replace("\\", "/")

            try:
                lines = fp.read_text(encoding="utf-8", errors="replace").splitlines()
            except Exception:
                continue

            for i, line in enumerate(lines, start=1):
                m = pattern.search(line)
                if not m:
                    continue

                start = max(1, i - req.contextLines)
                end = min(len(lines), i + req.contextLines)

                before = lines[start - 1 : i - 1]
                after = lines[i:end]

                matches.append(
                    {
                        "path": rel,
                        "line": i,
                        "column": m.start() + 1,
                        "preview": line[:200],
                        "before": before[:3],
                        "after": after[:3],
                    }
                )

                if len(matches) >= req.maxResults:
                    truncated = True
                    break

            if truncated:
                break

        return {
            "query": req.query,
            "matches": matches,
            "truncated": truncated,
            "files_searched": files_searched,
        }

    return await run_io(_do_search)


# ---------------------------
# Write operations
# ---------------------------


@app.get("/fs/stat")
async def stat_path(
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
    path: str = Query(...),
):
    """Get information about a file or directory."""
    _auth(x_api_key, authorization)

    p = _safe_path(path)

    if not p.exists():
        return StatResult(path=path, exists=False)

    try:
        st = p.stat()
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"Could not stat path: {e}")

    result = StatResult(
        path=path,
        exists=True,
        type="dir" if p.is_dir() else "file",
        size=st.st_size if p.is_file() else None,
        modifiedTime=datetime.fromtimestamp(st.st_mtime).isoformat(),
        createdTime=datetime.fromtimestamp(st.st_ctime).isoformat(),
    )

    # Check writability
    try:
        result.isWritable = os.access(p, os.W_OK)
    except Exception:
        result.isWritable = None

    return result


@app.post("/fs/ensureDir")
async def ensure_dir(
    req: EnsureDirRequest,
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    """Create a directory (and all parents) if it doesn't exist. Idempotent."""
    _auth(x_api_key, authorization)

    p = _safe_write_path(req.path)

    if p.exists():
        if p.is_dir():
            return OperationResult(
                ok=True, path=req.path, message="Directory already exists"
            )
        else:
            raise HTTPException(
                status_code=409, detail="Path exists but is a file, not a directory"
            )

    try:
        p.mkdir(parents=True, exist_ok=True)
        _audit_log("ensureDir", req.path, True)
        return OperationResult(ok=True, path=req.path, message="Directory created")
    except Exception as e:
        _audit_log("ensureDir", req.path, False, error=str(e))
        raise HTTPException(status_code=500, detail=f"Failed to create directory: {e}")


@app.post("/fs/writeFile")
async def write_file(
    req: WriteFileRequest,
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    """Write content to a file. Atomic write (temp file + rename). Requires overwrite=true for existing files."""
    _auth(x_api_key, authorization)

    p = _safe_write_path(req.path)

    # Decode content
    try:
        if req.encoding == "base64":
            content = base64.b64decode(req.content)
        else:
            content = req.content.encode("utf-8")
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"Failed to decode content: {e}")

    # Check size limit
    effective_limit = min(MAX_WRITE_SIZE, MAX_WRITE_SIZE_LIMIT)
    if len(content) > effective_limit:
        raise HTTPException(
            status_code=413,
            detail=f"Content size ({len(content)} bytes) exceeds limit ({effective_limit} bytes)",
        )

    # Check if file exists
    if p.exists():
        if not p.is_file():
            raise HTTPException(status_code=409, detail="Path exists but is not a file")
        if not req.overwrite:
            raise HTTPException(
                status_code=409, detail="File exists. Set overwrite=true to replace it."
            )

    # Create parent directories if needed
    if req.createDirs and not p.parent.exists():
        try:
            p.parent.mkdir(parents=True, exist_ok=True)
        except Exception as e:
            raise HTTPException(
                status_code=500, detail=f"Failed to create parent directories: {e}"
            )

    # Atomic write
    try:
        _atomic_write(p, content)
        _audit_log(
            "writeFile",
            req.path,
            True,
            size=len(content),
            extra={"overwrite": req.overwrite},
        )
        return OperationResult(
            ok=True, path=req.path, message=f"File written ({len(content)} bytes)"
        )
    except Exception as e:
        _audit_log("writeFile", req.path, False, size=len(content), error=str(e))
        raise HTTPException(status_code=500, detail=f"Failed to write file: {e}")


@app.post("/fs/appendFile")
async def append_file(
    req: AppendFileRequest,
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    """Append content to a file. Creates file if missing and createIfMissing=true."""
    _auth(x_api_key, authorization)

    p = _safe_write_path(req.path)

    # Decode content
    try:
        if req.encoding == "base64":
            content = base64.b64decode(req.content)
        else:
            content = req.content.encode("utf-8")
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"Failed to decode content: {e}")

    # Check size limit (for append operation)
    effective_limit = min(MAX_WRITE_SIZE, MAX_WRITE_SIZE_LIMIT)
    if len(content) > effective_limit:
        raise HTTPException(
            status_code=413,
            detail=f"Content size ({len(content)} bytes) exceeds limit ({effective_limit} bytes)",
        )

    # Check if file exists
    if not p.exists():
        if req.createIfMissing:
            # Create parent directories if needed
            if not p.parent.exists():
                try:
                    p.parent.mkdir(parents=True, exist_ok=True)
                except Exception as e:
                    raise HTTPException(
                        status_code=500,
                        detail=f"Failed to create parent directories: {e}",
                    )
        else:
            raise HTTPException(
                status_code=404,
                detail="File does not exist. Set createIfMissing=true to create it.",
            )

    if p.exists() and not p.is_file():
        raise HTTPException(status_code=409, detail="Path exists but is not a file")

    # Append
    try:
        with open(p, "ab") as f:
            f.write(content)
        _audit_log("appendFile", req.path, True, size=len(content))
        return OperationResult(
            ok=True, path=req.path, message=f"Appended {len(content)} bytes"
        )
    except Exception as e:
        _audit_log("appendFile", req.path, False, size=len(content), error=str(e))
        raise HTTPException(status_code=500, detail=f"Failed to append to file: {e}")


@app.post("/fs/deletePath")
async def delete_path(
    req: DeletePathRequest,
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    """Delete a file or directory. Set recursive=true for non-empty directories."""
    _auth(x_api_key, authorization)

    p = _safe_write_path(req.path)

    if not p.exists():
        raise HTTPException(status_code=404, detail="Path does not exist")

    try:
        if p.is_file():
            p.unlink()
            _audit_log("deletePath", req.path, True, extra={"type": "file"})
            return OperationResult(ok=True, path=req.path, message="File deleted")
        elif p.is_dir():
            if any(p.iterdir()) and not req.recursive:
                raise HTTPException(
                    status_code=400,
                    detail="Directory is not empty. Set recursive=true to delete.",
                )
            shutil.rmtree(p) if req.recursive else p.rmdir()
            _audit_log(
                "deletePath",
                req.path,
                True,
                extra={"type": "dir", "recursive": req.recursive},
            )
            return OperationResult(ok=True, path=req.path, message="Directory deleted")
        else:
            raise HTTPException(
                status_code=400, detail="Path is neither a file nor a directory"
            )
    except HTTPException:
        raise
    except Exception as e:
        _audit_log("deletePath", req.path, False, error=str(e))
        raise HTTPException(status_code=500, detail=f"Failed to delete: {e}")


@app.post("/fs/movePath")
async def move_path(
    req: MovePathRequest,
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    """Move or rename a file or directory."""
    _auth(x_api_key, authorization)

    src = _safe_write_path(req.source)
    dst = _safe_write_path(req.destination)

    if not src.exists():
        raise HTTPException(
            status_code=404, detail=f"Source path does not exist: {req.source}"
        )

    if dst.exists():
        if not req.overwrite:
            raise HTTPException(
                status_code=409,
                detail=f"Destination already exists: {req.destination}. Set overwrite=true to replace.",
            )
        try:
            if dst.is_dir():
                shutil.rmtree(dst)
            else:
                dst.unlink()
        except Exception as e:
            raise HTTPException(
                status_code=500, detail=f"Failed to remove existing destination: {e}"
            )

    # Ensure parent directory exists
    if not dst.parent.exists():
        try:
            dst.parent.mkdir(parents=True, exist_ok=True)
        except Exception as e:
            raise HTTPException(
                status_code=500, detail=f"Failed to create destination directory: {e}"
            )

    try:
        shutil.move(str(src), str(dst))
        _audit_log(
            "movePath",
            req.source,
            True,
            extra={
                "source": req.source,
                "destination": req.destination,
                "overwrite": req.overwrite,
            },
        )
        return OperationResult(
            ok=True, path=req.destination, message=f"Moved from {req.source}"
        )
    except Exception as e:
        _audit_log("movePath", req.source, False, error=str(e))
        raise HTTPException(status_code=500, detail=f"Failed to move: {e}")


# ---------------------------
# Diff, Patch & Snapshot endpoints
# ---------------------------

import subprocess

SNAPSHOT_DIR = CODE_ROOT / ".rogue-snapshots"
MAX_SNAPSHOTS = 50


def _run_git(args: List[str]) -> str:
    try:
        result = subprocess.run(
            ["git"] + args,
            cwd=str(CODE_ROOT),
            capture_output=True,
            text=True,
            timeout=30,
        )
        return result.stdout
    except Exception:
        return ""


@app.post("/write")
async def write_file_alias(
    req: WriteFileRequest,
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    _auth(x_api_key, authorization)

    file_path = _safe_write_path(req.path)

    if file_path.exists() and not req.overwrite:
        raise HTTPException(
            status_code=409,
            detail=f"File already exists: {req.path}. Set overwrite=true to replace.",
        )

    if req.createDirs and not file_path.parent.exists():
        try:
            file_path.parent.mkdir(parents=True, exist_ok=True)
        except Exception as e:
            raise HTTPException(
                status_code=500,
                detail=f"Failed to create parent directories: {e}",
            )

    try:
        if req.encoding == "base64":
            content = base64.b64decode(req.content)
            with open(file_path, "wb") as f:
                f.write(content)
        else:
            with open(file_path, "w", encoding="utf-8") as f:
                f.write(req.content)

        _audit_log("writeFile", req.path, True, extra={"encoding": req.encoding})
        return OperationResult(ok=True, path=req.path, message="File written")
    except Exception as e:
        _audit_log("writeFile", req.path, False, error=str(e))
        raise HTTPException(status_code=500, detail=f"Failed to write file: {e}")


@app.get("/diff")
async def get_diff(
    path: str = Query("", description="File path (empty = all files)"),
    base: str = Query("HEAD", description="Base commit/branch"),
    include_untracked: bool = Query(True, description="Include untracked files"),
    max_diffs: int = Query(50, ge=1, le=200, description="Max diffs to return"),
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    _auth(x_api_key, authorization)

    diffs = []
    truncated = False

    args = ["diff", "HEAD", "-U3", "--no-color"]
    if path:
        args.extend(["--", path])

    diff_output = _run_git(args)

    if diff_output.strip():
        current_diff = None
        current_hunks = []

        for line in diff_output.split("\n"):
            if line.startswith("diff --git "):
                if current_diff:
                    if len(diffs) >= max_diffs:
                        truncated = True
                        break
                    diffs.append(
                        {
                            "path": current_diff,
                            "status": "modified",
                            "hunks": current_hunks[:5],
                            "raw": diff_output[:5000],
                        }
                    )
                parts = line.split(" ")
                if len(parts) >= 4:
                    current_diff = parts[3].lstrip("b/")
                    current_hunks = []
            elif line.startswith("@@") and current_diff:
                current_hunks.append(line)

        if current_diff and not truncated:
            if len(diffs) >= max_diffs:
                truncated = True
            else:
                diffs.append(
                    {
                        "path": current_diff,
                        "status": "modified",
                        "hunks": current_hunks[:5],
                        "raw": diff_output[:5000],
                    }
                )

    if include_untracked and not truncated and not path:
        untracked_args = ["ls-files", "--others", "--exclude-standard"]
        untracked_output = _run_git(untracked_args)

        for untracked_file in untracked_output.strip().split("\n"):
            if untracked_file:
                if len(diffs) >= max_diffs:
                    truncated = True
                    break
                try:
                    file_path = _safe_path(untracked_file)
                    if file_path.exists() and file_path.is_file():
                        size = file_path.stat().st_size
                        diffs.append(
                            {
                                "path": untracked_file,
                                "status": "untracked",
                                "size": size,
                                "hunks": [],
                                "raw": f"Untracked file: {untracked_file} ({size} bytes)",
                            }
                        )
                except Exception:
                    pass

    if not diffs:
        return {"diffs": [], "message": "No uncommitted changes"}

    return {"diffs": diffs, "truncated": truncated}


class PatchRequest(BaseModel):
    patch_text: str
    path: str = ""
    dry_run: bool = False


@app.post("/patch")
async def apply_patch(
    req: PatchRequest,
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    _auth(x_api_key, authorization)

    import tempfile

    with tempfile.NamedTemporaryFile(mode="w", suffix=".patch", delete=False) as f:
        f.write(req.patch_text)
        patch_file = f.name

    try:
        args = ["apply", "--stat"]
        if req.dry_run:
            args.append("--check")
        args.append(patch_file)

        result = subprocess.run(
            ["git"] + args,
            cwd=str(CODE_ROOT),
            capture_output=True,
            text=True,
            timeout=30,
        )

        if result.returncode != 0:
            return {
                "success": False,
                "path": req.path,
                "hunks_applied": 0,
                "hunks_total": 0,
                "error": result.stderr or "Patch failed",
            }

        return {
            "success": True,
            "path": req.path,
            "hunks_applied": 1,
            "hunks_total": 1,
        }
    finally:
        os.unlink(patch_file)


class CreateSnapshotRequest(BaseModel):
    paths: List[str]
    description: str = ""


@app.post("/snapshot")
async def create_snapshot(
    req: CreateSnapshotRequest,
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    _auth(x_api_key, authorization)

    SNAPSHOT_DIR.mkdir(parents=True, exist_ok=True)

    files = {}
    for path in req.paths:
        try:
            file_path = _safe_path(path)
            if file_path.exists() and file_path.is_file():
                with open(file_path, "rb") as f:
                    content_hash = hashlib.sha256(f.read()).hexdigest()[:16]
                files[path] = content_hash
        except Exception:
            pass

    snap_id = hashlib.sha256(
        (
            datetime.now(timezone.utc).isoformat() + json.dumps(files, sort_keys=True)
        ).encode()
    ).hexdigest()[:12]

    timestamp = datetime.now(timezone.utc).isoformat()

    snap_path = SNAPSHOT_DIR / snap_id
    snap_path.mkdir(parents=True, exist_ok=True)

    for path in files.keys():
        try:
            src = _safe_path(path)
            dst = snap_path / path.replace("/", "_").replace("\\", "_")
            if src.exists():
                shutil.copy2(src, dst)
        except Exception:
            pass

    meta = {
        "id": snap_id,
        "timestamp": timestamp,
        "description": req.description,
        "files": files,
    }

    with open(snap_path / "snapshot.json", "w") as f:
        json.dump(meta, f, indent=2)

    return {
        "id": snap_id,
        "timestamp": timestamp,
        "files": files,
        "description": req.description,
    }


@app.get("/snapshots")
async def list_snapshots(
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    _auth(x_api_key, authorization)

    if not SNAPSHOT_DIR.exists():
        return {"snapshots": []}

    snapshots = []
    for item in SNAPSHOT_DIR.iterdir():
        if item.is_dir():
            meta_path = item / "snapshot.json"
            if meta_path.exists():
                try:
                    with open(meta_path, "r") as f:
                        snapshots.append(json.load(f))
                except Exception:
                    pass

    snapshots.sort(key=lambda x: x.get("timestamp", ""), reverse=True)
    return {"snapshots": snapshots[:MAX_SNAPSHOTS]}


@app.get("/snapshot/{snap_id}")
async def restore_snapshot(
    snap_id: str,
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    _auth(x_api_key, authorization)

    snap_path = SNAPSHOT_DIR / snap_id
    if not snap_path.exists():
        raise HTTPException(status_code=404, detail=f"Snapshot not found: {snap_id}")

    meta_path = snap_path / "snapshot.json"
    if not meta_path.exists():
        raise HTTPException(
            status_code=404, detail=f"Snapshot metadata missing: {snap_id}"
        )

    with open(meta_path, "r") as f:
        meta = json.load(f)

    restored = []
    for path in meta.get("files", {}).keys():
        try:
            src = snap_path / path.replace("/", "_").replace("\\", "_")
            dst = _safe_write_path(path)
            if src.exists():
                dst.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(src, dst)
                restored.append(path)
        except Exception:
            pass

    return {"ok": True, "path": snap_id, "message": f"Restored {len(restored)} files"}


@app.delete("/snapshot/{snap_id}")
async def delete_snapshot(
    snap_id: str,
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    _auth(x_api_key, authorization)

    snap_path = SNAPSHOT_DIR / snap_id
    if not snap_path.exists():
        raise HTTPException(status_code=404, detail=f"Snapshot not found: {snap_id}")

    try:
        shutil.rmtree(snap_path)
        return {"ok": True, "path": snap_id, "message": "Snapshot deleted"}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to delete snapshot: {e}")


# ---------------------------
# Ollama proxy helpers + routes
# ---------------------------


async def _ollama_proxy(
    method: str, path: str, body: Optional[Dict[str, Any]] = None
) -> Any:
    url = f"{OLLAMA_BASE}{path}"

    try:
        async with httpx.AsyncClient(timeout=120.0) as client:
            r = await client.request(method, url, json=body)
    except httpx.RequestError as e:
        raise HTTPException(status_code=502, detail=f"Ollama unreachable: {e}")

    if r.status_code >= 400:
        raise HTTPException(
            status_code=502, detail=f"Ollama error {r.status_code}: {r.text}"
        )

    try:
        return r.json()
    except Exception:
        raise HTTPException(status_code=502, detail="Ollama returned non-JSON response")


@app.get("/ollama/tags")
async def ollama_tags(
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    _auth(x_api_key, authorization)
    return await _ollama_proxy("GET", "/api/tags")


@app.post("/ollama/generate")
async def ollama_generate(
    payload: Dict[str, Any],
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    _auth(x_api_key, authorization)
    payload["stream"] = False
    return await _ollama_proxy("POST", "/api/generate", payload)


@app.post("/ollama/chat")
async def ollama_chat(
    payload: Dict[str, Any],
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    _auth(x_api_key, authorization)
    payload["stream"] = False
    return await _ollama_proxy("POST", "/api/chat", payload)


# ---------------------------
# Project Context endpoints
# ---------------------------

PROJECT_CONTEXT = {
    "project_root": str(CODE_ROOT),
    "rogue_protocol": {
        "FVA": {
            "name": "FastVectorArray",
            "description": "Stable handles for UI/Editor references",
            "use_cases": ["Road Segments", "Districts", "UI Selection"],
            "stability": "Stable indices across frame boundaries",
        },
        "SIV": {
            "name": "StableIndexVector",
            "description": "Safety for high-churn entities with validity tracking",
            "use_cases": ["Buildings", "Agents", "Props"],
            "stability": "Stable with validity checks - safe for async operations",
        },
        "CIV": {
            "name": "IndexVector",
            "description": "Raw speed for internal calculations",
            "use_cases": [
                "Internal algorithms",
                "Temporary calculations",
                "One-frame operations",
            ],
            "stability": "Unstable - indices may change between frames",
        },
        "RogueWorker": {
            "threshold_ms": 10,
            "description": "Threading for operations >10ms",
        },
    },
    "aesp": {
        "Access": {"description": "Ease of entry", "high_for": "Retail"},
        "Exposure": {"description": "Visibility", "high_for": "Commercial/Civic"},
        "Service": {"description": "Utility capacity", "high_for": "Industrial"},
        "Privacy": {"description": "Seclusion", "high_for": "Residential"},
    },
    "layers": {
        "Core": {
            "name": "Core",
            "owns": ["Pure C++20, Math, Types"],
            "forbidden": ["OpenGL", "ImGui"],
        },
        "Generators": {
            "name": "Generators",
            "owns": ["Algorithms (RK4, WFC, AESP)"],
            "forbidden": ["ImGui", "OpenGL"],
        },
        "Visualizer": {
            "name": "Visualizer",
            "owns": ["ImGui", "OpenGL", "User Input"],
            "forbidden": [],
        },
    },
    "rules": [
        "No ImGui/OpenGL references in Core layer",
        "Use FVA for Road Segments, Districts (stable UI handles)",
        "Use SIV for Buildings, Agents, Props (high-churn, safety)",
        "Use CIV for internal calculations only (no handles)",
        "Operations >10ms must use RogueWorker threading",
    ],
}


@app.get("/context")
async def get_project_context(
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    _auth(x_api_key, authorization)

    result = dict(PROJECT_CONTEXT)

    agents_path = CODE_ROOT / ".github" / "Agents.md"
    if agents_path.exists():
        try:
            with open(agents_path, "r", encoding="utf-8") as f:
                result["agents_md"] = f.read()[:5000]
        except Exception:
            pass

    return result


@app.get("/context/suggest-vector")
async def suggest_vector_type(
    entityType: str = Query(..., description="Type of entity"),
    useCase: str = Query("", description="How it will be used"),
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    _auth(x_api_key, authorization)

    entity_lower = entityType.lower()
    use_case_lower = useCase.lower()

    if any(kw in entity_lower for kw in ["road", "segment", "district", "zone"]):
        return {
            "recommended": "FVA",
            "reason": "Road Segments and Districts need stable handles for UI/Editor",
            "alternative": "SIV if destruction is possible",
            "use_cases": ["Road Segments", "Districts", "Zones"],
        }

    if any(
        kw in entity_lower for kw in ["building", "agent", "prop", "vehicle", "npc"]
    ):
        return {
            "recommended": "SIV",
            "reason": "High-churn entities need validity tracking",
            "alternative": "FVA if never destroyed",
            "use_cases": ["Buildings", "Agents", "Props", "Vehicles"],
        }

    if any(
        kw in use_case_lower
        for kw in ["internal", "calculation", "temporary", "algorithm"]
    ):
        return {
            "recommended": "CIV",
            "reason": "Internal calculations don't need stable handles",
            "alternative": "SIV if used across frames",
            "use_cases": ["Pathfinding nodes", "Mesh scratch buffers", "Temp geometry"],
        }

    if any(kw in use_case_lower for kw in ["ui", "editor", "selection", "display"]):
        return {
            "recommended": "FVA",
            "reason": "UI/Editor references need stable indices",
            "alternative": "SIV for safety-critical selections",
            "use_cases": ["UI Selection", "Editor handles", "Inspector references"],
        }

    return {
        "recommended": "SIV",
        "reason": "Default choice - provides safety with reasonable performance",
        "alternative": "FVA for stable entities, CIV for pure internal use",
        "use_cases": ["General purpose", "Mixed churn entities"],
    }


class ValidateRequest(BaseModel):
    path: str
    content: Optional[str] = None


class ValidationViolation(BaseModel):
    rule_id: str
    severity: str
    line: int
    message: str
    context: Optional[str] = None


@app.post("/validate")
async def validate_file(
    req: ValidateRequest,
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    _auth(x_api_key, authorization)

    violations = []

    try:
        file_path = _safe_path(req.path)
    except HTTPException:
        return {"path": req.path, "layer": None, "violations": []}

    content = req.content
    if content is None:
        if file_path.exists():
            try:
                with open(file_path, "r", encoding="utf-8") as f:
                    content = f.read()
            except Exception:
                return {"path": req.path, "layer": None, "violations": []}
        else:
            return {"path": req.path, "layer": None, "violations": []}

    parts = [p.lower() for p in file_path.parts]
    layer = None
    if "core" in parts:
        layer = "Core"
    elif "generators" in parts:
        layer = "Generators"
    elif "visualizer" in parts:
        layer = "Visualizer"

    forbidden_patterns = []
    if layer == "Core":
        forbidden_patterns = [
            (r"ImGui\.", "ImGui references not allowed in Core layer"),
            (r"GL\.", "OpenGL references not allowed in Core layer"),
            (r"OpenGL", "OpenGL references not allowed in Core layer"),
        ]
    elif layer == "Generators":
        forbidden_patterns = [
            (r"ImGui\.", "ImGui references not allowed in Generators layer"),
        ]

    for pattern, message in forbidden_patterns:
        for match in re.finditer(pattern, content, re.IGNORECASE):
            line_num = content[: match.start()].count("\n") + 1
            violations.append(
                {
                    "rule_id": f"layer_{layer.lower() if layer else 'unknown'}_forbidden",
                    "severity": "error",
                    "line": line_num,
                    "message": message,
                    "context": content.split("\n")[line_num - 1].strip()[:100]
                    if line_num <= len(content.split("\n"))
                    else "",
                }
            )

    civ_ui_pattern = r"CIV.*UI|UI.*CIV|IndexVector.*Editor|Editor.*IndexVector"
    for match in re.finditer(civ_ui_pattern, content, re.IGNORECASE):
        line_num = content[: match.start()].count("\n") + 1
        violations.append(
            {
                "rule_id": "civ_ui_reference",
                "severity": "warning",
                "line": line_num,
                "message": "CIV/IndexVector should not be used for UI/Editor references - use FVA or SIV",
                "context": content.split("\n")[line_num - 1].strip()[:100]
                if line_num <= len(content.split("\n"))
                else "",
            }
        )

    return {
        "path": req.path,
        "layer": layer,
        "violations": violations[:50],
    }


# ---------------------------
# GPT Actions aliases (operationId-style paths)
# ---------------------------


@app.get("/listFiles")
async def alias_list_files(
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
    path: str = "",
    recursive: bool = False,
    includeHidden: bool = False,
    maxEntries: int = Query(default=5000, ge=1, le=20000),
):
    return await list_files(
        x_api_key, authorization, path, recursive, includeHidden, maxEntries
    )


@app.get("/readFile")
async def alias_read_file(
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
    path: str = Query(...),
    maxBytes: int = Query(default=262144, ge=1, le=5242880),
):
    return await read_file(x_api_key, authorization, path, maxBytes)


@app.post("/writeFile")
async def alias_write_file(
    req: WriteFileRequest,
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    return await write_file_alias(req, x_api_key, authorization)


@app.post("/searchCode")
async def alias_search_code(
    req: SearchRequest,
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    return await search_code(req, x_api_key, authorization)


@app.get("/getDiff")
async def alias_get_diff(
    path: str = Query("", description="File path (empty = all files)"),
    base: str = Query("HEAD", description="Base commit/branch"),
    include_untracked: bool = Query(True, description="Include untracked files"),
    max_diffs: int = Query(50, ge=1, le=200, description="Max diffs to return"),
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    return await get_diff(
        path, base, include_untracked, max_diffs, x_api_key, authorization
    )


@app.post("/applyPatch")
async def alias_apply_patch(
    req: PatchRequest,
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    return await apply_patch(req, x_api_key, authorization)


@app.post("/createSnapshot")
async def alias_create_snapshot(
    req: CreateSnapshotRequest,
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    return await create_snapshot(req, x_api_key, authorization)


@app.get("/restoreSnapshot/{snap_id}")
async def alias_restore_snapshot(
    snap_id: str,
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    return await restore_snapshot(snap_id, x_api_key, authorization)


@app.delete("/deleteSnapshot/{snap_id}")
async def alias_delete_snapshot(
    snap_id: str,
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    return await delete_snapshot(snap_id, x_api_key, authorization)
