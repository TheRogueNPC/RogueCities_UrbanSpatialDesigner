#!/usr/bin/env python3
"""Summarize and prioritize VS Code problems export diagnostics."""

from __future__ import annotations

import argparse
import json
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Tuple


SEVERITY_WEIGHT = {
    "Error": 3,
    "Warning": 2,
    "Information": 1,
    "Hint": 0,
}


@dataclass(frozen=True)
class FlatDiagnostic:
    file_path: str
    severity: str
    source: str
    code: str
    message: str
    line: int
    character: int


def load_export(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def flatten(payload: dict) -> List[FlatDiagnostic]:
    rows: List[FlatDiagnostic] = []
    for file_item in payload.get("files", []):
        file_path = str(file_item.get("filePath", ""))
        for diag in file_item.get("diagnostics", []):
            start = diag.get("range", {}).get("start", {})
            rows.append(
                FlatDiagnostic(
                    file_path=file_path,
                    severity=str(diag.get("severity", "Unknown")),
                    source=str(diag.get("source", "")),
                    code=str(diag.get("code", "")),
                    message=str(diag.get("message", "")),
                    line=int(start.get("line", 0)),
                    character=int(start.get("character", 0)),
                )
            )
    return rows


def category_for(row: FlatDiagnostic) -> Tuple[str, str]:
    code = row.code or ""
    source = row.source or ""
    msg = row.message or ""

    if "unused-includes" in code:
        return ("unused_includes", "Run include cleanup (mostly low-risk).")
    if source == "cSpell":
        return ("spelling_dictionary", "Add project terms to cSpell dictionary.")
    if "default_member_initializer_not_yet_parsed" in code:
        return (
            "clang_parser_msvc_mismatch",
            "Likely clang-vs-MSVC parse mismatch; verify build, then restart VS Code/clangd and re-export diagnostics.",
        )
    if "-Wswitch" in code:
        return ("switch_exhaustiveness", "Handle missing enum values or add intentional default.")
    if "note_pragma_pack_here" in code:
        return ("pragma_pack_note", "Informational note only; pair with associated parser warning.")

    key = f"{source}:{code}" if source or code else "uncategorized"
    return (key, "Review manually.")


def summarize(rows: List[FlatDiagnostic]) -> dict:
    severity_counts: Dict[str, int] = defaultdict(int)
    file_counts: Dict[str, int] = defaultdict(int)
    category_counts: Dict[str, int] = defaultdict(int)
    category_files: Dict[str, set] = defaultdict(set)
    category_samples: Dict[str, FlatDiagnostic] = {}
    category_actions: Dict[str, str] = {}

    for row in rows:
        severity_counts[row.severity] += 1
        file_counts[row.file_path] += 1
        category, action = category_for(row)
        category_counts[category] += 1
        category_files[category].add(row.file_path)
        category_actions[category] = action
        if category not in category_samples:
            category_samples[category] = row

    sorted_categories = sorted(
        category_counts.keys(),
        key=lambda c: (
            -max(SEVERITY_WEIGHT.get(r.severity, 0) for r in rows if category_for(r)[0] == c),
            -category_counts[c],
            c,
        ),
    )

    categories = []
    for category in sorted_categories:
        sample = category_samples[category]
        worst_sev = max(
            SEVERITY_WEIGHT.get(r.severity, 0)
            for r in rows
            if category_for(r)[0] == category
        )
        categories.append(
            {
                "category": category,
                "count": category_counts[category],
                "affectedFiles": len(category_files[category]),
                "worstSeverity": worst_sev,
                "action": category_actions[category],
                "sample": {
                    "filePath": sample.file_path,
                    "line": sample.line,
                    "character": sample.character,
                    "severity": sample.severity,
                    "source": sample.source,
                    "code": sample.code,
                    "message": sample.message,
                },
            }
        )

    top_files = sorted(file_counts.items(), key=lambda kv: (-kv[1], kv[0]))
    return {
        "total": len(rows),
        "severityCounts": dict(sorted(severity_counts.items(), key=lambda kv: (-SEVERITY_WEIGHT.get(kv[0], 0), kv[0]))),
        "topFiles": [{"filePath": p, "count": c} for p, c in top_files],
        "categories": categories,
    }


def render(summary: dict, max_items: int) -> None:
    print("Problems Triage")
    print("=" * 15)
    print(f"Total diagnostics: {summary['total']}")
    print("Severity counts:")
    for sev, count in summary["severityCounts"].items():
        print(f"  - {sev}: {count}")

    print("")
    print("Top files:")
    for item in summary["topFiles"][:max_items]:
        print(f"  - {item['count']:>3}  {item['filePath']}")

    print("")
    print("Root-cause buckets:")
    for item in summary["categories"][:max_items]:
        s = item["sample"]
        print(f"  - {item['category']} ({item['count']} issues, {item['affectedFiles']} files)")
        print(f"    action: {item['action']}")
        print(
            "    sample: "
            f"{s['severity']} {s['filePath']}:{s['line']}:{s['character']} "
            f"[{s['source']}:{s['code']}] {s['message']}"
        )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", default=".vscode/problems.export.json")
    parser.add_argument("--max-items", type=int, default=10)
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--write", default="", help="Optional output path for triage JSON.")
    args = parser.parse_args()

    path = Path(args.input)
    if not path.exists():
        print(f"[ERROR] Input file not found: {path}")
        return 2

    payload = load_export(path)
    rows = flatten(payload)
    summary = summarize(rows)

    if args.write:
        out_path = Path(args.write)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text(json.dumps(summary, indent=2), encoding="utf-8")

    if args.json:
        print(json.dumps(summary, indent=2))
    else:
        render(summary, max(1, args.max_items))

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
