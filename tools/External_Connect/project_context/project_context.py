# File: project_context.py
# Project context and Rogue Protocol validation for RogueCities
#
# Provides:
# - Rogue Protocol rules (FVA, SIV, CIV, RogueWorker)
# - Layer architecture validation
# - Code validation against project conventions

import re
from pathlib import Path
from typing import Dict, Any, List, Optional, Tuple
from dataclasses import dataclass, field


@dataclass
class LayerRule:
    name: str
    allowed_imports: List[str]
    forbidden_imports: List[str]
    forbidden_patterns: List[str]


@dataclass
class VectorType:
    name: str
    description: str
    use_cases: List[str]
    stability: str


LAYER_RULES: Dict[str, LayerRule] = {
    "Core": LayerRule(
        name="Core",
        allowed_imports=[
            "System",
            "System.Collections.Generic",
            "System.Numerics",
            "Godot",
        ],
        forbidden_imports=["ImGui", "OpenGL", "Silk.NET", "OpenTK"],
        forbidden_patterns=[
            r"ImGui\.",
            r"GL\.",
            r"OpenGL",
            r"Silk\.NET",
            r"OpenTK",
        ],
    ),
    "Generators": LayerRule(
        name="Generators",
        allowed_imports=["Core", "System", "Godot"],
        forbidden_imports=["ImGui", "OpenGL"],
        forbidden_patterns=[
            r"ImGui\.",
            r"GL\.",
        ],
    ),
    "Visualizer": LayerRule(
        name="Visualizer",
        allowed_imports=["Core", "Generators", "ImGui", "OpenGL", "System", "Godot"],
        forbidden_imports=[],
        forbidden_patterns=[],
    ),
}

VECTOR_TYPES: Dict[str, VectorType] = {
    "FVA": VectorType(
        name="FastVectorArray",
        description="Stable handles for UI/Editor references",
        use_cases=[
            "Road Segments",
            "Districts",
            "Any entity referenced by UI/Editor",
        ],
        stability="Stable indices across frame boundaries",
    ),
    "SIV": VectorType(
        name="StableIndexVector",
        description="Safety for high-churn entities with validity tracking",
        use_cases=[
            "Buildings",
            "Agents",
            "Props",
            "Dynamic entities that may be destroyed",
        ],
        stability="Stable with validity checks - safe for async operations",
    ),
    "CIV": VectorType(
        name="IndexVector",
        description="Raw speed for internal calculations",
        use_cases=[
            "Internal algorithms",
            "Temporary calculations",
            "One-frame operations",
        ],
        stability="Unstable - indices may change between frames",
    ),
}

ROGUE_WORKER_THRESHOLD_MS = 10

VALIDATION_RULES = [
    {
        "id": "no_imgui_in_core",
        "severity": "error",
        "pattern": r"ImGui\.",
        "files": r"Core/.*\.cs$",
        "message": "ImGui references not allowed in Core layer",
    },
    {
        "id": "no_opengl_in_core",
        "severity": "error",
        "pattern": r"(GL\.|OpenGL|Silk\.NET|OpenTK)",
        "files": r"Core/.*\.cs$",
        "message": "OpenGL references not allowed in Core layer",
    },
    {
        "id": "civ_ui_reference",
        "severity": "warning",
        "pattern": r"CIV.*UI|UI.*CIV|IndexVector.*Editor|Editor.*IndexVector",
        "files": r".*\.cs$",
        "message": "CIV/IndexVector should not be used for UI/Editor references - use FVA or SIV",
    },
    {
        "id": "long_operation_sync",
        "severity": "warning",
        "pattern": r"(for\s*\(|while\s*\().{500,}",
        "files": r".*\.cs$",
        "message": "Long-running operations should use RogueWorker threading",
    },
]


def detect_layer(file_path: str) -> Optional[str]:
    path = Path(file_path)
    parts = [p.lower() for p in path.parts]

    for layer in ["Core", "Generators", "Visualizer"]:
        if layer.lower() in parts:
            return layer

    for i, part in enumerate(parts):
        if part in ["core", "generators", "visualizer"]:
            return part.capitalize()

    return None


def validate_file_content(content: str, file_path: str) -> List[Dict[str, Any]]:
    violations = []
    layer = detect_layer(file_path)

    if layer and layer in LAYER_RULES:
        rule = LAYER_RULES[layer]

        for pattern in rule.forbidden_patterns:
            matches = list(re.finditer(pattern, content, re.IGNORECASE))
            for match in matches:
                line_num = content[: match.start()].count("\n") + 1
                violations.append(
                    {
                        "rule_id": f"layer_{layer.lower()}_forbidden",
                        "severity": "error",
                        "line": line_num,
                        "message": f"Layer violation: {pattern} not allowed in {layer}",
                        "context": content.split("\n")[line_num - 1].strip()[:100]
                        if line_num <= len(content.split("\n"))
                        else "",
                    }
                )

    for rule in VALIDATION_RULES:
        if re.search(rule["files"], file_path, re.IGNORECASE):
            matches = list(
                re.finditer(rule["pattern"], content, re.IGNORECASE | re.DOTALL)
            )
            for match in matches[:3]:
                line_num = content[: match.start()].count("\n") + 1
                violations.append(
                    {
                        "rule_id": rule["id"],
                        "severity": rule["severity"],
                        "line": line_num,
                        "message": rule["message"],
                        "context": content.split("\n")[line_num - 1].strip()[:100]
                        if line_num <= len(content.split("\n"))
                        else "",
                    }
                )

    return violations


def suggest_vector_type(entity_type: str, use_case: str) -> Dict[str, Any]:
    entity_lower = entity_type.lower()
    use_case_lower = use_case.lower()

    if any(kw in entity_lower for kw in ["road", "segment", "district", "zone"]):
        return {
            "recommended": "FVA",
            "reason": "Road Segments and Districts need stable handles for UI/Editor",
            "alternative": "SIV if destruction is possible",
        }

    if any(
        kw in entity_lower for kw in ["building", "agent", "prop", "vehicle", "npc"]
    ):
        return {
            "recommended": "SIV",
            "reason": "High-churn entities need validity tracking",
            "alternative": "FVA if never destroyed",
        }

    if any(
        kw in use_case_lower
        for kw in ["internal", "calculation", "temporary", "algorithm"]
    ):
        return {
            "recommended": "CIV",
            "reason": "Internal calculations don't need stable handles",
            "alternative": "SIV if used across frames",
        }

    if any(kw in use_case_lower for kw in ["ui", "editor", "selection", "display"]):
        return {
            "recommended": "FVA",
            "reason": "UI/Editor references need stable indices",
            "alternative": "SIV for safety-critical selections",
        }

    return {
        "recommended": "SIV",
        "reason": "Default choice - provides safety with reasonable performance",
        "alternative": "FVA for stable entities, CIV for pure internal use",
    }


def get_project_context() -> Dict[str, Any]:
    return {
        "rogue_protocol": {
            "vector_types": {
                name: {
                    "name": vt.name,
                    "description": vt.description,
                    "use_cases": vt.use_cases,
                    "stability": vt.stability,
                }
                for name, vt in VECTOR_TYPES.items()
            },
            "rogue_worker": {
                "threshold_ms": ROGUE_WORKER_THRESHOLD_MS,
                "description": "Operations exceeding this duration should use RogueWorker threading",
            },
        },
        "layers": {
            name: {
                "name": rule.name,
                "allowed_imports": rule.allowed_imports,
                "forbidden_imports": rule.forbidden_imports,
                "forbidden_patterns": rule.forbidden_patterns,
            }
            for name, rule in LAYER_RULES.items()
        },
        "validation_rules": VALIDATION_RULES,
    }


def check_threading_need(operation_duration_ms: float) -> Dict[str, Any]:
    if operation_duration_ms < ROGUE_WORKER_THRESHOLD_MS:
        return {
            "needs_threading": False,
            "reason": f"Operation ({operation_duration_ms}ms) below threshold ({ROGUE_WORKER_THRESHOLD_MS}ms)",
        }
    return {
        "needs_threading": True,
        "reason": f"Operation ({operation_duration_ms}ms) exceeds threshold ({ROGUE_WORKER_THRESHOLD_MS}ms)",
        "recommendation": "Use RogueWorker for this operation",
    }
