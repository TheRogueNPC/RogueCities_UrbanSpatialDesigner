#!/usr/bin/env python3
"""UI compliance guard for RogueCities visualizer panels.

Fails the build when panel files bypass the token/wrapper architecture.
"""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PANELS_DIR = ROOT / "visualizer" / "src" / "ui" / "panels"


FORBIDDEN_PATTERNS = [
    (re.compile(r"\bImGui::Begin\s*\("), "Use `Components::BeginTokenPanel()` or `RC_UI::BeginDockableWindow()`."),
    (re.compile(r"\bIM_COL32\s*\("), "Use `UITokens` (or token helpers), never literal IM_COL32 in panels."),
    (
        re.compile(r"\bImGui::PushStyleColor\s*\(\s*ImGuiCol_WindowBg\b"),
        "Window background styling must come from panel wrappers/tokens, not raw PushStyleColor.",
    ),
]


def iter_panel_sources() -> list[Path]:
    if not PANELS_DIR.exists():
        return []
    return sorted(PANELS_DIR.glob("*.cpp"))


def main() -> int:
    violations: list[str] = []

    for path in iter_panel_sources():
        text = path.read_text(encoding="utf-8", errors="replace")
        for pattern, guidance in FORBIDDEN_PATTERNS:
            for match in pattern.finditer(text):
                line = text.count("\n", 0, match.start()) + 1
                rel = path.relative_to(ROOT).as_posix()
                violations.append(f"{rel}:{line}: {guidance}")

    if violations:
        print("UI compliance violations detected:")
        for entry in violations:
            print(f"  - {entry}")
        return 1

    tool_contract_script = ROOT / "tools" / "check_tool_wiring_contract.py"
    if tool_contract_script.exists():
        result = subprocess.run(
            [sys.executable, str(tool_contract_script)],
            cwd=str(ROOT),
            capture_output=True,
            text=True,
            check=False,
        )
        if result.stdout.strip():
            print(result.stdout.strip())
        if result.stderr.strip():
            print(result.stderr.strip())
        if result.returncode != 0:
            return result.returncode

    print("UI compliance check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
