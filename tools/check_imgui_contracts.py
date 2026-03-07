#!/usr/bin/env python3
"""ImGui contract guard for RogueCities visualizer UI layers.

Checks:
- DrawContent() functions must not open top-level windows.
- Loop interactive calls with static labels/ids should be ID-scoped (PushID) to
  avoid collisions.
- Legacy direct ImGuiIO state writes are forbidden in main loop.
- Panel interactive/widget actions must route through RC_UI::API, not raw ImGui.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PANELS_DIR = ROOT / "visualizer" / "src" / "ui" / "panels"
MAIN_GUI = ROOT / "visualizer" / "src" / "main_gui.cpp"
INPUT_GATE_CPP = ROOT / "visualizer" / "src" / "ui" / "rc_ui_input_gate.cpp"
VISUALIZER_SRC = ROOT / "visualizer" / "src"
KEYMAP_CPP = ROOT / "visualizer" / "src" / "ui" / "rc_ui_root.cpp"

FORBIDDEN_IN_DRAW_CONTENT = [
    (re.compile(r"\bImGui::Begin\s*\("), "DrawContent() must not call ImGui::Begin()."),
    (re.compile(r"\bComponents::BeginTokenPanel\s*\("), "DrawContent() must not open token panels."),
    (re.compile(r"\bRC_UI::BeginDockableWindow\s*\("), "DrawContent() must not open dockable windows."),
    (re.compile(r"\bAPI::BeginPanel\s*\("), "DrawContent() must not open API panels."),
]

LEGACY_IO_ASSIGNMENTS = [
    re.compile(r"\bio\.MousePos\s*="),
    re.compile(r"\bio\.MouseDown\s*\["),
    re.compile(r"\bio\.KeyCtrl\s*="),
    re.compile(r"\bio\.KeyShift\s*="),
    re.compile(r"\bio\.KeyAlt\s*="),
]

WIDGET_IN_LOOP = re.compile(
    r"\b(?:ImGui::(?:Button|SmallButton|InvisibleButton|Selectable|Checkbox|RadioButton|InputText|InputTextMultiline|TreeNode|BeginCombo|Combo|CollapsingHeader|MenuItem)"
    r"|API::(?:Button|DragFloat|DragInt|SliderScalar|ActionButton|SectionHeader))\s*\(\s*\"([^\"]+)\""
)

WIDGET_WITH_LITERAL_LABEL = re.compile(
    r"\b(?:ImGui::(?:Button|SmallButton|InvisibleButton|Selectable|Checkbox|RadioButton|InputText|InputTextMultiline|TreeNode|BeginCombo|Combo|CollapsingHeader|MenuItem)"
    r"|API::(?:Button|DragFloat|DragInt|SliderScalar|ActionButton|SectionHeader))\s*\(\s*\"([^\"]+)\""
)


API_INCLUDE = re.compile(r'#include\s+"ui/api/rc_imgui_api\.h"')
FORBIDDEN_RAW_WIDGET_CALLS = [
    re.compile(r"\bImGui::Button\s*\("),
    re.compile(r"\bImGui::SmallButton\s*\("),
    re.compile(r"\bImGui::InvisibleButton\s*\("),
    re.compile(r"\bImGui::Checkbox\s*\("),
    re.compile(r"\bImGui::RadioButton\s*\("),
    re.compile(r"\bImGui::Selectable\s*\("),
    re.compile(r"\bImGui::InputText\s*\("),
    re.compile(r"\bImGui::InputTextMultiline\s*\("),
    re.compile(r"\bImGui::InputInt\s*\("),
    re.compile(r"\bImGui::InputFloat\s*\("),
    re.compile(r"\bImGui::InputFloat2\s*\("),
    re.compile(r"\bImGui::InputScalar\s*\("),
    re.compile(r"\bImGui::DragFloat\s*\("),
    re.compile(r"\bImGui::DragInt\s*\("),
    re.compile(r"\bImGui::SliderFloat\s*\("),
    re.compile(r"\bImGui::SliderInt\s*\("),
    re.compile(r"\bImGui::SliderScalar\s*\("),
    re.compile(r"\bImGui::SliderAngle\s*\("),
    re.compile(r"\bImGui::Combo\s*\("),
    re.compile(r"\bImGui::BeginCombo\s*\("),
    re.compile(r"\bImGui::EndCombo\s*\("),
    re.compile(r"\bImGui::CollapsingHeader\s*\("),
    re.compile(r"\bImGui::MenuItem\s*\("),
    re.compile(r"\bImGui::TextDisabled\s*\("),
    re.compile(r"\bImGui::SetNextItemWidth\s*\("),
    re.compile(r"\bImGui::Indent\s*\("),
    re.compile(r"\bImGui::Unindent\s*\("),
    re.compile(r"\bImGui::BeginDisabled\s*\("),
    re.compile(r"\bImGui::EndDisabled\s*\("),
    re.compile(r"\bImGui::Spacing\s*\("),
    re.compile(r"\bImGui::Separator\s*\("),
    re.compile(r"\bImGui::SameLine\s*\("),
    re.compile(r"\bImGui::OpenPopup\s*\("),
    re.compile(r"\bImGui::BeginPopup\s*\("),
    re.compile(r"\bImGui::BeginPopupModal\s*\("),
    re.compile(r"\bImGui::EndPopup\s*\("),
    re.compile(r"\bImGui::CloseCurrentPopup\s*\("),
]

FORBIDDEN_DIRECT_KEY_QUERIES = [
    re.compile(r"\bImGui::IsKeyPressed\s*\("),
    re.compile(r"\bImGui::IsKeyDown\s*\("),
]


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


def check_duplicate_labels_in_draw_content(path: Path, text: str) -> list[str]:
    lines = text.splitlines()
    violations: list[str] = []

    in_draw_content = False
    brace_depth = 0
    waiting_for_open_brace = False
    id_scope_depth = 0
    seen_labels: dict[str, int] = {}

    for idx, line in enumerate(lines, start=1):
        if not in_draw_content:
            if re.search(r"\bDrawContent\s*\(", line):
                waiting_for_open_brace = True

        if waiting_for_open_brace and "{" in line:
            in_draw_content = True
            waiting_for_open_brace = False
            brace_depth = line.count("{") - line.count("}")
            id_scope_depth = 0
            seen_labels.clear()
            if brace_depth <= 0:
                in_draw_content = False
            continue

        if in_draw_content:
            id_scope_depth += line.count("PushID(")
            id_scope_depth -= line.count("PopID(")
            if id_scope_depth < 0:
                id_scope_depth = 0

            match = WIDGET_WITH_LITERAL_LABEL.search(line)
            if match:
                label = match.group(1)
                if "##" not in label and id_scope_depth == 0:
                    first_line = seen_labels.get(label)
                    if first_line is None:
                        seen_labels[label] = idx
                    else:
                        violations.append(
                            f"{rel(path)}:{idx}: Duplicate widget label '{label}' in DrawContent() without ID scope (first at line {first_line})."
                        )

            brace_depth += line.count("{") - line.count("}")
            if brace_depth <= 0:
                in_draw_content = False

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


def check_panel_widget_wrapper_contract(path: Path, text: str) -> list[str]:
    violations: list[str] = []
    if "API::" in text and not API_INCLUDE.search(text):
        violations.append(
            f"{rel(path)}: Panel uses API:: calls but is missing #include \"ui/api/rc_imgui_api.h\"."
        )

    lines = text.splitlines()
    for idx, line in enumerate(lines, start=1):
        for pattern in FORBIDDEN_RAW_WIDGET_CALLS:
            if pattern.search(line):
                violations.append(
                    f"{rel(path)}:{idx}: Raw ImGui widget call detected; use RC_UI::API wrappers (or API::Mutate for explicit exceptions)."
                )
                break
    return violations


def check_keymap_query_contract() -> list[str]:
    violations: list[str] = []
    if not VISUALIZER_SRC.exists():
        return violations

    allowed = {KEYMAP_CPP.resolve()}
    for path in sorted(VISUALIZER_SRC.rglob("*")):
        if not path.is_file() or path.suffix not in {".cpp", ".h"}:
            continue
        if path.resolve() in allowed:
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        lines = text.splitlines()
        for idx, line in enumerate(lines, start=1):
            for pattern in FORBIDDEN_DIRECT_KEY_QUERIES:
                if pattern.search(line):
                    violations.append(
                        f"{rel(path)}:{idx}: Direct key query detected; route keyboard checks through RC_UI::Keymap."
                    )
                    break
    return violations


def main() -> int:
    violations: list[str] = []

    for path in iter_panel_sources():
        text = path.read_text(encoding="utf-8", errors="replace")
        violations.extend(check_draw_content_contracts(path, text))
        violations.extend(check_loop_id_safety(path, text))
        violations.extend(check_duplicate_labels_in_draw_content(path, text))
        violations.extend(check_panel_widget_wrapper_contract(path, text))

    violations.extend(check_legacy_io_writes())
    violations.extend(check_input_gate_contract())
    violations.extend(check_keymap_query_contract())

    if violations:
        print("ImGui contract violations detected:")
        for v in violations:
            print(f"  - {v}")
        return 1

    print("ImGui contract check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
