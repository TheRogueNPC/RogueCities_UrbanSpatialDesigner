# File: roguefs-agent.py
# RogueFS Agent - Secure, paginated filesystem access
#
# Provides bounded directory listing, ranged file reads, and fast ripgrep search
# All paths are validated against allowlist and denylist

import os
import re
import json
import base64
import subprocess
import shutil
from datetime import datetime, timezone
from pathlib import Path
from typing import Optional, Dict, Any, List, Tuple

from fastapi import FastAPI, Header, HTTPException, Query
from pydantic import BaseModel, Field

APP_NAME = "RogueFS Agent"
APP_VERSION = "1.0.0"

# ---------------------------
# Configuration
# ---------------------------

ROGUEFS_PORT = int(os.getenv("ROGUEFS_PORT", "7078"))

# Parse allowlist
_allowlist_str = os.getenv("ROGUEFS_ALLOWLIST", "")
if _allowlist_str:
    ROGUEFS_ALLOWLIST = []
    for p in _allowlist_str.split(";"):
        p = p.strip()
        if p:
            try:
                resolved = Path(p).resolve()
                if resolved.exists() and resolved.is_dir():
                    ROGUEFS_ALLOWLIST.append(resolved)
            except Exception:
                pass
else:
    ROGUEFS_ALLOWLIST = [Path(os.getcwd()).resolve()]

# Parse denylist
ROGUEFS_DENYLIST = [
    p.strip() for p in os.getenv("ROGUEFS_DENYLIST", "").split(";") if p.strip()
]

API_KEY = os.getenv(
    "API_KEY",
    "Rogue_Ollama_Says_Hi_R46U8FJ48fjhKAUhfSSFEs64fs6f4SFF4SDSDFWETHARGSADFGEdfwfge4gasdgadsfgasdf4a7744354s9777645",
)

MAX_LIST_PAGE = int(os.getenv("ROGUEFS_MAX_LIST_PAGE", "1000"))
MAX_READ_CHUNK = int(os.getenv("ROGUEFS_MAX_READ_CHUNK", "524288"))
SEARCH_TIMEOUT_MS = int(os.getenv("ROGUEFS_SEARCH_TIMEOUT_MS", "10000"))

# Standard denylist (always enforced)
DEFAULT_DENYLIST = [
    r"\.git[/\\]",
    r"\.vs[/\\]",
    r"\.idea[/\\]",
    r"\.vscode[/\\]",
    r"node_modules[/\\]",
    r"__pycache__[/\\]",
    r"[/\\]build[/\\]",
    r"[/\\]bin[/\\]",
    r"[/\\]obj[/\\]",
    r"\.pfx$",
    r"\.key$",
    r"\.pem$",
    r"\.p12$",
    r"\.env$",
    r"\.env\.",
    r"credentials",
    r"\.secret",
    r"secrets?",
    r"[/\\]AppData[/\\]",
    r"[/\\]Users[/\\]",
    r"[/\\]Windows[/\\]",
    r"[/\\]Program Files[/\\]",
    r"\.log$",
    r"\.tmp$",
    r"\.temp$",
    r"\.bak$",
]

app = FastAPI(title=APP_NAME, version=APP_VERSION)


# ---------------------------
# Schemas
# ---------------------------


class ErrorResponse(BaseModel):
    error: str
    detail: Optional[str] = None


class RootInfo(BaseModel):
    path: str
    name: str
    writable: bool


class RootsResponse(BaseModel):
    roots: List[RootInfo]
    version: str


class EntryInfo(BaseModel):
    name: str
    type: str = Field(pattern="^(file|dir)$")
    size: Optional[int] = None
    modified_time: Optional[str] = None


class ListResponse(BaseModel):
    path: str
    entries: List[EntryInfo]
    cursor: Optional[str] = None
    has_more: bool


class ReadResponse(BaseModel):
    path: str
    encoding: str = Field(pattern="^(text|base64)$")
    content: str
    offset: int
    length: int
    total_size: int
    eof: bool


class StatResponse(BaseModel):
    path: str
    exists: bool
    type: Optional[str] = None
    size: Optional[int] = None
    modified_time: Optional[str] = None
    created_time: Optional[str] = None
    is_readable: Optional[bool] = None
    is_writable: Optional[bool] = None


class SearchMatch(BaseModel):
    path: str
    line: int
    column: Optional[int] = None
    preview: str
    context: List[str] = []


class SearchResponse(BaseModel):
    query: str
    root: str
    matches: List[SearchMatch]
    truncated: bool


# ---------------------------
# Auth helpers
# ---------------------------


def _bearer_token(authorization: Optional[str]) -> Optional[str]:
    if not authorization:
        return None
    parts = authorization.strip().split(" ", 1)
    if len(parts) == 2 and parts[0].lower() == "bearer":
        return parts[1].strip()
    return None


def _auth(x_api_key: Optional[str], authorization: Optional[str] = None):
    candidate = x_api_key or _bearer_token(authorization)
    if not candidate or candidate != API_KEY:
        raise HTTPException(status_code=401, detail="Unauthorized")


# ---------------------------
# Path helpers
# ---------------------------


def _normalize_path(path: str) -> str:
    return path.replace("\\", "/").strip("/")


def _is_denied(path: str) -> bool:
    normalized = _normalize_path(path)
    full_path = "/" + normalized + "/"

    all_patterns = DEFAULT_DENYLIST + ROGUEFS_DENYLIST

    for pattern in all_patterns:
        try:
            if re.search(pattern, full_path, re.IGNORECASE):
                return True
            if re.search(pattern, normalized, re.IGNORECASE):
                return True
        except re.error:
            if pattern.lower() in normalized.lower():
                return True
    return False


def _find_root(rel_path: str) -> Tuple[Path, str]:
    rel_path = _normalize_path(rel_path)

    for root in ROGUEFS_ALLOWLIST:
        if not rel_path:
            return root, ""
        try:
            candidate = (root / rel_path).resolve()
            candidate.relative_to(root)
            return root, rel_path
        except ValueError:
            continue

    if ROGUEFS_ALLOWLIST:
        return ROGUEFS_ALLOWLIST[0], rel_path

    raise HTTPException(status_code=403, detail="No allowlist configured")


def _safe_resolve(root: Path, rel_path: str) -> Path:
    rel_path = _normalize_path(rel_path)
    if not rel_path:
        return root

    resolved = (root / rel_path).resolve()

    try:
        resolved.relative_to(root)
    except ValueError:
        raise HTTPException(status_code=400, detail="Path escapes root")

    if resolved.exists() or resolved.is_symlink():
        real_path = Path(os.path.realpath(resolved))
        try:
            real_path.relative_to(root)
        except ValueError:
            raise HTTPException(status_code=403, detail="Symlink escapes root")

    if _is_denied(rel_path):
        raise HTTPException(status_code=403, detail="Path matches denylist")

    return resolved


def _encode_cursor(offset: int) -> str:
    data = json.dumps({"o": offset}).encode("utf-8")
    return base64.urlsafe_b64encode(data).decode("ascii").rstrip("=")


def _decode_cursor(cursor: str) -> int:
    if not cursor:
        return 0
    try:
        padding = 4 - (len(cursor) % 4)
        if padding != 4:
            cursor += "=" * padding
        data = base64.urlsafe_b64decode(cursor.encode("ascii"))
        obj = json.loads(data.decode("utf-8"))
        return obj.get("o", 0)
    except Exception:
        return 0


# ---------------------------
# Request logging
# ---------------------------


def _log_request(operation: str, path: str, extra: Optional[Dict] = None):
    entry = {
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "operation": operation,
        "path": path,
    }
    if extra:
        entry.update(extra)
    print(json.dumps(entry), flush=True)


# ---------------------------
# Routes
# ---------------------------


@app.get("/")
async def root():
    return {
        "name": APP_NAME,
        "version": APP_VERSION,
        "endpoints": [
            "/v1/health",
            "/v1/roots",
            "/v1/list",
            "/v1/read",
            "/v1/stat",
            "/v1/search",
        ],
    }


@app.get("/v1/health")
async def health():
    return {
        "status": "ok",
        "version": APP_VERSION,
        "allowlistCount": len(ROGUEFS_ALLOWLIST),
        "denylistCount": len(DEFAULT_DENYLIST) + len(ROGUEFS_DENYLIST),
    }


@app.get("/v1/roots")
async def list_roots(
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
):
    _auth(x_api_key, authorization)
    _log_request("list_roots", "")

    roots = []
    for r in ROGUEFS_ALLOWLIST:
        roots.append(
            RootInfo(
                path=str(r).replace("\\", "/"),
                name=r.name,
                writable=os.access(r, os.W_OK) if r.exists() else False,
            )
        )

    return RootsResponse(roots=roots, version=APP_VERSION)


@app.get("/v1/list")
async def fs_list(
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
    path: str = Query(default=""),
    limit: int = Query(default=200, ge=1, le=1000),
    cursor: Optional[str] = Query(default=None),
    includeHidden: bool = Query(default=False),
):
    _auth(x_api_key, authorization)
    _log_request("fs_list", path, {"limit": limit, "cursor": cursor})

    fs_root, rel_path = _find_root(path)
    target = _safe_resolve(fs_root, rel_path)

    if not target.exists():
        raise HTTPException(status_code=404, detail="Path not found")
    if not target.is_dir():
        raise HTTPException(status_code=400, detail="Not a directory")

    offset = _decode_cursor(cursor) if cursor else 0

    entries = []
    count = 0
    has_more = False

    try:
        all_entries = sorted(target.iterdir(), key=lambda x: x.name.lower())
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to list directory: {e}")

    for entry in all_entries:
        if count >= limit:
            has_more = True
            break

        entry_name = entry.name
        rel_entry_path = str(entry.relative_to(fs_root)).replace("\\", "/")

        if _is_denied(rel_entry_path):
            continue

        if not includeHidden and entry_name.startswith("."):
            continue

        try:
            stat = entry.stat()
            if entry.is_dir():
                entries.append(
                    EntryInfo(
                        name=entry_name,
                        type="dir",
                    )
                )
            elif entry.is_file():
                entries.append(
                    EntryInfo(
                        name=entry_name,
                        type="file",
                        size=stat.st_size,
                        modified_time=datetime.fromtimestamp(stat.st_mtime).isoformat(),
                    )
                )
        except Exception:
            continue

        count += 1

    next_cursor = _encode_cursor(offset + count) if has_more else None

    return ListResponse(
        path=path,
        entries=entries,
        cursor=next_cursor,
        has_more=has_more,
    )


@app.get("/v1/read")
async def fs_read(
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
    path: str = Query(...),
    offset: int = Query(default=0, ge=0),
    length: int = Query(default=65536, ge=1),
):
    _auth(x_api_key, authorization)
    _log_request("fs_read", path, {"offset": offset, "length": length})

    effective_length = min(length, MAX_READ_CHUNK)

    fs_root, rel_path = _find_root(path)
    target = _safe_resolve(fs_root, rel_path)

    if not target.exists():
        raise HTTPException(status_code=404, detail="File not found")
    if not target.is_file():
        raise HTTPException(status_code=400, detail="Not a file")

    try:
        file_size = target.stat().st_size
    except Exception:
        raise HTTPException(status_code=500, detail="Failed to stat file")

    if offset >= file_size:
        return ReadResponse(
            path=path,
            encoding="text",
            content="",
            offset=offset,
            length=0,
            total_size=file_size,
            eof=True,
        )

    read_length = min(effective_length, file_size - offset)
    eof = (offset + read_length) >= file_size

    try:
        with open(target, "rb") as f:
            f.seek(offset)
            data = f.read(read_length)
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to read file: {e}")

    try:
        content = data.decode("utf-8")
        encoding = "text"
    except UnicodeDecodeError:
        content = base64.b64encode(data).decode("ascii")
        encoding = "base64"

    return ReadResponse(
        path=path,
        encoding=encoding,
        content=content,
        offset=offset,
        length=len(data),
        total_size=file_size,
        eof=eof,
    )


@app.get("/v1/stat")
async def fs_stat(
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
    path: str = Query(...),
):
    _auth(x_api_key, authorization)
    _log_request("fs_stat", path)

    fs_root, rel_path = _find_root(path)
    target = _safe_resolve(fs_root, rel_path)

    if not target.exists():
        return StatResponse(path=path, exists=False)

    try:
        stat = target.stat()
        is_file = target.is_file()
        is_dir = target.is_dir()

        return StatResponse(
            path=path,
            exists=True,
            type="file" if is_file else "dir" if is_dir else "other",
            size=stat.st_size if is_file else None,
            modified_time=datetime.fromtimestamp(stat.st_mtime).isoformat(),
            created_time=datetime.fromtimestamp(stat.st_ctime).isoformat(),
            is_readable=os.access(target, os.R_OK),
            is_writable=os.access(target, os.W_OK),
        )
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to stat: {e}")


@app.get("/v1/search")
async def fs_search(
    x_api_key: Optional[str] = Header(default=None, alias="X-API-Key"),
    authorization: Optional[str] = Header(default=None, alias="Authorization"),
    q: str = Query(..., min_length=1),
    root: str = Query(default=""),
    glob: str = Query(default="*.py,*.js,*.ts,*.json,*.yaml,*.md,*.txt"),
    limit: int = Query(default=100, ge=1, le=500),
):
    _auth(x_api_key, authorization)
    _log_request("fs_search", root, {"query": q, "limit": limit})

    if not shutil.which("rg"):
        raise HTTPException(
            status_code=500, detail="ripgrep (rg) not found. Please install ripgrep."
        )

    fs_root, rel_path = _find_root(root)
    search_path = _safe_resolve(fs_root, rel_path)

    if not search_path.exists():
        raise HTTPException(status_code=404, detail="Search path not found")
    if not search_path.is_dir():
        raise HTTPException(status_code=400, detail="Search path is not a directory")

    glob_patterns = [g.strip() for g in glob.split(",") if g.strip()] if glob else ["*"]

    matches = []
    truncated = False

    for glob_pattern in glob_patterns:
        if truncated:
            break

        try:
            remaining = limit - len(matches)
            if remaining <= 0:
                truncated = True
                break

            cmd = [
                "rg",
                "--json",
                "--no-heading",
                "--max-count",
                str(remaining),
                "--glob",
                glob_pattern,
                "--",
                q,
                str(search_path),
            ]
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=SEARCH_TIMEOUT_MS / 1000,
            )

            for line in result.stdout.strip().split("\n"):
                if not line or len(matches) >= limit:
                    truncated = len(matches) >= limit
                    break

                try:
                    obj = json.loads(line)
                    if obj.get("type") == "match":
                        data = obj.get("data", {})
                        file_path = data.get("path", {}).get("text", "")
                        line_number = data.get("line_number", 1)
                        lines_data = data.get("lines", {})
                        line_text = lines_data.get("text", "") if lines_data else ""

                        if file_path:
                            try:
                                rel_file_path = str(
                                    Path(file_path).relative_to(fs_root)
                                ).replace("\\", "/")
                            except ValueError:
                                rel_file_path = file_path.replace("\\", "/")

                            if _is_denied(rel_file_path):
                                continue

                            preview = line_text.split("\n")[0] if line_text else ""
                            context_lines = (
                                line_text.split("\n")[1:4] if line_text else []
                            )

                            matches.append(
                                SearchMatch(
                                    path=rel_file_path,
                                    line=line_number,
                                    column=None,
                                    preview=preview[:200] if preview else "",
                                    context=context_lines[:4],
                                )
                            )

                            if len(matches) >= limit:
                                truncated = True
                                break

                except json.JSONDecodeError:
                    continue

        except subprocess.TimeoutExpired:
            truncated = True
        except Exception as e:
            raise HTTPException(status_code=500, detail=f"Search failed: {e}")

    matches.sort(key=lambda x: (x.path, x.line))
    if len(matches) > limit:
        matches = matches[:limit]
        truncated = True

    return SearchResponse(
        query=q,
        root=root,
        matches=matches,
        truncated=truncated,
    )


if __name__ == "__main__":
    import uvicorn

    uvicorn.run(app, host="127.0.0.1", port=ROGUEFS_PORT)
