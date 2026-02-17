#!/usr/bin/env python3
"""Contract checks for clang/parser safety and builder-facing invariants."""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HEADER_ROOTS = [
    ROOT / "core" / "include" / "RogueCity",
    ROOT / "generators" / "include" / "RogueCity",
    ROOT / "app" / "include" / "RogueCity",
]
GLOBAL_STATE_H = ROOT / "core" / "include" / "RogueCity" / "Core" / "Editor" / "GlobalState.hpp"

ALLOW_DEFAULT_CONFIG_MARKER = "contract-allow-default-config-arg"

DEFAULT_CONFIG_ARG_RE = re.compile(
    r"const\s+(?:[A-Za-z_]\w*::)*Config\s*&\s*[A-Za-z_]\w*\s*=\s*(?:[A-Za-z_]\w*::)*Config\s*\{\s*\}",
    flags=re.MULTILINE,
)
FVA_USAGE_RE = re.compile(r"\bfva::")
FAST_INCLUDE_RE = re.compile(r'#include\s+"[^"]*(FastVectorArray\.hpp|fast_array\.hpp)"')

GLOBAL_STATE_REQUIRED = [
    '#include "RogueCity/Core/Util/FastVectorArray.hpp"  // IWYU pragma: keep',
    '#include "RogueCity/Core/Util/IndexVector.hpp"  // IWYU pragma: keep',
    '#include "RogueCity/Core/Util/StableIndexVector.hpp"  // IWYU pragma: keep',
]


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def iter_headers() -> list[Path]:
    out: list[Path] = []
    for root in HEADER_ROOTS:
        if not root.exists():
            continue
        out.extend(sorted(root.rglob("*.h")))
        out.extend(sorted(root.rglob("*.hpp")))
    return out


def line_for_offset(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def line_text(text: str, line: int) -> str:
    lines = text.splitlines()
    idx = max(0, min(len(lines) - 1, line - 1))
    return lines[idx] if lines else ""


def check_default_config_args(violations: list[str]) -> None:
    for path in iter_headers():
        text = path.read_text(encoding="utf-8", errors="replace")
        for match in DEFAULT_CONFIG_ARG_RE.finditer(text):
            line = line_for_offset(text, match.start())
            if ALLOW_DEFAULT_CONFIG_MARKER in line_text(text, line):
                continue
            violations.append(
                f"{rel(path)}:{line}: default Config reference args are forbidden; "
                "declare an overload and keep the config-taking overload explicit."
            )


def check_fva_direct_include(violations: list[str]) -> None:
    for path in iter_headers():
        text = path.read_text(encoding="utf-8", errors="replace")
        if not FVA_USAGE_RE.search(text):
            continue
        if FAST_INCLUDE_RE.search(text):
            continue
        violations.append(
            f"{rel(path)}: uses fva:: but does not include FastVectorArray header directly."
        )


def check_global_state_invariants(violations: list[str]) -> None:
    if not GLOBAL_STATE_H.exists():
        violations.append(f"Missing required file: {rel(GLOBAL_STATE_H)}")
        return

    text = GLOBAL_STATE_H.read_text(encoding="utf-8", errors="replace")
    for required in GLOBAL_STATE_REQUIRED:
        if required not in text:
            violations.append(
                f"{rel(GLOBAL_STATE_H)}: missing required include invariant: {required}"
            )


def main() -> int:
    violations: list[str] = []
    check_default_config_args(violations)
    check_fva_direct_include(violations)
    check_global_state_invariants(violations)

    if violations:
        print("Clang/builder contract violations detected:")
        for violation in violations:
            print(f"  - {violation}")
        return 1

    print("Clang/builder contract check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
