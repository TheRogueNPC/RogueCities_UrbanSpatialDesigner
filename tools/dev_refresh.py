#!/usr/bin/env python3
"""One-click environment refresh:
configure -> compile_commands -> build -> problems diff -> triage.
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path


def run(command: str, prefer_windows_shell: bool = False) -> int:
    if prefer_windows_shell and os.name != "nt":
        wrapped = f'cmd.exe /d /c "{command}"'
    else:
        wrapped = command
    print(f"[RUN] {command}", flush=True)
    proc = subprocess.run(wrapped, shell=True, check=False)
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

    rc = run(f"cmake --preset {args.configure_preset}", prefer_windows_shell=True)
    if rc != 0:
        print("[FAIL] Configure step failed.")
        return rc

    gen_cmd = (
        f"(py -3 tools\\generate_compile_commands.py --build-dir build_vs --config {args.compile_config} "
        f"|| python tools\\generate_compile_commands.py --build-dir build_vs --config {args.compile_config})"
    )
    rc = run(gen_cmd, prefer_windows_shell=True)
    if rc != 0:
        print("[FAIL] compile_commands generation failed.")
        return rc

    if not args.skip_build:
        rc = run(f"cmake --build --preset {args.build_preset}", prefer_windows_shell=True)
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
        diff_cmd = (
            f"{sys.executable} tools/problems_diff.py --baseline \"{before_snapshot}\" "
            f"--current .vscode/problems.export.json --snapshot-current"
        )
    else:
        diff_cmd = f"{sys.executable} tools/problems_diff.py --current .vscode/problems.export.json --snapshot-current"
    run(diff_cmd, prefer_windows_shell=False)

    triage_cmd = f"{sys.executable} tools/problems_triage.py --input .vscode/problems.export.json --max-items 8"
    run(triage_cmd, prefer_windows_shell=False)

    print("[OK] Refresh complete.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
