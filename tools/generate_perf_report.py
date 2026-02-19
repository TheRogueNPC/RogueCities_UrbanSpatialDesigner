#!/usr/bin/env python3
"""Generate a JSON performance report from a CTest log."""

from __future__ import annotations

import argparse
import json
import os
import re
from datetime import datetime, timezone


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate performance report from ctest output")
    parser.add_argument("--ctest-log", required=True, help="Path to LastTest.log")
    parser.add_argument("--output", required=True, help="Output JSON path")
    return parser.parse_args()


def parse_ctest_log(path: str) -> dict:
    if not os.path.exists(path):
        return {
            "status": "missing_log",
            "tests": [],
            "note": f"ctest log not found: {path}",
        }

    test_rows = []
    tests_by_name = {}
    row_patterns = (
        re.compile(
            r"^\s*\d+/\d+\s+Test\s+#\d+:\s+([^\s]+)\s+\.+\s+([^\s]+)\s+([0-9.]+)\s+sec",
            re.MULTILINE,
        ),
        re.compile(
            r"Test\s+#\d+:\s+([^\s]+)\s+\.+\s+([^\s]+)\s+([0-9.]+)\s+sec",
            re.MULTILINE,
        ),
    )
    metric_pattern = re.compile(r"PERF_METRIC\s+([A-Za-z0-9_]+)\s+([0-9]+(?:\.[0-9]+)?)")

    with open(path, "r", encoding="utf-8", errors="replace") as f:
        content = f.read()

    for row_pattern in row_patterns:
        for name, result, seconds in row_pattern.findall(content):
            tests_by_name[name] = {
                "name": name,
                "result": result,
                "seconds": float(seconds),
            }

    if tests_by_name:
        test_rows = sorted(tests_by_name.values(), key=lambda row: row["name"])

    metric_samples = []
    metrics = {}
    for metric_name, metric_value in metric_pattern.findall(content):
        parsed = float(metric_value)
        metric_samples.append(
            {
                "name": metric_name,
                "value_ms": parsed,
            }
        )
        metrics[metric_name] = parsed

    return {
        "status": "ok",
        "tests": test_rows,
        "total_tests": len(test_rows),
        "metrics_ms": metrics,
        "metric_samples_ms": metric_samples,
    }


def main() -> int:
    args = parse_args()
    report = parse_ctest_log(args.ctest_log)
    report["generated_at_utc"] = datetime.now(timezone.utc).isoformat()

    os.makedirs(os.path.dirname(args.output) or ".", exist_ok=True)
    with open(args.output, "w", encoding="utf-8") as f:
        json.dump(report, f, indent=2)
        f.write("\n")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
