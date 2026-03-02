#!/usr/bin/env python3
"""One-click environment refresh:
configure -> compile_commands -> build -> problems diff -> triage.
"""

from __future__ import annotations

import argparse
import os
import shlex
import shutil
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path


def run(args: list[str], prefer_windows_shell: bool = False) -> int:
    # On Windows, prefer_windows_shell doesn't change much for list-based subprocess.run
    # On non-Windows, if prefer_windows_shell is requested, we try to use cmd.exe if available
    # but we should not fail if it is missing unless it is strictly necessary.
    # Given the environment, it's better to only use cmd.exe if we are actually on Windows or
    # if it's explicitly available.
    if prefer_windows_shell and os.name != "nt" and shutil.which("cmd.exe"):
        command_str = shlex.join(args)
        wrapped = ["cmd.exe", "/d", "/c", command_str]
    else:
        wrapped = args
    print(f"[RUN] {shlex.join(args)}", flush=True)
    proc = subprocess.run(wrapped, shell=False, check=False)
    return proc.returncode


def snapshot_file(path: Path, out_dir: Path, prefix: str) -> Path | None:
    if not path.exists():
        return None
    out_dir.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S")
    target = out_dir / f"{prefix}_{stamp}.json"
    shutil.copy2(path, target)
    return target


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--configure-preset", default="dev")
    parser.add_argument("--build-preset", default="gui-release")
    parser.add_argument("--compile-config", default="Debug")
    parser.add_argument("--skip-build", action="store_true")
    parser.add_argument("--skip-problems", action="store_true")
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[1]
    os.chdir(root)

    problems_path = root / ".vscode" / "problems.export.json"
    history_dir = root / ".vscode" / "problems-history"
    before_snapshot = snapshot_file(problems_path, history_dir, "before_refresh")

    rc = run(["cmake", "--preset", args.configure_preset], prefer_windows_shell=True)
    if rc != 0:
        print("[FAIL] Configure step failed.")
        return rc

    gen_args_base = ["tools/generate_compile_commands.py", "--build-dir", "build_vs", "--config", args.compile_config]
    # Try py -3 first, then fallback to python
    rc = run(["py", "-3"] + gen_args_base, prefer_windows_shell=True)
    if rc != 0:
        rc = run(["python"] + gen_args_base, prefer_windows_shell=True)

    if rc != 0:
        print("[FAIL] compile_commands generation failed.")
        return rc

    if not args.skip_build:
        rc = run(["cmake", "--build", "--preset", args.build_preset], prefer_windows_shell=True)
        if rc != 0:
            print("[FAIL] Build step failed.")
            return rc

    if args.skip_problems:
        print("[OK] Refresh complete (problems analysis skipped).")
        return 0

    if not problems_path.exists():
        print(
            "[WARN] .vscode/problems.export.json is missing. "
            "Open VS Code and run command: 'Problems Bridge: Export Diagnostics Now'."
        )
        return 0

    if before_snapshot:
        diff_args = [
            sys.executable, "tools/problems_diff.py",
            "--baseline", str(before_snapshot),
            "--current", ".vscode/problems.export.json",
            "--snapshot-current"
        ]
    else:
        diff_args = [
            sys.executable, "tools/problems_diff.py",
            "--current", ".vscode/problems.export.json",
            "--snapshot-current"
        ]
    run(diff_args, prefer_windows_shell=False)

    triage_args = [
        sys.executable, "tools/problems_triage.py",
        "--input", ".vscode/problems.export.json",
        "--max-items", "8"
    ]
    run(triage_args, prefer_windows_shell=False)

    print("[OK] Refresh complete.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
