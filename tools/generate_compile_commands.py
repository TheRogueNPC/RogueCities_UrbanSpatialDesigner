#!/usr/bin/env python3
"""Generate compile_commands.json from CMake File API target replies.

Visual Studio generators do not emit compile_commands.json. This script
converts target reply JSON files under build_vs/.cmake/api/v1/reply into a
portable compile database for clangd/cpptools.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Dict, List


WINDOWS_DRIVE_RE = re.compile(r"^([A-Za-z]):/(.*)$")


def _norm_slashes(text: str) -> str:
    return text.replace("\\", "/")


def _windows_to_wsl(path: str) -> str:
    match = WINDOWS_DRIVE_RE.match(path)
    if not match:
        return path
    drive, remainder = match.groups()
    return f"/mnt/{drive.lower()}/{remainder}"


def _read_cmake_home_directory(reply_dir: Path) -> str | None:
    cache_files = sorted(reply_dir.glob("cache-v2-*.json"), reverse=True)
    for cache_file in cache_files:
        obj = json.loads(cache_file.read_text(encoding="utf-8"))
        for entry in obj.get("entries", []):
            if entry.get("name") == "CMAKE_HOME_DIRECTORY":
                value = entry.get("value")
                if isinstance(value, str) and value:
                    return _norm_slashes(value)
    return None


def _resolve_path(raw_path: str, repo_root: Path, source_hint: str | None) -> str:
    path = _norm_slashes(raw_path)

    # Project-local relative paths are the most common in target reply files.
    if not WINDOWS_DRIVE_RE.match(path) and not Path(path).is_absolute():
        return (repo_root / path).as_posix()

    # Convert known source-root-prefixed Windows paths to local workspace paths.
    if source_hint:
        if path == source_hint:
            return repo_root.as_posix()
        source_prefix = f"{source_hint}/"
        if path.startswith(source_prefix):
            suffix = path[len(source_prefix) :]
            return (repo_root / suffix).as_posix()

    # On WSL, map C:/... -> /mnt/c/... when possible.
    mapped = _windows_to_wsl(path)
    if mapped != path and Path(mapped).exists():
        return Path(mapped).as_posix()

    # Keep original absolute path when no safe mapping exists.
    return path


def _split_fragment(fragment: str) -> List[str]:
    # VS fragments are shell-like and may contain quoted tokens.
    return [token for token in shlex.split(fragment, posix=False) if token]


def _normalize_msvc_flag(token: str) -> str:
    if token.startswith("-std:c++"):
        return f"/std:{token[len('-std:'):]}"
    if token in {"-MD", "-MDd", "-MT", "-MTd"}:
        return f"/{token[1:]}"
    return token


def _is_unsupported_clangd_flag(token: str) -> bool:
    # clangd commonly fails on CMake/MSVC precompiled-header object flags.
    return token.startswith("/Yu") or token.startswith("/Fp")


def _collect_msvc_include_dirs(repo_root: Path) -> List[str]:
    include_value = os.environ.get("INCLUDE", "")

    # Fallback: query INCLUDE from a dev shell if current environment lacks it.
    if not include_value and shutil.which("cmd.exe"):
        try:
            cmd = [
                "cmd.exe",
                "/d",
                "/c",
                ".vscode\\vsdev-init.cmd >NUL && set INCLUDE",
            ]
            result = subprocess.run(
                cmd,
                cwd=str(repo_root),
                capture_output=True,
                text=True,
                check=False,
            )
            for line in result.stdout.splitlines():
                if line.upper().startswith("INCLUDE="):
                    include_value = line.split("=", 1)[1]
                    break
        except OSError:
            pass

    paths: List[str] = []
    seen = set()
    for raw in include_value.split(";"):
        candidate = _norm_slashes(raw.strip())
        if not candidate:
            continue
        if candidate in seen:
            continue
        seen.add(candidate)
        paths.append(candidate)
    return paths


def _build_arguments(compile_group: dict, source_file: str, toolchain_includes: List[str]) -> List[str]:
    language = compile_group.get("language", "CXX")
    arguments: List[str] = ["cl.exe"]

    for fragment in compile_group.get("compileCommandFragments", []):
        text = fragment.get("fragment")
        if isinstance(text, str) and text.strip():
            for token in _split_fragment(text):
                token = _normalize_msvc_flag(token)
                if _is_unsupported_clangd_flag(token):
                    continue
                arguments.append(token)

    include_paths = []
    seen_includes = set()
    for define in compile_group.get("defines", []):
        value = define.get("define")
        if isinstance(value, str) and value:
            arguments.append(f"/D{value}")

    for include in compile_group.get("includes", []):
        path = include.get("path")
        if isinstance(path, str) and path:
            normalized = _norm_slashes(path)
            if normalized in seen_includes:
                continue
            seen_includes.add(normalized)
            include_paths.append(normalized)

    for include in toolchain_includes:
        normalized = _norm_slashes(include)
        if normalized in seen_includes:
            continue
        seen_includes.add(normalized)
        include_paths.append(normalized)

    for include in include_paths:
        arguments.append(f"/I{include}")

    # Keep language explicit for extension-less generated sources.
    arguments.append("/TC" if language == "C" else "/TP")
    arguments.append("/c")
    arguments.append(source_file)
    return arguments


def generate_compile_database(build_dir: Path, config: str, output: Path) -> int:
    reply_dir = build_dir / ".cmake" / "api" / "v1" / "reply"
    if not reply_dir.exists():
        print(f"[ERROR] Missing CMake reply directory: {reply_dir}", file=sys.stderr)
        return 2

    repo_root = Path(__file__).resolve().parents[1]
    source_hint = _read_cmake_home_directory(reply_dir)
    toolchain_includes = _collect_msvc_include_dirs(repo_root)
    pattern = f"target-*-{config}-*.json" if config else "target-*.json"

    entries_by_file: Dict[str, dict] = {}

    for target_file in sorted(reply_dir.glob(pattern)):
        data = json.loads(target_file.read_text(encoding="utf-8"))
        compile_groups = data.get("compileGroups", [])
        sources = data.get("sources", [])
        if not compile_groups or not sources:
            continue

        for source in sources:
            source_path = source.get("path")
            group_index = source.get("compileGroupIndex")
            if not isinstance(source_path, str) or not isinstance(group_index, int):
                continue
            if not (0 <= group_index < len(compile_groups)):
                continue

            resolved_source = _resolve_path(source_path, repo_root, source_hint)
            if resolved_source in entries_by_file:
                continue

            compile_group = dict(compile_groups[group_index])
            normalized_includes = []
            for include in compile_group.get("includes", []):
                item = dict(include)
                raw_include = item.get("path")
                if isinstance(raw_include, str):
                    item["path"] = _resolve_path(raw_include, repo_root, source_hint)
                normalized_includes.append(item)
            compile_group["includes"] = normalized_includes

            entries_by_file[resolved_source] = {
                "directory": repo_root.as_posix(),
                "file": resolved_source,
                "arguments": _build_arguments(compile_group, resolved_source, toolchain_includes),
            }

    if not entries_by_file:
        print(
            f"[ERROR] No compile entries found in {reply_dir} for pattern '{pattern}'.",
            file=sys.stderr,
        )
        return 3

    output.parent.mkdir(parents=True, exist_ok=True)
    ordered = [entries_by_file[key] for key in sorted(entries_by_file)]
    output.write_text(json.dumps(ordered, indent=2), encoding="utf-8")
    print(f"[OK] Wrote {len(ordered)} entries to {output.as_posix()}")
    return 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--build-dir",
        default="build_vs",
        help="CMake build directory containing .cmake/api/v1/reply",
    )
    parser.add_argument(
        "--config",
        default="Debug",
        help="Configuration to extract from target reply files (Debug/Release/etc).",
    )
    parser.add_argument(
        "--output",
        default=None,
        help="Output path for compile_commands.json (defaults to <build-dir>/compile_commands.json).",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    build_dir = Path(args.build_dir).resolve()
    output = Path(args.output).resolve() if args.output else build_dir / "compile_commands.json"
    return generate_compile_database(build_dir, args.config, output)


if __name__ == "__main__":
    raise SystemExit(main())
