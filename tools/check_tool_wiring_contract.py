#!/usr/bin/env python3
"""Contract checks for catalog-driven tool wiring.

Validates that every tool action is cataloged and dispatchable (or explicitly disabled).
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTRACT_H = ROOT / "visualizer" / "src" / "ui" / "tools" / "rc_tool_contract.h"
CONTRACT_CPP = ROOT / "visualizer" / "src" / "ui" / "tools" / "rc_tool_contract.cpp"
DISPATCH_CPP = ROOT / "visualizer" / "src" / "ui" / "tools" / "rc_tool_dispatcher.cpp"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def parse_action_enum(header_text: str) -> list[str]:
    match = re.search(r"enum\s+class\s+ToolActionId\s*:\s*\w+\s*\{(?P<body>.*?)\};", header_text, re.S)
    if not match:
        return []

    values: list[str] = []
    for raw_line in match.group("body").splitlines():
        line = raw_line.split("//", 1)[0].strip()
        if not line:
            continue
        line = line.rstrip(",")
        if "=" in line:
            line = line.split("=", 1)[0].strip()
        if line:
            values.append(line)
    return [v for v in values if v != "Count"]


def parse_catalog(contract_cpp_text: str) -> dict[str, tuple[str, str]]:
    # action -> (availability, disabled_reason)
    pattern = re.compile(
        r"\{\s*ToolActionId::(?P<action>\w+)\s*,"
        r".*?ToolAvailability::(?P<availability>\w+)\s*,\s*\"(?P<reason>[^\"]*)\"\s*\}",
        re.S,
    )

    catalog: dict[str, tuple[str, str]] = {}
    for match in pattern.finditer(contract_cpp_text):
        action = match.group("action")
        availability = match.group("availability")
        reason = match.group("reason")
        catalog[action] = (availability, reason)
    return catalog


def parse_dispatch_cases(dispatch_cpp_text: str) -> set[str]:
    return set(re.findall(r"case\s+ToolActionId::(\w+)\s*:", dispatch_cpp_text))


def main() -> int:
    violations: list[str] = []

    for required in (CONTRACT_H, CONTRACT_CPP, DISPATCH_CPP):
        if not required.exists():
            violations.append(f"Missing required file: {required.relative_to(ROOT).as_posix()}")

    if violations:
        print("Tool wiring contract violations detected:")
        for item in violations:
            print(f"  - {item}")
        return 1

    enum_actions = parse_action_enum(read(CONTRACT_H))
    catalog = parse_catalog(read(CONTRACT_CPP))
    dispatch_cases = parse_dispatch_cases(read(DISPATCH_CPP))

    if not enum_actions:
        violations.append("Failed to parse ToolActionId enum values.")

    enum_set = set(enum_actions)
    catalog_set = set(catalog.keys())

    missing_from_catalog = sorted(enum_set - catalog_set)
    if missing_from_catalog:
        violations.append("Actions missing from catalog: " + ", ".join(missing_from_catalog))

    orphan_catalog = sorted(catalog_set - enum_set)
    if orphan_catalog:
        violations.append("Catalog contains unknown actions: " + ", ".join(orphan_catalog))

    for action in sorted(enum_set & catalog_set):
        availability, disabled_reason = catalog[action]
        if availability == "Available" and action not in dispatch_cases:
            violations.append(f"Available action '{action}' has no dispatch case.")
        if availability == "Disabled" and not disabled_reason.strip():
            violations.append(f"Disabled action '{action}' is missing a disabled reason.")

    unexpected_dispatch = sorted((dispatch_cases - enum_set) - {"Count"})
    if unexpected_dispatch:
        violations.append("Dispatch switch contains unknown actions: " + ", ".join(unexpected_dispatch))

    if violations:
        print("Tool wiring contract violations detected:")
        for item in violations:
            print(f"  - {item}")
        return 1

    print("Tool wiring contract check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
