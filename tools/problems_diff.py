#!/usr/bin/env python3
"""Diff two VS Code problems export snapshots."""

from __future__ import annotations

import argparse
import json
import shutil
from collections import Counter
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, Iterable, List, Set


@dataclass(frozen=True)
class DiagKey:
    file_path: str
    severity: str
    source: str
    code: str
    message: str
    line: int
    character: int


def load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def flatten_keys(payload: dict) -> Set[DiagKey]:
    out: Set[DiagKey] = set()
    for file_item in payload.get("files", []):
        file_path = str(file_item.get("filePath", ""))
        for diag in file_item.get("diagnostics", []):
            start = diag.get("range", {}).get("start", {})
            out.add(
                DiagKey(
                    file_path=file_path,
                    severity=str(diag.get("severity", "Unknown")),
                    source=str(diag.get("source", "")),
                    code=str(diag.get("code", "")),
                    message=str(diag.get("message", "")),
                    line=int(start.get("line", 0)),
                    character=int(start.get("character", 0)),
                )
            )
    return out


def pick_latest_snapshot(history_dir: Path) -> Path | None:
    if not history_dir.exists():
        return None
    candidates = sorted(history_dir.glob("problems_*.json"))
    if not candidates:
        return None
    return candidates[-1]


def save_snapshot(current: Path, history_dir: Path) -> Path:
    history_dir.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S")
    target = history_dir / f"problems_{stamp}.json"
    shutil.copy2(current, target)
    return target


def severity_counter(keys: Iterable[DiagKey]) -> Dict[str, int]:
    c = Counter(k.severity for k in keys)
    order = ["Error", "Warning", "Information", "Hint", "Unknown"]
    return {name: c[name] for name in order if c[name] > 0}


def render_list(title: str, rows: List[DiagKey], limit: int) -> None:
    print(title)
    if not rows:
        print("  - none")
        return
    for i, row in enumerate(rows[:limit], start=1):
        print(
            f"  {i:02d}. {row.severity} {row.file_path}:{row.line}:{row.character} "
            f"[{row.source}:{row.code}] {row.message}"
        )
    if len(rows) > limit:
        print(f"  ... {len(rows) - limit} more")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--current", default=".vscode/problems.export.json")
    parser.add_argument("--baseline", default="", help="Optional baseline snapshot path.")
    parser.add_argument("--history-dir", default=".vscode/problems-history")
    parser.add_argument("--snapshot-current", action="store_true")
    parser.add_argument("--limit", type=int, default=15)
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()

    current_path = Path(args.current)
    if not current_path.exists():
        print(f"[ERROR] Current diagnostics file not found: {current_path}")
        return 2

    history_dir = Path(args.history_dir)
    baseline_path = Path(args.baseline) if args.baseline else pick_latest_snapshot(history_dir)
    if baseline_path is None or not baseline_path.exists():
        print("[WARN] No baseline snapshot found. Use --snapshot-current to create one.")
        if args.snapshot_current:
            snap = save_snapshot(current_path, history_dir)
            print(f"[OK] Snapshot created: {snap}")
        return 0

    current_payload = load_json(current_path)
    baseline_payload = load_json(baseline_path)
    current_keys = flatten_keys(current_payload)
    baseline_keys = flatten_keys(baseline_payload)

    added = sorted(current_keys - baseline_keys, key=lambda k: (k.file_path, k.line, k.character, k.message))
    removed = sorted(baseline_keys - current_keys, key=lambda k: (k.file_path, k.line, k.character, k.message))
    unchanged = current_keys & baseline_keys

    summary = {
        "current": str(current_path),
        "baseline": str(baseline_path),
        "counts": {
            "current": len(current_keys),
            "baseline": len(baseline_keys),
            "added": len(added),
            "removed": len(removed),
            "unchanged": len(unchanged),
            "delta": len(current_keys) - len(baseline_keys),
        },
        "severity": {
            "current": severity_counter(current_keys),
            "baseline": severity_counter(baseline_keys),
            "added": severity_counter(added),
            "removed": severity_counter(removed),
        },
        "added": [
            {
                "filePath": k.file_path,
                "severity": k.severity,
                "source": k.source,
                "code": k.code,
                "line": k.line,
                "character": k.character,
                "message": k.message,
            }
            for k in added
        ],
        "removed": [
            {
                "filePath": k.file_path,
                "severity": k.severity,
                "source": k.source,
                "code": k.code,
                "line": k.line,
                "character": k.character,
                "message": k.message,
            }
            for k in removed
        ],
    }

    if args.json:
        print(json.dumps(summary, indent=2))
    else:
        c = summary["counts"]
        print("Problems Diff")
        print("=" * 13)
        print(f"Baseline: {baseline_path}")
        print(f"Current : {current_path}")
        print(f"Counts  : baseline={c['baseline']} current={c['current']} delta={c['delta']:+d}")
        print(f"Changes : added={c['added']} removed={c['removed']} unchanged={c['unchanged']}")
        print("")
        render_list("Added diagnostics:", added, max(1, args.limit))
        print("")
        render_list("Removed diagnostics:", removed, max(1, args.limit))

    if args.snapshot_current:
        snap = save_snapshot(current_path, history_dir)
        print(f"[OK] Snapshot created: {snap}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
