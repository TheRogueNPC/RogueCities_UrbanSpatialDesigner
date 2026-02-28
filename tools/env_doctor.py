#!/usr/bin/env python3
"""RogueCities environment doctor.

Checks the local VS Code/CMake/MSVC/diagnostics bridge setup and reports
actionable PASS/WARN/FAIL items.
"""

from __future__ import annotations

import argparse
import json
import os
import platform
import shlex
import subprocess
import sys
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import List


@dataclass
class CheckResult:
    name: str
    status: str  # PASS / WARN / FAIL
    message: str
    details: List[str]


def run_command(command: str, prefer_windows_shell: bool = False) -> subprocess.CompletedProcess:
    if prefer_windows_shell and os.name != "nt":
        wrapped = f'cmd.exe /d /c "{command}"'
    else:
        wrapped = command
    return subprocess.run(
        wrapped,
        shell=True,
        capture_output=True,
        text=True,
        check=False,
    )


def parse_json(path: Path) -> dict | None:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return None


def check_workspace(root: Path) -> CheckResult:
    required = [
        root / "CMakeLists.txt",
        root / "CMakePresets.json",
        root / "tools" / "generate_compile_commands.py",
    ]
    missing = [p for p in required if not p.exists()]
    if missing:
        return CheckResult(
            "workspace",
            "FAIL",
            "Missing required workspace files.",
            [str(p.relative_to(root)) for p in missing],
        )
    return CheckResult("workspace", "PASS", "Workspace structure looks valid.", [])


def check_vscode_settings(root: Path) -> CheckResult:
    settings_path = root / ".vscode" / "settings.json"
    settings = parse_json(settings_path)
    if settings is None:
        return CheckResult("vscode_settings", "FAIL", "Cannot parse .vscode/settings.json.", [])

    required_keys = {
        "cmake.defaultConfigurePreset": "dev",
        "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
        "C_Cpp.default.compileCommands": "${workspaceFolder}/build_vs/compile_commands.json",
        "problemsBridge.autoExport": True,
        "problemsBridge.outputPath": ".vscode/problems.export.json",
    }

    problems: List[str] = []
    for key, expected in required_keys.items():
        if key not in settings:
            problems.append(f"Missing key: {key}")
            continue
        if settings[key] != expected:
            problems.append(f"{key} is '{settings[key]}' (expected '{expected}')")

    if problems:
        return CheckResult(
            "vscode_settings",
            "WARN",
            "VS Code settings need alignment.",
            problems,
        )
    return CheckResult("vscode_settings", "PASS", "VS Code settings are aligned.", [])


def check_compile_commands(root: Path) -> CheckResult:
    settings = parse_json(root / ".vscode" / "settings.json") or {}
    presets = parse_json(root / "CMakePresets.json") or {}

    expected_path_setting = settings.get("C_Cpp.default.compileCommands")
    expected_path: Path | None = None
    if isinstance(expected_path_setting, str) and expected_path_setting:
        expected_str = expected_path_setting.replace("${workspaceFolder}", str(root))
        expected_path = Path(expected_str).resolve()

    candidate_paths = []
    if expected_path is not None:
        candidate_paths.append(expected_path)
    candidate_paths.extend(
        [
            root / "build_ninja" / "compile_commands.json",
            root / "build_vs" / "compile_commands.json",
            root / "cmake-build-debug" / "compile_commands.json",
        ]
    )
    # Preserve order while de-duplicating.
    deduped_paths: List[Path] = []
    seen = set()
    for p in candidate_paths:
        key = str(p)
        if key not in seen:
            seen.add(key)
            deduped_paths.append(p)

    cc_path = next((p for p in deduped_paths if p.exists()), None)
    if cc_path is None:
        is_vs_preset = False
        for preset in presets.get("configurePresets", []):
            generator = str(preset.get("generator", ""))
            if "Visual Studio" in generator:
                is_vs_preset = True
                break
        if is_vs_preset:
            return CheckResult(
                "compile_commands",
                "WARN",
                "No compile_commands.json found; valid for Visual Studio generator workflow.",
                [
                    "VS generators may not emit compile_commands.json.",
                    "Optional: generate via Ninja: cmake -S . -B build_ninja -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
                ],
            )
        return CheckResult(
            "compile_commands",
            "FAIL",
            "compile_commands.json not found.",
            ["Generate one: cmake -S . -B build_ninja -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"],
        )
    obj = parse_json(cc_path)
    if not isinstance(obj, list):
        return CheckResult("compile_commands", "FAIL", "compile_commands.json is invalid JSON list.", [])
    if len(obj) == 0:
        return CheckResult("compile_commands", "WARN", "compile_commands.json is empty.", [])

    has_std = False
    has_msvc_or_sdk_include = False
    has_any_include_flag = False
    for entry in obj:
        args = entry.get("arguments", [])
        if not args:
            command = entry.get("command", "")
            if isinstance(command, str) and command.strip():
                try:
                    args = shlex.split(command, posix=False)
                except ValueError:
                    args = command.split()
        if any(isinstance(a, str) and a.startswith("/std:") for a in args):
            has_std = True
        if any(isinstance(a, str) and a.startswith("-std=") for a in args):
            has_std = True
        if any(
            isinstance(a, str)
            and a.startswith("/I")
            and ("Microsoft Visual Studio" in a or "Windows Kits" in a)
            for a in args
        ):
            has_msvc_or_sdk_include = True
        if any(isinstance(a, str) and (a.startswith("/I") or a.startswith("-I")) for a in args):
            has_any_include_flag = True
        if has_std and (has_msvc_or_sdk_include or has_any_include_flag):
            break

    details: List[str] = [f"path={cc_path}", f"entries={len(obj)}"]
    if not has_std:
        details.append("No C++ standard flag found (/std: or -std=).")
    if not has_any_include_flag:
        details.append("No include flag found (/I or -I).")
    elif not has_msvc_or_sdk_include:
        details.append("No MSVC/Windows SDK include path found (valid for non-MSVC Ninja toolchains).")

    status = "PASS" if has_std and has_any_include_flag else "WARN"
    message = "compile_commands.json looks healthy." if status == "PASS" else "compile_commands.json may be incomplete."
    return CheckResult("compile_commands", status, message, details)


def check_toolchain(root: Path) -> CheckResult:
    cmd = ".vscode\\vsdev-init.cmd >NUL && set VSCMD_VER && where cl && where cmake && where msbuild"
    result = run_command(cmd, prefer_windows_shell=True)
    if result.returncode != 0:
        return CheckResult(
            "toolchain",
            "FAIL",
            "MSVC toolchain shell check failed.",
            [line.strip() for line in (result.stderr or result.stdout).splitlines() if line.strip()][:10],
        )

    lines = [line.strip() for line in result.stdout.splitlines() if line.strip()]
    has_vscmd_ver = any(
        line.startswith("VSCMD_VER=")
        and line not in {"VSCMD_VER=", "VSCMD_VER=%VSCMD_VER%"}
        for line in lines
    )
    has_cl = any(line.lower().endswith("cl.exe") for line in lines)
    has_cmake = any(line.lower().endswith("cmake.exe") for line in lines)
    has_msbuild = any(line.lower().endswith("msbuild.exe") for line in lines)

    status = "PASS" if all([has_vscmd_ver, has_cl, has_cmake, has_msbuild]) else "WARN"
    details = []
    if not has_vscmd_ver:
        details.append("VSCMD_VER missing")
    if not has_cl:
        details.append("cl.exe missing")
    if not has_cmake:
        details.append("cmake.exe missing")
    if not has_msbuild:
        details.append("msbuild.exe missing")
    if not details:
        details = lines[-4:]
    return CheckResult("toolchain", status, "Toolchain probe complete.", details)


def check_problems_bridge(root: Path) -> CheckResult:
    export_path = root / ".vscode" / "problems.export.json"
    if not export_path.exists():
        return CheckResult(
            "problems_bridge",
            "FAIL",
            "Problems export file not found.",
            ["Ensure VS Code extension 'local.vscode-problems-bridge' is installed and workspace reloaded."],
        )

    payload = parse_json(export_path)
    if payload is None:
        return CheckResult("problems_bridge", "FAIL", "problems.export.json is invalid JSON.", [])

    total = payload.get("totalCount")
    generated_at = payload.get("generatedAt")
    details = [f"totalCount={total}", f"generatedAt={generated_at}", f"path={export_path}"]
    status = "PASS" if isinstance(total, int) else "WARN"
    message = "Problems bridge export is readable." if status == "PASS" else "Problems bridge export has unexpected shape."
    return CheckResult("problems_bridge", status, message, details)


def check_code_cli() -> CheckResult:
    result = run_command("code --version", prefer_windows_shell=True)
    if result.returncode != 0:
        return CheckResult("code_cli", "WARN", "VS Code CLI not found in PATH.", [])
    version = (result.stdout.splitlines() or ["unknown"])[0]
    return CheckResult("code_cli", "PASS", "VS Code CLI available.", [f"version={version}"])


def render_human(results: List[CheckResult]) -> None:
    print("RogueCities Env Doctor")
    print("=" * 24)
    print(f"Host: {platform.system()} {platform.release()} | Python: {platform.python_version()}")
    print("")
    for item in results:
        print(f"[{item.status}] {item.name}: {item.message}")
        for detail in item.details:
            print(f"  - {detail}")
    print("")
    counts = {
        "PASS": sum(1 for r in results if r.status == "PASS"),
        "WARN": sum(1 for r in results if r.status == "WARN"),
        "FAIL": sum(1 for r in results if r.status == "FAIL"),
    }
    print(f"Summary: PASS={counts['PASS']} WARN={counts['WARN']} FAIL={counts['FAIL']}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--json", action="store_true", help="Emit machine-readable JSON.")
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[1]
    results = [
        check_workspace(root),
        check_vscode_settings(root),
        check_compile_commands(root),
        check_toolchain(root),
        check_problems_bridge(root),
        check_code_cli(),
    ]

    if args.json:
        payload = {
            "workspace": str(root),
            "results": [asdict(r) for r in results],
        }
        print(json.dumps(payload, indent=2))
    else:
        render_human(results)

    has_fail = any(r.status == "FAIL" for r in results)
    return 1 if has_fail else 0


if __name__ == "__main__":
    raise SystemExit(main())
