---
name: AiTelemetrySkill
description: Skill for instantly debugging Compile Time and Run Time errors in the RogueCities Urban Spatial Designer using the AI Telemetry JSON dumps.
---

# AI Telemetry Debugging Skill

This skill explains how to respond to developer queries such as "the build failed", "the visualizer crashed", or "there's an assertion violation" efficiently without asking for log extracts.

## 1. Run-Time Crashes (`runtime_crash.json`)

If the user reports a hard crash, an assertion failure, or a sudden shutdown, the telemetry sink (`RogueCity::Core::Validation::AiTelemetry`) has likely caught the error context.

**ACTION**:
Use `view_file` to read `AI/diagnostics/runtime_crash.json`.

**JSON Structure**:
```json
{
  "event": "fatal_assertion",
  "timestamp": "2026-02-27T16:45:00Z",
  "module": "RogueCityGenerators",
  "file": "src/Core/Roads/RoadMDP.cpp",
  "line": 142,
  "message": "Boost Geometry: intersection invalid. Tensor bounds exceeded."
}
```

Once parsed, immediately jump to the specified `file` at `line` using `view_file`, read the condition, and formulate your repair plan.

## 2. Compile-Time Errors (`last_compile_error.json`)

If the user states "it didn't compile," use the background telemetry dump from the compile tracker (if available).

**ACTION**:
Use `view_file` to read `AI/diagnostics/last_compile_error.json` or `.vscode/problems.export.json` if a direct dump isn't available. Use `rc-problems` via PowerShell.

## 3. UI Misalignment (`ui_snapshot.json`)

If the user complains about "bad layout" or "missing UI elements":

**ACTION**:
Read `AI/diagnostics/ui_snapshot.json` or `visualizer/RC_UI_Mockup_state.html`. Locate the conflicting ImGui IDs, bounds overlap, or absent node structures. Check `PanelRegistry.cpp` to ensure the correct module is loaded.
