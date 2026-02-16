#!/usr/bin/env python3
"""ImGui contract guard for RogueCities visualizer UI layers.

Checks:
- DrawContent() functions must not open top-level windows.
- Loop widgets with static labels should be ID-scoped (PushID) to avoid collisions.
- Legacy direct ImGuiIO state writes are forbidden in main loop.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PANELS_DIR = ROOT / "visualizer" / "src" / "ui" / "panels"
MAIN_GUI = ROOT / "visualizer" / "src" / "main_gui.cpp"
INPUT_GATE_CPP = ROOT / "visualizer" / "src" / "ui" / "rc_ui_input_gate.cpp"

FORBIDDEN_IN_DRAW_CONTENT = [
    (re.compile(r"\bImGui::Begin\s*\("), "DrawContent() must not call ImGui::Begin()."),
    (re.compile(r"\bComponents::BeginTokenPanel\s*\("), "DrawContent() must not open token panels."),
    (re.compile(r"\bRC_UI::BeginDockableWindow\s*\("), "DrawContent() must not open dockable windows."),
]

LEGACY_IO_ASSIGNMENTS = [
    re.compile(r"\bio\.MousePos\s*="),
    re.compile(r"\bio\.MouseDown\s*\["),
    re.compile(r"\bio\.KeyCtrl\s*="),
    re.compile(r"\bio\.KeyShift\s*="),
    re.compile(r"\bio\.KeyAlt\s*="),
]

WIDGET_IN_LOOP = re.compile(
    r"\bImGui::(?:Button|SmallButton|InvisibleButton|Selectable|Checkbox|RadioButton|InputText|InputTextMultiline|TreeNode|BeginCombo)\s*\(\s*\"([^\"]+)\""
)


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def iter_panel_sources() -> list[Path]:
    if not PANELS_DIR.exists():
        return []
    return sorted(PANELS_DIR.glob("*.cpp"))


def check_draw_content_contracts(path: Path, text: str) -> list[str]:
    lines = text.splitlines()
    violations: list[str] = []

    in_draw_content = False
    brace_depth = 0
    waiting_for_open_brace = False

    for idx, line in enumerate(lines, start=1):
        if not in_draw_content:
            if re.search(r"\bDrawContent\s*\(", line):
                waiting_for_open_brace = True

        if waiting_for_open_brace and "{" in line:
            in_draw_content = True
            waiting_for_open_brace = False
            brace_depth = line.count("{") - line.count("}")
            if brace_depth <= 0:
                in_draw_content = False
            continue

        if in_draw_content:
            for pattern, message in FORBIDDEN_IN_DRAW_CONTENT:
                if pattern.search(line):
                    violations.append(f"{rel(path)}:{idx}: {message}")

            brace_depth += line.count("{") - line.count("}")
            if brace_depth <= 0:
                in_draw_content = False

    return violations


def check_loop_id_safety(path: Path, text: str) -> list[str]:
    lines = text.splitlines()
    violations: list[str] = []

    loop_stack: list[dict[str, int | bool]] = []
    brace_depth = 0

    for idx, line in enumerate(lines, start=1):
        stripped = line.strip()

        if re.search(r"\bfor\s*\(", stripped):
            loop_stack.append({"depth": brace_depth, "has_push_id": False})

        if loop_stack and "PushID" in line:
            loop_stack[-1]["has_push_id"] = True

        if loop_stack:
            match = WIDGET_IN_LOOP.search(line)
            if match:
                label = match.group(1)
                if "##" not in label:
                    has_id_scope = any(bool(loop["has_push_id"]) for loop in loop_stack)
                    if not has_id_scope:
                        violations.append(
                            f"{rel(path)}:{idx}: Widget '{label}' in loop appears without nearby PushID()."
                        )

        brace_depth += line.count("{") - line.count("}")

        while loop_stack and brace_depth <= int(loop_stack[-1]["depth"]):
            loop_stack.pop()

    return violations


def check_legacy_io_writes() -> list[str]:
    if not MAIN_GUI.exists():
        return []

    lines = MAIN_GUI.read_text(encoding="utf-8", errors="replace").splitlines()
    violations: list[str] = []
    for idx, line in enumerate(lines, start=1):
        for pattern in LEGACY_IO_ASSIGNMENTS:
            if pattern.search(line):
                violations.append(
                    f"{rel(MAIN_GUI)}:{idx}: Legacy direct io.* write detected; use io.Add*Event() API."
                )
                break
    return violations


def check_input_gate_contract() -> list[str]:
    if not INPUT_GATE_CPP.exists():
        return [f"Missing required file: {rel(INPUT_GATE_CPP)}"]

    text = INPUT_GATE_CPP.read_text(encoding="utf-8", errors="replace")
    violations: list[str] = []

    if "!state.imgui_wants_mouse" in text or "!io.WantCaptureMouse" in text:
        violations.append(
            f"{rel(INPUT_GATE_CPP)}: viewport-local gate must not hard-block on WantCaptureMouse."
        )

    return violations


def main() -> int:
    violations: list[str] = []

    for path in iter_panel_sources():
        text = path.read_text(encoding="utf-8", errors="replace")
        violations.extend(check_draw_content_contracts(path, text))
        violations.extend(check_loop_id_safety(path, text))

    violations.extend(check_legacy_io_writes())
    violations.extend(check_input_gate_contract())

    if violations:
        print("ImGui contract violations detected:")
        for v in violations:
            print(f"  - {v}")
        return 1

    print("ImGui contract check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
