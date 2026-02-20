---
? "{\"file_name\":\"toolspass2.md\",\"master_reference\":\"RogueCities Editor Tools: Full Audit & Refactor Plan (Last Updated 2026-02-19)\",\"refer_back_protocol\":\"Refer to sections as <anchor>@L<start> and code fences as CF[start-end]; line numbers are authoritative; reopen cited ranges when uncertain.\",\"section_tree\":[[\"roguecities-editor-tools-full-audit-refactor-plan\",1,1,\"RogueCities Editor Tools: Full Audit & Refactor Plan\"],[\"the-one-structural-change-that-enables-all-of-this-missioncr\",5,2,\"The One Structural Change That Enables All of This #missioncritical\"],[\"atomic-checklist-actionables\",29,1,\"ATOMIC CHECKLIST & ACTIONABLES\"],[\"tier-0-structural-refactor-do-first-unlocks-everything\",36,2,\"TIER 0 — STRUCTURAL REFACTOR (Do First, Unlocks Everything)\"],[\"0-1-processnonaxiomviewportinteraction-pipeline-split\",39,3,\"0.1 — ProcessNonAxiomViewportInteraction Pipeline Split\"],[\"tier-1-p0-critical-bugs-ship-nothing-until-these-are-fixed\",54,2,\"TIER 1 — P0 CRITICAL BUGS (Ship Nothing Until These Are Fixed)\"],[\"1-1-propertyhistory-unify-with-global-commandhistory\",57,3,\"1.1 — PropertyHistory: Unify with Global CommandHistory\"],[\"1-2-inputfloat-dragfloat-command-per-frame-flooding-fix\",64,3,\"1.2 — InputFloat/DragFloat Command-Per-Frame Flooding Fix\"],[\"1-3-primaryviewport-pan-wire-input\",72,3,\"1.3 — PrimaryViewport Pan: Wire Input\"],[\"1-4-primaryviewport-zoom-wire-input\",78,3,\"1.4 — PrimaryViewport Zoom: Wire Input\"],[\"1-5-road-tool-create-stub-implementation\",84,3,\"1.5 — Road Tool: Create Stub Implementation\"],[\"1-6-road-rendering-hierarchy-color-thickness\",95,3,\"1.6 — Road Rendering: Hierarchy Color + Thickness\"],[\"tier-2-p1-high-needed-for-a-usable-session\",102,2,\"TIER 2 — P1 HIGH (Needed for a Usable Session)\"],[\"2-1-iviewporttool-interface-generalize-the-axiom-pattern\",105,3,\"2.1 — IViewportTool Interface: Generalize the Axiom Pattern\"],[\"2-2-selectionmanager-wire-into-primaryviewport\",112,3,\"2.2 — SelectionManager: Wire into PrimaryViewport\"],[\"2-3-box-select-wire-viewport-boxselect-state\",119,3,\"2.3 — Box Select: Wire `Viewport_BoxSelect` State\"],[\"2-4-road-vertex-drag\",124,3,\"2.4 — Road Vertex Drag\"],[\"2-5-district-tool-create-implementation\",130,3,\"2.5 — District Tool: Create Implementation\"],[\"2-6-water-body-rendering\",140,3,\"2.6 — Water Body Rendering\"],[\"part-1-tools-editors-catalog-what-exists-expected-behavior\",144,2,\"PART 1 — TOOLS & EDITORS CATALOG (What Exists + Expected Behavior)\"],[\"layer-1-tool-action-ids-rc-tool-contract-h\",146,3,\"Layer 1: Tool Action IDs (`rc_tool_contract.h`)\"],[\"layer-2-app-layer-tool-objects-app-include-roguecity-app-tools\",164,3,\"Layer 2: App-Layer Tool Objects (`app/include/RogueCity/App/Tools/`)\"],[\"layer-3-ui-tool-wiring-visualizer-src-ui-tools\",195,3,\"Layer 3: UI Tool Wiring (`visualizer/src/ui/tools/`)\"],[\"layer-4-ui-panels-visualizer-src-ui-panels\",206,3,\"Layer 4: UI Panels (`visualizer/src/ui/panels/`)\"],[\"layer-5-integration-layer-app-include-roguecity-app-integration\",234,3,\"Layer 5: Integration Layer (`app/include/RogueCity/App/Integration/`)\"],[\"part-2-the-bug-full-city-generation-on-axiom-placement\",246,2,\"PART 2 — THE BUG: Full City Generation on Axiom Placement\"],[\"part-3-comprehensive-refactor-plan\",295,2,\"PART 3 — COMPREHENSIVE REFACTOR PLAN\"],[\"phase-a-introduce-generationscope-gate-what-gets-applied\",297,3,\"Phase A: Introduce `GenerationScope` — Gate What Gets Applied\"],[\"phase-b-two-tier-generation-in-realtimepreview\",361,3,\"Phase B: Two-Tier Generation in `RealTimePreview`\"],[\"phase-c-fix-the-drawcontent-trigger-in-rc-panel-axiom-editor-cpp\",399,3,\"Phase C: Fix the `DrawContent()` Trigger in `rc_panel_axiom_editor.cpp`\"],[\"phase-d-add-the-generate-full-city-confirmation-gate\",435,3,\"Phase D: Add the \\\"Generate Full City?\\\" Confirmation Gate\"],[\"structural-issues-friction-points\",464,2,\"Structural Issues & Friction Points\"],[\"what-s-already-excellent-don-t-touch\",565,2,\"What's Already Excellent (Don't Touch)\"],[\"the-big-missing-visual-layer-what-should-be-drawn-but-isn-t\",708,2,\"The Big Missing Visual Layer: What Should Be Drawn But Isn't\"],[\"global-viewport-tools-what-you-need-and-where-they-live\",749,2,\"Global Viewport Tools: What You Need and Where They Live\"],[\"what-global-means-in-your-architecture\",755,2,\"What \\\"Global\\\" Means in Your Architecture\"],[\"the-global-tools-to-add-grouped-by-priority\",763,2,\"The Global Tools To Add — Grouped by Priority\"],[\"tier-1-fix-before-anything-else-these-are-broken-stubs\",767,2,\"Tier 1 — Fix Before Anything Else (These Are Broken Stubs)\"],[\"tier-2-high-value-architecturally-clean-to-add\",832,2,\"Tier 2 — High Value, Architecturally Clean to Add\"],[\"tier-3-for-when-2d-is-solid-before-40xx\",891,2,\"Tier 3 — For When 2D Is Solid, Before 40XX\"],[\"mapping-to-your-existing-structures\",901,2,\"Mapping to Your Existing Structures\"],[\"part-3-y2k-aesthetic-map-visualization-upgrade\",943,2,\"Part 3 — Y2K Aesthetic Map Visualization Upgrade\"],[\"grid-system\",970,2,\"Grid System\"],[\"road-rendering-the-missing-renderroads-method\",984,2,\"Road Rendering (the missing `RenderRoads()` method)\"],[\"what-s-actually-wired-end-to-end\",1171,2,\"What's actually wired end-to-end ✅\"],[\"what-is-missing-from-app-src-tools\",1181,2,\"What is MISSING from `app/src/Tools/`[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/tree/main/app/src/Tools)​\"],[\"part-2-expected-viewport-tool-execution-spec-per-tool\",1202,2,\"PART 2: EXPECTED VIEWPORT TOOL EXECUTION — SPEC PER TOOL\"],[\"a-pan-tool-viewport-pan\",1206,2,\"[A] PAN TOOL (`Viewport_Pan`)\"],[\"b-select-tool-viewport-select\",1221,2,\"[B] SELECT TOOL (`Viewport_Select`)\"],[\"c-road-tool-viewport-drawroad-the-most-complex\",1240,2,\"[C] ROAD TOOL (`Viewport_DrawRoad`) — the most complex\"],[\"d-district-tool-editing-districts\",1276,2,\"[D] DISTRICT TOOL (`Editing_Districts`)\"],[\"part-3-the-2d-correctness-checklist-priority-order\",1335,2,\"PART 3: THE 2D CORRECTNESS CHECKLIST (Priority Order)\"],[\"critical-missing-ship-nothing-until-fixed\",1339,2,\"CRITICAL MISSING — Ship Nothing Until Fixed\"],[\"high-needed-for-usable-session\",1370,2,\"HIGH — Needed for usable session\"],[\"medium-quality-and-correctness-polish\",1386,2,\"MEDIUM — Quality and correctness polish\"],[\"part-4-y2k-map-visualization-upgrade\",1402,2,\"PART 4: Y2K MAP VISUALIZATION UPGRADE\"],[\"color-system-replace-the-flat-palette-with-a-semantic-one-ad\",1406,2,\"Color System — Replace the flat palette with a semantic one added to our token system\"],[\"layered-rendering-order-painter-s-algorithm\",1439,2,\"Layered rendering order (painter's algorithm)\"]],\"indexed_code_fences\":[[13,22,\"cpp\"],[176,185,\"cpp\"],[252,268,\"cpp\"],[272,280,\"cpp\"],[301,317,\"cpp\"],[321,357,\"cpp\"],[365,378,\"cpp\"],[382,395,\"cpp\"],[403,411,\"cpp\"],[415,431,\"cpp\"],[490,499,null],[779,803,null],[934,941,null],[981,983,null],[1006,1010,null],[1042,1053,\"cpp\"],[1093,1100,null],[1122,1129,null],[1155,1160,null],[1208,1209,null],[1210,1215,null],[1225,1234,null],[1246,1261,null],[1280,1288,null],[1298,1303,null],[1313,1318,null],[1326,1331,null],[1346,1355,null],[1408,1437,\"cpp\"],[1443,1453,null],[1460,1463,null],[1466,1469,null],[1472,1475,null],[1478,1484,null],[1487,1491,null],[1494,1498,null],[1501,1507,null]],\"file_paths\":{\"visualizer_ui_tools\":[[\"GlobalNavigationHandler.cpp\",40],[\"GlobalSelectHandler.cpp\",41],[\"rc_tool_contract.h\",146],[\"rc_tool_contract.h/.cpp\",199],[\"rc_tool_dispatcher.h/.cpp\",200],[\"rc_tool_geometry_policy.h/.cpp\",201],[\"rc_tool_interaction_metrics.h/.cpp\",202],[\"visualizer/src/ui/tools/rc_tool_interaction_metrics.h\",212],[\"visualizer/src/ui/tools/rc_tool_contract.h\",274],[\"visualizer/src/ui/tools/rc_tool_dispatcher.h/.cpp\",830]],\"visualizer_ui_viewport\":[[\"rc_minimap_renderer.cpp\",743],[\"visualizer/src/ui/viewport/rc_scene_frame.h\",770],[\"rc_viewport_overlays.h\",945],[\"visualizer/src/ui/viewport/rc_viewport_overlays.h\",946],[\"visualizer/src/ui/viewport/rc_viewport_overlays.cpp\",947]],\"visualizer_ui_panels\":[[\"rc_panel_axiom_editor.cpp\",210],[\"rc_panel_axiom_bar.cpp\",211],[\"rc_panel_tools.cpp\",212],[\"rc_panel_inspector.cpp\",213],[\"rc_panel_city_spec.cpp\",214],[\"rc_panel_road_index.cpp\",215],[\"rc_panel_district_index.cpp\",216],[\"rc_panel_lot_index.cpp\",217],[\"rc_panel_building_index.cpp\",218],[\"rc_panel_building_control.cpp\",219],[\"rc_panel_lot_control.cpp\",220],[\"rc_panel_water_control.cpp\",221],[\"rc_panel_zoning_control.cpp\",222],[\"rc_panel_dev_shell.cpp\",223],[\"rc_panel_ai_console.cpp\",224],[\"rc_panel_log.cpp\",225],[\"rc_panel_telemetry.cpp\",226],[\"rc_panel_system_map.cpp\",227],[\"rc_panel_ui_agent.cpp\",228],[\"rc_panel_ui_settings.cpp\",229],[\"RcMasterPanel.cpp\",453],[\"RcPanelDrawers.cpp\",486],[\"rc_minimap_renderer.cpp\",743],[\"ContextWindowPopup.cpp\",1183],[\"rc_panel_property_editor.cpp\",1231]],\"app_tools\":[[\"AxiomPlacementTool.hpp\",168],[\"AxiomVisual.hpp\",169],[\"AxiomAnimationController.hpp\",170],[\"AxiomIcon.hpp\",171],[\"ContextWindowPopup.hpp\",172],[\"app/src/Tools/RoadTool.hpp\",85],[\"app/src/Tools/RoadTool.cpp\",86],[\"RoadTool.cpp\",93],[\"app/include/RogueCity/App/Tools/IViewportTool.hpp\",106],[\"app/src/Tools/DistrictTool.hpp\",131],[\"DistrictTool.cpp\",131],[\"LotTool.cpp\",1189],[\"BuildingTool.cpp\",1191],[\"WaterTool.cpp\",1193],[\"app/src/Tools/AxiomPlacementTool.cpp\",1392]],\"app_viewports\":[[\"app/src/Viewports/PrimaryViewport.cpp\",1198]],\"app_integration\":[[\"CityOutputApplier.hpp\",299],[\"RealTimePreview.cpp\",380]],\"app_editor\":[[\"app/src/Editor/EditorManipulation.cpp\",690]],\"core_editor\":[[\"EditorState.hpp\",1196],[\"core/include/RogueCity/Core/Editor/EditorState.hpp\",1197],[\"core/include/RogueCity/Core/Editor/GlobalState.hpp\",1254],[\"GlobalState.hpp\",1389],[\"core/include/RogueCity/Core/Editor/ViewportIndex.hpp\",1394],[\"SelectionManager.hpp\",1236],[\"SelectionSync.hpp\",1236]],\"misc\":[[\"rc_property_editor.cpp\",60],[\"RoadType\",986],[\"ToolInteractionMetrics\",24],[\"PanelType\",527],[\"on_message\",533],[\"ViewportIndexBuilder\",112],[\"PickFromViewportIndex\",114]]},\"namespaces\":{\"declared\":[[\"Y2KPalette\",1410]],\"implied\":[[\"RogueCity::Core::Editor\",[[\"GlobalState\",299],[\"CommandHistory\",57],[\"DirtyLayer\",89],[\"SelectionManager\",112],[\"ViewportIndex/VpProbeData\",113]]],[\"RogueCity::App\",[[\"Tools\",164],[\"Integration\",234],[\"ApplyCityOutputToGlobalState\",272]]],[\"RogueCity::Generators::CityGenerator\",[[\"CityOutput\",272],[\"Config\",365],[\"AxiomInput\",365]]],[\"ImGui/ImDrawList\",[[\"input routing\",72],[\"render passes\",96]]]],\"conflicts\":[[\"input_routing_duality\",\"L7-L143\",\"ProcessNonAxiomViewportInteraction vs PrimaryViewport::update/IViewportTool\"],[\"road_type_enum_mismatch\",\"L95-L100;L984-L1012;L1408-L1437\",\"RoadType/RoadHierarchy variants inconsistent\"],[\"command_history_ownership\",\"L57-L71;L521-L527\",\"Property editor local vs global CommandHistory/DrawContext propagation\"]]},\"integration_points\":[{\"id\":\"ip_nonaxiom_pipeline_split\",\"line_range\":\"L39-L52\",\"target\":\"ProcessNonAxiomViewportInteraction\",\"section_anchor\":\"0-1-processnonaxiomviewportinteraction-pipeline-split\"},{\"id\":\"ip_property_editor_history\",\"line_range\":\"L57-L63\",\"target\":\"rc_property_editor.cpp DrawContext/CommandHistory\",\"section_anchor\":\"1-1-propertyhistory-unify-with-global-commandhistory\"},{\"id\":\"ip_property_editor_commit_flood\",\"line_range\":\"L64-L71\",\"target\":\"DrawSingle* widgets batching\",\"section_anchor\":\"1-2-inputfloat-dragfloat-command-per-frame-flooding-fix\"},{\"id\":\"ip_pan_zoom_input\",\"line_range\":\"L72-L83\",\"target\":\"GlobalNavigationHandler/PrimaryViewport camera\",\"section_anchor\":\"1-3-primaryviewport-pan-wire-input\"},{\"id\":\"ip_road_tool_stub\",\"line_range\":\"L84-L94\",\"target\":\"app/src/Tools/RoadTool.* + HFSM Tool_Roads\",\"section_anchor\":\"1-5-road-tool-create-stub-implementation\"},{\"id\":\"ip_iviewporttool_dispatch\",\"line_range\":\"L105-L111\",\"target\":\"PrimaryViewport::update + active_tool_\",\"section_anchor\":\"2-1-iviewporttool-interface-generalize-the-axiom-pattern\"},{\"id\":\"ip_selection_manager\",\"line_range\":\"L112-L118\",\"target\":\"ViewportIndexBuilder/PickFromViewportIndex/Selection highlights\",\"section_anchor\":\"2-2-selectionmanager-wire-into-primaryviewport\"},{\"id\":\"ip_generation_scope\",\"line_range\":\"L297-L360\",\"target\":\"CityOutputApplier ApplyCityOutputToGlobalState + options.scope\",\"section_anchor\":\"phase-a-introduce-generationscope-gate-what-gets-applied\"},{\"id\":\"ip_generation_depth\",\"line_range\":\"L361-L398\",\"target\":\"RealTimePreview depth + CityGenerator::Config flags\",\"section_anchor\":\"phase-b-two-tier-generation-in-realtimepreview\"},{\"id\":\"ip_axiom_panel_trigger_fix\",\"line_range\":\"L399-L434\",\"target\":\"rc_panel_axiom_editor.cpp DrawContent live preview gating\",\"section_anchor\":\"phase-c-fix-the-drawcontent-trigger-in-rc-panel-axiom-editor-cpp\"},{\"id\":\"ip_viewport_overlays_roads\",\"line_range\":\"L984-L1012\",\"target\":\"ViewportOverlays::RenderRoads\",\"section_anchor\":\"road-rendering-the-missing-renderroads-method\"},{\"id\":\"ip_y2k_palette_tokens\",\"line_range\":\"L1408-L1437\",\"target\":\"Y2KPalette tokens + painter order\",\"section_anchor\":\"color-system-replace-the-flat-palette-with-a-semantic-one-ad\"}],\"dependency_graph\":{\"T0.pipeline_split\":[],\"T1.history_unify\":[\"T0.pipeline_split\"],\"T1.command_flood_fix\":[\"T1.history_unify\"],\"T1.pan_zoom\":[\"T0.pipeline_split\"],\"T1.road_tool_stub\":[\"T1.pan_zoom\",\"T1.history_unify\"],\"T2.iviewporttool\":[\"T1.pan_zoom\"],\"T2.selection_manager\":[\"T2.iviewporttool\"],\"T2.box_select\":[\"T2.selection_manager\"],\"T2.road_vertex_drag\":[\"T2.selection_manager\"],\"T2.district_tool\":[\"T2.iviewporttool\",\"T2.selection_manager\"],\"T2.water_render\":[\"T2.iviewporttool\"],\"GEN.scope\":[\"T1.history_unify\"],\"GEN.depth\":[\"GEN.scope\"],\"GEN.axiom_panel_trigger_fix\":[\"GEN.depth\"],\"VIS.y2k_palette\":[\"T1.road_tool_stub\"],\"VIS.viewport_overlays_render_roads\":[\"VIS.y2k_palette\"]},\"risk_nodes\":[[\"r0_dual_input_system\",\"L7-L143\",\"Two competing input tool architectures\"],[\"r1_pipeline_split_regression\",\"L39-L52\",\"Handler ordering/consumed semantics regressions\"],[\"r2_command_history_merge\",\"L57-L63\",\"Undo stack unification pitfalls\"],[\"r3_generation_scope_partial_apply\",\"L297-L360\",\"Partial apply must preserve locked entities + indices\"],[\"r4_threaded_preview_depth\",\"L361-L398\",\"RealTimePreview threading/cancellation risks\"],[\"r5_command_only_mutations\",\"L1175-L1175;L1392-L1395\",\"Enforce ICommand-only GlobalState mutations + DirtyLayer\"],[\"r6_visual_contrast_order\",\"L1402-L1453\",\"Palette + painter order may reduce selection/hover clarity\"]],\"blocking_questions\":[\"Choose the single source of truth for viewport input routing: visualizer-level ProcessNonAxiomViewportInteraction pipeline (Tier0) vs app-level PrimaryViewport::update + IViewportTool (Tier2).\",\"Confirm the canonical road classification enum and values before implementing hierarchy styling + RenderRoads.\",\"Define regeneration contract for GenerationDepth==AxiomBounds: allowed GlobalState layers + DirtyLayer/ViewportIndex rebuild triggering.\",\"Clarify UX for 'Generate Full City?' gate (Phase D): modal vs toggle, state ownership, interaction with live preview debounce.\"],\"compression_loss_risks\":[{\"description\":\"Tier0 handler ordering/consumed semantics is load-bearing.\",\"line_range\":\"L13-L52\",\"section_anchor\":\"0-1-processnonaxiomviewportinteraction-pipeline-split\",\"why_at_risk\":\"Compression may drop subtle ordering constraints and 'never consumed' rules.\",\"recovery_hint\":\"Preserve exact 7-call order + 'never consumed' notes verbatim.\"},{\"description\":\"Axiom live preview trigger + OnComplete callback define the full-city-gen bug.\",\"line_range\":\"L248-L286\",\"section_anchor\":\"part-2-the-bug-full-city-generation-on-axiom-placement\",\"why_at_risk\":\"Summaries may blur the per-frame condition and where ApplyCityOutput applies everything.\",\"recovery_hint\":\"Keep the exact if-condition and SetOnComplete blocks.\"},{\"description\":\"GenerationScope defaults + ApplyCityOutputToGlobalState conditional layers are invariants.\",\"line_range\":\"L297-L360\",\"section_anchor\":\"part-3-comprehensive-refactor-plan\",\"why_at_risk\":\"Compression can hide default scope and which layers always/conditionally apply.\",\"recovery_hint\":\"Retain enum values + CityOutputApplyOptions defaults + apply pseudo-code.\"},{\"description\":\"IViewportTool method set + dispatch guards must match ImGui event semantics.\",\"line_range\":\"L105-L111;L1346-L1355\",\"section_anchor\":\"2-1-iviewporttool-interface-generalize-the-axiom-pattern\",\"why_at_risk\":\"Abbreviation may omit callbacks or hovered/viewport guards causing missed inputs.\",\"recovery_hint\":\"Treat IViewportTool struct + dispatch as acceptance criteria.\"},{\"description\":\"Per-tool execution specs include gestures and thresholds (e.g., merge/split distance).\",\"line_range\":\"L1208-L1331\",\"section_anchor\":\"part-2-expected-viewport-tool-execution-spec-per-tool\",\"why_at_risk\":\"Summaries may omit modifiers, thresholds, or pick priority rules.\",\"recovery_hint\":\"Use each spec block as canonical test cases.\"},{\"description\":\"Y2K palette values + painter's algorithm order control readability.\",\"line_range\":\"L1402-L1453\",\"section_anchor\":\"part-4-y2k-map-visualization-upgrade\",\"why_at_risk\":\"Compression may drop exact ImU32 values and render-layer ordering.\",\"recovery_hint\":\"Keep Y2KPalette block + final layered order list intact.\"}]}"
:
---
`# RogueCities Editor Tools: Full Audit & Refactor Plan

```markdown
# Repo Audit Mapping Artifact (Codex Context)

## Purpose

This artifact captures a repo verification pass against `audit_refactor_plan.md` (symbols + key files), plus the minimal set of changes needed to align implementation with the plan.

Limitations noted in the source text: GitHub code-search responses were capped, so the mapping may be incomplete; use repo UI searches to confirm.

## High-level findings

### Already present (good base)

- Non-axiom viewport interaction pipeline exists (pan/zoom + selection + outcome classification).
    
- GenerationCoordinator + RealTimePreview exist (debounced preview, background generation, on-complete callback).
    
- ViewportIndexBuilder + pick helpers exist (priority picking rules).
    

### Missing / needs change (P0/P1)

- Property editor uses a **local** `CommandHistory` (undo/redo not unified).
    
- Property editor commits on every keystroke (undo stack flooding).
    
- Live preview currently requests a full generation path; applier applies layers unconditionally (needs **GenerationScope/GenerationDepth** gating).
    
- App-layer tool implementations are mostly missing (`RoadTool`, `DistrictTool`, etc.); only `AxiomPlacementTool` exists.
    
- Road rendering is present but visually flat; hierarchy color/thickness and render ordering are not implemented as specified.
    
- `PrimaryViewport::update()` is stubbed; there is no canonical `IViewportTool.hpp`.
    

## Verified files (by subsystem)

### Visualizer viewport input

- `visualizer/src/ui/viewport/rc_viewport_interaction.cpp`
    
- `visualizer/src/ui/viewport/handlers/rc_viewport_non_axiom_pipeline.cpp`
    
- `visualizer/src/ui/viewport/handlers/rc_viewport_handler_common.cpp`
    
- `visualizer/src/ui/viewport/rc_viewport_overlays.cpp`
    

### Visualizer panels

- `visualizer/src/ui/panels/rc_panel_axiom_editor.cpp`
    
- `visualizer/src/ui/panels/rc_property_editor.cpp`
    

### App integration

- `app/include/RogueCity/App/Integration/RealTimePreview.hpp`
    
- `app/src/Integration/RealTimePreview.cpp`
    
- `app/src/Integration/GenerationCoordinator.cpp`
    
- `app/include/RogueCity/App/Integration/CityOutputApplier.hpp`
    
- `app/src/Integration/CityOutputApplier.cpp`
    

### App viewport + editor

- `app/include/RogueCity/App/Viewports/PrimaryViewport.hpp`
    
- `app/src/Viewports/PrimaryViewport.cpp`
    
- `app/include/RogueCity/App/Editor/ViewportIndexBuilder.hpp`
    
- `app/src/Editor/ViewportIndexBuilder.cpp`
    

### Contracts, generators, tests

- `tools/check_generator_viewport_contract.py`
    
- `docs/20_specs/generator-viewport-contract.md`
    
- `docs/20_specs/tool-viewport-generation-contract.md`
    
- `generators/src/Generators/Urban/RoadGenerator.cpp`
    
- `tests/integration/test_full_pipeline.cpp`
    

## Required changes (prioritized)

1. **Unify undo history**
    

- Replace property editor’s local static `CommandHistory` with a global history reference (e.g., via `DrawContext`).
    
- Acceptance: Ctrl+Z works across viewport + property panel on the same stack.
    

2. **Stop per-keystroke commits**
    

- Use `ImGui::IsItemActivated()` (capture before) and `ImGui::IsItemDeactivatedAfterEdit()` (commit once) for Input/Drag widgets.
    
- Acceptance: one undo entry per edit gesture, not per frame/keystroke.
    

3. **Add partial-generation gating**
    

- Add `GenerationDepth` (e.g., `AxiomBounds` vs `FullPipeline`) to `RealTimePreview::request_regeneration` and forward through `GenerationCoordinator`.
    
- Add `GenerationScope` to `CityOutputApplyOptions` and gate `ApplyCityOutputToGlobalState` (skip districts/lots/buildings when scope != `FullCity`).
    
- Update `rc_panel_axiom_editor.cpp` live preview regeneration requests to use `GenerationDepth::AxiomBounds`.
    
- Acceptance: Live axiom edits do not apply full city; explicit “ForceGenerate” still does.
    

4. **Stub missing app tools**
    

- Add missing tool implementations under `app/src/Tools/` (at least stubs) beginning with `RoadTool`.
    
- Acceptance: Tools register/build; no-op stubs compile and can be activated.
    

5. **Improve road rendering hierarchy**
    

- Update road render loop (or introduce `RenderRoads()` in overlays) to apply hierarchy-based color + thickness and ordering.
    
- Acceptance: Visual distinction by road classification; no regressions in selection/highlighting.
    

6. **(Optional) ID lookup acceleration**
    

- Replace repeated O(n) `FindXById` scans with ID→index maps where hot (property panel/overlays), if profiling warrants.
    

7. **Introduce canonical `IViewportTool.hpp` and dispatch**
    

- Create a single interface header; migrate tools gradually. Keep diffs constrained.
    

8. **Update tests as needed**
    

- Some tests assume full output application; after gating, add/adjust tests for partial-scope modes.
    
- Run `tools/check_generator_viewport_contract.py` as a validation step.
    

## Invariants to preserve

- Single generation-to-application path remains: GenerationCoordinator → RealTimePreview → ApplyCityOutputToGlobalState.
    
- Non-axiom pipeline remains authoritative for current selection/pick behavior unless intentionally migrated.
    
- GlobalState mutations remain command-driven where required by the contract.
    

## Validation checklist

- Build: clean compile (all targets).
    
- Manual: property edits generate single undo entries; undo works across panels/viewport.
    
- Manual: live axiom preview does not trigger full city regeneration/apply.
    
- Visual: roads show hierarchy styling; selection/hover remains readable.
    
- Automated: contract script passes; integration tests updated/passing.
```

### NavForAI_AuditDetails
```json
{"file_name":"repo_audit_mapping_notes.md","master_reference":"Repo verification notes vs audit_refactor_plan.md (GitHub search-backed mapping)","refer_back_protocol":"Refer to sections as <anchor>@L<start>; line numbers refer to the pasted text; reopen the cited line ranges before editing code.","section_tree":[["repo-audit-mapping-notes-verification-pass-vs-audit-refactor-plan",1,1,"Repo Audit Mapping Notes (Verification pass vs audit_refactor_plan.md)"],["high-level-summary-quick",7,2,"High-level summary (quick)"],["detailed-mapping-plan-item-status-file-pointers-recommended-next-step",18,2,"Detailed mapping (plan item → status, file pointers, recommended next step)"],["tier-0-processnonaxiomviewportinteraction-pipeline",20,3,"TIER 0 — ProcessNonAxiomViewportInteraction Pipeline"],["tier-1-propertyhistory-unify-with-global-commandhistory",29,3,"TIER 1 — PropertyHistory unify with Global CommandHistory"],["tier-1-inputfloat-dragfloat-flood-commits",42,3,"TIER 1 — InputFloat / DragFloat flood commits"],["tier-1-primaryviewport-pan-zoom-wiring",50,3,"TIER 1 — PrimaryViewport Pan & Zoom wiring"],["tier-1-roadtool-stub",60,3,"TIER 1 — RoadTool (stub)"],["tier-1-road-rendering-hierarchy-color-thickness",68,3,"TIER 1 — Road rendering: hierarchy color + thickness"],["generation-live-preview-generationcope-generationdepth-gating",77,3,"GENERATION / LIVE PREVIEW — GenerationScope & GenerationDepth gating"],["applycityoutputtoglobalstate-behavior-applier-gating",91,3,"ApplyCityOutputToGlobalState behavior (applier gating)"],["rc-panel-axiom-editor-cpp-live-preview-trigger",100,3,"rc_panel_axiom_editor.cpp live-preview trigger"],["selectionmanager-viewportindex-wiring",108,3,"SelectionManager / ViewportIndex wiring"],["iviewporttool-abstraction-and-primaryviewport-update",117,3,"IViewportTool abstraction and PrimaryViewport::update"],["other-useful-finds-small-wins-low-effort",127,3,"Other useful finds & small wins (low-effort)"],["concrete-prioritized-next-steps-smallest-impact-first",133,3,"Concrete prioritized next steps (smallest-impact first)"],["copilot-continuation-audit-notes",142,2,"Copilot continuation (audit notes)"],["what-i-checked-file-short-finding",144,3,"What I checked (file → short finding)"],["concrete-status-summary-plan-items-current-state",207,3,"Concrete status summary (plan items → current state)"],["files-i-opened-representative",224,3,"Files I opened (representative)"],["observed-invariants-and-implications-facts",243,3,"Observed invariants and implications (facts)"],["coverage-note",250,3,"Coverage note"],["checklist-to-comply-with-audit-refactor-plan",254,2,"Checklist to comply with audit/refactor plan"],["critical-refactor-and-implementation-work",258,3,"Critical refactor and implementation work"],["1-partial-generation-generationscope-generationdepth-support",260,4,"1. Partial Generation: \"GenerationScope\" & \"GenerationDepth\" Support"],["2-undo-redo-stack-integration",274,4,"2. Undo/Redo Stack Integration"],["3-flooding-commandhistory-input-commit-pattern",284,4,"3. Flooding CommandHistory: Input Commit Pattern"],["4-app-tool-implementation-viewport-tool-system",294,4,"4. App Tool Implementation (Viewport_Tool System)"],["5-iviewporttool-interface",305,4,"5. IViewportTool Interface"],["6-road-rendering-hierarchy-colors-and-thickness",316,4,"6. Road Rendering: Hierarchy Colors and Thickness"],["7-property-editor-and-selectionmanager-integration",327,4,"7. Property Editor and SelectionManager Integration"],["8-tool-execution-contracts-boxselect-road-vertex-drag-district-polygon-etc",335,4,"8. Tool Execution Contracts: BoxSelect, Road Vertex Drag, District Polygon, etc."],["9-tests-and-contract-checks",344,4,"9. Tests and Contract Checks"],["optional-polish-or-architectural",352,3,"Optional/Polish or Architectural"],["summary-table",361,3,"Summary Table"],["final-statement",377,3,"Final Statement"]],"indexed_code_fences":[],"file_paths":{"visualizer_viewport":[["visualizer/src/ui/viewport/rc_viewport_interaction.cpp",24],["visualizer/src/ui/viewport/handlers/rc_viewport_non_axiom_pipeline.cpp",25],["visualizer/src/ui/viewport/rc_viewport_overlays.cpp",73],["visualizer/src/ui/viewport/handlers/rc_viewport_handler_common.cpp",184]],"visualizer_panels":[["visualizer/src/ui/panels/rc_property_editor.cpp",33],["visualizer/src/ui/panels/rc_panel_axiom_editor.cpp",104]],"app_integration":[["app/include/RogueCity/App/Integration/RealTimePreview.hpp",81],["app/src/Integration/RealTimePreview.cpp",81],["app/src/Integration/GenerationCoordinator.cpp",82],["app/include/RogueCity/App/Integration/CityOutputApplier.hpp",83],["app/src/Integration/CityOutputApplier.cpp",84]],"app_viewports":[["app/include/RogueCity/App/Viewports/PrimaryViewport.hpp",154],["app/src/Viewports/PrimaryViewport.cpp",56]],"app_tools":[["app/src/Tools/AxiomPlacementTool.cpp",64],["app/include/RogueCity/App/Tools/IViewportTool.hpp",121],["app/src/Tools/RoadTool.hpp",66],["app/src/Tools/RoadTool.cpp",60]],"app_editor":[["app/include/RogueCity/App/Editor/ViewportIndexBuilder.hpp",181],["app/src/Editor/ViewportIndexBuilder.cpp",181]],"contracts_and_validation":[["tools/check_generator_viewport_contract.py",129],["docs/20_specs/generator-viewport-contract.md",204],["docs/20_specs/tool-viewport-generation-contract.md",204]],"generators_and_tests":[["generators/src/Generators/Urban/RoadGenerator.cpp",238],["tests/integration/test_full_pipeline.cpp",239]]},"namespaces":{"declared":[["GenerationScope",254],["GenerationDepth",77],["CityOutputApplyOptions",83],["CommandHistory",29],["DrawContext",37],["IViewportTool",117]],"implied":[["RogueCity::App::Integration",[["GenerationCoordinator",82],["RealTimePreview",81],["CityOutputApplier",83],["ApplyCityOutputToGlobalState",91]]],["RogueCity::App::Viewports",[["PrimaryViewport",50]]],["RogueCity::App::Editor",[["ViewportIndexBuilder",108]]],["RogueCity::Core::Editor",[["GlobalState",243],["SelectionManager",108],["ViewportIndex/VpProbeData",108]]],["ImGui",[["IsItemActivated/IsItemDeactivatedAfterEdit",42]]]],"conflicts":[["undo_history_split","L29-L41","Property editor local CommandHistory diverges from global undo stack"],["live_preview_full_apply","L77-L107","Live preview requests full pipeline; applier applies all layers unconditionally"],["input_routing_duality","L50-L126","Non-axiom pipeline handles input while PrimaryViewport::update is stub; canonical IViewportTool missing"],["tests_expect_full_apply","L230-L249;L344-L350","Existing tests assume full pipeline apply; scope gating will require updates"]]},"integration_points":[{"id":"ip_global_command_history","line_range":"L29-L41","target":"visualizer/src/ui/panels/rc_property_editor.cpp + DrawContext construction sites","section_anchor":"tier-1-propertyhistory-unify-with-global-commandhistory"},{"id":"ip_commit_on_deactivate","line_range":"L42-L49","target":"visualizer/src/ui/panels/rc_property_editor.cpp (Input/Drag widgets)","section_anchor":"tier-1-inputfloat-dragfloat-flood-commits"},{"id":"ip_generation_depth_plumb","line_range":"L77-L90","target":"RealTimePreview::request_regeneration + GenerationCoordinator::RequestRegeneration","section_anchor":"generation-live-preview-generationscope-generationdepth-gating"},{"id":"ip_generation_scope_apply","line_range":"L91-L99","target":"CityOutputApplyOptions + ApplyCityOutputToGlobalState gating","section_anchor":"applycityoutputtoglobalstate-behavior-applier-gating"},{"id":"ip_axiom_panel_depth","line_range":"L100-L107","target":"visualizer/src/ui/panels/rc_panel_axiom_editor.cpp (live preview requests)","section_anchor":"rc-panel-axiom-editor-cpp-live-preview-trigger"},{"id":"ip_road_tool_stub","line_range":"L60-L67","target":"app/src/Tools/RoadTool.* (new) + tool registration","section_anchor":"tier-1-roadtool-stub"},{"id":"ip_road_render_hierarchy","line_range":"L68-L76","target":"app/src/Viewports/PrimaryViewport.cpp or visualizer/src/ui/viewport/rc_viewport_overlays.cpp","section_anchor":"tier-1-road-rendering-hierarchy-color-thickness"},{"id":"ip_viewport_index_selection","line_range":"L108-L116","target":"ViewportIndexBuilder + PickFromViewportIndex + SelectionManager usage","section_anchor":"selectionmanager-viewportindex-wiring"},{"id":"ip_contract_validation","line_range":"L127-L132","target":"tools/check_generator_viewport_contract.py + docs specs","section_anchor":"other-useful-finds-small-wins-low-effort"}],"dependency_graph":{"T1.undo_unify":["T0.input_arch"],"T1.commit_batch":["T1.undo_unify"],"GEN.scope_apply":["T1.undo_unify"],"GEN.depth_plumb":["GEN.scope_apply"],"GEN.axiom_panel_update":["GEN.depth_plumb"],"T1.road_tool_stub":["T0.input_arch"],"VIS.road_hierarchy_render":["T1.road_tool_stub"],"T2.iviewporttool_header":["T0.input_arch"],"TEST.update":["GEN.scope_apply"]},"risk_nodes":[["r0_undo_regression","L29-L49","Undo/redo semantics may change when unifying histories and batching commits"],["r1_preview_regression","L77-L107","Depth/scope changes may break live preview or force-generate flows"],["r2_partial_apply_invariants","L91-L99","Scope gating must preserve indices, selection stability, and dirty flags"],["r3_tool_registration_churn","L60-L67","Adding tools requires CMake/registry updates; keep minimal diffs"],["r4_render_readability","L68-L76","Hierarchy styling could reduce selection/hover clarity if palette/order wrong"],["r5_test_failures","L344-L374","Existing tests assume full apply; gating will require new test cases"]],"blocking_questions":["Choose the single source of truth for viewport input/tool dispatch short-term: keep non-axiom pipeline authoritative or migrate into PrimaryViewport + IViewportTool dispatch.","Define exact GenerationDepth contract for AxiomBounds (which generator stages run; what CityOutput fields are valid).","Define GenerationScope semantics (which GlobalState layers apply, how ViewportIndex rebuild is triggered, what dirty layers are set).","Confirm how ForceGenerate / FullCity UI should bypass live-preview gating (button/modal/setting)."],"compression_loss_risks":[{"description":"Exact file pointers + 'status' claims are load-bearing for triage.","line_range":"L20-L133","section_anchor":"detailed-mapping-plan-item-status-file-pointers-recommended-next-step","why_at_risk":"Compression may drop or reorder file references, obscuring where changes must land.","recovery_hint":"Re-open each referenced file in the repo UI and reconcile against plan items."},{"description":"Scope/depth gating requires consistent semantics across preview, coordinator, panel, and applier.","line_range":"L77-L107","section_anchor":"generation-live-preview-generationscope-generationdepth-gating","why_at_risk":"Summaries may blur the difference between GenerationDepth (what to generate) and GenerationScope (what to apply).","recovery_hint":"Treat depth as generator stage selection; treat scope as applier layer gating; document defaults explicitly."},{"description":"Undo batching must preserve existing CommitLambda/CommitValueChange behavior.","line_range":"L29-L49","section_anchor":"tier-1-inputfloat-dragfloat-flood-commits","why_at_risk":"Simplification may omit edge cases (cancel edits, multi-field widgets, drag vs type).","recovery_hint":"Apply a consistent activated/deactivated pattern and add spot tests for each widget class."},{"description":"Test expectations change when partial apply is introduced.","line_range":"L344-L374","section_anchor":"9-tests-and-contract-checks","why_at_risk":"Compression may omit which tests assume full pipeline output.","recovery_hint":"Add new tests for partial modes; keep existing full pipeline tests passing by defaulting scope=FullCity."}]}
```
---
# ## Critical Refactor and Implementation Work
# important must read
### 1. **Partial Generation: "GenerationScope" & "GenerationDepth" Support**

- **What’s missing:**
    
    - **CityOutputApplyOptions** (in `CityOutputApplier.hpp`) does _not_ have a scope field (enum GenerationScope: RoadsOnly, RoadsAndBounds, FullCity).
    - **ApplyCityOutputToGlobalState** (in `CityOutputApplier.cpp`) always applies roads, districts, lots, buildings; it lacks conditional application based on scope.
    - **RealTimePreview, GenerationCoordinator, rc_panel_axiom_editor.cpp** only request full generation; “AxiomBounds” preview is not supported.
- **What to do:**
    
    - Add a `GenerationScope` field to `CityOutputApplyOptions` and update `ApplyCityOutputToGlobalState` to honor the scope (skip lots/buildings except for FullCity).
    - Extend `RealTimePreview` and `GenerationCoordinator` to accept/forward `GenerationDepth` (AxiomBounds vs FullPipeline).
    - Update axiom panel’s live preview request to use “AxiomBounds” depth (shows only road skeleton + axiom boundaries).
    - Make explicit full city generation via button (ForceGenerate) use "FullCity"/"FullPipeline".

---

### 2. **Undo/Redo Stack Integration**

- **What’s missing:**
    
    - Property editors (especially `rc_property_editor.cpp`) still use a local static `CommandHistory`. Undo/redo isn’t unified across the editor.
- **What to do:**
    
    - Pass the global `CommandHistory` instance via `DrawContext` (in `IPanelDrawer.h`) and update all property-editor logic to use the shared history.
    - Remove the local static CommandHistory in `PropertyHistory()`.

---

### 3. **Flooding CommandHistory: Input Commit Pattern**

- **What’s missing:**
    - All ImGui input (`InputFloat`, `InputFloat2`, `DragFloat`, etc.) in property editor commits a change **every keystroke**: this floods the undo stack.
- **What to do:**
    - Replace per-character commit with:
        - Store snapshot on `ImGui::IsItemActivated()`
        - Only commit when `ImGui::IsItemDeactivatedAfterEdit()` is true.

---

### 4. **App Tool Implementation (Viewport_Tool System)**

- **What’s missing:**
    
    - Only `AxiomPlacementTool` is implemented in `app/src/Tools/`.
    - No source files for: `RoadTool`, `DistrictTool`, `LotTool`, `BuildingTool`, `WaterTool`.
- **What to do:**
    
    - Implement at least stubs for each tool (`RoadTool.cpp/.hpp`, `DistrictTool.cpp/.hpp`, etc.), inheriting the same interface style as `AxiomPlacementTool`. Follow the contracts in your audit doc.
    - Register tools in panel registry/CMake.

---

### 5. **IViewportTool Interface**

- **What’s missing:**
    
    - No canonical `IViewportTool.hpp` that defines mouse/scroll/render interface for tool classes.
    - Tool system is not fully abstracted for switching between active tools.
- **What to do:**
    
    - Create the interface header and refactor existing and new tools to use this abstraction.
    - Update `PrimaryViewport` to hold and dispatch input to `IViewportTool*`.

---

### 6. **Road Rendering: Hierarchy Colors and Thickness**

- **What’s missing:**
    
    - PrimaryViewport renders roads in flat cyan (except basic thickness tweak for Highway/Arterial).
    - No painter’s algorithm ordering or color scheme as specified.
- **What to do:**
    
    - Refactor road rendering (in `PrimaryViewport::render` and/or move to a new `RenderRoads()` overlay function) to use type-based color/thickness following audit palette.
    - Implement painter’s algorithm (two-pass render: casing then fill, layered with districts/lots/buildings/water).

---

### 7. **Property Editor and SelectionManager Integration**

- **What’s missing:**
    - Property editor, overlays, and index panels scan arrays for entity lookup (O(n)), do not use ID-to-index maps.
- **What to do:**
    - Optionally, add unordered maps for ID lookup in `GlobalState` for roads, districts, lots, buildings (for faster selection, editing, overlays).

---

### 8. **Tool Execution Contracts: BoxSelect, Road Vertex Drag, District Polygon, etc.**

- **What’s missing:**
    
    - Only partial implementation for selection and box select; road vertex drag, district vertex insert/remove, lot editing, and building placement are unimplemented at tool level.
- **What to do:**
    
    - Wire up handlers in the non-axiom pipeline and app tool classes for these behaviors. Use `ViewportIndexBuilder` and `SelectionManager` after generation to rebuild pickable index.

---

### 9. **Tests and Contract Checks**

- **What’s missing:**
    - Tests and scripts assume full city application. These may need updating after partial/scope-based application.
- **What to do:**
    - Update tests as needed, and run `tools/check_generator_viewport_contract.py` to confirm compliance.

---

## Optional/Polish or Architectural

- Implement ID->index maps for entity lookup (`O(1)` instead of array scans).
- Refactor panel drawers ("one file per panel") for maintainability.
- Enhance overlays with Y2K palette, grid labeling, interactive highlights.
- Add missing “stamp axiom” quick-place pattern and “Inspector” ephemeral tool as described.

---

## Summary Table

|Task|Implemented?|File(s)|To Do|
|---|---|---|---|
|GenerationScope/Depth|❌|CityOutputApplier, RealTimePreview|Add fields, gating, param propagation|
|Undo/Redo unified stack|❌|rc_property_editor.cpp, DrawContext|Use shared CommandHistory|
|Commit per-edit, not per-char|❌|rc_property_editor.cpp|Use IsItemActivated/IsItemDeactivatedAfterEdit|
|Road/District/Lot/Building/Water tools|❌|app/src/Tools/|Code stubs, implement tool contracts|
|IViewportTool abstraction|❌|app/include/|Add interface header, refactor|
|Hierarchy-based road rendering|❌|PrimaryViewport.cpp|Refactor render to use color/thickness|
|O(1) ID lookups|❓ (partial)|overlays, index panels|Add maps if needed|
|Selection/box select/vertex drag|❓ (partial)|pipeline, SelectionManager, EditorManipulation|Wire up handlers|
|Contract tests|❓ (partial)|tests/integration/, tools/check_generator_viewport_contract.py|Update after partial apply|

---

## Final Statement

**Major next steps:**

- Implement partial generation gating (GenerationScope/Depth, conditional applier, update preview/panel API).
- Unify undo/redo stacks and update property editor commit pattern.
- Stub/default app tools (Road, District, Lot, Building, Water), add interface abstraction.
- Refactor road rendering for visual hierarchy.
- Wire up any missing box select, road vertex drag, and district editing features.

**Once you do these, your codebase will align with the architectural contract and the actionable audit plan.**

---

**If you want, I can generate the diffs and/or open a PR for any subset of these tasks. Just specify which you want first.**

[audit_refactor_plan.mdv2](https://github.com/copilot/c/8a6b0c58-2d01-4816-846a-468319e661a4#)

### You said: what am i not considering in this

what am i not considering in this plan? the two jsn inputs are for you

### Copilot said: You've asked what **is not being considered**

You've asked what **is not being considered** in your plan, given your detailed audit, the repo mapping, and the JSON "section_tree" metadata. Below is a concrete checklist of what’s missing or could cause delivery/tech debt risk, based on best practices for large refactor/feature integration in a C++ ImGui-based editor codebase like RogueCities.

---

## 1. **Migration & Upgrade Path for Existing Users**

- **Data Migration:** When introducing partial apply/generation-scope and new entity ID maps, will old city files/scenes load correctly? If selection or history serialization changes format, will you write migration scripts or support fallback format decoding?
- **Command History Format:** Unifying undo stacks requires careful attention to any external serialization of command history or playback files.
- **User Impact Notification:** When users open a new version, is it clear what’s changed, especially about undo/redo, tool split, or how preview works? (UI message, log output, or migration dialog.)

---

## 2. **Inter-tool Consistency and Edge Behavior**

- **Switching Tools Mid-action:** What if the user starts drawing a road, then switches tools? Are ghost geometries, temp selections, and undo stack entries finalized, canceled, or left in limbo? The refactor should clean up or commit any in-progress tool action on tool switch or editor state switch.
- **ESC/Cancel Workflow:** Each tool needs a robust and consistent cancel/escape handler—abandoning a tool's temp state without leaking memory or leaving the UI inconsistent.

---

## 3. **UI Feedback, Error, and State Recovery**

- **Feedback for Partial Operations:** If GenerationScope blocks full city generation during live preview, does the UI _inform_ the user that they’re seeing only part of the city? Is it obvious when “lots/buildings” aren’t present because you’re in partial mode?
- **Error Handling:** If an intermediate apply fails, or a tool logic error occurs (e.g. failed to merge a road or district), will the user be notified, and will the editor recover gracefully?
- **Conflicting Edits and Edge Cases:** Consider the case where undo/redo affects objects that are partially present (as a result of GenerationScope). For example, what if the user undoes the last axiom/road and is left in a state where full output is impossible or inconsistent?

---

## 4. **Performance & Scaling Risks**

- **Indexing Maps Cache & Invalidation:** If you add ID→index maps for O(1) lookups, do you have a single source of truth and clear contract for when these are rebuilt (on every mutation, only after batch apply, etc.)? Forgetting to rebuild can cause hard-to-find bugs.
- **ViewportIndexBuilder and Large Scenes:** When the city is very large (10,000+ roads/lots), is the probe-building process parallelized or chunked, so the UI doesn't freeze on each rebuild? Are overlays or rendering passes similarly profiled for large scenes?

---

## 5. **Testing, Regression, and Validation Risks**

- **Test Rewrites Post-Gating:** Many tests assume full output is always applied. After adding GenerationScope and gating, you’ll need to write both old- and new-behavior tests and do regression runs, possibly with both full and partial-output modes.
- **Contract Validation CI:** Does `tools/check_generator_viewport_contract.py` run in CI and block merges if new contract violations are detected? If not, consider enforcing this.
- **Automated UI or Interaction Tests:** Complex tool refactors often break subtle UI flows, especially around selection/multi-modal input. If you lack automated UI tests, budget time for careful manual QA.

---

## 6. **Incremental Rollout, Migration Churn & Backwards Compatibility**

- **Partial Migrations:** If you implement, say, only the RoadTool and leave other tools as stubs, is your UI robust/friendly in the presence of “not yet implemented” tools? (e.g., disables buttons or gives tooltips if a tool isn’t ready?)
- **Backwards Compatibility with Scripts/Plugins:** If advanced users/scriptors have written plugins, overlays, or batch operations that assume the old (single-tool/monolith undo stack), will they break, and is there a plan for a compatibility layer or helper script conversion?

---

## 7. **Ownership, Layering, and Source of Truth**

- **Input Routing Contract:** You have the dual input routing concern: ProcessNonAxiomViewportInteraction pipeline vs `PrimaryViewport::update`/`IViewportTool`. Are you choosing and documenting a clear single source of truth for all future tool/device inputs? (Risk: tool implementers or new contributors split the architecture again.)
- **Dirty Settle/Propagation:** Is it clear which layer (panel, tool, or pipeline) is responsible for setting dirty flags, and which is responsible for triggering generation? Consistency here is crucial.
- **Command-Only Mutations:** Is enforcement (in variant tools that come after) via asserts or helper APIs, or is it left to code review?

---

## 8. **User Experience and Discoverability**

- **Help and Documentation:** Are you updating in-editor help, tooltips, and/or documentation pages to reflect new behaviors (e.g. what GenerationScope does, new tool workflows, undo unification)?
- **Selection/Highlight Consistency:** If you introduce selection highlighting at a deeper layer (e.g., in overlays), ensure state is always visually correct when undo/redo or tool switching happens.

---

## 9. **Concurrent Workflows & Multiple Users**

- **Merge/Rebase Hazards:** If other contributors are working on the same codebase (or you sync with upstream/public forks), is the migration staged to minimize conflicts?
- **Team Onboarding:** Will future team members "get" how all tool integration points work after the refactor is merged, or is onboarding documentation/training needed?

---

## 10. **Summary Table—New Problems to Solve / Not in Plan**

|Area|Not Yet Considered / Planned|
|---|---|
|Data/scene compatibility|Migration for old scenes, history/selection|
|Error cases|Consistency after half-applies/undo|
|Usability|User cues for partial applies, tool state|
|Testing|Regression/new behavior test coverage|
|Performance|UI freezes, scaling of maps/indices|
|Architecture|Enforced input routing/documentation|
|Partial rollout|Disabled/hidden unfinished tools|
|Team/process|CI blocking, doc updates, onboarding|

---

## Practical Next Steps

- **Before launching the refactor:** Write a doc or checklist for the above risks so you (and any future contributor) know what architectural decisions, migrations, and user-experience issues to test, document, and communicate as you implement.
- **During code changes:** Add comments where enforcing new contracts or backward compatibility.
- **Pre-merge:** Run manual & automated test passes for UI, data, and undo/redo on large projects. Communicate migration steps to collaborators.
- i Loaded the repository code-search ability and performed lexical searches for the main symbols/files referenced in audit_refactor_plan.md (ProcessNonAxiomViewportInteraction, rc_panel_axiom_editor.cpp, RealTimePreview, CityOutputApplier, PrimaryViewport, rc_property_editor.cpp, ApplyCityOutputToGlobalState, etc.).
- Opened the most relevant files to confirm implementation details and copied exact locations below.
- Note: code search responses were limited in number, so results may be incomplete — check the listed files in the GitHub UI for full context:
    - Repo root: [https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner)
    - Useful quick view (search results came from): [https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/search](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/search)

High-level summary (quick)

- Many Tier-0 / architecture pieces are already present: non-axiom pipeline and a GenerationCoordinator + RealTimePreview exist.
- Several P0 / Tier-1 items still need changes (some small, some structural):
    - Property editor uses a local CommandHistory (needs unification).
    - Live preview currently requests full generation path and ApplyCityOutputToGlobalState applies all layers unconditionally (needs generation-scope gating).
    - Road tool implementations are missing in app/src/Tools/.
    - PrimaryViewport code renders roads in a single flat color and thickness; RenderRoads/hierarchy styling needs work.
    - Input/interaction plumbing (pan/zoom, selection) is largely implemented in the non-axiom pipeline, but PrimaryViewport::update() is still a no-op and the IViewportTool abstraction is absent as a single canonical header.
- I’ve linked each finding to exact files and suggested the minimal required change.

Detailed mapping (plan item → status, file pointers, recommended next step)

TIER 0 — ProcessNonAxiomViewportInteraction Pipeline

- Status: IMPLEMENTED (pipeline exists)
- Where I checked:
    - visualizer/src/ui/viewport/rc_viewport_interaction.cpp — delegates to pipeline and contains axiom-tool dispatch and pan/zoom handling: [https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/visualizer/src/ui/viewport/rc_viewport_interaction.cpp](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/visualizer/src/ui/viewport/rc_viewport_interaction.cpp)
    - visualizer/src/ui/viewport/handlers/rc_viewport_non_axiom_pipeline.cpp — pipeline implementation: [https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/visualizer/src/ui/viewport/handlers/rc_viewport_non_axiom_pipeline.cpp](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/visualizer/src/ui/viewport/handlers/rc_viewport_non_axiom_pipeline.cpp)
- Notes / next step:
    - The split pipeline you asked for is already present as ProcessNonAxiomViewportInteractionPipeline. Verify that the seven handler functions in your plan exist as separate handler files; the pipeline file already references Handlers::ViewportInteractionContext. If any handler is still monolithic, split into per-handler .cpp as planned.

TIER 1 — PropertyHistory unify with Global CommandHistory

- Status: NOT DONE (needs small change)
- Where I checked:
    - visualizer/src/ui/panels/rc_property_editor.cpp — currently has:
        - static CommandHistory history{};
        - CommitLambda/CommitValueChange usage (local history)
        - file: [https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/visualizer/src/ui/panels/rc_property_editor.cpp](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/visualizer/src/ui/panels/rc_property_editor.cpp)
- Recommended change (small, one-line + callsites):
    - Add CommandHistory& to DrawContext (or otherwise pass global history) and replace local static history() with ctx.command_history (update all construction sites of DrawContext).
    - Replace CommitLambda usage to use the global history reference.
    - Manual test: Ctrl+Z in property editor and in viewport share the same stack.

TIER 1 — InputFloat / DragFloat flood commits

- Status: OBSERVED (property editor uses immediate CommitLambda pattern)
- Where I checked:
    - visualizer/src/ui/panels/rc_property_editor.cpp (see CommitLambda / Input widgets) — same file as above.
- Recommended change (small but repetitive):
    - Replace per-character commits with begin/end pattern using ImGui::IsItemActivated() and ImGui::IsItemDeactivatedAfterEdit() and commit only once per edit on deactivation. Apply to DrawSingleDistrict, DrawSingleRoad, DrawSingleLot, DrawSingleBuilding, and batch editors.

TIER 1 — PrimaryViewport Pan & Zoom wiring

- Status: LARGELY IMPLEMENTED (in non-axiom pipeline)
- Where I checked:
    - rc_viewport_interaction.cpp contains zoom and pan handling and forwards axiom tool events: [https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/visualizer/src/ui/viewport/rc_viewport_interaction.cpp](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/visualizer/src/ui/viewport/rc_viewport_interaction.cpp)
    - rc_viewport_non_axiom_pipeline.cpp contains pan/orbit/zoom logic: [https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/visualizer/src/ui/viewport/handlers/rc_viewport_non_axiom_pipeline.cpp](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/visualizer/src/ui/viewport/handlers/rc_viewport_non_axiom_pipeline.cpp)
    - PrimaryViewport::update() is currently a stub (no input handling inside the class): [https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/app/src/Viewports/PrimaryViewport.cpp](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/app/src/Viewports/PrimaryViewport.cpp)
- Notes / next step:
    - The pipeline already moves camera via params.primary_viewport->set_camera_position() so pan/zoom are functional. Add the small UX pieces from plan (cursor change during pan, Spacebar HFSM transition) — those are incremental.

TIER 1 — RoadTool (stub)

- Status: MISSING
- Where I checked:
    - app/src/Tools/ contains only AxiomPlacementTool; RoadTool.cpp/hpp are not present (plan noted missing tools). Verified app/src/Tools/AxiomPlacementTool.cpp exists: [https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/app/src/Tools/AxiomPlacementTool.cpp](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/app/src/Tools/AxiomPlacementTool.cpp)
- Recommended change (small-to-medium):
    - Add app/src/Tools/RoadTool.hpp and RoadTool.cpp implementing IViewportTool-like callbacks and register in InitializePanelRegistry/CMakeLists as the plan prescribes.

TIER 1 — Road rendering: hierarchy color + thickness

- Status: PARTIALLY (roads draw, but flat color & single thickness)
- Where I checked:
    - app/src/Viewports/PrimaryViewport.cpp currently renders roads in flat cyan and single thickness: [https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/app/src/Viewports/PrimaryViewport.cpp](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/app/src/Viewports/PrimaryViewport.cpp)
    - visualizer/src/ui/viewport/rc_viewport_overlays.cpp contains helper find-by-id and overlay logic (no RenderRoads two-pass with hierarchy styling yet): [https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/visualizer/src/ui/viewport/rc_viewport_overlays.cpp](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/visualizer/src/ui/viewport/rc_viewport_overlays.cpp)
- Recommended change:
    - Implement RenderRoads (or update PrimaryViewport::render road block) to switch on road.type / VpProbeData.road_hierarchy and set color + thickness per plan. This is a focused change in a small number of files.

GENERATION / LIVE PREVIEW — GenerationScope & GenerationDepth gating

- Status: PARTIAL (coordinator & preview exist, but no scope gating)
- Where I checked:
    - app/include/RogueCity/App/Integration/RealTimePreview.hpp and app/src/Integration/RealTimePreview.cpp — RealTimePreview exists and request_regeneration currently takes (axioms, config) only: [https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/app/include/RogueCity/App/Integration/RealTimePreview.hpp](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/app/include/RogueCity/App/Integration/RealTimePreview.hpp)
    - app/src/Integration/GenerationCoordinator.cpp and header exist and relay to preview: [https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/app/src/Integration/GenerationCoordinator.cpp](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/app/src/Integration/GenerationCoordinator.cpp)
    - app/include/RogueCity/App/Integration/CityOutputApplier.hpp defines CityOutputApplyOptions without GenerationScope: [https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/app/include/RogueCity/App/Integration/CityOutputApplier.hpp](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/app/include/RogueCity/App/Integration/CityOutputApplier.hpp)
    - app/src/Integration/CityOutputApplier.cpp currently applies roads/districts/lots/buildings unconditionally (collects generated_roads, districts, lots, buildings): [https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/app/src/Integration/CityOutputApplier.cpp](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/app/src/Integration/CityOutputApplier.cpp)
    - rc_panel_axiom_editor.cpp requests regeneration on axiom interactions using RequestRegeneration(...) (no depth param) — so live preview currently triggers full pipeline: [https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/visualizer/src/ui/panels/rc_panel_axiom_editor.cpp](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/visualizer/src/ui/panels/rc_panel_axiom_editor.cpp)
- Recommended change (medium, but well-scoped):
    - Add GenerationDepth (AxiomBounds vs FullPipeline) to RealTimePreview::request_regeneration and to GenerationCoordinator::RequestRegeneration (default to FullPipeline where appropriate).
    - Add GenerationScope to CityOutputApplyOptions and honor it in ApplyCityOutputToGlobalState to skip lots/buildings when scope is RoadsOnly / RoadsAndBounds.
    - Update rc_panel_axiom_editor.cpp to request AxiomBounds depth when live-axiom previewing (so axiom edits only produce road skeleton + tensor output), and keep FullCity for explicit ForceGenerate.

ApplyCityOutputToGlobalState behavior (applier gating)

- Status: APPLIER IMPLEMENTS FULL DUMP (needs gating)
- Where I checked:
    - app/src/Integration/CityOutputApplier.cpp — it prepares generated_roads/districts/lots/buildings and proceeds to assign them; there is currently no generation-scope switch: [https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/app/src/Integration/CityOutputApplier.cpp](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/app/src/Integration/CityOutputApplier.cpp)
    - app/include/RogueCity/App/Integration/CityOutputApplier.hpp — no GenerationScope in the options: [https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/app/include/RogueCity/App/Integration/CityOutputApplier.hpp](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/app/include/RogueCity/App/Integration/CityOutputApplier.hpp)
- Recommended change:
    - Add enum GenerationScope and CityOutputApplyOptions::scope defaulting to FullCity. Gate application of districts/lots/buildings accordingly. Also add options to avoid unnecessary ViewportIndex rebuilds when only roads changed.

rc_panel_axiom_editor.cpp live-preview trigger

- Status: PRESENT (requests regeneration for live preview currently)
- Where I checked:
    - visualizer/src/ui/panels/rc_panel_axiom_editor.cpp — it calls s_generation_coordinator->RequestRegeneration(...) when axioms change and s_live_preview true: [https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/visualizer/src/ui/panels/rc_panel_axiom_editor.cpp](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/visualizer/src/ui/panels/rc_panel_axiom_editor.cpp)
- Recommended change:
    - Change RequestRegeneration call to pass GenerationDepth::AxiomBounds for live axiom editing. Optionally, set s_external_dirty = false after request as currently done, but ensure applier honors scope so full city is not applied.

SelectionManager / ViewportIndex wiring

- Status: PARTIAL
- Where I checked:
    - rc_viewport_overlays.cpp contains FindRoadById/FindDistrictById scanning arrays (no ID maps yet): [https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/visualizer/src/ui/viewport/rc_viewport_overlays.cpp](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/visualizer/src/ui/viewport/rc_viewport_overlays.cpp)
    - There is ViewportIndex and VpProbeData structures in core/include (VpProbeData exists) — the index builder invocation is referenced in the plan but make sure it's called after generation: see tools/docs generator-viewport contract and the generator applier options to rebuild index.
- Recommended change:
    - Add ID->index unordered_maps in GlobalState or maintain VpProbeData flat array rebuild after generation; wire PickFromViewportIndex to be used by GlobalSelectHandler and by box-select code (non-axiom pipeline uses Selection logic, but confirm the actual pick call is present).

IViewportTool abstraction and PrimaryViewport::update

- Status: PARTIAL
- Where I checked:
    - There is no app/include/RogueCity/App/Tools/IViewportTool.hpp currently (search found no results).
    - AxiomPlacementTool already exposes on_mouse_down/on_mouse_up/on_mouse_move style functions: app/src/Tools/AxiomPlacementTool.cpp — it uses that shape: [https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/app/src/Tools/AxiomPlacementTool.cpp](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/app/src/Tools/AxiomPlacementTool.cpp)
    - PrimaryViewport::update() is an empty stub in PrimaryViewport.cpp: [https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/app/src/Viewports/PrimaryViewport.cpp](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/app/src/Viewports/PrimaryViewport.cpp)
- Recommended change:
    - Add IViewportTool.hpp and update PrimaryViewport to hold IViewportTool* active_tool_ and dispatch events (or keep dispatcher in rc_viewport_interaction but explicit interface header will help). Refactor Road/District/Water tools to implement the interface.

Other useful finds & small wins (low-effort)

- tools/check_generator_viewport_contract.py exists and codifies the contract — use it to validate changes: [https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/tools/check_generator_viewport_contract.py](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/tools/check_generator_viewport_contract.py)
- GenerationCoordinator + RealTimePreview are solid bases for debounced preview + on_complete callbacks; adding depth/scope flags will slot in cleanly.
- Many utility functions for rendering overlays and selection anchors exist (rc_viewport_overlays.cpp) — reusing those will minimize rendering changes.

Concrete prioritized next steps (smallest-impact first)

1. Property editor: Replace local static CommandHistory with global CommandHistory in DrawContext (one-line change + ctor updates). Then convert InputFloat commits to commit-on-deactivate pattern. (High priority / small code churn)
2. Add GenerationScope to CityOutputApplyOptions and gate ApplyCityOutputToGlobalState — skip lots/buildings when scope != FullCity. (High impact, contained to applier)
3. Add GenerationDepth param to RealTimePreview::request_regeneration and GenerationCoordinator::RequestRegeneration, default to existing behavior; adapt rc_panel_axiom_editor to request AxiomBounds for live axiom edits. (Medium)
4. Implement RoadTool stub (add files, register in registry & CMake). (Medium)
5. Implement road hierarchy rendering: change PrimaryViewport::render road loop (or add ViewportOverlays::RenderRoads) to switch on road type -> color/thickness. (Small)
6. Replace ad-hoc array scans with optional ID->index maps where lookups are frequent (property panel, overlays). (Medium/optional)
7. Add IViewportTool.hpp and migrate other tools when convenient (refactor rather than rewrite). (Lower priority)
8. ### Copilot said: Continuing the audit from where I left

What I checked (file → short finding)

- visualizer/src/ui/viewport/rc_viewport_interaction.cpp
    
    - Confirms axiom tool dispatch, zoom/pan handling, and delegates non-axiom work to the pipeline (calls ProcessNonAxiomViewportInteraction). The file contains logic wiring axiom tool mouse callbacks into the viewport.
    - (Found: pan/zoom mouse handling and axiom tool on_mouse_down/up/move/right_click.)
- visualizer/src/ui/viewport/handlers/rc_viewport_non_axiom_pipeline.cpp
    
    - Contains the non-axiom pipeline implementation and the interaction context used by handlers. The pipeline exposes interaction result fields (handled/outcome/requires_explicit_generation).
    - (Found: pan/orbit/zoom, selection handling, dirty-layer detection, and outcome classification.)
- app/include/RogueCity/App/Viewports/PrimaryViewport.hpp and app/src/Viewports/PrimaryViewport.cpp
    
    - PrimaryViewport class exists with camera setters/getters and screen/world transforms.
    - PrimaryViewport::update(float) is currently a stub (no internal input dispatch); rendering code exists and draws roads/districts from a CityOutput pointer.
    - PrimaryViewport::render currently draws roads in a flat cyan color with a small branch for Highway/Arterial thickness — but it does not implement the richer two-pass / hierarchy color system described in the plan.
- visualizer/src/ui/panels/rc_panel_axiom_editor.cpp (+ .h)
    
    - Panel initializes GenerationCoordinator and wires an OnComplete callback that currently calls ApplyCityOutputToGlobalState(output, gs) directly (no scope gating).
    - Panel requests regeneration for live-axiom edits via s_generation_coordinator->RequestRegeneration(...), with the existing API (no generation-depth param).
    - Panel uses ProcessNonAxiomViewportInteraction for non-axiom input (compliant with contract).
- app/include/RogueCity/App/Integration/RealTimePreview.hpp and app/src/Integration/RealTimePreview.cpp
    
    - RealTimePreview exists: debounced preview, background generation thread, set_on_complete callback, request_regeneration/force_regeneration APIs. Current APIs do not take a generation depth/scope parameter.
- app/include/RogueCity/App/Integration/CityOutputApplier.hpp and app/src/Integration/CityOutputApplier.cpp
    
    - ApplyCityOutputToGlobalState exists and currently applies generated roads, districts, lots, buildings (it builds generated_roads/districts/lots/buildings and proceeds to assign them).
    - CityOutputApplyOptions currently lacks a GenerationScope; no gating is implemented in the applier.
- app/src/Integration/GenerationCoordinator.cpp + header
    
    - GenerationCoordinator wraps RealTimePreview and forwards RequestRegeneration/ForceRegeneration calls; it offers SetOnComplete and Update() and exposes the preview pointer. Current RequestRegeneration signature does not include depth.
- visualizer/src/ui/panels/rc_property_editor.cpp
    
    - The property editor defines a local static CommandHistory via PropertyHistory() { static CommandHistory history{}; } and uses CommitLambda/CommitValueChange to commit edits.
    - Many ImGui InputFloat/InputFloat2 calls commit immediately inside the Input* branch (flood-on-typing behavior).
- visualizer/src/ui/viewport/rc_viewport_overlays.cpp
    
    - Has FindXById helpers and ResolveAnchor helpers used by overlays; useful rendering/anchor helpers are present. No dedicated RenderRoads two-pass function with hierarchy color/thickness is found (existing road rendering is in PrimaryViewport::render and minimap).
- app/src/Editor/ViewportIndexBuilder.cpp and app/include/RogueCity/App/Editor/ViewportIndexBuilder.hpp
    
    - ViewportIndexBuilder::Build exists and populates gs.viewport_index from gs.districts/lots/buildings/roads/waterbodies — the builder is present and constructs the probe array.
- visualizer/src/ui/viewport/handlers/rc_viewport_handler_common.cpp and related pick code
    
    - ProbeContainsPoint / PickFromViewportIndex / QueryRegionFromViewportIndex exist and implement priority rules (Building > Lot > Road > District > Water > Axiom). These functions iterate the viewport_index and check geometric containment/distance.
- visualizer/src/ui/viewport/handlers/rc_viewport_non_axiom_pipeline.cpp (selection wiring)
    
    - The non-axiom pipeline calls PickFromViewportIndex and applies selection via gs.selection_manager, sets explicit_generation flags when the domain requires it, and classifies InteractionOutcome (Mutation/Selection/GizmoInteraction/NoEligibleTarget).
- app/src/Tools/AxiomPlacementTool.cpp
    
    - Implements axiom tool behavior and has on_mouse_down/on_mouse_move/on_mouse_up style methods; it already follows the pattern you suggested for tools.
- app/src/Tools/ — RoadTool/DistrictTool/WaterTool
    
    - The Tools directory contains AxiomPlacementTool but there is no RoadTool/DistrictTool/LotTool/BuildingTool/WaterTool implementation files present (these tool implementations are missing).
- generators (RoadGenerator, StreamlineTracer, LotGenerator, etc.)
    
    - Generators produce road/district/lot/building outputs. Road generation produces Road.type fields used for classification. The generator code supports incremental stages (tensor → roads → districts → lots → buildings) in various places.
- tests/integration test files
    
    - Tests call ApplyCityOutputToGlobalState and assert that after full pipeline generation the GlobalState contains roads/districts/lots/buildings and that the viewport_index is built. Tests assume the applier applies the full pipeline output.
- docs / enforcement scripts
    
    - docs/20_specs/generator-viewport-contract.md and docs/20_specs/tool-viewport-generation-contract.md codify the intended ownership and invariants (e.g., panels must use GenerationCoordinator and the applier).
    - tools/check_generator_viewport_contract.py exists and checks for many of the contract invariants (presence of applier call in the panel, use of GenerationCoordinator, ProcessNonAxiomViewportInteraction usage, etc.).

Concrete status summary (plan items → current state)

- ProcessNonAxiomViewportInteraction pipeline: implemented (pipeline file present).
- Global navigation / pan & zoom: implemented in pipeline; viewport camera setters used by pipeline; PrimaryViewport::update is stubbed.
- Axiom live preview generation: implemented via GenerationCoordinator + RealTimePreview + panel request, but API currently has no depth/scope parameter (so live preview requests trigger full pipeline and applier applies everything).
- ApplyCityOutputToGlobalState: implemented and currently applies full output (roads + districts + lots + buildings). No GenerationScope gating yet.
- RealTimePreview: implemented (debounced, background, on_complete), but lacks depth flag.
- GenerationCoordinator: implemented (forwards to preview).
- Road rendering in PrimaryViewport: basic implementation exists (flat cyan + small thickness tweak for Highway/Arterial) — not yet the full hierarchy color/thickness/painter-order described in the plan.
- RenderRoads two-pass painter algorithm: not present as a dedicated implementation; roads are drawn directly in PrimaryViewport::render and the minimap.
- IViewportTool interface/header: not found (AxiomPlacementTool implements the shape but there is no canonical IViewportTool.hpp).
- App-layer tool implementations for Road/District/Lot/Building/Water: mostly missing (Axiom tool is the only fully implemented app tool).
- SelectionManager / ViewportIndex pick: implemented (PickFromViewportIndex + ViewportIndexBuilder exist and are wired into the non-axiom pipeline).
- Property editor undo history: NOT unified — property panel uses a local static CommandHistory.
- ImGui per-character commit patterns: present in property editor (CommitLambda called inside InputFloat/DragFloat branches) — risk of flooding CommandHistory.
- Tests: integration tests assume ApplyCityOutputToGlobalState applies full pipeline output and that viewport_index is constructed; these tests validate current behavior.

Files I opened (representative)

- visualizer/src/ui/viewport/rc_viewport_interaction.cpp
- visualizer/src/ui/viewport/handlers/rc_viewport_non_axiom_pipeline.cpp
- app/src/Viewports/PrimaryViewport.cpp / app/include/RogueCity/App/Viewports/PrimaryViewport.hpp
- visualizer/src/ui/panels/rc_panel_axiom_editor.cpp / .h
- app/include/RogueCity/App/Integration/RealTimePreview.hpp / app/src/Integration/RealTimePreview.cpp
- app/include/RogueCity/App/Integration/CityOutputApplier.hpp / app/src/Integration/CityOutputApplier.cpp
- app/src/Integration/GenerationCoordinator.cpp
- visualizer/src/ui/panels/rc_property_editor.cpp
- visualizer/src/ui/viewport/rc_viewport_overlays.cpp
- app/src/Editor/ViewportIndexBuilder.cpp
- visualizer/src/ui/viewport/handlers/rc_viewport_handler_common.cpp
- app/src/Tools/AxiomPlacementTool.cpp
- generators/src/Generators/Urban/RoadGenerator.cpp and related generator code
- tests/integration/test_full_pipeline.cpp
- docs/20_specs/generator-viewport-contract.md
- tools/check_generator_viewport_contract.py

Observed invariants and implications (facts)

- The codebase enforces a single generation-to-application path: GenerationCoordinator → RealTimePreview → ApplyCityOutputToGlobalState (panel sets OnComplete to call ApplyCityOutputToGlobalState). Because the applier currently applies the full output, any live preview requests that use the existing RequestRegeneration API will result in full-state application on completion.
- The non-axiom pipeline already implements selection, pick priority, and explicit-generation signaling (requires_explicit_generation). The viewport index builder exists and is suitable for fast pick queries once called after generation.
- The property editor currently maintains a separate undo stack; this is a real divergence from the rest of the editor undo model.
- Tests assume the applier applies full output (so changing applier behavior will need test updates).

Coverage note

- The files I opened cover the highest-impact areas called out in your plan (non-axiom pipeline, preview/coordinator, applier, axiom panel, property editor, viewport index, pick/selection, PrimaryViewport rendering, tool implementations). The repo search I used returned these hits; there may be more supporting files, helpers, or variant implementations

Here’s a detailed checklist of **what still needs to be done** to fully comply with the audit/refactor plan (`audit_refactor_plan.md`), based on the current state of your codebase as revealed by all searches and file inspections:

---

## Critical Refactor and Implementation Work

### 1. **Partial Generation: "GenerationScope" & "GenerationDepth" Support**
- **What’s missing:**
  - **CityOutputApplyOptions** (in `CityOutputApplier.hpp`) does _not_ have a scope field (enum GenerationScope: RoadsOnly, RoadsAndBounds, FullCity).
  - **ApplyCityOutputToGlobalState** (in `CityOutputApplier.cpp`) always applies roads, districts, lots, buildings; it lacks conditional application based on scope.
  - **RealTimePreview, GenerationCoordinator, rc_panel_axiom_editor.cpp** only request full generation; “AxiomBounds” preview is not supported.

- **What to do:**
  - Add a `GenerationScope` field to `CityOutputApplyOptions` and update `ApplyCityOutputToGlobalState` to honor the scope (skip lots/buildings except for FullCity).
  - Extend `RealTimePreview` and `GenerationCoordinator` to accept/forward `GenerationDepth` (AxiomBounds vs FullPipeline).
  - Update axiom panel’s live preview request to use “AxiomBounds” depth (shows only road skeleton + axiom boundaries).
  - Make explicit full city generation via button (ForceGenerate) use "FullCity"/"FullPipeline".

---

### 2. **Undo/Redo Stack Integration**
- **What’s missing:**
  - Property editors (especially `rc_property_editor.cpp`) still use a local static `CommandHistory`. Undo/redo isn’t unified across the editor.

- **What to do:**
  - Pass the global `CommandHistory` instance via `DrawContext` (in `IPanelDrawer.h`) and update all property-editor logic to use the shared history.
  - Remove the local static CommandHistory in `PropertyHistory()`.

---

### 3. **Flooding CommandHistory: Input Commit Pattern**
- **What’s missing:**
  - All ImGui input (`InputFloat`, `InputFloat2`, `DragFloat`, etc.) in property editor commits a change **every keystroke**: this floods the undo stack.
- **What to do:**
  - Replace per-character commit with:
    - Store snapshot on `ImGui::IsItemActivated()`
    - Only commit when `ImGui::IsItemDeactivatedAfterEdit()` is true.

---

### 4. **App Tool Implementation (Viewport_Tool System)**
- **What’s missing:**
  - Only `AxiomPlacementTool` is implemented in `app/src/Tools/`.
  - No source files for: `RoadTool`, `DistrictTool`, `LotTool`, `BuildingTool`, `WaterTool`.

- **What to do:**
  - Implement at least stubs for each tool (`RoadTool.cpp/.hpp`, `DistrictTool.cpp/.hpp`, etc.), inheriting the same interface style as `AxiomPlacementTool`. Follow the contracts in your audit doc.
  - Register tools in panel registry/CMake.

---

### 5. **IViewportTool Interface**
- **What’s missing:**
  - No canonical `IViewportTool.hpp` that defines mouse/scroll/render interface for tool classes.
  - Tool system is not fully abstracted for switching between active tools.

- **What to do:**
  - Create the interface header and refactor existing and new tools to use this abstraction.
  - Update `PrimaryViewport` to hold and dispatch input to `IViewportTool*`.

---

### 6. **Road Rendering: Hierarchy Colors and Thickness**
- **What’s missing:**
  - PrimaryViewport renders roads in flat cyan (except basic thickness tweak for Highway/Arterial).
  - No painter’s algorithm ordering or color scheme as specified.

- **What to do:**
  - Refactor road rendering (in `PrimaryViewport::render` and/or move to a new `RenderRoads()` overlay function) to use type-based color/thickness following audit palette.
  - Implement painter’s algorithm (two-pass render: casing then fill, layered with districts/lots/buildings/water).

---

### 7. **Property Editor and SelectionManager Integration**
- **What’s missing:** 
  - Property editor, overlays, and index panels scan arrays for entity lookup (O(n)), do not use ID-to-index maps.
- **What to do:**
  - Optionally, add unordered maps for ID lookup in `GlobalState` for roads, districts, lots, buildings (for faster selection, editing, overlays).

---

### 8. **Tool Execution Contracts: BoxSelect, Road Vertex Drag, District Polygon, etc.**
- **What’s missing:**
  - Only partial implementation for selection and box select; road vertex drag, district vertex insert/remove, lot editing, and building placement are unimplemented at tool level.

- **What to do:**
  - Wire up handlers in the non-axiom pipeline and app tool classes for these behaviors. Use `ViewportIndexBuilder` and `SelectionManager` after generation to rebuild pickable index.

---

### 9. **Tests and Contract Checks**
- **What’s missing:** 
  - Tests and scripts assume full city application. These may need updating after partial/scope-based application.
- **What to do:**
  - Update tests as needed, and run `tools/check_generator_viewport_contract.py` to confirm compliance.

---

## Optional/Polish or Architectural

- Implement ID->index maps for entity lookup (`O(1)` instead of array scans).
- Refactor panel drawers ("one file per panel") for maintainability.
- Enhance overlays with Y2K palette, grid labeling, interactive highlights.
- Add missing “stamp axiom” quick-place pattern and “Inspector” ephemeral tool as described.

---

## Summary Table

| Task                                      | Implemented? | File(s) | To Do |
|-------------------------------------------|--------------|---------|-------|
| GenerationScope/Depth                     | ❌           | CityOutputApplier, RealTimePreview | Add fields, gating, param propagation |
| Undo/Redo unified stack                   | ❌           | rc_property_editor.cpp, DrawContext | Use shared CommandHistory |
| Commit per-edit, not per-char              | ❌           | rc_property_editor.cpp | Use IsItemActivated/IsItemDeactivatedAfterEdit |
| Road/District/Lot/Building/Water tools    | ❌           | app/src/Tools/ | Code stubs, implement tool contracts |
| IViewportTool abstraction                 | ❌           | app/include/ | Add interface header, refactor |
| Hierarchy-based road rendering            | ❌           | PrimaryViewport.cpp | Refactor render to use color/thickness |
| O(1) ID lookups                           | ❓ (partial) | overlays, index panels | Add maps if needed |
| Selection/box select/vertex drag          | ❓ (partial) | pipeline, SelectionManager, EditorManipulation | Wire up handlers |
| Contract tests                            | ❓ (partial) | tests/integration/, tools/check_generator_viewport_contract.py | Update after partial apply |

---

## Final Statement

**Major next steps:**  
- Implement partial generation gating (GenerationScope/Depth, conditional applier, update preview/panel API).
- Unify undo/redo stacks and update property editor commit pattern.
- Stub/default app tools (Road, District, Lot, Building, Water), add interface abstraction.
- Refactor road rendering for visual hierarchy.
- Wire up any missing box select, road vertex drag, and district editing features.

**Once you do these, your codebase will align with the architectural contract and the actionable audit plan.**

---

 


## The One Structural Change That Enables All of This #missioncritical 

Right now `ProcessNonAxiomViewportInteraction` is a monolith that handles pan, zoom, domain placement, gizmo, box-select, lasso, hover, and click-select in a single ~600-line function with a consumed/unconsumed flag threading through it. Every global tool you add makes it longer.

Before Tier 2, split it into a pipeline of discrete handlers with an early-exit consumed flag:

cpp

```cpp
// Ordered pipeline — first handler to set consumed = true wins
GlobalNavigationHandler(gs, params, result);   // pan, zoom, orbit — never consumed
GlobalSelectHandler(gs, params, result, state); // V key, Esc
GlobalGizmoHandler(gs, params, result, state);  // existing gizmo code
GlobalLassoBoxHandler(gs, params, result, state); // existing box/lasso
DomainPlacementHandler(gs, params, result, state); // existing HandleDomainPlacementActions
GlobalInspectHandler(gs, params, result);       // hover tooltip
GlobalHoverHandler(gs, params, result);         // existing hover/click-select
```

Each handler is a free function in its own .cpp file. `NonAxiomInteractionState` gets split into sub-state structs per handler. `ToolInteractionMetrics` stays the single source of world-space pick radii shared across all of them.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/visualizer/src/ui/tools/rc_tool_interaction_metrics.h)​

This gives you a clean insertion point for every future global tool without touching existing domain logic.

---
# ATOMIC CHECKLIST & ACTIONABLES
# RogueCities — Viewport Tool Execution Master Pass
# Last Updated: 2026-02-19
# Status Legend: [ ] = Open  [x] = Done  [~] = In Progress  [!] = Blocked
---

## ═══════════════════════════════════════════
## TIER 0 — STRUCTURAL REFACTOR (Do First, Unlocks Everything)
## ═══════════════════════════════════════════

### 0.1 — ProcessNonAxiomViewportInteraction Pipeline Split
- [ ] **0.1.1** Create `GlobalNavigationHandler(gs, params, result)` as a free function in `GlobalNavigationHandler.cpp` — handles pan + zoom, **never** sets `consumed`
- [ ] **0.1.2** Create `GlobalSelectHandler(gs, params, result, state)` as a free function in `GlobalSelectHandler.cpp` — handles `V` key + `Esc`
- [ ] **0.1.3** Create `GlobalGizmoHandler(gs, params, result, state)` — extract existing gizmo code verbatim into its own `.cpp`
- [ ] **0.1.4** Create `GlobalLassoBoxHandler(gs, params, result, state)` — extract existing box/lasso code verbatim into its own `.cpp`
- [ ] **0.1.5** Create `DomainPlacementHandler(gs, params, result, state)` — extract existing `HandleDomainPlacementActions` verbatim
- [ ] **0.1.6** Create `GlobalInspectHandler(gs, params, result)` — hover tooltip dispatch
- [ ] **0.1.7** Create `GlobalHoverHandler(gs, params, result)` — extract existing hover/click-select code
- [ ] **0.1.8** Replace the monolithic `ProcessNonAxiomViewportInteraction` body with an ordered pipeline of the 7 handler calls above, each passing the `consumed` flag as an early-exit gate
- [ ] **0.1.9** Split `NonAxiomInteractionState` into per-handler sub-state structs (one struct per `.cpp` above)
- [ ] **0.1.10** Verify `ToolInteractionMetrics` is the single shared source of world-space pick radii passed to all handlers — remove any duplicate radius constants

---

## ═══════════════════════════════════════════
## TIER 1 — P0 CRITICAL BUGS (Ship Nothing Until These Are Fixed)
## ═══════════════════════════════════════════

### 1.1 — PropertyHistory: Unify with Global CommandHistory
- [ ] **1.1.1** Add `CommandHistory& command_history` field to `DrawContext` struct (one-line change)
- [ ] **1.1.2** Update every `DrawContext` construction site to pass the global `CommandHistory` reference
- [ ] **1.1.3** In `rc_property_editor.cpp`, delete `static CommandHistory history` (the local static)
- [ ] **1.1.4** Replace every reference to the local `history` with `ctx.command_history` throughout `rc_property_editor.cpp`
- [ ] **1.1.5** Verify `Ctrl+Z` in the property editor and `Ctrl+Z` in the viewport now operate on the same stack — write a manual test: edit a road type, draw a road, undo twice, confirm both undo correctly

### 1.2 — InputFloat/DragFloat Command-Per-Frame Flooding Fix
- [ ] **1.2.1** In `DrawSingleDistrict`, wrap every `ImGui::InputFloat2` vertex edit with `IsItemActivated` (capture `before`) and `IsItemDeactivatedAfterEdit` (commit `CommitLambda`) — remove the current per-character `CommitLambda` call
- [ ] **1.2.2** Apply the same `IsItemActivated` / `IsItemDeactivatedAfterEdit` pattern to every `InputFloat`, `InputScalar`, and `DragFloat` in `DrawSingleRoad`
- [ ] **1.2.3** Apply same pattern to `DrawSingleLot`
- [ ] **1.2.4** Apply same pattern to `DrawSingleBuilding`
- [ ] **1.2.5** Apply same pattern to `DrawBatchEditor` / `ApplyBatchValue` fields
- [ ] **1.2.6** Manual test: rapid-type a value into a district vertex field, release focus, confirm exactly **1** entry appears in `CommandHistory`, not N

### 1.3 — PrimaryViewport Pan: Wire Input
- [ ] **1.3.1** In `ProcessNonAxiomViewportInteraction` (or `GlobalNavigationHandler` after 0.1 refactor), add MMB pan block: on `io.MouseDown[ImGuiMouseButton_Middle]`, compute `delta_world = {-io.MouseDelta.x / zoom * pan_speed, -io.MouseDelta.y / zoom * pan_speed}`, call `params.primary_viewport->set_camera_position(cam + delta_world, camera_z)`
- [ ] **1.3.2** Add `Shift+RMB` as alternate pan modifier (same logic)
- [ ] **1.3.3** Set `ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll)` while pan is active
- [ ] **1.3.4** Add `EditorState::Viewport_Pan` HFSM transition on `Spacebar` held (Blender-style), revert to previous state on release

### 1.4 — PrimaryViewport Zoom: Wire Input
- [ ] **1.4.1** In the global pre-dispatch block, add scroll zoom: `if (io.MouseWheel != 0.0f && params.in_viewport && !params.minimap_hovered)` → compute `new_z = clamp(camera_z * pow(1.12f, -io.MouseWheel), 80.0f, 4000.0f)` → call `set_camera_position(camera_xy, new_z)`
- [ ] **1.4.2** Set `result.outcome = InteractionOutcome::ActivateOnly` (do NOT return — zoom must not consume clicks)
- [ ] **1.4.3** Add `Ctrl+=` / `Ctrl+-` keyboard zoom steps (same clamp, fixed step factor of 1.25)
- [ ] **1.4.4** Add `Ctrl+0` reset-to-fit: compute bounding box of all entities, set `camera_xy` to centroid, set `camera_z` derived from `bbox_diagonal / viewport_zoom`

### 1.5 — Road Tool: Create Stub Implementation
- [ ] **1.5.1** Create `app/src/Tools/RoadTool.hpp` — declare `class RoadTool : public IViewportTool`
- [ ] **1.5.2** Create `app/src/Tools/RoadTool.cpp` — implement `on_mouse_down`: commit `point[0] = world_pos`, enter `Mode::Drawing`
- [ ] **1.5.3** Implement `on_mouse_move`: update ghost segment endpoint to `mouse_world`; if `snap_to_grid` is enabled, snap to `round(x / snap_size) * snap_size`
- [ ] **1.5.4** Implement subsequent `on_mouse_down` (while `Mode::Drawing`): if within 10m of existing road endpoint → mark as **MERGE** junction; if within 10m of existing road midpoint → mark as **SPLIT** junction; otherwise append point to polyline
- [ ] **1.5.5** Implement `on_double_click` / `Enter` key: finalize road, create `Road` entity, push to `GlobalState.roads`, set `DirtyLayer::Roads`, push `AddRoadCommand` to `CommandHistory`
- [ ] **1.5.6** Implement `Escape`: clear ghost, return to `Mode::Idle`
- [ ] **1.5.7** Implement `on_right_click` on existing road: open context menu with options `Set Hierarchy` (submenu: Local / Collector / Arterial / Highway), `Delete`, `Split Here`
- [ ] **1.5.8** Register `RoadTool` in `InitializePanelRegistry` / tool dispatcher; wire `EditorEvent::Tool_Roads` HFSM transition to activate it
- [ ] **1.5.9** Add `RoadTool.cpp` to `app/CMakeLists.txt`

### 1.6 — Road Rendering: Hierarchy Color + Thickness
- [ ] **1.6.1** In `PrimaryViewport::render()`, replace the single `road_color = IM_COL32(0,255,255,255)` with a switch on `road.type` (or `VpProbeData.road_hierarchy`): Highway → `IM_COL32(255,160,40,255)` amber / Arterial → `IM_COL32(0,220,180,255)` teal / Collector → `IM_COL32(80,180,255,255)` sky blue / Local → `IM_COL32(40,130,200,200)` dim blue
- [ ] **1.6.2** Replace hardcoded `thickness` logic with the same switch: Highway=6.0f / Arterial=4.0f / Collector=3.0f / Local=2.0f

---

## ═══════════════════════════════════════════
## TIER 2 — P1 HIGH (Needed for a Usable Session)
## ═══════════════════════════════════════════

### 2.1 — IViewportTool Interface: Generalize the Axiom Pattern
- [ ] **2.1.1** Create `app/include/RogueCity/App/Tools/IViewportTool.hpp` — declare pure virtual interface: `on_mouse_down(Vec2, ImGuiMouseButton)`, `on_mouse_up(Vec2, ImGuiMouseButton)`, `on_mouse_move(Vec2)`, `on_scroll(float)`, `render(ImDrawList*, const PrimaryViewport&)`, virtual destructor
- [ ] **2.1.2** Refactor `AxiomPlacementTool` to inherit from `IViewportTool` (should be near-zero work — it already implements this shape)
- [ ] **2.1.3** Add `IViewportTool* active_tool_` member to `PrimaryViewport`
- [ ] **2.1.4** In `PrimaryViewport::update(float dt)`, replace the `(void)delta_time` stub with: `if (active_tool_ && is_hovered()) { dispatch ImGui mouse state → active_tool_->on_mouse_down/up/move/scroll }`
- [ ] **2.1.5** In `PrimaryViewport::render()`, call `if (active_tool_) active_tool_->render(draw_list, *this)` after city entity rendering

### 2.2 — SelectionManager: Wire into PrimaryViewport
- [ ] **2.2.1** After every city generation completes, call `ViewportIndexBuilder` to rebuild the `VpProbeData` flat array
- [ ] **2.2.2** In `GlobalSelectHandler` (or `Viewport_Select` path), on left-click: call `PickFromViewportIndex(mouse_world, probes)` using the priority order Building(5) > Lot(4) > Road(3) > District(2) > Water(1) > Axiom(0)
- [ ] **2.2.3** On hit: call `SelectionManager::SetSelection({item})` / on miss: call `SelectionManager::ClearSelection()`
- [ ] **2.2.4** On `Shift+click`: call `SelectionManager::AppendSelection({item})`
- [ ] **2.2.5** In `PrimaryViewport::render()`, for each entity in the active selection, draw a highlight accent (white outline rect or polygon, 1px, full alpha)

### 2.3 — Box Select: Wire `Viewport_BoxSelect` State
- [ ] **2.3.1** In `GlobalLassoBoxHandler`, on `Alt+Shift+LMB` drag: record `box_start = world_pos`
- [ ] **2.3.2** On `on_mouse_move` while box-selecting: draw dashed white selection rectangle overlay via `ImDrawList::AddRect`
- [ ] **2.3.3** On `on_mouse_up`: collect all `VpProbeData` whose world AABB intersects the drag rect → push to `SelectionManager::SetSelection(results)`

### 2.4 — Road Vertex Drag
- [ ] **2.4.1** In `Viewport_Select` mode with a road selected: in `render()`, draw each `road.points[i]` as a 5×5 white square at `world_to_screen(p)`
- [ ] **2.4.2** In `on_mouse_down`, if mouse is within 8px of a road vertex square: enter `Mode::DraggingVertex`, record `vertex_index` and `drag_start_snapshot`
- [ ] **2.4.3** In `on_mouse_move` while `Mode::DraggingVertex`: call `EditorManipulation::MoveRoadVertex(road, vertex_index, world_pos)`
- [ ] **2.4.4** In `on_mouse_up`: push `ModifyRoadVertexCommand` to `CommandHistory` with before/after snapshots

### 2.5 — District Tool: Create Implementation
- [ ] **2.5.1** Create `app/src/Tools/DistrictTool.hpp` + `DistrictTool.cpp` implementing `IViewportTool`
- [ ] **2.5.2** `on_mouse_down`: append `world_pos` to in-progress `pending_border` vector; if within 15m of `pending_border[0]` and `pending_border.size() >= 3` → close polygon → push `AddDistrictCommand`
- [ ] **2.5.3** `on_mouse_move`: draw ghost edge from `pending_border.back()` to `mouse_world`
- [ ] **2.5.4** `Shift+drag` on existing border edge: call `EditorManipulation::InsertDistrictVertex` at edge midpoint, push `InsertDistrictVertexCommand`
- [ ] **2.5.5** `Alt+click` on existing vertex: call `EditorManipulation::RemoveDistrictVertex` (guard `border.size() > 3`), push `RemoveDistrictVertexCommand`
- [ ] **2.5.6** Drag existing vertex: call `EditorManipulation::ApplyTranslate` on that single vertex, push `ModifyDistrictVertexCommand`
- [ ] **2.5.7** `Right-click` on closed district: open context menu → `Set DistrictType` submenu (Residential / Commercial / Industrial / Mixed / Park)
- [ ] **2.5.8** Wire `EditorEvent::Tool_Districts` HFSM transition; add to `CMakeLists.txt`

### 2.6 — Water Body Rendering
- [ ] **2.6.1** In `PrimaryViewport::render()`, add a water body render pass after the lot pass: for each `WaterBody` in `GlobalState.waterbodies`, project `boundary` points to screen, call `ImDrawList::AddConvexPolyFilled` with `IM_COL32(0,80,180,80)`, then `AddPolyline` with `IM_COL32(0,200,255,255)`, thickness 2.0f
- [ ] **2.6.2** Add `VpProbeData.flood` visualization: if `probe.flood > 

## PART 1 — TOOLS & EDITORS CATALOG (What Exists + Expected Behavior)

### Layer 1: Tool Action IDs (`rc_tool_contract.h`)

These are the canonical tool verbs the entire UI dispatches through. Every button, pie menu, and command palette ultimately fires one of these:

|Domain|Tools|
|---|---|
|**Axiom**|Organic, Grid, Radial, Hexagonal, Stem, LooseGrid, Suburban, Superblock, Linear, GridCorrective|
|**Water**|Flow, Contour, Erode, Select, Mask, Inspect|
|**WaterSpline**|Selection, DirectSelect, Pen, ConvertAnchor, AddRemoveAnchor, HandleTangents, SnapAlign, JoinSplit, Simplify|
|**Road**|Spline, Grid, Bridge, Select, Disconnect, Stub, Curve, Strengthen, Inspect|
|**RoadSpline**|Selection, DirectSelect, Pen, ConvertAnchor, AddRemoveAnchor, HandleTangents, SnapAlign, JoinSplit, Simplify|
|**District**|Zone, Paint, Split, Select, Merge, Inspect|
|**Lot**|Plot, Slice, Align, Select, Merge, Inspect|
|**Building**|Place, Scale, Rotate, Select, Assign, Inspect|
|**Future**|FloorPlan, Paths, Flow, Furniture (stubs)|

---

### Layer 2: App-Layer Tool Objects (`app/include/RogueCity/App/Tools/`)

|File|Class|Role|
|---|---|---|
|`AxiomPlacementTool.hpp`|`AxiomPlacementTool`|Places/drags/deletes AxiomVisual objects in the viewport. Owns undo history.|
|`AxiomVisual.hpp`|`AxiomVisual`|Render+data representation of a single placed axiom (type, position, radius, rotation, per-type params)|
|`AxiomAnimationController.hpp`|`AxiomAnimationController`|Drives ring animation pulse on axiom visuals|
|`AxiomIcon.hpp`|`DrawAxiomIcon()`|Draws the icon badge for each axiom type in the library picker|
|`ContextWindowPopup.hpp`|`ContextWindowPopup`|Knob/ring popup context on right-click|

**`AxiomPlacementTool` Modes** (as defined):

```cpp
enum class Mode {
    Idle,          // No interaction
    Placing,       // Click to place new axiom
    DraggingSize,  // Dragging to set radius
    DraggingAxiom, // Dragging axiom position
    DraggingKnob,  // Adjusting ring radius via knob
    Hovering       // Mouse over existing axiom
};
```

**Expected behavior:** Place axiom → drag to set radius → release → axiom is a spatial influence marker only. It should **NOT trigger full city generation automatically**. It should only:

1. Mark itself as a tensor influence zone
2. Merge/blend its boundary outline with neighboring axioms
3. Show the road network skeleton/outline only

---

### Layer 3: UI Tool Wiring (`visualizer/src/ui/tools/`)

|File|Role|
|---|---|
|`rc_tool_contract.h/.cpp`|`ToolActionId` enum, `ToolActionSpec` registry, catalog lookup|
|`rc_tool_dispatcher.h/.cpp`|`DispatchToolAction()` - routes tool IDs to HFSM state transitions|
|`rc_tool_geometry_policy.h/.cpp`|Geometry constraints/snapping rules per tool|
|`rc_tool_interaction_metrics.h/.cpp`|Tracks tool usage counts, hover time, drag distances|

---

### Layer 4: UI Panels (`visualizer/src/ui/panels/`)

|Panel|File|Expected Role|
|---|---|---|
|**AxiomEditor**|`rc_panel_axiom_editor.cpp`|Primary viewport + axiom placement + minimap + live preview wiring|
|**AxiomBar**|`rc_panel_axiom_bar.cpp`|Horizontal tool strip for axiom type selection|
|**Tools**|`rc_panel_tools.cpp`|Main left-side toolbox, all domain tool strips|
|**Inspector**|`rc_panel_inspector.cpp`|Property editor for selected entity|
|**CitySpec**|`rc_panel_city_spec.cpp`|City-wide config (seed, size, density)|
|**RoadIndex**|`rc_panel_road_index.cpp`|List of generated roads|
|**DistrictIndex**|`rc_panel_district_index.cpp`|List of generated districts|
|**LotIndex**|`rc_panel_lot_index.cpp`|List of lots|
|**BuildingIndex**|`rc_panel_building_index.cpp`|List of building sites|
|**BuildingControl**|`rc_panel_building_control.cpp`|Building generation parameters|
|**LotControl**|`rc_panel_lot_control.cpp`|Lot subdivision parameters|
|**WaterControl**|`rc_panel_water_control.cpp`|Water body parameters|
|**ZoningControl**|`rc_panel_zoning_control.cpp`|Zone type painting controls|
|**DevShell**|`rc_panel_dev_shell.cpp`|Debug console / CLI commands|
|**AIConsole**|`rc_panel_ai_console.cpp`|AI generation prompts|
|**Log**|`rc_panel_log.cpp`|Output log|
|**Telemetry**|`rc_panel_telemetry.cpp`|Generation stats|
|**SystemMap**|`rc_panel_system_map.cpp`|High-level map overview|
|**UIAgent**|`rc_panel_ui_agent.cpp`|AI agent panel|
|**UISettings**|`rc_panel_ui_settings.cpp`|Theme/layout preferences|
|**PropertyEditor**|`rc_property_editor.cpp`|Entity property editing|

---

### Layer 5: Integration Layer (`app/include/RogueCity/App/Integration/`)

|Class|Role|
|---|---|
|`GenerationCoordinator`|Serializes requests, tracks serial numbers, debounces|
|`RealTimePreview`|Background threaded generation via `jthread`, phases: Idle→InitStreetSweeper→Sweeping→StreetsSwept|
|`CityOutputApplier`|Applies `CityOutput` to `GlobalState` — writes roads, districts, lots, buildings, blocks, texture space|
|`GeneratorBridge`|Validates axiom inputs|
|`ZoningBridge`|World-constraint + plan validation|

---

## PART 2 — THE BUG: Full City Generation on Axiom Placement

### Root Cause (Confirmed in Source)

In `rc_panel_axiom_editor.cpp`'s `DrawContent()`, this block runs every frame:

```cpp
// Debounced live preview
const bool domain_live_preview_enabled = 
    gs.generation_policy.IsLive(gs.tool_runtime.active_domain);

if (s_generation_coordinator && s_live_preview 
    && domain_live_preview_enabled
    && (ui_modified_axiom || s_external_dirty 
        || s_axiom_tool->is_interacting()
        || s_axiom_tool->consume_dirty()))
{
    s_generation_coordinator->RequestRegeneration(
        axiom_inputs, config,
        GenerationRequestReason::LivePreview);
    ...
}
```

And when that generation completes, the `SetOnComplete` callback fires:

```cpp
s_generation_coordinator->SetOnComplete([minimap](
    const RogueCity::Generators::CityGenerator::CityOutput& output) 
{
    auto& gs = RogueCity::Core::Editor::GetGlobalState();
    RogueCity::App::ApplyCityOutputToGlobalState(output, gs);  // ← APPLIES EVERYTHING
    ...
});
```

And `ApplyCityOutputToGlobalState` writes **roads + districts + blocks + lots + buildings** all at once to `GlobalState`. The `CityGenerator` pipeline (via `StreetSweeper`) generates the full hierarchy in one pass — road network → districts → lots → buildings — and the applier dumps all of it immediately.

### What Should Happen

Axiom placement should only trigger **Phase 1 of the pipeline: road network skeleton + axiom boundary tensor field**. The output in the viewport should be:

- Axiom influence rings with **boundary outlines** merging with neighboring axioms
- **Major road network only** (highways/arterials derived from tensor field)
- A **"city portion tensor spot"** shape showing where a city chunk would grow
- **NO lots, NO buildings, NO districts populated** during this phase

---

## PART 3 — COMPREHENSIVE REFACTOR PLAN

### Phase A: Introduce `GenerationScope` — Gate What Gets Applied

**New enum in `CityOutputApplier.hpp`:**

```cpp
// What: Limits applier to only apply the pipeline layers you want
// Where: app/include/RogueCity/App/Integration/CityOutputApplier.hpp

enum class GenerationScope : uint8_t {
    RoadsOnly        = 0,  // Axiom phase: tensor + road skeleton only
    RoadsAndBounds   = 1,  // Adds district boundary outlines (no zoning)
    FullCity         = 2,  // All layers: districts, lots, buildings
};

struct CityOutputApplyOptions {
    bool rebuild_viewport_index{ true };
    bool mark_dirty_layers_clean{ true };
    bool preserve_locked_user_entities{ true };
    GenerationScope scope{ GenerationScope::FullCity }; // ← ADD THIS
};
```

**Modified `ApplyCityOutputToGlobalState` (partial):**

```cpp
// What: Respects scope to skip lots/buildings during axiom phase
// Where: app/src/Integration/CityOutputApplier.cpp

void ApplyCityOutputToGlobalState(
    const Generators::CityGenerator::CityOutput& output,
    Core::Editor::GlobalState& gs,
    const CityOutputApplyOptions& options)
{
    // ... existing user-entity preservation ...

    // Always apply roads
    gs.roads.clear();
    for (const auto& road : generated_roads) { gs.roads.add(road); }
    for (const auto& road : locked_user_roads) { gs.roads.add(road); }

    // District borders only if scope >= RoadsAndBounds
    if (options.scope >= GenerationScope::RoadsAndBounds) {
        gs.districts.clear();
        for (const auto& d : generated_districts) { gs.districts.add(d); }
        for (const auto& d : locked_user_districts) { gs.districts.add(d); }
    }

    // Lots + buildings only if FullCity
    if (options.scope == GenerationScope::FullCity) {
        gs.lots.clear();
        gs.buildings.clear();
        gs.blocks.clear();
        for (const auto& lot : generated_lots) { gs.lots.add(lot); }
        for (const auto& b : generated_buildings) { gs.buildings.push_back(b); }
        for (const auto& bl : output.blocks) { gs.blocks.add(bl); }
    }

    // tensor/texture always written
    // ...
}
```

---

### Phase B: Two-Tier Generation in `RealTimePreview`

**Add a `GenerationDepth` to the request:**

```cpp
// What: Limits CityGenerator pipeline stages
// Where: app/include/RogueCity/App/Integration/RealTimePreview.hpp

enum class GenerationDepth : uint8_t {
    AxiomBounds,   // Tensor field + road skeleton only (fast, ~10ms)
    FullPipeline,  // All stages (slow, 200ms+)
};

void request_regeneration(
    const std::vector<Generators::CityGenerator::AxiomInput>& axioms,
    const Generators::CityGenerator::Config& config,
    GenerationDepth depth = GenerationDepth::AxiomBounds); // ← DEFAULT changes
```

**In `RealTimePreview.cpp` start_generation():**

```cpp
// DO: Pass depth flag to CityGenerator config
// When depth == AxiomBounds, set config flags to skip lot/building stages
void RealTimePreview::start_generation() {
    auto cfg = pending_config_;
    if (pending_depth_ == GenerationDepth::AxiomBounds) {
        cfg.generate_lots      = false;  // ← Skip lot subdivision
        cfg.generate_buildings = false;  // ← Skip building placement
        cfg.generate_districts = false;  // ← Skip full district fill
        cfg.axiom_bounds_only  = true;   // ← New flag in CityGenerator::Config
    }
    // launch jthread with cfg...
}
```

---

### Phase C: Fix the `DrawContent()` Trigger in `rc_panel_axiom_editor.cpp`

**Current (WRONG — triggers full generation on every axiom change):**

```cpp
// BUG: This fires full city gen including lots/buildings during placement
if (s_live_preview && domain_live_preview_enabled
    && (s_axiom_tool->consume_dirty() || s_axiom_tool->is_interacting()))
{
    s_generation_coordinator->RequestRegeneration(axiom_inputs, config,
        GenerationRequestReason::LivePreview);
}
```

**Fixed:**

```cpp
// FIX: During axiom editing, only request road-bounds preview
// DO NOT trigger full generation
if (s_live_preview && axiom_mode
    && (s_axiom_tool->consume_dirty() || s_axiom_tool->is_interacting()))
{
    // ← AxiomBounds depth only — shows road skeleton + merge outline
    s_generation_coordinator->RequestRegeneration(
        axiom_inputs, config,
        GenerationRequestReason::LivePreview,
        GenerationDepth::AxiomBounds);         // ← new param
    s_external_dirty = false;
}

// Full generation is EXPLICIT only — via button or domain transition
// (handled in ForceGenerate() which already exists correctly)
```

---

### Phase D: Add the "Generate Full City?" Confirmation Gate

After axiom placement is done and the user transitions out of `Editing_Axioms` state, show a step prompt:

```cpp
// What: Modal prompt when leaving axiom mode
// Where: rc_panel_axiom_editor.cpp or RcMasterPanel.cpp in state transition handler

// In your HFSM OnExit(Editing_Axioms) or equivalent transition callback:
void OnExitAxiomMode(GlobalState& gs) {
    if (!gs.axioms.empty() && gs.roads.size() > 0) {
        gs.tool_runtime.pending_full_generation_prompt = true;
        // This flag causes the master panel to show:
        // [Generate Full City] [Continue Stepping] [Cancel]
    }
}
```

**In `RcMasterPanel.cpp` render loop:**

```cpp
// Modal prompt after axiom phase
if (gs.tool_runtime.pending_full_generation_prompt) {
    ImGui::OpenPopup("##FullCityPrompt");
}
if (ImGui::
```


## Structural Issues & Friction Points

## 1. `PropertyHistory()` Is Isolated from the Global `CommandHistory`

**Problem:** `rc_property_editor.cpp` creates its own local static `CommandHistory` that is entirely separate from the editor-wide `CommandHistory`. This means Ctrl+Z in the property panel and Ctrl+Z elsewhere operate on different stacks. A user editing a road type in the property editor and then undoing a road draw will get the wrong result.

**Fix:** Pass the global `CommandHistory&` through `DrawContext` or access it via `GlobalState`. Replace:

cpp

```cpp
static CommandHistory history{};
```

with a reference to `ctx.global_state.command_history` (or whichever field holds the canonical history). `PropertyEditor::Draw(GlobalState& gs)` already has access — just thread it through.

## 2. Linear Search for Entities Is O(n)

**Problem:** `FindRoad`, `FindDistrict`, `FindLot`, `FindBuilding` all iterate `gs.roads`, `gs.districts`, etc. by ID. With hundreds or thousands of entities this becomes noticeable in the property panel, which redraws every frame.

**Fix:** Add ID-to-index maps on `GlobalState` (e.g., `unordered_map<uint32_t, size_t> road_index_map`) rebuilt on structural changes. The property editor and batch editor both become O(1) lookups. Alternatively, if entities are stored sorted by ID, use `lower_bound`.

## 3. `RcPanelDrawers.cpp` Will Become Unmaintainable

**Problem:** Every panel drawer is defined in a single file. With 21 panel types (and 3 more behind `ROGUE_AI_DLC_ENABLED`), that file grows linearly. Adding a new panel requires editing this monolith, which violates Open/Closed Principle.

**Fix:** Split into one file per category or one file per panel, registered via a factory function. Each panel's `CreateDrawer()` factory is already declared at the bottom of each namespace — just move each into its own `.cpp` and have `InitializePanelRegistry()` call them all. Example structure:

text

```cpp
panels/drawers/road_index_drawer.cpp
panels/drawers/district_index_drawer.cpp
panels/drawers/zoning_control_drawer.cpp
...
```

`InitializePanelRegistry()` becomes a list of `registry.Register(RoadIndex::CreateDrawer())` calls. This also makes the AI-DLC gating cleaner — just conditionally compile those 3 source files.

## 4. District Border Vertex Editor Has a Command-Per-Frame Bug Potential

**Problem:** `DrawSingleDistrict` calls `CommitLambda` inside `ImGui::InputFloat2` for each vertex every time the value changes. ImGui's `InputFloat2` fires on every character input, meaning rapid typing can flood `CommandHistory` with hundreds of micro-commits.

**Fix:** Use a begin/end edit pattern. Capture `before` on `ImGui::IsItemActivated()` and commit only on `ImGui::IsItemDeactivatedAfterEdit()`:



```cpp
static std::vector<Vec2> vertex_edit_before;
if (ImGui::IsItemActivated()) vertex_edit_before = district.border;
if (ImGui::IsItemDeactivatedAfterEdit()) {
    CommitLambda(history, "District Boundary Vertex", ...);
}
```

This applies identically to all `InputFloat`, `InputScalar`, `DragFloat` fields in the property editor.

## 5. `DrawContext` Lacks `CommandHistory`

**Problem:** `DrawContext` passes `GlobalState&`, `EditorHFSM&`, and `UiIntrospector&`, but not the command history. Any drawer that needs undo-redo has to either get it from `GlobalState` directly or reinvent a local one (as `rc_property_editor.cpp` does).

**Fix:** Add `CommandHistory& command_history` to `DrawContext`. This is a one-line change to the struct and a one-line change to wherever `DrawContext` is constructed.

## 6. `PanelType` Is a `uint8_t` Enum That Will Overflow

**Problem:** You currently have 21 types + `COUNT`. `uint8_t` gives you 255 slots. That's comfortable now, but the AI DLC adds 3 more and your roadmap clearly includes more panel types (FloorPlan, Furnature subtools are already mentioned as stubs). More pressingly, if `COUNT` is used for range checks, inserting a type mid-enum shifts all subsequent values and breaks serialization.

**Fix:** Stop using enum value as a stable ID for serialization. Add a `PanelTypeToString()` / `PanelTypeFromString()` round-trip (magic_enum works here) and serialize by name, not by integer value. Keep `uint8_t` for runtime storage — it's fine — just decouple it from persistence.

## 7. `on_message` Has a Void Pointer Interface

**Problem:** The `on_message(const char* event_type, void* data)` virtual in `IPanelDrawer` is an untyped message bus that will cause bugs as the codebase grows. There's no way to validate the type of `data` at compile time.

**Fix:** Either replace it with a typed variant-based event system, or remove it until you have a concrete use case. A lightweight approach:



```cpp
struct PanelEvent { std::string type; std::any payload; };
virtual void on_message(const PanelEvent& event) {}
```

This keeps it flexible but makes misuse detectable at runtime.

---

## The `is_visible(DrawContext& ctx)` Hook — Use It

This is currently defaulted to `return true` on all drawers. It's the right place to implement:

- Hiding `DevShell` in non-debug builds
    
- Hiding `AiConsole` / `UiAgent` / `CitySpec` when `ROGUE_AI_DLC_ENABLED` isn't defined at runtime (as opposed to compile time — gives you a runtime toggle)
    
- Showing `WaterControl` only when the water layer is active
    
- Showing `AxiomEditor` only when an axiom is selected
    

This costs zero architectural work — the hook is already there. Wire it up in each drawer's `is_visible` override.

## What's Already Excellent (Don't Touch)

- `DrawEnumCombo<T>` with magic_enum — perfect, use this pattern everywhere
    
- `ApplyBatchValue<>` template — solid, extend to more fields as needed
    
- `CommitValueChange<TValue>` — clean, only commits if `before != after`
    
- `MarkDirtyForKind()` cascade logic — correct dependency order
    
- `DrawContext` struct pattern — right abstraction level
    
- `IPanelDrawer` interface with optional lifecycle hooks — production-quality design
    
- `PanelRegistry` with `unordered_map` + sorted output — correct and efficient
    
- The `#if defined(ROGUE_AI_DLC_ENABLED)` gating in `RcPanelDrawers.cpp` — clean compile-time feature flagging
    
- `DrawQuerySelection` inline in the property panel — genuinely useful for debugging city state
**What is missing / broken:**

1. **No cursor preview line.** When using `Pen`, there is zero feedback showing where the next point will land until after the click commits. A professional tool draws a live "rubber band" line from the last placed point to the current mouse position every frame.
    
2. **No road width visual.** Roads are drawn as a polyline with uniform 1px screen thickness regardless of type. A Highway should visually differ from a Lane — there is a `RoadType` enum with 7 values but the overlay draws them identically.
    
3. **Vertex handles are invisible.** The drag system works perfectly but there are zero circles drawn on road vertices when in an editing subtool. The user cannot see what they can grab.
    
4. **No connection snap.** When ending a road near an existing road's endpoint, there is no snap-to-connect behavior. Roads don't know about each other as a graph — they're independent polylines.
    
5. **`Disconnect`, `Stub`, `Curve`, `Strengthen` subtools exist in the enum but have zero interaction code.** `HandleDomainPlacementActions` has no case for them.

## Water Tool (River/Lake/Ocean/Pond)

**What works:**

- Pen: creates new `WaterBody` with `M_user` tag on first click, appends boundary points on subsequent clicks
    
- Full vertex drag with falloff weighting for `Erode`/`Flow`/`Contour` subtools — this is the most sophisticated tool in the codebase. The `ComputeFalloffWeight` radius system is genuinely good
    
- `AddRemoveAnchor`, `JoinSplit`, `ConvertAnchor`, `Simplify` all implemented
    
- River rendered as depth-scaled polyline with directional dot arrows every 10 segments
    
- Lake/Ocean/Pond as filled polygon with optional sandy shore outline
    

**What is missing:**

1. **No preview cursor.** Same issue as roads — Pen mode has no live preview of the next point
    
2. **River width is purely visual (line thickness from `depth`) but depth is a scalar set in the property editor, not visually editable in viewport.** No drag-handle for width
    
3. **Shore line is a straight `AddPolyline` around the boundary** — no waviness, no animation, no visual distinction between shallow and deep water
    
4. **`Flow`, `Contour`, `Mask`, `Inspect` subtools in the water domain have zero panel UI** and the falloff behavior on the vertex drag is the only expression of them
    
5. **`Select` subtool doesn't differ from `Selection` spline subtool** — same code path for both
    

---

## District Tool

**What works:**

- `Paint`/`Zone`: left-click places a square boundary centered at click point, size from `district_placement_half_extent` (45 world units)
    
- Boundary vertex drag with grid snap
    
- Insert/delete vertex via `insert_mode`/`delete_mode` flags
    
- `Merge`: click a second district within `merge_radius_world` (40 world units) — concatenates border arrays (note: this is a raw point concatenation, not a convex hull, so the result is topologically wrong)
    
- Zone colors rendered as filled polygons with district-type color coding from `DistrictColorScheme`[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/visualizer/src/ui/viewport/rc_viewport_overlays.cpp)​
    

**What is missing:**

1. **Square placement is the only placement shape.** There is no freehand paint, no polygon draw, no circular brush. The district boundary editor requires manual vertex manipulation after placement
    
2. **The Merge is broken topologically.** Concatenating two separate polygon's border points does not produce a merged polygon — it creates a star shape or figure-8. A proper merge needs a union operation (use Boost.Geometry union_ instead)
    
3. **`Split` subtool has no viewport interaction.** The `DistrictSubtool::Split` name exists but the interaction code only uses it to trigger `insert_mode` — there is no click-to-slice-district behavior
    
4. **No orientation handle.** `district.orientation` field exists but there is no viewport gizmo to visually rotate it
    
5. **Zone color fill is `AddConvexPolyFilled`.** This only works for convex polygons. Any concave district boundary renders with fill artifacts
    

---

## Lot Tool

**What works:**

- `Plot`: places a square lot at click position, size from `lot_placement_half_extent` (16 world units)
    
- `Slice`: splits a lot into two rectangles along the axis closest to the click offset from center — vertical or horizontal split based on click position relative to centroid
    
- `Align`: snaps lot centroid to grid and rebuilds a square boundary at computed half-extent from `sqrt(area)`
    
- `Merge`: concatenates boundaries of two lots within `merge_radius_world` — same topological problem as district merge
    
- AESP heatmap renders scores (Access, Exposure, Service, Privacy) as per-lot color fills
    

**What is missing:**

1. **No freehand lot drawing.** Every lot starts as a rectangle. For irregular parcels along curved roads this is a major limitation
    
2. **AESP scores are read-only display** — they appear to be set by the generator, not computed live in the viewport from the city state. There's no visible update trigger
    
3. **Lot boundaries rendered as `AddPolyline` at 1px** — same visibility problem as roads. At typical zoom they're nearly invisible
    
4. **`Align` rebuilds a square from area** — this destroys irregular lot shapes without warning
    

---

## Building Tool

**What works:**

- `Place`: click to drop a `BuildingSite` at cursor. Scale from `building_default_scale` (1.0)
    
- `Assign`: click a selected building to cycle through `BuildingType` enum (Residential → Rowhome → Retail → MixedUse → Industrial → Civic → Luxury → Utility and back)
    
- Gizmo Translate/Rotate/Scale all wire to `ApplyTranslate/ApplyRotate/ApplyScale` in `EditorManipulation`
    
- Buildings rendered as fixed-size colored squares (size = `4.0f * zoom`) with white outline
    
- `BuildingType` maps to distinct colors (blue residential, green retail, red industrial, yellow civic, purple luxury)
    

**What is missing:**

1. **Building size is `4.0f * zoom`** — it grows with zoom level, which is backwards. At city scale every building looks the same dot. At street scale they're huge boxes. Size should be in world units scaled to screen, not in screen pixels scaled arbitrarily
    
2. **No footprint polygon.** Buildings are points with a square screen-space marker. There is no `footprint` field on `BuildingSite` and no way to draw an oriented rectangle representing the actual floor area
    
3. **`Scale`, `Rotate`, `Inspect` subtools exist in enum but `Place` and `Assign` are the only ones with interaction code.** `Rotate` wires the gizmo correctly but `Scale` only triggers the gizmo mode — no direct drag to scale


## The Big Missing Visual Layer: What Should Be Drawn But Isn't

Currently `ViewportOverlays::Render()` has this overlay stack (in draw order):[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/visualizer/src/ui/viewport/rc_viewport_overlays.cpp)​

1. Zone colors (filled district polygons)
    
2. AESP heatmap (per-lot colored fills)
    
3. Road labels (text at midpoint)
    
4. Budget bars
    
5. Slope/NoBuild/Nature/Height/Tensor/Zone field heatmaps (grid-sampled dot clouds)
    
6. Lot boundaries
    
7. Water bodies
    
8. Building sites
    
9. Gizmos
    
10. Grid
    
11. Selection outlines + hover outlines
    

**Gaps in the rendering stack that hurt usability:**

- **Roads are not drawn at all in this overlay stack.** The `show_roads = true` config flag exists in `OverlayConfig` but there is no `RenderRoads()` method. Roads only appear via selection outlines when selected. An unselected road is invisible
    
- **Vertex handles are never rendered.** When in road/water/district vertex editing mode, handle circles should appear on each editable vertex. Nothing draws them
    
- **No tool cursor / preview geometry.** The active tool's "ghost" of what would be placed if you clicked now doesn't exist anywhere
    
- **The minimap** (`rc_minimap_renderer.cpp`) only calls `set_city_output(frame.output)` — it's a stub that defers all rendering to `MinimapViewport` which is in the App layer, not the UI layer[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/visualizer/src/ui/viewport/rc_minimap_renderer.cpp)​
    
- **No connection/intersection indicators** at road endpoints that are close to other roads
    

---
## Global Viewport Tools: What You Need and Where They Live

This is grounded entirely in what exists in your codebase. The `EditorState` enum, `ToolRuntimeState`, `ToolActionId`, and `ProcessNonAxiomViewportInteraction` all expose exactly what is and isn't there.

---

## What "Global" Means in Your Architecture

A global viewport tool is one that operates **regardless of active domain** — it runs before `HandleDomainPlacementActions()` consumes the click, works across all entity types, and routes through `EditorState` transitions that aren't domain-specific. Currently you have exactly three: the gizmo (Translate/Rotate/Scale), Box Select (`Alt+Shift+LMB`), and Lasso Select (`Alt+LMB`). Everything else is domain-locked.

The HFSM already has the right slots for true global tools — `Viewport_Pan`, `Viewport_Select`, `Viewport_PlaceAxiom`, `Viewport_DrawRoad`, `Viewport_BoxSelect` are all first-class `EditorState` values, not sub-states of a domain. That's your architectural contract for what belongs at the global level.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/core/include/RogueCity/Core/Editor/EditorState.hpp)​

---

## The Global Tools To Add — Grouped by Priority

---

## Tier 1 — Fix Before Anything Else (These Are Broken Stubs)

**1. Pan Tool (`Viewport_Pan`)**  
The `EditorState` entry exists. The Axiom path has pan implemented via `Alt+LMB` orbit and `MMB` pan. The non-Axiom path has **zero pan code** — there is no `Viewport_Pan` handler in `ProcessNonAxiomViewportInteraction`. Navigation in the 2D viewport is completely unimplemented unless you're in Axiom mode.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/visualizer/src/ui/tools/rc_tool_contract.h)​

What to build:

```cpp
// In ProcessNonAxiomViewportInteraction, before domain dispatch:
if (io.MouseDown[ImGuiMouseButton_Middle] ||
    (io.KeyShift && io.MouseDown[ImGuiMouseButton_Right])) {
    const float zoom = params.primary_viewport->world_to_screen_scale(1.0f);
    const float pan_speed = gs.params.viewport_pan_speed;
    const Vec2 delta_world(
        -io.MouseDelta.x / zoom * pan_speed,
        -io.MouseDelta.y / zoom * pan_speed);
    auto cam = params.primary_viewport->get_camera_xy();
    cam.x += delta_world.x;
    cam.y += delta_world.y;
    params.primary_viewport->set_camera_position(cam, camera_z);
    result.outcome = InteractionOutcome::ActivateOnly;
    return result;
}
```

Add `EditorState::Viewport_Pan` transition via Spacebar held (Blender-style) or dedicated hotkey. Cursor should show a hand icon — use `ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll)` while active.

---

**2. Zoom Tool (scroll wheel + keyboard)**  
The Axiom path has scroll zoom clamped to `[80, 4000]` on `camera_z`. The non-Axiom path has zero zoom code. The `SceneFrame` carries `camera_z` but nothing modifies it in the non-Axiom interaction path.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/visualizer/src/ui/viewport/rc_scene_frame.h)​

Add to the global pre-dispatch block:


```cpp
if (io.MouseWheel != 0.0f && params.in_viewport && !params.minimap_hovered) {
    const float z = params.primary_viewport->get_camera_z();
    const float factor = std::pow(1.12f, -io.MouseWheel);
    const float new_z = std::clamp(z * factor, 80.0f, 4000.0f);
    params.primary_viewport->set_camera_position(
        params.primary_viewport->get_camera_xy(), new_z);
    result.outcome = InteractionOutcome::ActivateOnly;
    // Don't return — zoom doesn't consume click
}
```

Also add `Ctrl+=` / `Ctrl+-` keyboard zoom and `Ctrl+0` for reset-to-fit (frame all entities).

---

**3. Universal Select (`V` key / `Viewport_Select` state)**  
Currently `Viewport_Select` is a `EditorState` value with no corresponding tool path. A press of `V` (or `Esc` from any domain) should drop into a cross-domain select mode where:

- Left click picks the highest-priority entity under cursor via `PickFromViewportIndex()` regardless of current domain
    
- The HFSM transitions to `Viewport_Select`
    
- The active domain in `ToolRuntimeState` stays where it was so returning to the prior tool works
    

This is the "arrow tool" present in every professional editor. The `PickFromViewportIndex` function already handles cross-domain priority (Building=5 > Lot=4 > Road=3 > District=2) — you just need the mode and hotkey wired.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/visualizer/src/ui/tools/rc_tool_contract.h)​

---

## Tier 2 — High Value, Architecturally Clean to Add

**4. Frame / Focus (`F` key)**  
`F` over a selection → zoom + pan camera to frame the bounding box of the selected entities. `Shift+F` → frame the entire city. This is a navigation tool every editor has and yours doesn't.

Implementation calls `ResolveSelectionAnchor()` for each selected item to get a bounding box, then sets `camera_xy` to the centroid and `camera_z` to a value derived from the diagonal of the bounding box divided by `viewport_zoom`. All the pieces are in `ProcessNonAxiomViewportInteraction` already.

---

**5. Terrain Brush — Global Paint Mode**  
`GlobalState` already has `ApplyTerrainBrush(const TerrainBrush::Stroke& stroke)` and `ApplyTexturePaint(const TexturePainting::Stroke& stroke)` as fully implemented methods. The `TextureSpace` has height, slope, nature, tensor, and zone layers. But there is **zero viewport interaction** wired to them — no `ToolDomain::Flow` or terrain brush activation path anywhere in `HandleDomainPlacementActions`.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/core/include/RogueCity/Core/Editor/GlobalState.hpp)​

This needs to be a global tool (not domain-locked) because terrain affects all entity types. The domain `ToolDomain::Flow` already exists in the enum — wire it:

text

```
EditorEvent::Tool_Water → already transitions HFSM
ToolDomain::Flow        → already in enum but no interaction code
TerrainBrush::Stroke    → already has position, radius, strength, layer
```

In the viewport: hold LMB + drag → sample `world_pos` each frame → call `gs.ApplyTerrainBrush()` → mark `DirtyLayer::ViewportIndex`. The brush radius should render as a world-space circle on the cursor — a `AddCircle` at `world_pos` with `WorldToScreenScale(brush_radius)`.

---

**6. Ruler / Measure Tool**  
There is no distance measurement anywhere in the tool contract. For urban spatial design this is critical — a designer needs to know if a road segment is 100m or 400m, whether a district is large enough for its axiom, etc.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/visualizer/src/ui/tools/rc_tool_contract.h)​

Implementation: a global tool with two states (placing first point, dragging to second point). Draw a line from start to cursor, display the world-space distance as a label at the midpoint. Add to `GlobalState`:



```cpp
struct MeasureState {
    bool active{ false };
    Vec2 start{};
    Vec2 end{};
    bool has_end{ false };
};
MeasureState measure{};
```

Hotkey: `M`. No `ToolActionId` entry needed — it's purely a viewport overlay tool that doesn't mutate city data.

---

**7. Eyedropper / Inspector (`I` key or `Building_Inspect` / `Road_Inspect` subtools)**  
The `Inspect` subtool exists in every domain's enum (`Road_Inspect`, `District_Inspect`, `Lot_Inspect`, `Building_Inspect`) but has exactly zero interaction code. The `Inspect` concept should be a global cross-domain tool: hover or click any entity, display a floating tooltip with its key properties without changing selection.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/visualizer/src/ui/tools/rc_tool_contract.h)​

This is different from the Property Editor — it's inline, zero-click, ephemeral. Implementation: in the hover path of `ProcessNonAxiomViewportInteraction`, when `active_domain` is any `*_Inspect` subtool, render an `ImGui::BeginTooltip()` with the entity's primary fields. Uses `PickFromViewportIndex` + `FindRoad/District/Lot/Building` exactly as the property editor does, but renders at cursor instead.

---

**8. Stamp Tool (Global Axiom Placement)**  
The `AxiomPlacementTool` and `Viewport_PlaceAxiom` state exist. But Axiom placement right now is a mode fork — the code splits at `ProcessAxiomViewportInteraction` vs `ProcessNonAxiomViewportInteraction`, meaning Axiom placement disables all other tools entirely. A stamp tool would let you drop an axiom at cursor position without entering full Axiom mode — quick-place from any domain via `Alt+A` or a hotkey, dispatch `DispatchToolAction(ToolActionId::Axiom_Organic, ...)` and return to the previous domain.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/core/include/RogueCity/Core/Editor/EditorState.hpp)​

---

## Tier 3 — For When 2D Is Solid, Before 40XX

**9. Camera Orbit / Tilt (Pre-3D Prep)**  
The `SceneFrame` already has `camera_yaw` and the Axiom path already implements orbit via `Alt+LMB` and yaw-aware pan. In 2D this appears as a rotation of the map view — useful for aligning roads to screen. In the 40XX path this becomes the pitch/yaw/roll camera for 3D terrain inspection. Wire `camera_yaw` into `BuildSceneFrame()` so it feeds `ViewportOverlays::WorldToScreen()` — the yaw rotation is already in the `WorldToScreen` formula using `cos(yaw)` / `sin(yaw)`, it just needs a non-zero input.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/visualizer/src/ui/viewport/rc_scene_frame.h)​

**10. Multi-viewport Sync Tool**  
`gs.config.ui_multi_viewport_enabled` exists, `ViewportSyncManager` is referenced in `SceneControllerUpdateInput`, but there is no UI to manage split-viewport layouts. For a city designer comparing two generation strategies side by side this is high value. Add a global toggle: one viewport locked to bird's-eye, one to street-level inspection. The `SceneFrame` + `SceneControllerUpdateInput` is already structured for this.

---

## Mapping to Your Existing Structures

Here is where each global tool fits architecturally:

|Tool|EditorState|ToolActionId|New Dispatch Needed|HFSM Event|
|---|---|---|---|---|
|Pan|`Viewport_Pan`|None (MMB/modifier)|No|`Viewport_Pan`|
|Zoom|None (always active)|None|No|None|
|Universal Select|`Viewport_Select`|None (`V`)|No|`Viewport_Select`|
|Frame/Focus|None (`F` hotkey)|None|No|None|
|Terrain Brush|`Editing_Water` + `Flow` domain|`Future_Flow` (rename)|Yes|`Tool_Water`|
|Ruler|New global overlay state|None (`M`)|No|None|
|Inspector|Any `*_Inspect` subtool|Existing inspect IDs|Yes — wire handlers|None|
|Stamp Axiom|`Viewport_PlaceAxiom`|Existing axiom IDs|Yes — quick-place path|`AxiomPlaced`|
|Orbit/Tilt|`Viewport_Pan`|None|No|`Viewport_Pan`|

---

## The One Structural Change That Enables All of This

Right now `ProcessNonAxiomViewportInteraction` is a monolith that handles pan, zoom, domain placement, gizmo, box-select, lasso, hover, and click-select in a single ~600-line function with a consumed/unconsumed flag threading through it. Every global tool you add makes it longer.

Before Tier 2, split it into a pipeline of discrete handlers with an early-exit consumed flag:



```cpp
// Ordered pipeline — first handler to set consumed = true wins
GlobalNavigationHandler(gs, params, result);   // pan, zoom, orbit — never consumed
GlobalSelectHandler(gs, params, result, state); // V key, Esc
GlobalGizmoHandler(gs, params, result, state);  // existing gizmo code
GlobalLassoBoxHandler(gs, params, result, state); // existing box/lasso
DomainPlacementHandler(gs, params, result, state); // existing HandleDomainPlacementActions
GlobalInspectHandler(gs, params, result);       // hover tooltip
GlobalHoverHandler(gs, params, result);         // existing hover/click-select
```

Each handler is a free function in its own .cpp file. `NonAxiomInteractionState` gets split into sub-state structs per handler. `ToolInteractionMetrics` stays the single source of world-space pick radii shared across all of them.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/visualizer/src/ui/tools/rc_tool_interaction_metrics.h)​

This gives you a clean insertion point for every future global tool without touching existing domain logic.


## Part 3 — Y2K Aesthetic Map Visualization Upgrade

The `DistrictColorScheme` comment already says "Y2K palette". The infrastructure to go full Y2K is present — `ThemeManager` is referenced in `RenderGridOverlay`. Here is a concrete, implementable Y2K visual language for the 2D viewport:[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/visualizer/src/ui/viewport/rc_viewport_overlays.h)​

## Color System

Replace the current muted RGBA values with a high-saturation, slightly phosphorescent palette:

|Entity|Current|Y2K Target|
|---|---|---|
|Road: Highway|(none visible)|`#FF6B35` amber-orange, 3px stroke|
|Road: Arterial|(none visible)|`#FFD700` gold, 2.5px stroke|
|Road: Street|(none visible)|`#E0E0E0` cool white, 1.5px stroke|
|Road: Lane/Alley|(none visible)|`#808080` steel, 1px stroke|
|District: Residential|`(0.3, 0.5, 0.9, 0.6)`|`#3A6FFF` electric blue, 30% fill alpha|
|District: Commercial|`(0.3, 0.9, 0.5, 0.6)`|`#00FF9F` neon mint, 30% fill alpha|
|District: Industrial|`(0.9, 0.3, 0.3, 0.6)`|`#FF3B3B` terminal red, 30% fill alpha|
|District: Civic|`(0.9, 0.7, 0.3, 0.6)`|`#FFAA00` amber, 30% fill alpha|
|Water: River|`(0.31, 0.59, 0.86, 0.7)`|`#00CFFF` cyan-blue, animated width pulse|
|Water: Lake|`(0.20, 0.47, 0.78, 0.6)`|`#0055AA` deep blue with `#00CFFF` 1px shore ring|
|Grid|`theme.primary_accent * 0.15`|`#00FF41` Matrix green at 8% alpha (major), 4% (minor)|
|Selection|`IM_COL32(255, 220, 60, 230)`|`#FFFF00` pure yellow with `sin(t*8)` pulse on thickness|
|Hover|`IM_COL32(120, 220, 255, 220)`|`#00FFFF` pure cyan, static 2px|
|Building: Residential|`(0.4, 0.6, 0.95)`|`#4488FF` with white `+` cross marker|
|Building: Commercial|`(0.4, 0.95, 0.6)`|`#00DD77` with diamond marker|
|Building: Industrial|`(0.95, 0.4, 0.4)`|`#FF4444` with `X` marker|

## Grid System

The current grid is uniform 50-world-unit lines. Y2K map grids are hierarchical:

text

```
Major grid: every 500 world units — #00FF41 at 12% alpha, 1.2px
Minor grid: every 100 world units — #00FF41 at 6% alpha, 0.8px
Sub grid:   every 50 world units — #00FF41 at 3% alpha, 0.5px (zoom-dependent, only shown at close range)
```

Show coordinate labels at major grid intersections (`[0,0]`, `[500,0]`, etc.) in the same phosphor green at 40% alpha.

## Road Rendering (the missing `RenderRoads()` method)

This is the most critical missing piece. The function to add to `ViewportOverlays`:


```cpp
void ViewportOverlays::RenderRoads(const GlobalState& gs) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    struct RoadStyle { float width; ImU32 color; ImU32 casing; };
    auto GetRoadStyle = [&](RoadType t, float opacity) -> RoadStyle {
        switch (t) {
            case RoadType::Highway:  return { 5.0f, IM_COL32(255,107, 53,int(230*opacity)), IM_COL32(20,20,20,int(200*opacity)) };
            case RoadType::Arterial: return { 3.5f, IM_COL32(255,215,  0,int(220*opacity)), IM_COL32(20,20,20,int(200*opacity)) };
            case RoadType::Avenue:   return { 2.5f, IM_COL32(220,220,220,int(200*opacity)), IM_COL32(20,20,20,int(180*opacity)) };
            case RoadType::Boulevard:return { 3.0f, IM_COL32(200,200,255,int(200*opacity)), IM_COL32(20,20,20,int(180*opacity)) };
            case RoadType::Street:   return { 2.0f, IM_COL32(180,180,180,int(190*opacity)), IM_COL32(0,0,0,0) };
            case RoadType::Lane:     return { 1.5f, IM_COL32(120,120,120,int(180*opacity)), IM_COL32(0,0,0,0) };
            case RoadType::Alleyway: return { 1.0f, IM_COL32(80, 80, 80, int(160*opacity)), IM_COL32(0,0,0,0) };
            default:                 return { 1.5f, IM_COL32(160,160,160,int(180*opacity)), IM_COL32(0,0,0,0) };
        }
    };
    
    // Two-pass: draw casings first, then fills
    for (int pass = 0; pass < 2; ++pass) {
        for (const auto& road : gs.roads)
```


## The Global Tools To Add — Grouped by Priority

---

## Tier 1 — Fix Before Anything Else (These Are Broken Stubs)

**1. Pan Tool (`Viewport_Pan`)**  
The `EditorState` entry exists. The Axiom path has pan implemented via `Alt+LMB` orbit and `MMB` pan. The non-Axiom path has **zero pan code** — there is no `Viewport_Pan` handler in `ProcessNonAxiomViewportInteraction`. Navigation in the 2D viewport is completely unimplemented unless you're in Axiom mode.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/visualizer/src/ui/tools/rc_tool_contract.h)​

What to build:

```cpp
// In ProcessNonAxiomViewportInteraction, before domain dispatch:
if (io.MouseDown[ImGuiMouseButton_Middle] ||
    (io.KeyShift && io.MouseDown[ImGuiMouseButton_Right])) {
    const float zoom = params.primary_viewport->world_to_screen_scale(1.0f);
    const float pan_speed = gs.params.viewport_pan_speed;
    const Vec2 delta_world(
        -io.MouseDelta.x / zoom * pan_speed,
        -io.MouseDelta.y / zoom * pan_speed);
    auto cam = params.primary_viewport->get_camera_xy();
    cam.x += delta_world.x;
    cam.y += delta_world.y;
    params.primary_viewport->set_camera_position(cam, camera_z);
    result.outcome = InteractionOutcome::ActivateOnly;
    return result;
}
```

Add `EditorState::Viewport_Pan` transition via Spacebar held (Blender-style) or dedicated hotkey. Cursor should show a hand icon — use `ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll)` while active.

---

**2. Zoom Tool (scroll wheel + keyboard)**  
The Axiom path has scroll zoom clamped to `[80, 4000]` on `camera_z`. The non-Axiom path has zero zoom code. The `SceneFrame` carries `camera_z` but nothing modifies it in the non-Axiom interaction path.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/visualizer/src/ui/viewport/rc_scene_frame.h)​

Add to the global pre-dispatch block:


```cpp
if (io.MouseWheel != 0.0f && params.in_viewport && !params.minimap_hovered) {
    const float z = params.primary_viewport->get_camera_z();
    const float factor = std::pow(1.12f, -io.MouseWheel);
    const float new_z = std::clamp(z * factor, 80.0f, 4000.0f);
    params.primary_viewport->set_camera_position(
        params.primary_viewport->get_camera_xy(), new_z);
    result.outcome = InteractionOutcome::ActivateOnly;
    // Don't return — zoom doesn't consume click
}
```

Also add `Ctrl+=` / `Ctrl+-` keyboard zoom and `Ctrl+0` for reset-to-fit (frame all entities).

---

**3. Universal Select (`V` key / `Viewport_Select` state)**  
Currently `Viewport_Select` is a `EditorState` value with no corresponding tool path. A press of `V` (or `Esc` from any domain) should drop into a cross-domain select mode where:

- Left click picks the highest-priority entity under cursor via `PickFromViewportIndex()` regardless of current domain
    
- The HFSM transitions to `Viewport_Select`
    
- The active domain in `ToolRuntimeState` stays where it was so returning to the prior tool works
    

This is the "arrow tool" present in every professional editor. The `PickFromViewportIndex` function already handles cross-domain priority (Building=5 > Lot=4 > Road=3 > District=2) — you just need the mode and hotkey wired.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/visualizer/src/ui/tools/rc_tool_contract.h)​

---

## Tier 2 — High Value, Architecturally Clean to Add

**4. Frame / Focus (`F` key)**  
`F` over a selection → zoom + pan camera to frame the bounding box of the selected entities. `Shift+F` → frame the entire city. This is a navigation tool every editor has and yours doesn't.

Implementation calls `ResolveSelectionAnchor()` for each selected item to get a bounding box, then sets `camera_xy` to the centroid and `camera_z` to a value derived from the diagonal of the bounding box divided by `viewport_zoom`. All the pieces are in `ProcessNonAxiomViewportInteraction` already.

---

**5. Terrain Brush — Global Paint Mode**  
`GlobalState` already has `ApplyTerrainBrush(const TerrainBrush::Stroke& stroke)` and `ApplyTexturePaint(const TexturePainting::Stroke& stroke)` as fully implemented methods. The `TextureSpace` has height, slope, nature, tensor, and zone layers. But there is **zero viewport interaction** wired to them — no `ToolDomain::Flow` or terrain brush activation path anywhere in `HandleDomainPlacementActions`.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/core/include/RogueCity/Core/Editor/GlobalState.hpp)​

This needs to be a global tool (not domain-locked) because terrain affects all entity types. The domain `ToolDomain::Flow` already exists in the enum — wire it:

text

```
EditorEvent::Tool_Water → already transitions HFSM
ToolDomain::Flow        → already in enum but no interaction code
TerrainBrush::Stroke    → already has position, radius, strength, layer
```

In the viewport: hold LMB + drag → sample `world_pos` each frame → call `gs.ApplyTerrainBrush()` → mark `DirtyLayer::ViewportIndex`. The brush radius should render as a world-space circle on the cursor — a `AddCircle` at `world_pos` with `WorldToScreenScale(brush_radius)`.

---

**6. Ruler / Measure Tool**  
There is no distance measurement anywhere in the tool contract. For urban spatial design this is critical — a designer needs to know if a road segment is 100m or 400m, whether a district is large enough for its axiom, etc.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/visualizer/src/ui/tools/rc_tool_contract.h)​

Implementation: a global tool with two states (placing first point, dragging to second point). Draw a line from start to cursor, display the world-space distance as a label at the midpoint. Add to `GlobalState`:

cpp

```
struct MeasureState {
    bool active{ false };
    Vec2 start{};
    Vec2 end{};
    bool has_end{ false };
};
MeasureState measure{};
```

Hotkey: `M`. No `ToolActionId` entry needed — it's purely a viewport overlay tool that doesn't mutate city data.

---

**7. Eyedropper / Inspector (`I` key or `Building_Inspect` / `Road_Inspect` subtools)**  
The `Inspect` subtool exists in every domain's enum (`Road_Inspect`, `District_Inspect`, `Lot_Inspect`, `Building_Inspect`) but has exactly zero interaction code. The `Inspect` concept should be a global cross-domain tool: hover or click any entity, display a floating tooltip with its key properties without changing selection.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/visualizer/src/ui/tools/rc_tool_contract.h)​

This is different from the Property Editor — it's inline, zero-click, ephemeral. Implementation: in the hover path of `ProcessNonAxiomViewportInteraction`, when `active_domain` is any `*_Inspect` subtool, render an `ImGui::BeginTooltip()` with the entity's primary fields. Uses `PickFromViewportIndex` + `FindRoad/District/Lot/Building` exactly as the property editor does, but renders at cursor instead.

---

**8. Stamp Tool (Global Axiom Placement)**  
The `AxiomPlacementTool` and `Viewport_PlaceAxiom` state exist. But Axiom placement right now is a mode fork — the code splits at `ProcessAxiomViewportInteraction` vs `ProcessNonAxiomViewportInteraction`, meaning Axiom placement disables all other tools entirely. A stamp tool would let you drop an axiom at cursor position without entering full Axiom mode — quick-place from any domain via `Alt+A` or a hotkey, dispatch `DispatchToolAction(ToolActionId::Axiom_Organic, ...)` and return to the previous domain.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/core/include/RogueCity/Core/Editor/EditorState.hpp)​

---

## Tier 3 — For When 2D Is Solid, Before 40XX

**9. Camera Orbit / Tilt (Pre-3D Prep)**  
The `SceneFrame` already has `camera_yaw` and the Axiom path already implements orbit via `Alt+LMB` and yaw-aware pan. In 2D this appears as a rotation of the map view — useful for aligning roads to screen. In the 40XX path this becomes the pitch/yaw/roll camera for 3D terrain inspection. Wire `camera_yaw` into `BuildSceneFrame()` so it feeds `ViewportOverlays::WorldToScreen()` — the yaw rotation is already in the `WorldToScreen` formula using `cos(yaw)` / `sin(yaw)`, it just needs a non-zero input.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/visualizer/src/ui/viewport/rc_scene_frame.h)​

**10. Multi-viewport Sync Tool**  
`gs.config.ui_multi_viewport_enabled` exists, `ViewportSyncManager` is referenced in `SceneControllerUpdateInput`, but there is no UI to manage split-viewport layouts. For a city designer comparing two generation strategies side by side this is high value. Add a global toggle: one viewport locked to bird's-eye, one to street-level inspection. The `SceneFrame` + `SceneControllerUpdateInput` is already structured for this.

---

## Mapping to Your Existing Structures

Here is where each global tool fits architecturally:

| Tool             | EditorState                     | ToolActionId           | New Dispatch Needed    | HFSM Event        |
| ---------------- | ------------------------------- | ---------------------- | ---------------------- | ----------------- |
| Pan              | `Viewport_Pan`                  | None (MMB/modifier)    | No                     | `Viewport_Pan`    |
| Zoom             | None (always active)            | None                   | No                     | None              |
| Universal Select | `Viewport_Select`               | None (`V`)             | No                     | `Viewport_Select` |
| Frame/Focus      | None (`F` hotkey)               | None                   | No                     | None              |
| Terrain Brush    | `Editing_Water` + `Flow` domain | `Future_Flow` (rename) | Yes                    | `Tool_Water`      |
| Ruler            | New global overlay state        | None (`M`)             | No                     | None              |
| Inspector        | Any `*_Inspect` subtool         | Existing inspect IDs   | Yes — wire handlers    | None              |
| Stamp Axiom      | `Viewport_PlaceAxiom`           | Existing axiom IDs     | Yes — quick-place path | `AxiomPlaced`     |
| Orbit/Tilt       | `Viewport_Pan`                  | None                   | No                     | `Viewport_Pan`    |

---



## What's actually wired end-to-end ✅

|System|Status|
|---|---|
|**Axiom Placement Tool**|Fully implemented: place, drag-to-size, move, resize via knob, undo/redo, right-click delete, ghost preview, ICommand pattern|
|**PrimaryViewport rendering**|Roads (cyan lines, thickness by hierarchy), Districts (magenta wireframe + RES/COM/IND/MIX label at centroid), Lots (amber wireframe). All driven by `CityGenerator::CityOutput*` pointer|
|**Camera**|Pan (xy), zoom (z → 500/max(100,z)), yaw rotation. `screen_to_world` / `world_to_screen` with Catmull-Rom spline support in `EditorManipulation`|
|**EditorManipulation**|`ApplyTranslate`, `ApplyRotate`, `ApplyScale` all handle Road/District/Lot/Building/Water via SelectionItem. `MoveRoadVertex`, `InsertDistrictVertex`, `RemoveDistrictVertex` all exist|
|**VpProbeData / ViewportIndex**|Full entity kind enum: Axiom, Road, District, Lot, Building, Water, Block. StableID mapping, slope, flood, aesp[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/core/include/RogueCity/Core/Editor/ViewportIndex.hpp)​, zone_mask, road_hierarchy all in the probe struct|

## What is MISSING from `app/src/Tools/`[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/tree/main/app/src/Tools)​

The `Tools/` directory has only Axiom files + `ContextWindowPopup.cpp`. There are **zero implementations** for:

- `RoadTool.cpp` — no interactive road drawing
    
- `DistrictTool.cpp` — no district paint/edit
    
- `LotTool.cpp` — no lot subdivision interaction
    
- `BuildingTool.cpp` — no building placement interaction
    
- `WaterTool.cpp` — no water body painting
    

The `EditorState.hpp` declares `Editing_Roads`, `Editing_Districts`, `Editing_Lots`, `Editing_Buildings`, `Editing_Water`, `Tool_Roads`, `Tool_Buildings`, `Tool_Water` events, and the `Viewport_DrawRoad`, `Viewport_PlaceAxiom`, `Viewport_BoxSelect` states — **none of those viewport states route to a corresponding tool handler in PrimaryViewport** yet.[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/tree/main/core/include/RogueCity/Core/Editor)​

The PrimaryViewport `update()` is a stub: `(void)delta_time; // Reserved for camera smoothing, input handling` — it consumes no input at all.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/app/src/Viewports/PrimaryViewport.cpp)​

---

## PART 2: EXPECTED VIEWPORT TOOL EXECUTION — SPEC PER TOOL

This is what each tool _should_ do in 2D, expressed as a precise interaction contract.

## [A] PAN TOOL (`Viewport_Pan`)

text

```
MouseDown(middle) OR (alt+left)  → begin pan, cache anchor = screen_to_world(mouse)
MouseMove                        → camera_xy_ -= (world_pos - anchor) 
MouseUp                          → end pan
Scroll wheel                     → camera_z_ ±= delta * pan_speed; zoom_ recalculated
```

_Currently: zoom calculation exists via `set_camera_position` but no input wires into it._

---

## [B] SELECT TOOL (`Viewport_Select`)

text

```
Click on entity                  → probe ViewportIndex at mouse_world
                                   → set SelectionManager selection[0] = VpProbeData
                                   → highlight entity (draw accent rect/outline in render)
Shift+Click                      → append to multi-selection
Click empty                      → clear selection
BoxSelect drag                   → collect all probes whose world AABB intersects the drag rect
                                   → commit to SelectionManager
Right-click selected entity      → context menu (delete / properties / move to layer)
```

_`SelectionManager.hpp` and `SelectionSync.hpp` exist but aren't called from PrimaryViewport._[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/tree/main/core/include/RogueCity/Core/Editor)​

---

## [C] ROAD TOOL (`Viewport_DrawRoad`) — the most complex

**Expected 2D execution flow:**

text

```
1. MouseDown on empty            → begin polyline; commit point[0] = world_pos
2. MouseMove                     → ghost preview segment from point[0] to mouse_world
                                   → snap to grid if snap_to_grid enabled (snap_size granularity)
                                   → show snap indicator diamond
3. MouseDown (subsequent)        → commit point[n]; extend polyline
                                   → if within 10m of existing road endpoint → MERGE (T-junction or end-to-end)
                                   → if within 10m of existing road midpoint → SPLIT + T-junction
4. Double-click OR Enter         → finalize road; push AddRoadCommand into GlobalState.roads
                                   → trigger DirtyLayer::Roads → schedule regeneration of downstream (lots, buildings)
5. Escape                        → cancel, discard ghost
6. Right-click existing road     → context menu: Set Hierarchy (Local/Collector/Arterial/Highway), Delete, Split Here
7. Drag road vertex (select mode → road vertex sub-select)
                                 → EditorManipulation::MoveRoadVertex()
                                 → push ModifyRoadVertexCommand
```

**Visual feedback required:**

- Ghost segment: dashed cyan line from last committed point to mouse
    
- Snap indicator: small diamond at snap point, color shift to green when snapping
    
- Junction preview: yellow circle when near an existing endpoint (merge zone)
    
- Road hierarchy colors: Highway=`IM_COL32(255,120,0,255)` amber, Arterial=`IM_COL32(0,255,200,255)` teal, Local=`IM_COL32(0,180,255,255)` blue — instead of the current flat cyan for everything
    

---

## [D] DISTRICT TOOL (`Editing_Districts`)

text

```
1. Click to place polygon vertices (minimum 3)
2. MouseMove → ghost edge from last vertex to mouse
3. Click near first vertex (within 15m) → CLOSE polygon → push AddDistrictCommand
4. Shift+drag existing border → InsertDistrictVertex at edge midpoint
5. Alt+click existing vertex → RemoveDistrictVertex (if border.size() > 3)
6. Drag vertex → EditorManipulation::ApplyTranslate on single vertex
7. Right-click closed district → set DistrictType (Residential/Commercial/Industrial/Mixed/Park)
```

---

## [E] LOT TOOL (`Editing_Lots`)

_Lots are currently generator-output only — no manual subdivision exists._

text

```
Expected: 
1. Click inside a District → subdivide that district's area using LotSubdivider
2. Drag lot boundary edge → resize lot (LotToken.boundary manipulation)
3. Right-click lot → set zone type, merge with adjacent
```

This tool likely lives at the generator-interaction boundary, not pure editor interaction.

---

## [F] BUILDING TOOL (`Editing_Buildings`)

text

```
1. Click inside a Lot → place BuildingSite at centroid
2. Drag to orient → set rotation_radians
3. Right-click → set building type preset / afg_preset
4. Move via ApplyTranslate (already wired in EditorManipulation)
```

---

## [G] WATER TOOL (`Editing_Water` / `AI_INTEGRATION_TAG: V1_PASS1_TASK2_WATER_STATE`)

text

```
1. Paint mode: click+drag → add boundary vertices to WaterBody
2. Flood fill variant: click inside district → auto-expand to fill
3. VpProbeData.flood field is populated — needs visualization overlay
4. Right-click water body → set type (River / Lake / Canal)
```

---

## PART 3: THE 2D CORRECTNESS CHECKLIST (Priority Order)

These are the blockers before you can say any tool "works" in 2D.

## CRITICAL MISSING — Ship Nothing Until Fixed

**P0-A: PrimaryViewport input routing**  
`PrimaryViewport::update()` must route mouse events to the active tool. Implement an `IViewportTool` interface:

cpp

```
struct IViewportTool {
    virtual void on_mouse_down(const Core::Vec2& world_pos, ImGuiMouseButton btn) = 0;
    virtual void on_mouse_up(const Core::Vec2& world_pos, ImGuiMouseButton btn) = 0;
    virtual void on_mouse_move(const Core::Vec2& world_pos) = 0;
    virtual void on_scroll(float delta) = 0;
    virtual void render(ImDrawList*, const PrimaryViewport&) = 0;
    virtual ~IViewportTool() = default;
};
```

`PrimaryViewport` holds a `IViewportTool* active_tool_` and dispatches `ImGui::IsMouseClicked/Released/GetMousePos` in `update()`. The AxiomPlacementTool already implements this pattern exactly — generalize it.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/app/src/Tools/AxiomPlacementTool.cpp)​

**P0-B: Pan + Zoom must actually work**  
Currently `camera_xy_` and `camera_z_` have no input path. Middle-mouse pan and scroll-wheel zoom need to be live before any tool testing is meaningful.

**P0-C: Road Tool stub**  
Create `app/src/Tools/RoadTool.cpp` + `.hpp`. Minimum viable: click-to-place polyline vertices, Enter to commit, push to `GlobalState.roads`, set `DirtyLayer::Roads`. Use the ghost line pattern already established in AxiomPlacementTool's ghost circle.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/app/src/Tools/AxiomPlacementTool.cpp)​

**P0-D: Road rendering differentiates hierarchy**  
`PrimaryViewport::render()` currently draws all roads in flat `IM_COL32(0,255,255,255)` at hardcoded thickness. Road hierarchy data is in `VpProbeData.road_hierarchy` — use it to drive color AND thickness per the spec above.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/app/src/Viewports/PrimaryViewport.cpp)​

---

## HIGH — Needed for usable session

**P1-A: SelectionManager wired into PrimaryViewport**  
`SelectionManager.hpp` exists but nothing probes the ViewportIndex on click. Implement a `ViewportIndexBuilder` call chain after each generation, then hit-test on `Viewport_Select` mouse clicks.[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/tree/main/core/include/RogueCity/Core/Editor)​

**P1-B: Box selection in `Viewport_BoxSelect`**  
Draw selection rect overlay (dashed white border), collect all VpProbeData within world-space rect, push to SelectionManager. The `EditorState` event already declares this state.

**P1-C: Road vertex drag**  
`EditorManipulation::MoveRoadVertex` exists — needs to be invoked. In `Viewport_Select` mode, after selecting a road, draw its vertices as small squares; dragging one calls `MoveRoadVertex` and pushes a `ModifyRoadVertexCommand`.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/app/src/Editor/EditorManipulation.cpp)​

**P1-D: District tool**  
`InsertDistrictVertex` and `RemoveDistrictVertex` are implemented. Build the polygon placement UI around them.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/app/src/Editor/EditorManipulation.cpp)​

---

## MEDIUM — Quality and correctness polish

**P2-A: Grid snap in road/district/building tools**  
`EditorParameters.snap_to_grid` and `snap_size` fields exist in `GlobalState.hpp`[screenshot from images]. Snap world_pos to `round(x / snap_size) * snap_size` when enabled.

**P2-B: Undo/redo for all tools**  
AxiomPlacementTool has a working undo/redo pattern (ICommand + CommandHistory). Road/District/Building tools must use the same pattern. Do not allow `GlobalState` mutations outside of an `ICommand::Execute()`.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/app/src/Tools/AxiomPlacementTool.cpp)​

**P2-C: DirtyLayer propagation**  
`DirtyLayer` enum (Axioms, Tensor, Roads...) exists in `GlobalState.hpp`. Road tool must set the appropriate dirty flag, which triggers downstream regeneration. Currently nothing outside AxiomPlacementTool sets dirty flags.

**P2-D: Water body rendering**  
`PrimaryViewport::render()` has no water rendering path despite `GlobalState.waterbodies` existing and `VpProbeData.flood` being populated. Add a fill polygon (semi-transparent blue-teal) for water bodies.[](https://raw.githubusercontent.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/main/core/include/RogueCity/Core/Editor/ViewportIndex.hpp)​

---

## PART 4: Y2K MAP VISUALIZATION UPGRADE



## Color System — Replace the flat palette with a semantic one added to our token system

```cpp
// Y2K Cartographic Palette
namespace Y2KPalette {
    // Roads by hierarchy
    constexpr ImU32 kHighway    = IM_COL32(255, 160,  40, 255); // amber
    constexpr ImU32 kArterial   = IM_COL32(  0, 220, 180, 255); // teal
    constexpr ImU32 kCollector  = IM_COL32( 80, 180, 255, 255); // sky blue
    constexpr ImU32 kLocal      = IM_COL32( 40, 130, 200, 200); // dim blue
    
    // Districts (fill + border)
    constexpr ImU32 kResidential_Fill   = IM_COL32( 20,  80,  60,  60);
    constexpr ImU32 kResidential_Border = IM_COL32(  0, 255, 150, 200);
    constexpr ImU32 kCommercial_Fill    = IM_COL32( 80,  20,  80,  60);
    constexpr ImU32 kCommercial_Border  = IM_COL32(255,   0, 220, 200);
    constexpr ImU32 kIndustrial_Fill    = IM_COL32( 80,  50,  10,  60);
    constexpr ImU32 kIndustrial_Border  = IM_COL32(255, 160,   0, 200);
    
    // Lots
    constexpr ImU32 kLot_Unzoned  = IM_COL32(200, 200, 200,  30);
    constexpr ImU32 kLot_Zoned    = IM_COL32(255, 220, 100,  60);
    
    // Water
    constexpr ImU32 kWater_Fill   = IM_COL32(  0,  80, 180,  80);
    constexpr ImU32 kWater_Border = IM_COL32(  0, 200, 255, 255);
    
    // Grid
    constexpr ImU32 kGrid_Minor   = IM_COL32( 40,  50,  70, 100);
    constexpr ImU32 kGrid_Major   = IM_COL32( 60,  80, 120, 180);
}
```

## Layered rendering order (painter's algorithm)

text

```
1. Background fill (deep blue-black) ← already done
2. Minor grid (50m cells)            ← already done
3. Major grid (500m cells, brighter) ← ADD
4. Water body fills (semi-transparent)← ADD
5. District fills (semi-transparent)  ← ADD (currently wireframe only)
6. Lot fills (very subtle)            ← ADD
7. District borders                   ← exists, needs color system
8. Lot borders                        ← exists
9. Road lines (hierarchy-colored)     
````