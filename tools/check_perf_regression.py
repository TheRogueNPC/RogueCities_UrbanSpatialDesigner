#!/usr/bin/env python3
"""Check performance report against configured budgets."""

from __future__ import annotations

import argparse
import json
import os
from dataclasses import dataclass


@dataclass
class Regression:
    scope: str
    name: str
    observed: float
    budget: float


@dataclass
class MissingBudgetEntry:
    scope: str
    name: str


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Validate performance report against budget file")
    parser.add_argument("--report", required=True, help="Path to generated performance_report.json")
    parser.add_argument("--budgets", required=True, help="Path to performance budget JSON")
    parser.add_argument("--summary-file", default="", help="Optional markdown summary output path")
    parser.add_argument("--fail-on-regression", action="store_true", help="Exit non-zero on regression")
    parser.add_argument("--fail-on-missing", action="store_true", help="Exit non-zero when budget data is missing")
    parser.add_argument(
        "--emit-github-annotations",
        action="store_true",
        help="Emit ::error/::warning annotations for GitHub Actions",
    )
    return parser.parse_args()


def load_json(path: str) -> dict:
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def check_regressions(report: dict, budgets: dict) -> tuple[list[Regression], list[MissingBudgetEntry]]:
    regressions: list[Regression] = []
    missing: list[MissingBudgetEntry] = []

    report_metrics = report.get("metrics_ms", {})
    budget_metrics = budgets.get("metric_budgets_ms", {})
    for metric_name, budget_value in budget_metrics.items():
        observed = report_metrics.get(metric_name)
        if observed is None:
            missing.append(MissingBudgetEntry(scope="metric", name=metric_name))
            continue
        observed_f = float(observed)
        budget_f = float(budget_value)
        if observed_f > budget_f:
            regressions.append(
                Regression(
                    scope="metric",
                    name=metric_name,
                    observed=observed_f,
                    budget=budget_f,
                )
            )

    tests = report.get("tests", [])
    tests_by_name = {row.get("name"): row for row in tests if row.get("name")}
    budget_tests = budgets.get("test_runtime_budgets_sec", {})
    for test_name, budget_value in budget_tests.items():
        row = tests_by_name.get(test_name)
        if row is None:
            missing.append(MissingBudgetEntry(scope="test", name=test_name))
            continue
        observed_f = float(row.get("seconds", 0.0))
        budget_f = float(budget_value)
        if observed_f > budget_f:
            regressions.append(
                Regression(
                    scope="test",
                    name=test_name,
                    observed=observed_f,
                    budget=budget_f,
                )
            )

    return regressions, missing


def build_summary(
    regressions: list[Regression],
    missing: list[MissingBudgetEntry],
    report_path: str,
    budgets_path: str,
) -> str:
    lines: list[str] = []
    lines.append("## Performance Regression Check")
    lines.append("")
    lines.append(f"- Report: `{report_path}`")
    lines.append(f"- Budgets: `{budgets_path}`")
    lines.append("")

    if regressions:
        lines.append("### Regressions")
        lines.append("| Scope | Name | Observed | Budget |")
        lines.append("|---|---|---:|---:|")
        for item in regressions:
            lines.append(
                f"| {item.scope} | `{item.name}` | {item.observed:.3f} | {item.budget:.3f} |"
            )
    else:
        lines.append("### Regressions")
        lines.append("No regressions detected.")

    lines.append("")
    if missing:
        lines.append("### Missing Data")
        lines.append("| Scope | Name |")
        lines.append("|---|---|")
        for item in missing:
            lines.append(f"| {item.scope} | `{item.name}` |")
    else:
        lines.append("### Missing Data")
        lines.append("No missing budgeted entries.")

    lines.append("")
    lines.append(f"Overall status: **{'FAIL' if regressions else 'PASS'}**")
    return "\n".join(lines) + "\n"


def maybe_write_summary(path: str, summary: str) -> None:
    if not path:
        return
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    with open(path, "a", encoding="utf-8") as f:
        f.write(summary)


def emit_annotations(regressions: list[Regression], missing: list[MissingBudgetEntry]) -> None:
    for item in regressions:
        print(
            f"::error title=Performance regression::{item.scope}:{item.name} "
            f"observed={item.observed:.3f} budget={item.budget:.3f}"
        )
    for item in missing:
        print(f"::warning title=Missing performance data::{item.scope}:{item.name}")


def main() -> int:
    args = parse_args()
    report = load_json(args.report)
    budgets = load_json(args.budgets)

    report_status = str(report.get("status", "unknown"))
    if report_status != "ok":
        message = f"Performance report status is '{report_status}' (expected 'ok')"
        print(message)
        if args.emit_github_annotations:
            print(f"::error title=Performance report invalid::{message}")
        if args.fail_on_regression:
            return 1

    regressions, missing = check_regressions(report, budgets)
    summary = build_summary(regressions, missing, args.report, args.budgets)
    print(summary, end="")
    maybe_write_summary(args.summary_file, summary)

    if args.emit_github_annotations:
        emit_annotations(regressions, missing)

    if args.fail_on_regression and regressions:
        return 1
    if args.fail_on_missing and missing:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
