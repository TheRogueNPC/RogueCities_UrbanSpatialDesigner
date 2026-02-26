#!/usr/bin/env python3
"""Contract checks for generator->viewport ownership boundaries."""

from __future__ import annotations

import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]

APPLIER_H = ROOT / "app" / "include" / "RogueCity" / "App" / "Integration" / "CityOutputApplier.hpp"
APPLIER_CPP = ROOT / "app" / "src" / "Integration" / "CityOutputApplier.cpp"
COORD_H = ROOT / "app" / "include" / "RogueCity" / "App" / "Integration" / "GenerationCoordinator.hpp"
COORD_CPP = ROOT / "app" / "src" / "Integration" / "GenerationCoordinator.cpp"
VIEWPORT_PANEL = ROOT / "visualizer" / "src" / "ui" / "panels" / "rc_panel_axiom_editor.cpp"
APP_CMAKE = ROOT / "app" / "CMakeLists.txt"
VISUALIZER_CMAKE = ROOT / "visualizer" / "CMakeLists.txt"
VIEWPORT_INTERACTION_CPP = ROOT / "visualizer" / "src" / "ui" / "viewport" / "rc_viewport_interaction.cpp"
TOOL_METRICS_H = ROOT / "visualizer" / "src" / "ui" / "tools" / "rc_tool_interaction_metrics.h"
TOOL_METRICS_CPP = ROOT / "visualizer" / "src" / "ui" / "tools" / "rc_tool_interaction_metrics.cpp"
TOOL_GEOM_POLICY_H = ROOT / "app" / "include" / "RogueCity" / "App" / "Tools" / "GeometryPolicy.hpp"
TOOL_GEOM_POLICY_CPP = ROOT / "app" / "src" / "Tools" / "GeometryPolicy.cpp"
GLOBAL_STATE_H = ROOT / "core" / "include" / "RogueCity" / "Core" / "Editor" / "GlobalState.hpp"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def main() -> int:
    violations: list[str] = []

    for required in (
        APPLIER_H,
        APPLIER_CPP,
        COORD_H,
        COORD_CPP,
        VIEWPORT_PANEL,
        VIEWPORT_INTERACTION_CPP,
        TOOL_METRICS_H,
        TOOL_METRICS_CPP,
        TOOL_GEOM_POLICY_H,
        TOOL_GEOM_POLICY_CPP,
        GLOBAL_STATE_H,
        APP_CMAKE,
        VISUALIZER_CMAKE,
    ):
        if not required.exists():
            violations.append(f"Missing required file: {rel(required)}")

    if violations:
        print("Generator/viewport contract violations detected:")
        for violation in violations:
            print(f"  - {violation}")
        return 1

    panel_text = read(VIEWPORT_PANEL)
    interaction_text = read(VIEWPORT_INTERACTION_CPP)
    global_state_text = read(GLOBAL_STATE_H)
    cmake_text = read(APP_CMAKE)
    visualizer_cmake_text = read(VISUALIZER_CMAKE)

    if "SyncGlobalStateFromPreview" in panel_text:
        violations.append("rc_panel_axiom_editor.cpp still owns SyncGlobalStateFromPreview; must use CityOutputApplier.")
    if "ApplyCityOutputToGlobalState" not in panel_text:
        violations.append("rc_panel_axiom_editor.cpp must call ApplyCityOutputToGlobalState().")
    if "GenerationCoordinator" not in panel_text:
        violations.append("rc_panel_axiom_editor.cpp must use GenerationCoordinator.")
    if "std::unique_ptr<RogueCity::App::RealTimePreview> s_preview" in panel_text:
        violations.append("rc_panel_axiom_editor.cpp still stores direct RealTimePreview ownership.")
    if "ForceRegeneration(" not in panel_text or "RequestRegeneration(" not in panel_text:
        violations.append("rc_panel_axiom_editor.cpp must route generation requests through coordinator API.")
    if "ProcessNonAxiomViewportInteraction(" not in panel_text:
        violations.append("rc_panel_axiom_editor.cpp must route non-axiom viewport interaction through viewport utilities.")
    if "if (!axiom_mode && allow_viewport_mouse_actions)" in panel_text:
        violations.append("Legacy panel-local non-axiom interaction block detected; must use ProcessNonAxiomViewportInteraction().")
    if "##ViewportCanvas" not in panel_text:
        violations.append("rc_panel_axiom_editor.cpp must define canonical viewport canvas item (##ViewportCanvas).")
    if "viewport_canvas_hovered" not in panel_text:
        violations.append("rc_panel_axiom_editor.cpp must derive hover state from viewport canvas item.")
    if "BuildUiInputGateState(" not in panel_text or "viewport_canvas_active" not in panel_text:
        violations.append("rc_panel_axiom_editor.cpp must pass viewport canvas state into BuildUiInputGateState().")

    if "ProcessNonAxiomViewportInteraction(" not in interaction_text:
        violations.append("rc_viewport_interaction.cpp missing ProcessNonAxiomViewportInteraction() implementation.")
    if "HandleDomainPlacementActions(" not in interaction_text:
        violations.append("rc_viewport_interaction.cpp must include domain placement routing.")
    if "BuildToolInteractionMetrics(" not in interaction_text:
        violations.append("rc_viewport_interaction.cpp must use BuildToolInteractionMetrics().")
    if "ResolveToolGeometryPolicy(" not in interaction_text:
        violations.append("rc_viewport_interaction.cpp must use ResolveToolGeometryPolicy().")
    if "requires_explicit_generation" not in interaction_text:
        violations.append("rc_viewport_interaction.cpp must expose explicit-generation requirement in interaction result.")

    if "GenerationMutationPolicy" not in global_state_text:
        violations.append("GlobalState.hpp must define GenerationMutationPolicy.")
    if "GenerationPolicyState" not in global_state_text:
        violations.append("GlobalState.hpp must define domain generation policy mapping.")
    if "ToolDomain::Axiom" not in global_state_text or "LiveDebounced" not in global_state_text:
        violations.append("GlobalState.hpp must map Axiom domain to LiveDebounced generation policy.")

    if "src/Integration/CityOutputApplier.cpp" not in cmake_text:
        violations.append("app/CMakeLists.txt missing CityOutputApplier.cpp.")
    if "src/Integration/GenerationCoordinator.cpp" not in cmake_text:
        violations.append("app/CMakeLists.txt missing GenerationCoordinator.cpp.")
    if "src/Tools/GeometryPolicy.cpp" not in cmake_text:
        violations.append("app/CMakeLists.txt missing GeometryPolicy.cpp.")
    if "src/ui/tools/rc_tool_interaction_metrics.cpp" not in visualizer_cmake_text:
        violations.append("visualizer/CMakeLists.txt missing rc_tool_interaction_metrics.cpp.")

    if violations:
        print("Generator/viewport contract violations detected:")
        for violation in violations:
            print(f"  - {violation}")
        return 1

    print("Generator/viewport contract check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
