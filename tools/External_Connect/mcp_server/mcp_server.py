# File: mcp_server.py
# MCP Server for Claude Desktop & VS Code integration
#
# Implements Model Context Protocol (MCP) for AI assistants
# Exposes filesystem, diff/patch, and project context tools
#
# Usage:
#   python mcp_server.py --transport stdio
#   python mcp_server.py --transport sse --port 7079
#
# VS Code: Add to .vscode/mcp.json or settings.json
# Claude Desktop: Add to claude_desktop_config.json

import os
import sys
import json
import asyncio
import argparse
from pathlib import Path
from typing import Optional, Dict, Any, List

sys.path.insert(0, str(Path(__file__).parent.parent))

try:
    from mcp.server import Server
    from mcp.server.stdio import stdio_server
    from mcp.types import Tool, TextContent, Resource, ResourceTemplate
    from mcp.server.sse import SseServerTransport

    MCP_AVAILABLE = True
except ImportError:
    MCP_AVAILABLE = False
    print("MCP SDK not installed. Run: pip install mcp", file=sys.stderr)

from diff_patch.diff_patch import (
    generate_diff,
    generate_diff_uncommitted,
    apply_patch,
    create_snapshot,
    restore_snapshot,
    list_snapshots,
    delete_snapshot,
    parse_unified_diff,
)

APP_NAME = "rogue-mcp"
APP_VERSION = "1.0.0"

CODE_ROOT = Path(
    os.getenv("CODE_ROOT", Path(__file__).parent.parent.parent.parent)
).resolve()


def _resolve_path(path: str) -> Path:
    if not path:
        return CODE_ROOT
    p = Path(path)
    if not p.is_absolute():
        p = CODE_ROOT / path
    return p.resolve()


def _is_allowed(path: Path) -> bool:
    try:
        path.resolve().relative_to(CODE_ROOT.resolve())
        return True
    except ValueError:
        return False


def _read_file(path: str, offset: int = 0, limit: int = 2000) -> Dict[str, Any]:
    file_path = _resolve_path(path)
    if not _is_allowed(file_path):
        return {"error": "Path outside allowed root"}
    if not file_path.exists():
        return {"error": "File not found"}
    if not file_path.is_file():
        return {"error": "Not a file"}

    try:
        with open(file_path, "r", encoding="utf-8") as f:
            lines = f.readlines()

        total = len(lines)
        start = max(0, offset)
        end = min(total, start + limit)

        content_lines = []
        for i in range(start, end):
            content_lines.append(f"{i + 1}: {lines[i].rstrip()}")

        return {
            "path": str(file_path.relative_to(CODE_ROOT)),
            "content": "\n".join(content_lines),
            "total_lines": total,
            "offset": start,
            "limit": limit,
            "eof": end >= total,
        }
    except UnicodeDecodeError:
        return {"error": "Binary file - cannot read as text"}
    except Exception as e:
        return {"error": str(e)}


def _write_file(path: str, content: str) -> Dict[str, Any]:
    file_path = _resolve_path(path)
    if not _is_allowed(file_path):
        return {"error": "Path outside allowed root"}

    try:
        file_path.parent.mkdir(parents=True, exist_ok=True)
        with open(file_path, "w", encoding="utf-8") as f:
            f.write(content)
        return {"success": True, "path": str(file_path.relative_to(CODE_ROOT))}
    except Exception as e:
        return {"error": str(e)}


def _list_dir(path: str) -> Dict[str, Any]:
    dir_path = _resolve_path(path)
    if not _is_allowed(dir_path):
        return {"error": "Path outside allowed root"}
    if not dir_path.exists():
        return {"error": "Directory not found"}
    if not dir_path.is_dir():
        return {"error": "Not a directory"}

    entries = []
    try:
        for item in sorted(
            dir_path.iterdir(), key=lambda x: (not x.is_dir(), x.name.lower())
        ):
            if item.name.startswith(".git"):
                continue
            entries.append(
                {
                    "name": item.name,
                    "type": "dir" if item.is_dir() else "file",
                    "size": item.stat().st_size if item.is_file() else None,
                }
            )
    except Exception as e:
        return {"error": str(e)}

    return {"path": str(dir_path.relative_to(CODE_ROOT)), "entries": entries}


def _search_files(
    pattern: str,
    path: str = "",
    file_pattern: str = "*.py,*.cs,*.js,*.ts,*.json,*.yaml,*.md",
) -> Dict[str, Any]:
    import subprocess
    import shutil

    search_path = _resolve_path(path)
    if not _is_allowed(search_path):
        return {"error": "Path outside allowed root"}

    if not shutil.which("rg"):
        return {"error": "ripgrep (rg) not installed"}

    results = []
    for glob_pattern in file_pattern.split(","):
        glob_pattern = glob_pattern.strip()
        if not glob_pattern:
            continue
        try:
            cmd = [
                "rg",
                "--json",
                "-n",
                "--glob",
                glob_pattern,
                "--",
                pattern,
                str(search_path),
            ]
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)

            for line in result.stdout.strip().split("\n"):
                if not line:
                    continue
                try:
                    obj = json.loads(line)
                    if obj.get("type") == "match":
                        data = obj.get("data", {})
                        file_path = data.get("path", {}).get("text", "")
                        line_num = data.get("line_number", 1)
                        line_text = data.get("lines", {}).get("text", "")

                        if file_path:
                            try:
                                rel_path = str(Path(file_path).relative_to(CODE_ROOT))
                            except ValueError:
                                rel_path = file_path

                            results.append(
                                {
                                    "path": rel_path,
                                    "line": line_num,
                                    "preview": line_text[:200] if line_text else "",
                                }
                            )

                            if len(results) >= 100:
                                return {"matches": results, "truncated": True}
                except json.JSONDecodeError:
                    continue
        except subprocess.TimeoutExpired:
            return {"matches": results, "truncated": True, "error": "Search timed out"}
        except Exception as e:
            return {"error": str(e)}

    return {"matches": results, "truncated": False}


def _get_diff(path: str = "", base: str = "HEAD") -> Dict[str, Any]:
    diffs = generate_diff_uncommitted(CODE_ROOT, path if path else None)

    if not diffs:
        return {"diffs": [], "message": "No uncommitted changes"}

    result = []
    for d in diffs:
        result.append(
            {
                "path": d.path,
                "added": d.added,
                "deleted": d.deleted,
                "binary": d.binary,
                "hunks": len(d.hunks),
                "raw": d.raw[:10000],
            }
        )

    return {"diffs": result}


def _apply_patch(
    patch_text: str, path: str = "", dry_run: bool = False
) -> Dict[str, Any]:
    result = apply_patch(CODE_ROOT, patch_text, path if path else None, dry_run)
    return {
        "success": result.success,
        "path": result.path,
        "hunks_applied": result.hunks_applied,
        "hunks_total": result.hunks_total,
        "conflicts": result.conflicts,
        "error": result.error,
    }


def _create_snapshot(paths: List[str], description: str = "") -> Dict[str, Any]:
    snap = create_snapshot(CODE_ROOT, paths, description)
    return {
        "id": snap.id,
        "timestamp": snap.timestamp,
        "files": snap.files,
        "description": snap.description,
    }


def _restore_snapshot(snapshot_id: str) -> Dict[str, Any]:
    success, message = restore_snapshot(CODE_ROOT, snapshot_id)
    return {"success": success, "message": message}


def _list_snapshots() -> Dict[str, Any]:
    snaps = list_snapshots(CODE_ROOT)
    return {"snapshots": snaps}


def _delete_snapshot(snapshot_id: str) -> Dict[str, Any]:
    deleted = delete_snapshot(CODE_ROOT, snapshot_id)
    return {"success": deleted}


def _get_project_context() -> Dict[str, Any]:
    agents_path = CODE_ROOT / ".github" / "Agents.md"
    context = {
        "project_root": str(CODE_ROOT),
        "rogue_protocol": {
            "FVA": "FastVectorArray - Stable handles for UI/Editor (Road Segments, Districts)",
            "SIV": "StableIndexVector - Safety for high-churn entities (Buildings, Agents, Props)",
            "CIV": "IndexVector - Raw speed for internal calculations only",
            "RogueWorker": "Threading for operations >10ms",
        },
        "layers": {
            "Core": "No ImGui/OpenGL, pure logic",
            "Generators": "Procedural generation, rules engine",
            "Visualizer": "Rendering, ImGui, user interaction",
        },
        "rules": [
            "No ImGui/OpenGL references in Core layer",
            "Use FVA for Road Segments, Districts (stable UI handles)",
            "Use SIV for Buildings, Agents, Props (high-churn, safety)",
            "Use CIV for internal calculations only (no handles)",
            "Operations >10ms must use RogueWorker threading",
        ],
    }

    if agents_path.exists():
        try:
            with open(agents_path, "r", encoding="utf-8") as f:
                context["agents_md"] = f.read()[:5000]
        except Exception:
            pass

    return context


TOOLS = [
    {
        "name": "read_file",
        "description": "Read file contents with line numbers. Supports pagination for large files.",
        "input_schema": {
            "type": "object",
            "properties": {
                "path": {
                    "type": "string",
                    "description": "File path relative to project root",
                },
                "offset": {
                    "type": "integer",
                    "default": 0,
                    "description": "Starting line (0-indexed)",
                },
                "limit": {
                    "type": "integer",
                    "default": 2000,
                    "description": "Max lines to read",
                },
            },
            "required": ["path"],
        },
    },
    {
        "name": "write_file",
        "description": "Write content to a file. Creates parent directories if needed.",
        "input_schema": {
            "type": "object",
            "properties": {
                "path": {
                    "type": "string",
                    "description": "File path relative to project root",
                },
                "content": {"type": "string", "description": "Content to write"},
            },
            "required": ["path", "content"],
        },
    },
    {
        "name": "list_dir",
        "description": "List directory contents with type and size info.",
        "input_schema": {
            "type": "object",
            "properties": {
                "path": {
                    "type": "string",
                    "default": "",
                    "description": "Directory path relative to project root",
                },
            },
        },
    },
    {
        "name": "search_files",
        "description": "Search for pattern in files using ripgrep. Supports regex.",
        "input_schema": {
            "type": "object",
            "properties": {
                "pattern": {
                    "type": "string",
                    "description": "Search pattern (regex supported)",
                },
                "path": {
                    "type": "string",
                    "default": "",
                    "description": "Search root directory",
                },
                "file_pattern": {
                    "type": "string",
                    "default": "*.py,*.cs,*.js,*.ts,*.json,*.yaml,*.md",
                    "description": "Comma-separated glob patterns",
                },
            },
            "required": ["pattern"],
        },
    },
    {
        "name": "get_diff",
        "description": "Get uncommitted git diff for file or all files.",
        "input_schema": {
            "type": "object",
            "properties": {
                "path": {
                    "type": "string",
                    "default": "",
                    "description": "File path (empty = all files)",
                },
                "base": {
                    "type": "string",
                    "default": "HEAD",
                    "description": "Base commit/branch",
                },
            },
        },
    },
    {
        "name": "apply_patch",
        "description": "Apply unified diff patch to file. Use dry_run to test first.",
        "input_schema": {
            "type": "object",
            "properties": {
                "patch_text": {
                    "type": "string",
                    "description": "Unified diff patch text",
                },
                "path": {
                    "type": "string",
                    "default": "",
                    "description": "Target file path (extracted from patch if empty)",
                },
                "dry_run": {
                    "type": "boolean",
                    "default": False,
                    "description": "Test without applying",
                },
            },
            "required": ["patch_text"],
        },
    },
    {
        "name": "create_snapshot",
        "description": "Create snapshot of files for rollback before risky changes.",
        "input_schema": {
            "type": "object",
            "properties": {
                "paths": {
                    "type": "array",
                    "items": {"type": "string"},
                    "description": "File paths to snapshot",
                },
                "description": {
                    "type": "string",
                    "default": "",
                    "description": "Snapshot description",
                },
            },
            "required": ["paths"],
        },
    },
    {
        "name": "restore_snapshot",
        "description": "Restore files from a snapshot.",
        "input_schema": {
            "type": "object",
            "properties": {
                "snapshot_id": {
                    "type": "string",
                    "description": "Snapshot ID to restore",
                },
            },
            "required": ["snapshot_id"],
        },
    },
    {
        "name": "list_snapshots",
        "description": "List all available snapshots.",
        "input_schema": {"type": "object", "properties": {}},
    },
    {
        "name": "delete_snapshot",
        "description": "Delete a snapshot.",
        "input_schema": {
            "type": "object",
            "properties": {
                "snapshot_id": {
                    "type": "string",
                    "description": "Snapshot ID to delete",
                },
            },
            "required": ["snapshot_id"],
        },
    },
    {
        "name": "get_project_context",
        "description": "Get Rogue Protocol rules, layer architecture, and project-specific context.",
        "input_schema": {"type": "object", "properties": {}},
    },
]


async def handle_tool_call(name: str, args: Dict[str, Any]) -> str:
    result = {"error": f"Unknown tool: {name}"}

    if name == "read_file":
        result = _read_file(
            args.get("path", ""), args.get("offset", 0), args.get("limit", 2000)
        )
    elif name == "write_file":
        result = _write_file(args.get("path", ""), args.get("content", ""))
    elif name == "list_dir":
        result = _list_dir(args.get("path", ""))
    elif name == "search_files":
        result = _search_files(
            args.get("pattern", ""),
            args.get("path", ""),
            args.get("file_pattern", "*.py,*.cs,*.js,*.ts,*.json,*.yaml,*.md"),
        )
    elif name == "get_diff":
        result = _get_diff(args.get("path", ""), args.get("base", "HEAD"))
    elif name == "apply_patch":
        result = _apply_patch(
            args.get("patch_text", ""), args.get("path", ""), args.get("dry_run", False)
        )
    elif name == "create_snapshot":
        result = _create_snapshot(args.get("paths", []), args.get("description", ""))
    elif name == "restore_snapshot":
        result = _restore_snapshot(args.get("snapshot_id", ""))
    elif name == "list_snapshots":
        result = _list_snapshots()
    elif name == "delete_snapshot":
        result = _delete_snapshot(args.get("snapshot_id", ""))
    elif name == "get_project_context":
        result = _get_project_context()

    return json.dumps(result, indent=2)


if MCP_AVAILABLE:
    server = Server(APP_NAME)

    @server.list_tools()
    async def list_tools() -> List[Tool]:
        return [
            Tool(
                name=t["name"],
                description=t["description"],
                inputSchema=t["input_schema"],
            )
            for t in TOOLS
        ]

    @server.call_tool()
    async def call_tool(name: str, args: Dict[str, Any]) -> List[TextContent]:
        result = await handle_tool_call(name, args)
        return [TextContent(type="text", text=result)]

    @server.list_resources()
    async def list_resources() -> List[Resource]:
        resources = []
        try:
            for item in CODE_ROOT.iterdir():
                if item.name.startswith("."):
                    continue
                if item.is_file():
                    resources.append(
                        Resource(
                            uri=f"file:///{item.name}",
                            name=item.name,
                            mimeType="text/plain",
                        )
                    )
        except Exception:
            pass
        return resources

    @server.read_resource()
    async def read_resource(uri: str) -> str:
        if uri.startswith("file:///"):
            path = uri[8:]
            result = _read_file(path)
            return result.get("content", result.get("error", "Unknown error"))
        return f"Unknown resource: {uri}"

    async def run_stdio():
        async with stdio_server() as (read_stream, write_stream):
            await server.run(
                read_stream, write_stream, server.create_initialization_options()
            )

    async def run_sse(port: int):
        from starlette.applications import Starlette
        from starlette.routing import Route

        sse = SseServerTransport("/messages")

        async def handle_sse(request):
            async with sse.connect_sse(
                request.scope, request.receive, request._send
            ) as streams:
                await server.run(
                    streams[0], streams[1], server.create_initialization_options()
                )

        app = Starlette(routes=[Route("/sse", endpoint=handle_sse)])

        import uvicorn

        config = uvicorn.Config(app, host="127.0.0.1", port=port, log_level="warning")
        server_instance = uvicorn.Server(config)
        await server_instance.serve()

    def run_main():
        parser = argparse.ArgumentParser(description="Rogue MCP Server")
        parser.add_argument("--transport", choices=["stdio", "sse"], default="stdio")
        parser.add_argument("--port", type=int, default=7079)
        parser.add_argument(
            "--code-root", type=str, default="", help="Project root directory"
        )
        args = parser.parse_args()

        global CODE_ROOT
        if args.code_root:
            CODE_ROOT = Path(args.code_root).resolve()

        os.environ["CODE_ROOT"] = str(CODE_ROOT)

        if args.transport == "stdio":
            asyncio.run(run_stdio())
        else:
            asyncio.run(run_sse(args.port))

else:

    def run_main():
        print("MCP SDK not available. Install with: pip install mcp", file=sys.stderr)
        sys.exit(1)


def main():
    run_main()


if __name__ == "__main__":
    main()
