#!/usr/bin/env python3
"""Contract checks for unified viewport context command system."""

from __future__ import annotations

import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]

REQUIRED_FILES = [
    ROOT / "visualizer" / "src" / "ui" / "commands" / "rc_context_command_registry.h",
    ROOT / "visualizer" / "src" / "ui" / "commands" / "rc_context_command_registry.cpp",
    ROOT / "visualizer" / "src" / "ui" / "commands" / "rc_context_menu_smart.h",
    ROOT / "visualizer" / "src" / "ui" / "commands" / "rc_context_menu_smart.cpp",
    ROOT / "visualizer" / "src" / "ui" / "commands" / "rc_context_menu_pie.h",
    ROOT / "visualizer" / "src" / "ui" / "commands" / "rc_context_menu_pie.cpp",
    ROOT / "visualizer" / "src" / "ui" / "commands" / "rc_command_palette.h",
    ROOT / "visualizer" / "src" / "ui" / "commands" / "rc_command_palette.cpp",
    ROOT / "visualizer" / "src" / "ui" / "panels" / "rc_panel_axiom_editor.cpp",
]


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def main() -> int:
    violations: list[str] = []

    for path in REQUIRED_FILES:
        if not path.exists():
            violations.append(f"Missing required file: {rel(path)}")

    if violations:
        print("Context command contract violations detected:")
        for violation in violations:
            print(f"  - {violation}")
        return 1

    registry_cpp = read(ROOT / "visualizer" / "src" / "ui" / "commands" / "rc_context_command_registry.cpp")
    smart_cpp = read(ROOT / "visualizer" / "src" / "ui" / "commands" / "rc_context_menu_smart.cpp")
    pie_cpp = read(ROOT / "visualizer" / "src" / "ui" / "commands" / "rc_context_menu_pie.cpp")
    palette_cpp = read(ROOT / "visualizer" / "src" / "ui" / "commands" / "rc_command_palette.cpp")
    panel_cpp = read(ROOT / "visualizer" / "src" / "ui" / "panels" / "rc_panel_axiom_editor.cpp")

    if "GetToolActionCatalog" not in registry_cpp:
        violations.append("Command registry must source entries from tool action catalog (GetToolActionCatalog).")

    for name, text in (
        ("smart menu", smart_cpp),
        ("pie menu", pie_cpp),
        ("command palette", palette_cpp),
    ):
        if "ExecuteCommand(" not in text:
            violations.append(f"{name} must execute actions through Commands::ExecuteCommand().")
        if "DispatchToolAction(" in text:
            violations.append(f"{name} bypasses registry by calling DispatchToolAction() directly.")

    required_panel_tokens = [
        "RequestOpenSmartMenu",
        "RequestOpenPieMenu",
        "RequestOpenCommandPalette",
        "DrawSmartMenu",
        "DrawPieMenu",
        "DrawCommandPalette",
        "viewport_context_default_mode",
        "viewport_hotkey_space_enabled",
        "viewport_hotkey_slash_enabled",
        "viewport_hotkey_grave_enabled",
        "viewport_hotkey_p_enabled",
    ]
    for token in required_panel_tokens:
        if token not in panel_cpp:
            violations.append(f"Viewport panel missing command integration token: {token}")

    if violations:
        print("Context command contract violations detected:")
        for violation in violations:
            print(f"  - {violation}")
        return 1

    print("Context command contract check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
