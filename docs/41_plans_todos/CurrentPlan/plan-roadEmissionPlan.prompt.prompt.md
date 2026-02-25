Plan: Understanding and Enhancing Road Emission and Classification

TL;DR
The road system involves a tracing pipeline that emits `Core::Road` objects into `fva::Container<Road>`; classification is a two-stage process (graph/topology scoring + AESP mapping). This document captures the investigation plan to trace emission, define road attributes, link types to AESP axes, and identify integration points.

Steps

1. Understand Road Emission
- Inspect `CityGenerator` pipeline for `traceRoads()` / `RoadGenerator::generate()` to find where `Core::Road` objects are created and added to `CityOutput`.
- Locate graph-to-road conversion points and ensure stable id allocation logic (AssignEntityId / CityOutputApplier).

2. Define What Constitutes a Road
- Review `core/include/RogueCity/Core/Data/CityTypes.hpp` `struct Road` to record fields and invariants (`points`, `type`, `id`, `source_axiom_id`, `is_user_created`, `generation_tag`, `generation_locked`).
- Note geometry representation (`std::vector<Vec2> points`) and any expectations for emptiness/size.

3. Link Roads to Classification
- Trace `RoadClassifier` usage: `classifyNetwork`, `classifyGraph`, `classifyRoad`, `classifyScore`, and `bestAespType`.
- Inspect `generators/include/.../AESPClassifier.hpp` and `generators/src/.../AESPClassifier.cpp` for ACCESS/EXPOSURE/SERVICEABILITY/PRIVACY tables.
- Identify `RogueProfiler` adapters that map RoadType → A/E/S/P and district classification calls.

4. Analyze Runtime Storage & UI Consumers
- Confirm `GlobalState::roads` type (should be `fva::Container<Road>`) and selection handles (e.g., `fva::Handle<Road>`).
- Enumerate visualizer consumers: viewport overlays (`RenderRoadLabels`), editor tools (road spline tool, `AxiomEditor::ClearRoads`), and viewport interactions that create/edit roads.

5. Tests, Risks, and Compatibility
- Find and run tests that exercise AESP and road generation (`tests/test_aesp_scoring_profiles.cpp`, `test_city_generator_validation`, `tests/test_viewport_*`).
- Document compatibility risks: `RoadType` enum ordering, `road_type_count`, AESP table indexing, `Road` field changes, `fva` requirement for stable handles, serialization/Lua bindings.

Verification
- Create small unit tests that assert AESP lookup table indices map to specific `RoadType` enum values.
- Add a round-trip serialization test for `Core::Road` fields used in preservation/merge logic.
- Run generator validation targets and viewport interaction tests after any change.

Follow-ups (after investigation)
- If `RoadType` semantics need change: add explicit mapping tests, update `AESPClassifier` tables, and update consumers.
- If storage refactor needed: implement adapter preserving `fva::Container<Road>` semantics and handle mapping.
- Add visualization overlays for classification confidence and AESP axis indicators to aid tuning.

Notes
- Preserve `id`, `source_axiom_id`, `generation_tag`, and `generation_locked` semantics when making changes.
- Avoid replacing `fva::Container<Road>` in `GlobalState` without an adapter.

Files referenced (primary)
- `core/include/RogueCity/Core/Data/CityTypes.hpp` — `struct Road`, `enum class RoadType`
- `generators/src/Generators/Urban/RoadGenerator.cpp` — emission loop
- `generators/src/Generators/Roads/RoadClassifier.cpp` — classification pipeline
- `generators/include/.../AESPClassifier.hpp` — AESP tables & mapping
- `generators/src/Generators/Scoring/RogueProfiler.cpp` — adapters
- `generators/src/Generators/Pipeline/CityGenerator.cpp` — pipeline and CityOutput
- `app/src/Integration/CityOutputApplier.cpp` — apply generated roads to `GlobalState`
- `visualizer/src/ui/viewport/rc_viewport_overlays.cpp` — labels/overlays
- `visualizer/src/ui/viewport/rc_viewport_interaction.cpp` — editing & creation
- `tests/test_aesp_scoring_profiles.cpp` — classification tests

---

This file is a working prompt for next steps: refining classification mappings, adding tests, and scheduling a non-breaking refactor if required.
