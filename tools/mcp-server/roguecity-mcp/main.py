import argparse
import importlib.util
import json
import os
import shutil
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, List, Optional, Sequence, Tuple
from urllib.parse import urlparse, urlunparse

import httpx
from pydantic import Field

try:
    from mcp.server.fastmcp import FastMCP
except Exception:  # pragma: no cover
    FastMCP = None

TRANSPORT_TYPE = "stdio"
SERVER_PORT = 3110
TOOLSERVER_URL = os.getenv("RC_AI_BRIDGE_BASE_URL", "http://127.0.0.1:7077")
REPO_ROOT = Path(__file__).resolve().parents[3]

mcp: Any = None

ALLOWED_BUILD_TARGETS = {
    "RogueCityVisualizerHeadless",
    "RogueCityVisualizerGui",
    "RogueCityCore",
    "RogueCityGenerators",
    "RogueCityApp",
}
ALLOWED_BUILD_PRESETS = {
    "gui-release",
    "gui-debug",
    "headless-release",
}
REQUIRED_OLLAMA_MODELS = [
    "functiongemma:latest",
    "embeddinggemma:latest",
    "codegemma:2b",
    "gemma3:4b",
    "gemma3:12b",
    "granite3.2-vision:latest",
    "glm-ocr:latest",
]
LOCAL_BRIDGE_PID_FILE = REPO_ROOT / "tools" / ".run" / "toolserver_local.pid"
LOCAL_BRIDGE_LOG_FILE = REPO_ROOT / "tools" / ".run" / "toolserver_local.log"


def parse_args() -> argparse.Namespace:
    global TRANSPORT_TYPE, SERVER_PORT, TOOLSERVER_URL, REPO_ROOT

    parser = argparse.ArgumentParser(
        description="RogueCity MCP server for bridge/build/perception orchestration",
    )
    parser.add_argument("--transport", type=str, choices=["stdio", "sse"], default="stdio")
    parser.add_argument("--port", type=int, default=3110)
    parser.add_argument("--toolserver-url", type=str, default=TOOLSERVER_URL)
    parser.add_argument("--repo-root", type=str, default=str(REPO_ROOT))
    parser.add_argument("--self-test", action="store_true", help="Run compatibility self-test and exit")
    args = parser.parse_args()

    TRANSPORT_TYPE = args.transport
    SERVER_PORT = args.port
    TOOLSERVER_URL = args.toolserver_url.rstrip("/")
    REPO_ROOT = Path(args.repo_root).resolve()
    return args


def _result(
    ok: bool,
    tool: str,
    summary: str,
    data: Optional[Dict[str, Any]] = None,
    warnings: Optional[List[str]] = None,
    errors: Optional[List[str]] = None,
    next_tools: Optional[List[str]] = None,
) -> Dict[str, Any]:
    return {
        "ok": ok,
        "tool": tool,
        "summary": summary,
        "data": data or {},
        "warnings": warnings or [],
        "errors": errors or [],
        "next_suggested_tools": next_tools or [],
    }


def _pick_powershell() -> str:
    if shutil.which("pwsh"):
        return "pwsh"
    if shutil.which("powershell"):
        return "powershell"
    return "powershell"


def _run_powershell(command: str, timeout_sec: int = 300) -> Dict[str, Any]:
    ps = _pick_powershell()
    cmd = [ps, "-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", command]
    proc = subprocess.run(
        cmd,
        cwd=str(REPO_ROOT),
        capture_output=True,
        text=True,
        timeout=timeout_sec,
    )
    return {
        "ok": proc.returncode == 0,
        "returncode": proc.returncode,
        "stdout": proc.stdout[-12000:],
        "stderr": proc.stderr[-12000:],
        "command": " ".join(cmd),
    }


def _api_get(path: str, timeout_sec: int = 10) -> Dict[str, Any]:
    last_exc: Optional[Exception] = None
    for base_url in _toolserver_url_candidates():
        url = f"{base_url}{path}"
        try:
            with httpx.Client(timeout=timeout_sec) as client:
                r = client.get(url)
            try:
                body = r.json()
            except Exception:
                body = {"raw": r.text[:2000]}
            return {"status_code": r.status_code, "body": body, "url": url}
        except httpx.RequestError as exc:
            last_exc = exc
            continue
    if last_exc:
        raise last_exc
    raise RuntimeError("No toolserver URL candidates available")


def _api_post(path: str, payload: Dict[str, Any], timeout_sec: int = 120) -> Dict[str, Any]:
    last_exc: Optional[Exception] = None
    for base_url in _toolserver_url_candidates():
        url = f"{base_url}{path}"
        try:
            with httpx.Client(timeout=timeout_sec) as client:
                r = client.post(url, json=payload)
            try:
                body = r.json()
            except Exception:
                body = {"raw": r.text[:4000]}
            return {"status_code": r.status_code, "body": body, "url": url}
        except httpx.RequestError as exc:
            last_exc = exc
            continue
    if last_exc:
        raise last_exc
    raise RuntimeError("No toolserver URL candidates available")


def _toolserver_url_candidates() -> List[str]:
    base = TOOLSERVER_URL.rstrip("/")
    out: List[str] = [base]
    parsed = urlparse(base)
    host = (parsed.hostname or "").lower()
    # Linux/WSL fallback for Windows-hosted localhost bridge.
    if os.name != "nt" and host in {"127.0.0.1", "localhost"}:
        alt_hosts: List[str] = ["host.docker.internal"]
        resolv = Path("/etc/resolv.conf")
        if resolv.exists():
            try:
                for line in resolv.read_text(encoding="utf-8", errors="ignore").splitlines():
                    if line.strip().startswith("nameserver "):
                        parts = line.split()
                        if len(parts) >= 2:
                            alt_hosts.append(parts[1].strip())
                        break
            except Exception:
                pass

        for h in alt_hosts:
            if not h:
                continue
            netloc = h
            if parsed.port:
                netloc = f"{h}:{parsed.port}"
            alt = urlunparse((parsed.scheme or "http", netloc, "", "", "", "")).rstrip("/")
            if alt not in out:
                out.append(alt)
    return out


def _normalize_base_url(raw: str) -> Optional[str]:
    text = str(raw or "").strip().rstrip("/")
    if not text:
        return None
    if "://" not in text:
        text = f"http://{text}"
    try:
        parsed = urlparse(text)
    except Exception:
        return None
    if not parsed.scheme or not parsed.netloc:
        return None
    return urlunparse((parsed.scheme, parsed.netloc, "", "", "", "")).rstrip("/")


def _ollama_base_url_candidates() -> List[str]:
    out: List[str] = []
    seen = set()

    def _push(raw: str) -> None:
        base = _normalize_base_url(raw)
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
    return out


def _pick_reachable_ollama_base(timeout_sec: int = 3) -> str:
    candidates = _ollama_base_url_candidates()
    for base in candidates:
        try:
            with httpx.Client(timeout=timeout_sec) as client:
                r = client.get(f"{base}/api/tags")
            if r.status_code == 200:
                return base
        except Exception:
            continue
    return candidates[0] if candidates else "http://127.0.0.1:11434"


def _repo_rel(path: str, must_exist: bool = False) -> Optional[str]:
    if not path:
        return None
    p = Path(path)
    candidate = p.resolve() if p.is_absolute() else (REPO_ROOT / p).resolve()
    try:
        rel = candidate.relative_to(REPO_ROOT)
    except ValueError:
        return None
    if must_exist and not candidate.exists():
        return None
    return rel.as_posix()


def _sanitize_rel_list(paths: Sequence[str], must_exist: bool) -> Tuple[List[str], List[str]]:
    accepted: List[str] = []
    dropped: List[str] = []
    seen = set()
    for p in paths:
        rel = _repo_rel(str(p), must_exist=must_exist)
        if not rel:
            dropped.append(str(p))
            continue
        if rel not in seen:
            seen.add(rel)
            accepted.append(rel)
    return accepted, dropped


def _safe_output_rel(path: str, default_rel: str) -> str:
    rel = _repo_rel(path, must_exist=False)
    if rel:
        return rel
    return default_rel


def _headless_exe_path() -> Path:
    candidates = [
        REPO_ROOT / "bin" / "RogueCityVisualizerHeadless.exe",
        REPO_ROOT / "build_vs" / "bin" / "Release" / "RogueCityVisualizerHeadless.exe",
        REPO_ROOT / "build_ninja" / "bin" / "RogueCityVisualizerHeadless.exe",
    ]
    for c in candidates:
        if c.exists():
            return c
    return REPO_ROOT / "bin" / "RogueCityVisualizerHeadless.exe"


def _probe_ollama_models(timeout_sec: int = 5) -> Dict[str, Any]:
    last_error = ""
    tried: List[str] = []
    for base in _ollama_base_url_candidates():
        url = f"{base}/api/tags"
        tried.append(url)
        try:
            with httpx.Client(timeout=timeout_sec) as client:
                r = client.get(url)
            if r.status_code != 200:
                last_error = f"status_code={r.status_code}"
                continue
            payload = r.json()
            names = [str(m.get("name", "")) for m in payload.get("models", []) if isinstance(m, dict)]
            missing = [m for m in REQUIRED_OLLAMA_MODELS if m not in names]
            return {
                "reachable": True,
                "status_code": 200,
                "url": base,
                "models": names,
                "missing": missing,
                "tried": tried,
            }
        except Exception as e:
            last_error = str(e)
            continue
    return {
        "reachable": False,
        "status_code": 0,
        "error": last_error,
        "models": [],
        "missing": REQUIRED_OLLAMA_MODELS,
        "tried": tried,
    }


def _module_available(name: str) -> bool:
    return importlib.util.find_spec(name) is not None


def _health_url_for_host(host: str, port: int) -> str:
    probe_host = "127.0.0.1" if host in {"0.0.0.0", "::"} else host
    return f"http://{probe_host}:{int(port)}/health"


def _wait_http_ok(url: str, timeout_sec: float = 15.0) -> bool:
    deadline = datetime.now(timezone.utc).timestamp() + timeout_sec
    while datetime.now(timezone.utc).timestamp() < deadline:
        try:
            with httpx.Client(timeout=2.0) as client:
                r = client.get(url)
            if r.status_code == 200:
                return True
        except Exception:
            pass
        time.sleep(0.25)
    return False


def _tail_text(path: Path, max_chars: int = 4000) -> str:
    if not path.exists():
        return ""
    try:
        txt = path.read_text(encoding="utf-8", errors="ignore")
    except Exception:
        return ""
    return txt[-max_chars:]


def _compat_self_test() -> Dict[str, Any]:
    scripts = {
        "start_bridge": str((REPO_ROOT / "tools" / "Start_Ai_Bridge_Fixed.ps1").resolve()),
        "stop_bridge": str((REPO_ROOT / "tools" / "Stop_Ai_Bridge_Fixed.ps1").resolve()),
        "start_bridge_wsl": str((REPO_ROOT / "tools" / "start_ai_bridge_wsl.sh").resolve()),
        "stop_bridge_wsl": str((REPO_ROOT / "tools" / "stop_ai_bridge_wsl.sh").resolve()),
        "dev_shell": str((REPO_ROOT / "tools" / "dev-shell.ps1").resolve()),
        "toolserver": str((REPO_ROOT / "tools" / "toolserver.py").resolve()),
        "mcp_main": str((REPO_ROOT / "tools" / "mcp-server" / "roguecity-mcp" / "main.py").resolve()),
    }
    script_status = {k: Path(v).exists() for k, v in scripts.items()}

    deps = {
        "fastapi": _module_available("fastapi"),
        "uvicorn": _module_available("uvicorn"),
        "httpx": _module_available("httpx"),
        "pydantic": _module_available("pydantic"),
        "mcp": _module_available("mcp"),
    }

    health = None
    pipeline_probe = None
    perception_probe = None
    bridge_reachable = False
    try:
        health = _api_get("/health", timeout_sec=4)
        bridge_reachable = health.get("status_code") == 200
        pipeline_probe = _api_get("/pipeline/query", timeout_sec=4)
        perception_probe = _api_get("/perception/observe", timeout_sec=4)
    except Exception as e:
        health = {"status_code": 0, "error": str(e)}

    endpoint_present = {
        "pipeline_query": bool(pipeline_probe and pipeline_probe.get("status_code") != 404),
        "perception_observe": bool(perception_probe and perception_probe.get("status_code") != 404),
    }

    headless = _headless_exe_path()
    ollama = _probe_ollama_models(timeout_sec=5)

    warnings: List[str] = []
    core_failures: List[str] = []
    if not all(script_status.values()):
        warnings.append("missing_required_scripts")
        core_failures.append("missing_required_scripts")
    if not all(deps.values()):
        warnings.append("python_dependencies_missing")
        core_failures.append("python_dependencies_missing")
    if not bridge_reachable:
        warnings.append("bridge_unreachable")
        core_failures.append("bridge_unreachable")
    if bridge_reachable and not endpoint_present["pipeline_query"]:
        warnings.append("pipeline_query_missing")
        core_failures.append("pipeline_query_missing")
    if bridge_reachable and not endpoint_present["perception_observe"]:
        warnings.append("perception_observe_missing")
        core_failures.append("perception_observe_missing")
    if not ollama.get("reachable"):
        warnings.append("ollama_unreachable")
    if ollama.get("missing"):
        warnings.append("ollama_models_missing")

    ok_core = len(core_failures) == 0
    ok_full = ok_core and bool(ollama.get("reachable")) and not bool(ollama.get("missing"))
    runtime_recommendation = "powershell_primary" if not ok_full else "powershell_or_wsl"
    return {
        "ok": ok_core,
        "ok_core": ok_core,
        "ok_full": ok_full,
        "runtime_recommendation": runtime_recommendation,
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "toolserver_url": TOOLSERVER_URL,
        "repo_root": str(REPO_ROOT),
        "warnings": warnings,
        "scripts": script_status,
        "python_dependencies": deps,
        "bridge": {
            "reachable": bridge_reachable,
            "health": health,
            "pipeline_probe": pipeline_probe,
            "perception_probe": perception_probe,
        },
        "headless_exe": str(headless),
        "headless_exists": headless.exists(),
        "ollama": ollama,
    }


def _ensure_mcp() -> None:
    global mcp
    if FastMCP is None:
        raise RuntimeError("Missing dependency: mcp. Install with: pip install 'mcp[cli]'")
    mcp = FastMCP("roguecity-mcp")


def register_tools() -> None:
    @mcp.tool()
    def rc_bridge_status(
        toolserver_url: str = Field(default=TOOLSERVER_URL, description="Toolserver base URL"),
        timeout_sec: int = Field(default=5, description="HTTP timeout seconds"),
    ) -> Dict[str, Any]:
        """Check bridge health, mode, and pipeline/perception endpoint availability."""
        global TOOLSERVER_URL
        TOOLSERVER_URL = toolserver_url.rstrip("/")
        warnings: List[str] = []
        errors: List[str] = []
        try:
            health = _api_get("/health", timeout_sec=timeout_sec)
            pipeline_probe = _api_get("/pipeline/query", timeout_sec=timeout_sec)
            perception_probe = _api_get("/perception/observe", timeout_sec=timeout_sec)
            pipeline_available = pipeline_probe["status_code"] != 404
            perception_available = perception_probe["status_code"] != 404
            ok = health["status_code"] == 200
            if not pipeline_available:
                warnings.append("pipeline_query_endpoint_missing")
            if not perception_available:
                warnings.append("perception_observe_endpoint_missing")
            return _result(
                ok=ok,
                tool="rc_bridge_status",
                summary=(
                    f"health={health['status_code']} pipeline={pipeline_probe['status_code']} "
                    f"perception={perception_probe['status_code']}"
                ),
                data={
                    "toolserver_url": TOOLSERVER_URL,
                    "health": health,
                    "pipeline_probe": pipeline_probe,
                    "perception_probe": perception_probe,
                    "pipeline_available": pipeline_available,
                    "perception_available": perception_available,
                },
                warnings=warnings,
                errors=errors,
                next_tools=["rc_bridge_restart"] if not ok else ["rc_perceive_ui", "rc_env_validate"],
            )
        except Exception as e:
            return _result(
                ok=False,
                tool="rc_bridge_status",
                summary="bridge_unreachable",
                errors=[str(e)],
                next_tools=["rc_bridge_restart"],
            )

    @mcp.tool()
    def rc_bridge_restart(
        port: int = Field(default=7077, description="Bridge port"),
        live: bool = Field(default=True, description="Start live mode when true; mock mode when false"),
        force_restart: bool = Field(default=True, description="Force listener recycle before restart"),
    ) -> Dict[str, Any]:
        """Safely restart bridge using allowlisted scripts."""
        stop_script = (REPO_ROOT / "tools" / "Stop_Ai_Bridge_Fixed.ps1").resolve()
        start_script = (REPO_ROOT / "tools" / "Start_Ai_Bridge_Fixed.ps1").resolve()
        if not stop_script.exists() or not start_script.exists():
            return _result(
                ok=False,
                tool="rc_bridge_restart",
                summary="bridge_scripts_missing",
                errors=[f"Missing scripts under {REPO_ROOT / 'tools'}"],
            )

        live_arg = "$false" if live else "$true"
        force_arg = "$true" if force_restart else "$false"
        cmd = (
            f"& '{stop_script}' -NonInteractive -Port {int(port)}; "
            "Start-Sleep -Milliseconds 500; "
            f"& '{start_script}' -MockMode:{live_arg} -NonInteractive -ForceRestart:{force_arg} -Port {int(port)}"
        )
        run = _run_powershell(cmd, timeout_sec=240)
        post = _api_get("/health", timeout_sec=8) if run["ok"] else {"status_code": 0, "body": {}}
        ok = run["ok"] and post.get("status_code") == 200
        warnings = []
        if not ok:
            warnings.append("bridge_restart_failed_or_unhealthy")
        return _result(
            ok=ok,
            tool="rc_bridge_restart",
            summary="bridge_restarted" if ok else "bridge_restart_failed",
            data={"run": run, "post_health": post},
            warnings=warnings,
            next_tools=["rc_bridge_status", "rc_perceive_ui"] if ok else ["rc_bridge_status"],
        )

    @mcp.tool()
    def rc_bridge_start_local(
        port: int = Field(default=7077, description="Bridge port"),
        host: str = Field(default="127.0.0.1", description="Bind host (127.0.0.1 or 0.0.0.0)"),
        mock: bool = Field(default=True, description="Start in mock mode when true"),
        force_restart: bool = Field(default=True, description="Stop previously tracked local bridge process"),
    ) -> Dict[str, Any]:
        """Start bridge in the same runtime as MCP (WSL/Linux-friendly path)."""
        allowed_hosts = {"127.0.0.1", "0.0.0.0", "::1", "::"}
        if host not in allowed_hosts:
            return _result(
                ok=False,
                tool="rc_bridge_start_local",
                summary="invalid_host",
                errors=[f"host '{host}' must be one of {sorted(allowed_hosts)}"],
            )

        LOCAL_BRIDGE_PID_FILE.parent.mkdir(parents=True, exist_ok=True)

        previous_pid = None
        if LOCAL_BRIDGE_PID_FILE.exists():
            try:
                previous_pid = int(LOCAL_BRIDGE_PID_FILE.read_text(encoding="utf-8").strip())
            except Exception:
                previous_pid = None
        if previous_pid and force_restart:
            try:
                os.kill(previous_pid, 15)
                time.sleep(0.3)
            except Exception:
                pass
            try:
                os.kill(previous_pid, 9)
            except Exception:
                pass
            try:
                LOCAL_BRIDGE_PID_FILE.unlink()
            except Exception:
                pass

        env = dict(os.environ)
        ollama_base = env.get("OLLAMA_BASE_URL") or _pick_reachable_ollama_base(timeout_sec=2)
        env.setdefault("RC_AI_PIPELINE_V2", "on")
        env.setdefault("RC_AI_AUDIT_STRICT", "off")
        env.setdefault("RC_AI_CONTROLLER_MODEL", "functiongemma")
        env.setdefault("RC_AI_TRIAGE_MODEL", "codegemma:2b")
        env.setdefault("RC_AI_SYNTH_FAST_MODEL", "gemma3:4b")
        env.setdefault("RC_AI_SYNTH_ESCALATION_MODEL", "gemma3:12b")
        env.setdefault("RC_AI_EMBEDDING_MODEL", "embeddinggemma")
        env.setdefault("RC_AI_VISION_MODEL", "granite3.2-vision")
        env.setdefault("RC_AI_OCR_MODEL", "glm-ocr")
        env.setdefault("OLLAMA_FLASH_ATTENTION", "1")
        env.setdefault("OLLAMA_KV_CACHE_TYPE", "f16")
        env.setdefault("OLLAMA_BASE_URL", ollama_base)
        env["RC_AI_BRIDGE_BASE_URL"] = f"http://127.0.0.1:{int(port)}"
        if mock:
            env["ROGUECITY_TOOLSERVER_MOCK"] = "1"
        else:
            env.pop("ROGUECITY_TOOLSERVER_MOCK", None)

        cmd = [
            sys.executable,
            "-m",
            "uvicorn",
            "tools.toolserver:app",
            "--host",
            host,
            "--port",
            str(int(port)),
        ]

        log_handle = LOCAL_BRIDGE_LOG_FILE.open("a", encoding="utf-8")
        proc = subprocess.Popen(cmd, cwd=str(REPO_ROOT), env=env, stdout=log_handle, stderr=log_handle)
        LOCAL_BRIDGE_PID_FILE.write_text(str(proc.pid), encoding="utf-8")

        health_url = _health_url_for_host(host, int(port))
        ok = _wait_http_ok(health_url, timeout_sec=20.0)
        if not ok:
            try:
                os.kill(proc.pid, 9)
            except Exception:
                pass
            return _result(
                ok=False,
                tool="rc_bridge_start_local",
                summary="local_bridge_start_failed",
                data={
                    "pid": proc.pid,
                    "command": " ".join(cmd),
                    "log_path": str(LOCAL_BRIDGE_LOG_FILE),
                    "log_tail": _tail_text(LOCAL_BRIDGE_LOG_FILE),
                    "health_url": health_url,
                },
                errors=["health_check_failed"],
                next_tools=["rc_bridge_status", "rc_bridge_restart"],
            )

        return _result(
            ok=True,
            tool="rc_bridge_start_local",
            summary="local_bridge_started",
            data={
                "pid": proc.pid,
                "base_url": f"http://127.0.0.1:{int(port)}",
                "health_url": health_url,
                "mock": mock,
                "bind_host": host,
                "ollama_base_url": env.get("OLLAMA_BASE_URL"),
                "log_path": str(LOCAL_BRIDGE_LOG_FILE),
            },
            next_tools=["rc_bridge_status", "rc_perceive_ui"],
        )

    @mcp.tool()
    def rc_bridge_stop_local(
        port: int = Field(default=7077, description="Bridge port"),
    ) -> Dict[str, Any]:
        """Stop local runtime bridge started by rc_bridge_start_local."""
        stopped = False
        pid = None
        if LOCAL_BRIDGE_PID_FILE.exists():
            try:
                pid = int(LOCAL_BRIDGE_PID_FILE.read_text(encoding="utf-8").strip())
            except Exception:
                pid = None
        if pid:
            try:
                os.kill(pid, 15)
                time.sleep(0.2)
            except Exception:
                pass
            try:
                os.kill(pid, 9)
            except Exception:
                pass
            stopped = True
        try:
            LOCAL_BRIDGE_PID_FILE.unlink(missing_ok=True)  # type: ignore[arg-type]
        except Exception:
            pass

        # Also attempt graceful endpoint shutdown for whichever bridge answers on target port.
        try:
            with httpx.Client(timeout=2.0) as client:
                client.post(f"http://127.0.0.1:{int(port)}/admin/shutdown")
            stopped = True
        except Exception:
            pass

        return _result(
            ok=True,
            tool="rc_bridge_stop_local",
            summary="local_bridge_stopped" if stopped else "local_bridge_not_found",
            data={
                "pid": pid,
                "port": int(port),
            },
            next_tools=["rc_bridge_status", "rc_bridge_start_local"],
        )

    @mcp.tool()
    def rc_build_visualizer(
        target: str = Field(default="RogueCityVisualizerHeadless", description="CMake target to build"),
        preset: str = Field(default="gui-release", description="Build preset"),
        timeout_sec: int = Field(default=1200, description="Build timeout seconds"),
    ) -> Dict[str, Any]:
        """Build allowlisted visualizer targets via dev shell context."""
        if target not in ALLOWED_BUILD_TARGETS:
            return _result(
                ok=False,
                tool="rc_build_visualizer",
                summary="target_not_allowlisted",
                errors=[f"target '{target}' not in allowlist"],
            )
        if preset not in ALLOWED_BUILD_PRESETS:
            return _result(
                ok=False,
                tool="rc_build_visualizer",
                summary="preset_not_allowlisted",
                errors=[f"preset '{preset}' not in allowlist"],
            )

        cmd = (
            "Set-Location -LiteralPath '" + str(REPO_ROOT) + "'; "
            ". .\\tools\\dev-shell.ps1; "
            f"cmake --build --preset {preset} --target {target}"
        )
        run = _run_powershell(cmd, timeout_sec=timeout_sec)
        return _result(
            ok=run["ok"],
            tool="rc_build_visualizer",
            summary="build_succeeded" if run["ok"] else "build_failed",
            data=run,
            next_tools=["rc_run_headless_snapshot"] if run["ok"] else ["rc_build_visualizer"],
        )

    @mcp.tool()
    def rc_run_headless_snapshot(
        frames: int = Field(default=5, description="Headless frames"),
        snapshot_path: str = Field(
            default="AI/docs/ui/ui_introspection_headless_latest.json",
            description="Repo-relative snapshot output path",
        ),
        build_if_missing: bool = Field(default=True, description="Build headless target when executable is missing"),
    ) -> Dict[str, Any]:
        """Run headless visualizer snapshot export and return artifact path."""
        exe = _headless_exe_path()
        warnings: List[str] = []
        if not exe.exists() and build_if_missing:
            build = rc_build_visualizer(target="RogueCityVisualizerHeadless")
            if not build.get("ok"):
                return _result(
                    ok=False,
                    tool="rc_run_headless_snapshot",
                    summary="headless_build_failed",
                    data={"build": build},
                    errors=["headless_executable_missing_and_build_failed"],
                )
            exe = _headless_exe_path()

        if not exe.exists():
            return _result(
                ok=False,
                tool="rc_run_headless_snapshot",
                summary="headless_executable_missing",
                errors=[str(exe)],
            )

        rel_out = _safe_output_rel(snapshot_path, "AI/docs/ui/ui_introspection_headless_latest.json")
        out_path = (REPO_ROOT / rel_out).resolve()
        out_path.parent.mkdir(parents=True, exist_ok=True)
        cmd = [str(exe), "--frames", str(max(1, int(frames))), "--export-ui-snapshot", str(out_path)]
        proc = subprocess.run(cmd, cwd=str(REPO_ROOT), capture_output=True, text=True, timeout=180)
        ok = proc.returncode == 0 and out_path.exists()
        if not ok:
            warnings.append("snapshot_export_failed")
        return _result(
            ok=ok,
            tool="rc_run_headless_snapshot",
            summary="snapshot_exported" if ok else "snapshot_export_failed",
            data={
                "snapshot_path": str(out_path),
                "returncode": proc.returncode,
                "stdout": proc.stdout[-12000:],
                "stderr": proc.stderr[-12000:],
                "command": " ".join(cmd),
            },
            warnings=warnings,
            next_tools=["rc_perceive_ui", "rc_observe_and_map"] if ok else ["rc_build_visualizer"],
        )

    @mcp.tool()
    def rc_perceive_ui(
        mode: str = Field(default="quick", description="quick|full"),
        frames: int = Field(default=5, description="Headless frames when capture runs"),
        screenshot: bool = Field(default=False, description="Use screenshot evidence when available"),
        include_vision: bool = Field(default=True, description="Include vision model enrichment"),
        include_ocr: bool = Field(default=True, description="Include OCR model enrichment"),
        strict_contract: bool = Field(default=True, description="Enforce strict contract severity"),
    ) -> Dict[str, Any]:
        """Call toolserver perception observe endpoint."""
        if mode not in {"quick", "full"}:
            return _result(
                ok=False,
                tool="rc_perceive_ui",
                summary="invalid_mode",
                errors=["mode must be quick|full"],
            )
        payload = {
            "mode": mode,
            "frames": max(1, int(frames)),
            "screenshot": bool(screenshot),
            "include_vision": bool(include_vision),
            "include_ocr": bool(include_ocr),
            "strict_contract": bool(strict_contract),
            "mockup_path": "visualizer/RC_UI_Mockup.html",
        }
        resp = _api_post("/perception/observe", payload, timeout_sec=300)
        ok = resp["status_code"] == 200
        return _result(
            ok=ok,
            tool="rc_perceive_ui",
            summary="perception_observed" if ok else f"perception_failed_{resp['status_code']}",
            data=resp,
            next_tools=["rc_generate_audit_report", "rc_observe_and_map"] if ok else ["rc_bridge_status"],
        )

    @mcp.tool()
    def rc_pipeline_query(
        prompt: str = Field(description="User query"),
        mode: str = Field(default="normal", description="normal|audit"),
        context_files: List[str] = Field(default_factory=list, description="Repo-relative context files"),
        required_paths: List[str] = Field(default_factory=list, description="Required repo-relative paths"),
        search_hints: List[str] = Field(default_factory=list, description="Search hints"),
    ) -> Dict[str, Any]:
        """Call pipeline query endpoint with path sanitization and deterministic defaults."""
        if mode not in {"normal", "audit"}:
            return _result(
                ok=False,
                tool="rc_pipeline_query",
                summary="invalid_mode",
                errors=["mode must be normal|audit"],
            )

        safe_context, dropped_context = _sanitize_rel_list(context_files, must_exist=True)
        safe_required, dropped_required = _sanitize_rel_list(required_paths, must_exist=True)

        payload = {
            "prompt": prompt,
            "mode": mode,
            "context_files": safe_context,
            "required_paths": safe_required,
            "search_hints": [str(x) for x in search_hints[:20]],
            "include_repo_index": False,
            "include_semantic": False,
            "embedding_dimensions": 512,
            "temperature": 0.10,
            "top_p": 0.85,
            "num_predict": 256,
        }
        resp = _api_post("/pipeline/query", payload, timeout_sec=180)
        ok = resp["status_code"] == 200
        warnings = []
        if dropped_context:
            warnings.append(f"dropped_context_files={dropped_context}")
        if dropped_required:
            warnings.append(f"dropped_required_paths={dropped_required}")
        return _result(
            ok=ok,
            tool="rc_pipeline_query",
            summary="pipeline_query_ok" if ok else f"pipeline_query_failed_{resp['status_code']}",
            data=resp,
            warnings=warnings,
            next_tools=["rc_pipeline_eval", "rc_generate_audit_report"] if ok else ["rc_bridge_status"],
        )

    @mcp.tool()
    def rc_pipeline_eval(
        case_file: str = Field(default="tools/ai_eval_cases.json", description="Case file path"),
        max_cases: int = Field(default=6, description="Maximum cases"),
        mode: str = Field(default="normal", description="normal|audit"),
    ) -> Dict[str, Any]:
        """Call pipeline eval endpoint with safe case-file path handling."""
        if mode not in {"normal", "audit"}:
            return _result(
                ok=False,
                tool="rc_pipeline_eval",
                summary="invalid_mode",
                errors=["mode must be normal|audit"],
            )
        rel_case = _repo_rel(case_file, must_exist=True)
        if not rel_case:
            return _result(
                ok=False,
                tool="rc_pipeline_eval",
                summary="invalid_case_file",
                errors=[f"case_file must exist inside repo: {case_file}"],
            )
        payload = {
            "case_file": rel_case,
            "max_cases": max(1, int(max_cases)),
            "mode": mode,
            "include_repo_index": False,
            "include_semantic": False,
            "embedding_dimensions": 512,
            "temperature": 0.10,
            "top_p": 0.85,
            "num_predict": 256,
        }
        resp = _api_post("/pipeline/eval", payload, timeout_sec=420)
        ok = resp["status_code"] == 200
        return _result(
            ok=ok,
            tool="rc_pipeline_eval",
            summary="pipeline_eval_ok" if ok else f"pipeline_eval_failed_{resp['status_code']}",
            data=resp,
            next_tools=["rc_generate_audit_report"] if ok else ["rc_bridge_status"],
        )

    @mcp.tool()
    def rc_env_validate(
        strict: bool = Field(default=False, description="When true, requires bridge + endpoints + models"),
    ) -> Dict[str, Any]:
        """Project-specific compatibility audit across scripts, deps, bridge, and model stack."""
        report = _compat_self_test()
        warnings = list(report.get("warnings", []))
        errors: List[str] = []
        if strict:
            if not report.get("bridge", {}).get("reachable"):
                errors.append("bridge_unreachable")
            if not report.get("bridge", {}).get("pipeline_probe") or report["bridge"]["pipeline_probe"].get("status_code") == 404:
                errors.append("pipeline_query_missing")
            if report.get("ollama", {}).get("missing"):
                errors.append("ollama_models_missing")
            if not all(report.get("python_dependencies", {}).values()):
                errors.append("python_dependencies_missing")
        ok = len(errors) == 0 and report.get("ok", False)
        if strict:
            ok = len(errors) == 0
        return _result(
            ok=ok,
            tool="rc_env_validate",
            summary="compat_ok" if ok else "compat_issues",
            data=report,
            warnings=warnings,
            errors=errors,
            next_tools=["rc_bridge_status", "rc_bridge_restart", "rc_build_visualizer"],
        )

    @mcp.tool()
    def rc_observe_and_map(
        mode: str = Field(default="quick", description="quick|full"),
        include_vision: bool = Field(default=False, description="Enable vision in full mode"),
        include_ocr: bool = Field(default=False, description="Enable OCR in full mode"),
    ) -> Dict[str, Any]:
        """Runs snapshot + perception and maps findings to likely code paths via pipeline query."""
        snap = rc_run_headless_snapshot(frames=5)
        if not snap.get("ok"):
            return _result(
                ok=False,
                tool="rc_observe_and_map",
                summary="snapshot_failed",
                data={"snapshot": snap},
                errors=["snapshot_failed"],
                next_tools=["rc_build_visualizer", "rc_bridge_status"],
            )

        perceive = rc_perceive_ui(
            mode=mode,
            frames=5,
            screenshot=False,
            include_vision=include_vision,
            include_ocr=include_ocr,
            strict_contract=True,
        )
        if not perceive.get("ok"):
            return _result(
                ok=False,
                tool="rc_observe_and_map",
                summary="perception_failed",
                data={"snapshot": snap, "perception": perceive},
                errors=["perception_failed"],
                next_tools=["rc_bridge_status"],
            )

        pbody = perceive.get("data", {}).get("body", {})
        findings = pbody.get("contract_findings", [])
        prompt = (
            "Map current runtime UI contract findings to likely C++/ImGui implementation files and suggest fix order.\n"
            f"Findings JSON:\n{json.dumps(findings[:10], indent=2)}"
        )
        mapped = rc_pipeline_query(
            prompt=prompt,
            mode="normal",
            context_files=[
                "visualizer/RC_UI_Mockup.html",
                "tools/toolserver.py",
                "visualizer/src/ui/panels/rc_panel_ai_console.cpp",
            ],
            required_paths=["visualizer/RC_UI_Mockup.html", "tools/toolserver.py"],
            search_hints=["DockSpace", "PanelRegistry", "Mockup", "viewport"],
        )

        ok = bool(perceive.get("ok")) and bool(mapped.get("ok"))
        return _result(
            ok=ok,
            tool="rc_observe_and_map",
            summary="observe_and_map_complete" if ok else "observe_and_map_partial",
            data={
                "snapshot": snap,
                "perception": perceive,
                "mapping": mapped,
            },
            warnings=[] if ok else ["mapping_failed_or_partial"],
            next_tools=["rc_generate_audit_report", "rc_pipeline_eval"] if ok else ["rc_pipeline_query"],
        )

    @mcp.tool()
    def rc_generate_audit_report(
        output_path: str = Field(
            default="AI/docs/ui/perception_audit_latest.md",
            description="Repo-relative markdown report path",
        ),
        include_eval: bool = Field(default=True, description="Include pipeline eval summary"),
    ) -> Dict[str, Any]:
        """Generate consolidated markdown report from compatibility + perception + eval outputs."""
        now = datetime.now(timezone.utc).isoformat()
        compat = rc_env_validate(strict=False)
        perception = rc_perceive_ui(
            mode="quick",
            frames=5,
            screenshot=False,
            include_vision=False,
            include_ocr=False,
            strict_contract=True,
        )
        eval_result = rc_pipeline_eval() if include_eval else None

        lines = [
            f"# RogueCity Perception Audit ({now})",
            "",
            "## Compatibility",
            f"- ok: {compat.get('ok')}",
            f"- summary: {compat.get('summary')}",
            f"- warnings: {len(compat.get('warnings', []))}",
            "",
            "## Perception",
            f"- ok: {perception.get('ok')}",
            f"- summary: {perception.get('summary')}",
        ]
        if perception.get("data"):
            pdata = perception["data"].get("body", {})
            lines.append(f"- actionability: {pdata.get('actionability', 'unknown')}")
            lines.append(f"- findings: {len(pdata.get('contract_findings', []))}")
            lines.append(f"- warnings: {len(pdata.get('warnings', []))}")
            lines.append(f"- errors: {len(pdata.get('errors', []))}")

        if eval_result:
            lines += [
                "",
                "## Pipeline Eval",
                f"- ok: {eval_result.get('ok')}",
                f"- summary: {eval_result.get('summary')}",
            ]
            edata = eval_result.get("data", {}).get("body", {})
            if isinstance(edata, dict):
                lines.append(f"- ok_cases: {edata.get('ok_cases')}/{edata.get('total_cases')}")
                lines.append(f"- json_ok_cases: {edata.get('json_ok_cases')}")
                lines.append(f"- missing_required_cases: {edata.get('cases_missing_required_paths')}")
                lines.append(f"- invalid_path_cases: {edata.get('cases_with_invalid_paths')}")

        rel_out = _safe_output_rel(output_path, "AI/docs/ui/perception_audit_latest.md")
        out = (REPO_ROOT / rel_out).resolve()
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text("\n".join(lines) + "\n", encoding="utf-8")

        return _result(
            ok=True,
            tool="rc_generate_audit_report",
            summary="audit_report_generated",
            data={
                "output_path": str(out),
                "compat": compat,
                "perception": perception,
                "pipeline_eval": eval_result,
            },
            next_tools=["rc_observe_and_map", "rc_pipeline_eval"],
        )


def run_server() -> None:
    args = parse_args()
    if args.self_test:
        print(json.dumps(_compat_self_test(), indent=2))
        return

    _ensure_mcp()
    register_tools()
    if TRANSPORT_TYPE == "sse":
        mcp.settings.port = SERVER_PORT
    mcp.run(transport=TRANSPORT_TYPE)  # type: ignore[arg-type]


if __name__ == "__main__":
    run_server()
