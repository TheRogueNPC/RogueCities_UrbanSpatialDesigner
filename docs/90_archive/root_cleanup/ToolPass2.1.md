 #PROMPT
 Implement the plan below with minimal file churn and no stylistic rewrites. Prefer small, reviewable diffs. Do not invent new architecture beyond what is required to satisfy acceptance criteria.

GOAL
Fix two related clusters:
1) Axiom live preview must NOT trigger full city generation/apply; it must only preview road skeleton (and optionally bounds) with explicit scope/depth gating.
2) Viewport tools must be wired end-to-end with usable selection tolerance and basic vertex manipulation affordances; unblock missing tool stubs and fix known district merge/fill correctness issues.

PATCH DISCIPLINE
- Keep existing default behavior unchanged unless explicitly required.
- Add new enums/options with defaults that preserve prior behavior (FullPipeline + FullCity).
- Avoid renaming existing public functions unless necessary; prefer overloads or defaulted params.
- Add TODOs only when unavoidable; no large refactors.

TARGET FILE LIST (edit/create as needed; confirm exact paths by searching repo)
Axiom/Generation cluster (Steps 1–4 ship together):
- app/include/RogueCity/App/Integration/CityOutputApplier.hpp
- app/src/Integration/CityOutputApplier.cpp
- app/include/RogueCity/App/Integration/RealTimePreview.hpp (or GenerationCoordinator header if separate)
- app/src/Integration/RealTimePreview.cpp (or GenerationCoordinator.cpp)
- visualizer/src/ui/panels/rc_panel_axiom_editor.cpp (and any panel plumbing calling it)

Tool feel/wiring cluster (Steps 5–7):
- app/include/RogueCity/App/Tools/IViewportTool.hpp (NEW)
- visualizer/src/ui/viewport/PrimaryViewport.cpp (and header if needed)
- visualizer/src/ui/viewport/rc_viewport_handler_common.cpp
- visualizer/src/ui/viewport/rc_viewport_non_axiom_pipeline.cpp (or wherever MoveRoadVertex is wired)
- app/src/Tools/RoadTool.cpp (NEW if missing)
- app/src/Tools/DistrictTool.cpp (NEW if missing)
- app/src/Tools/WaterTool.cpp (NEW if missing)

Correctness polish (Steps 8–10):
- core/editor manipulation source (e.g., app/src/EditorManipulation.cpp or core/src/...; locate by searching for “MergeDistrict”, “BOOST”, “Union”, “AddConvexPolyFilled”)
- viewport overlay rendering (where zone/district fill is drawn)

FOUNDATIONAL SEARCH / RECON
1) Locate:
   - ApplyCityOutputToGlobalState and CityOutputApplier.{hpp,cpp}
   - The generation request path from rc_panel_axiom_editor.cpp (DrawContent -> RequestRegeneration -> SetOnComplete)
   - The generation coordinator / RealTimePreview request method signatures
   - GlobalState road struct/type and any existing flags/source markers
   - PickFromViewportIndex definition and call sites
   - EditorManipulation::MoveRoadVertex and district merge implementation
   - Any current BOOST integration helpers (polygon conversions, union calls)

IMPLEMENTATION PLAN (in this order)

=== PR 1 (tightly coupled): Steps 1–4 + “user-authored road source flag” ===

STEP 1: Add GenerationScope + apply options
- In CityOutputApplier.hpp:
  - Add enum class GenerationScope : uint8_t { RoadsOnly, RoadsAndBounds, FullCity };
  - Add struct CityOutputApplyOptions:
      bool rebuild_viewport_index { true };
      bool mark_dirty_layers_clean { true }; (only if already used; otherwise omit to avoid unused)
      bool preserve_locked_user_entities { true };
      GenerationScope scope { GenerationScope::FullCity };
- Ensure existing ApplyCityOutputToGlobalState keeps previous signature working:
  - Either overload: ApplyCityOutputToGlobalState(output, gs) calls ApplyCityOutputToGlobalState(output, gs, CityOutputApplyOptions{});
  - Or add defaulted options param while preserving call sites.

STEP 1.5 (critical contract): Add a source-of-truth “user-authored” marker on roads
- Locate road entity type stored in GlobalState (or equivalent).
- Add a minimal marker that can be checked in the applier:
  - Prefer enum class RoadSource { Generated, UserAuthored }; OR bool user_authored;
- Ensure the marker defaults to Generated to preserve existing behavior.
- Ensure tools that create roads stamp UserAuthored at commit time (see tool steps later).
- Update any serialization/clone/copy paths if the struct is persisted or duplicated.

STEP 2: Gate CityOutputApplier.cpp by scope and preserve user-authored entities
- In ApplyCityOutputToGlobalState(..., options):
  - Always apply roads for all scopes, but DO NOT overwrite user-authored roads when preserve_locked_user_entities is true.
    - Implement preservation as: when applying generated output, merge/replace only generated roads; keep user-authored roads intact.
    - If ids exist: preserve by stable id and source marker; if no ids exist: preserve by source marker by copying user-authored subset out before replace and re-inserting after.
    - Keep this logic local to applier to minimize churn.
  - If options.scope >= RoadsAndBounds: apply district boundary outlines only (no zone fill); implement as a separate ApplyDistrictBoundaries(...) or a limited path within existing district apply.
  - If options.scope == FullCity: apply districts/lots/buildings/blocks as before.
  - Rebuild viewport index only when options.rebuild_viewport_index is true.
- Make sure default options yield identical behavior to current (FullCity).

STEP 3: Add GenerationDepth to generation coordinator / RealTimePreview
- Add enum class GenerationDepth : uint8_t { AxiomBounds, FullPipeline };
- Extend the generation request API to accept depth with default FullPipeline:
  - request_regeneration(axioms, config, GenerationDepth depth = FullPipeline)
- Implement depth behavior:
  - AxiomBounds stops after tensor field + road skeleton stage (or the earliest stage that produces the preview output you already have).
  - FullPipeline runs all stages as before.
- Ensure existing call sites still compile and behave the same due to default param.

STEP 4: Update rc_panel_axiom_editor.cpp live preview to use AxiomBounds + RoadsOnly apply scope
- In DrawContent / axiom change path:
  - Call RequestRegeneration(..., GenerationDepth::AxiomBounds) for live preview.
  - In SetOnComplete callback used for live preview:
    - ApplyCityOutputToGlobalState with options:
      - opts.scope = GenerationScope::RoadsOnly (or RoadsAndBounds if your preview includes outlines)
      - opts.preserve_locked_user_entities = true
      - opts.rebuild_viewport_index = true (unless expensive; keep default)
- Ensure the explicit “Generate City” button/path uses:
  - GenerationDepth::FullPipeline
  - GenerationScope::FullCity
- Add a small safeguard to prevent full apply being called from the live preview path (e.g., separate callbacks or a flag), but do not refactor the panel heavily.

===  (unblocks tools): Steps 5–7 ===

STEP 5: Create canonical IViewportTool interface and wire PrimaryViewport::update()
- Create app/include/RogueCity/App/Tools/IViewportTool.hpp with the interface shape:
  - on_activate/on_deactivate
  - on_mouse_down/up/move/scroll
  - on_key
  - on_render
  - name()
- Wire PrimaryViewport:
  - Store active_tool_ (pointer/ref/unique_ptr depending on existing patterns; minimal change)
  - In update(): translate screen->world, dispatch mouse/key/scroll events to active tool; respect “consumed” return.
  - Preserve existing AxiomPlacementTool dispatch patterns; reuse existing code if present.
  - Ensure tool switching calls on_deactivate/on_activate.

STEP 6: Add selection tolerance to PickFromViewportIndex
- In rc_viewport_handler_common.cpp:
  - Extend PickFromViewportIndex(gs, world_pos, tolerance=...) where tolerance is effectively constant in screen-space, converted to world using current camera zoom/scale (Illustrator-like feel).
  - Implement:
    - For road segments: pick nearest segment within tolerance.
    - For vertices: pick nearest vertex within tolerance/2.
    - For polygonal areas: keep containment test; optionally allow boundary proximity if cheap.
- Update call sites to pass appropriate tolerance derived from camera zoom (or make PickFromViewportIndex take Camera2D / zoom to compute internally).
- Acceptance: selection is no longer pixel-perfect; a user can click near a road/vertex and select it.

STEP 7: Road vertex handles + MoveRoadVertex wiring
- When a road is selected:
  - Render vertex handles as ~6px squares in screen-space (convert to world when needed).
  - Implement click vs drag threshold around ~3px screen-space.
- Wire dragging to existing EditorManipulation::MoveRoadVertex (confirm signature and use it).
- Ensure dragging creates a single command per drag interaction (not per frame) if you have CommandHistory; otherwise at least do not flood.

=== PR 2 (correctness polish): Steps 8–10 ===

STEP 8: Replace district merge concatenation with BOOST union
- Locate district merge function (EditorManipulation or similar).
- Replace “concatenate boundaries” logic with a proper polygon union using BOOST (BOOSTUnion / UnaryUnion as appropriate).
- Handle multi-polygons: choose either largest polygon or store multipolygon if your model supports it; prefer minimal change that matches existing data model.
- Add basic validation: if union fails, keep originals or fallback gracefully.

STEP 9: Replace AddConvexPolyFilled for concave polygons
- Locate rendering path using ImDrawList::AddConvexPolyFilled for districts/zones.
- Implement triangulation (ear clipping) and draw triangles (AddTriangleFilled) or use ImDrawList::PrimReserve/PrimWrite if already used.
- Keep it self-contained: a helper function TriangulatePolygon(points)->indices.
- Acceptance: concave districts render filled correctly.

STEP 10: Stub missing tool .cpp files and ensure they register/activate
- Create minimal implementations for RoadTool, DistrictTool, WaterTool under app/src/Tools if missing.
- Each must:
  - Implement IViewportTool
  - Be constructible/registered in existing tool registry/menu
  - At minimum: activate, respond to click to create one primitive (e.g., road segment) or enter an editing state
  - For RoadTool commit: stamp RoadSource::UserAuthored (or user_authored=true) on created roads.

INTEGRATION POINTS (do not break)
- rc_panel_axiom_editor.cpp should remain responsive; live preview must not trigger full apply.
- CityOutputApplier must remain callable from existing full generation path with same defaults.
- ViewportIndexBuilder rebuild should still occur when needed for picking.
- Tool switching must not break AxiomPlacementTool.

PROTECTED INVARIANTS
- Default generation path (non-preview) still generates and applies full city exactly as before.
- No user-authored road is overwritten during live preview applies when preserve_locked_user_entities is true.
- No API break: existing callers compile without modification where possible (via overloads/default params).
- Avoid per-frame command flooding during drags/ImGui inputs.

VALIDATION STEPS
1) Build the project (whatever the repo uses: CMake/Ninja/MSBuild; follow existing instructions).
2) Run existing unit/integration tests if present; otherwise add a minimal targeted test if the repo has a testing harness:
   - Optional: a small test for applier preservation (user-authored roads remain after applying generated output).
3) Manual smoke checks (describe in PR notes):
   - Place/move an axiom: verify only road skeleton updates; districts/lots/buildings do not regenerate.
   - Click “Generate City”: verify full city appears.
   - Draw a user road: regenerate preview; verify user road persists.
   - Selection: click near road/vertex; selection works with tolerance.
   - Drag a road vertex: handle appears, drag updates geometry, no performance hitch.
   - Merge districts: union produces correct shape; no figure-8.
   - Concave district fill renders correctly.

ACCEPTANCE CRITERIA (must all be true)
- Axiom live edits no longer trigger full-city generation/apply; only preview layers update.
- CityOutputApplier supports GenerationScope with correct gating and defaults preserving prior behavior.
- GenerationDepth exists and is used by live preview to stop early.
- User-authored roads are distinguishable and preserved during partial applies.
- PrimaryViewport dispatches input to an active IViewportTool; at least AxiomPlacementTool continues working.
- Picking uses a tolerance that feels forgiving; road/vertex selection is not pixel-perfect.
- Road vertex handles render and dragging moves vertices via EditorManipulation::MoveRoadVertex.
- District merge uses BOOST union; concave fill is triangulated (no convex-only fill).

OUTPUT REQUIREMENTS
- Provide a concise summary of changes and list of modified/added files.
- For each PR group (1–3), keep commits logically separated if feasible, but do not over-engineer.
- Ensure code compiles.

Now implement the above in the repo.
Use the Below for helpful refernce
----
#TASK

# Nav For Ai 
```json
{"refer_back_protocol":"Reference section_tree anchors+line ranges; implement in priority_order sequence; preserve existing defaults via overloads/default params; minimal churn; treat code snippets as intent not exact identifiers.","section_tree":[["overview_problem_clusters",1,"Two Separate but Related Problem Clusters"],["p1_axiom_triggers_full_city",4,"Problem 1: Axiom Placement Triggers Full City Generation"],["p1_why_it_happens",5,"Why It Happens"],["p1_fix_three_layer_scope_gate",12,"The Fix: Three-Layer Scope Gate"],["p1_layer_a_generationscope_options",13,"Layer A — Add GenerationScope to CityOutputApplier.hpp"],["p1_layer_b_gate_applier_body",28,"Layer B — Gate the applier body in CityOutputApplier.cpp"],["p1_layer_c_generationdepth",51,"Layer C — Add GenerationDepth to RealTimePreview / GenerationCoordinator"],["p1_live_preview_callsite_change",64,"rc_panel_axiom_editor.cpp live preview call + scoped OnComplete"],["p1_preserve_user_placed_entities",82,"Preserving User-Placed Roads and Districts"],["p2_tools_feel_gap",85,"Problem 2: Tools Feel Unwieldy / Selection Too Exact"],["p2_root_cause_missing_wiring",86,"Root Cause: Missing Wiring and Interface"],["p2_fix_a_iviewporttool_interface",97,"Fix A: Canonical IViewportTool Interface"],["p2_fix_b_pick_tolerance",116,"Fix B: Selection Tolerance (the \"too exact\" problem)"],["p2_fix_c_spline_polygon_contracts",134,"Fix C: Spline and Polygon Tool Contracts (What's Actually Missing)"],["priority_order",143,"Priority Order (Do These In Sequence)"],["critical_missing_user_authored_flag",157,"One Critical Thing Your Plan Doesn't Fully Address"]],"indexed_code_fences":[[8,10,"cpp"],[15,20,"cpp"],[30,36,"cpp"],[53,57,"cpp"],[66,70,"cpp"],[100,114,"cpp"],[119,122,"cpp"]],"file_paths":{"axiom_generation_scope_depth":["visualizer/src/ui/panels/rc_panel_axiom_editor.cpp@L6,L64,L147","app/src/Integration/CityOutputApplier.cpp@L10,L28,L148","app/include/.../CityOutputApplier.hpp@L13","app/include/.../RealTimePreview.hpp@L59","app/src/.../RealTimePreview.cpp@L146","app/src/.../GenerationCoordinator.cpp@L146"],"tool_interface_and_dispatch":["app/include/RogueCity/App/Tools/IViewportTool.hpp@L89,L98,L149","visualizer/src/ui/viewport/PrimaryViewport.cpp@L149,L153"],"selection_and_manipulation":["visualizer/src/ui/viewport/rc_viewport_handler_common.cpp@L117,L150","visualizer/src/ui/viewport/rc_viewport_non_axiom_pipeline.cpp@L151","(core/app)EditorManipulation.cpp@L152"],"tool_stubs":["app/src/Tools/@L154"]},"namespaces":{"primary":["RogueCity::App","RogueCity::Core","ImGui"],"types_enums":["GenerationScope","CityOutputApplyOptions","GenerationDepth","IViewportTool","RoadFlags::UserPlaced (mentioned)","RoadSource/user_authored (proposed)"],"possible_conflicts":[{"sym":"GenerationScope","note":"Ensure no existing enum of same name in Integration namespace.","line":"L13-L20"},{"sym":"GenerationDepth","note":"Ensure depth concept doesn’t collide with existing ‘Config’ flags or stage enums.","line":"L51-L63"},{"sym":"IViewportTool","note":"Check for existing tool base type; avoid duplicate interface definitions.","line":"L97-L115"}]},"integration_points":[{"id":"ip_axiom_panel_triggers_regen","where":"DrawContent() triggers RequestRegeneration on axiom change; SetOnComplete applies output immediately","line_range":"L6-L11","anchor":"p1_why_it_happens"},{"id":"ip_applycityoutput_scope_gate","where":"ApplyCityOutputToGlobalState gates ApplyRoads/ApplyDistrictBoundaries/ApplyDistricts+Lots+Buildings+Blocks based on GenerationScope","line_range":"L12-L50","anchor":"p1_fix_three_layer_scope_gate"},{"id":"ip_generationdepth_short_circuit","where":"RealTimePreview/GenerationCoordinator accepts GenerationDepth; AxiomBounds stops after tensor field→road skeleton","line_range":"L51-L63","anchor":"p1_layer_c_generationdepth"},{"id":"ip_live_preview_scoped_apply","where":"Axiom live preview uses GenerationDepth::AxiomBounds and applies with GenerationScope::RoadsOnly + preserve_locked_user_entities","line_range":"L64-L81","anchor":"p1_live_preview_callsite_change"},{"id":"ip_user_entity_preservation_contract","where":"Applier must preserve user-placed roads (RoadFlags::UserPlaced or new RoadSource/user_authored); generator treats them as anchors","line_range":"L82-L84","anchor":"p1_preserve_user_placed_entities"},{"id":"ip_viewport_tool_dispatch","where":"PrimaryViewport holds active IViewportTool and forwards mouse/key/scroll/render; AxiomPlacementTool pattern reused","line_range":"L97-L115","anchor":"p2_fix_a_iviewporttool_interface"},{"id":"ip_pick_tolerance","where":"PickFromViewportIndex gains tolerance scaling with zoom; nearest segment/vertex selection within radius","line_range":"L116-L133","anchor":"p2_fix_b_pick_tolerance"},{"id":"ip_boost_union_merge","where":"District merge uses Boost.Geometry union_ instead of boundary concatenation","line_range":"L134-L142","anchor":"p2_fix_c_spline_polygon_contracts"},{"id":"ip_concave_fill_triangulation","where":"Zone fill replaces AddConvexPolyFilled with ear-clipping/triangulation for concave polygons","line_range":"L140-L142","anchor":"p2_fix_c_spline_polygon_contracts"}],"dependency_graph":{"1_generationscope_gate_applier":[],"2_generationdepth_in_preview_coordinator":["1_generationscope_gate_applier"],"3_axiom_panel_pass_axiombounds":["2_generationdepth_in_preview_coordinator"],"4_preserve_locked_user_entities":["1_generationscope_gate_applier","critical_user_authored_flag"],"5_iviewporttool_and_primaryviewport_update":[],"6_pick_tolerance":["5_iviewporttool_and_primaryviewport_update"],"7_vertex_handles_movevertex_wiring":["6_pick_tolerance"],"8_boost_union_merge":[],"9_concave_fill_earclip":[],"10_tool_stubs_register_activate":["5_iviewporttool_and_primaryviewport_update"],"critical_user_authored_flag":[]},"risk_nodes":[{"id":"risk_unintended_full_apply_default_change","line_range":"L12-L50","section_anchor":"p1_fix_three_layer_scope_gate","desc":"Adding scope/depth must preserve existing FullCity/FullPipeline defaults; avoid regressions in non-preview generation."},{"id":"risk_preservation_semantics_unclear","line_range":"L82-L84","section_anchor":"p1_preserve_user_placed_entities","desc":"‘Preserve user roads’ requires stable marker and merge semantics; risk of duplicate/ordering/id conflicts when blending generator output."},{"id":"risk_tool_dispatch_integration","line_range":"L97-L115","section_anchor":"p2_fix_a_iviewporttool_interface","desc":"PrimaryViewport update currently stub; wiring input may break existing camera/nav handlers or AxiomPlacementTool if event ownership unclear."},{"id":"risk_pick_performance","line_range":"L116-L133","section_anchor":"p2_fix_b_pick_tolerance","desc":"Nearest-segment/vertex within tolerance may increase per-frame cost if spatial index not leveraged; ensure uses ViewportIndex/accelerations."},{"id":"risk_polygon_union_edge_cases","line_range":"L134-L142","section_anchor":"p2_fix_c_spline_polygon_contracts","desc":"Boost.Geometry union may return multipolygons/invalid geometries; data model must handle or select appropriate output."},{"id":"risk_triangulation_correctness","line_range":"L140-L142","section_anchor":"p2_fix_c_spline_polygon_contracts","desc":"Ear-clipping/triangulation must handle winding/holes/self-intersections; rendering artifacts possible if inputs not sanitized."}],"blocking_questions":["Where is the canonical Road entity struct defined (GlobalState) and does it already have IDs/flags suitable for preservation? (Needed for preserve_locked_user_entities + user_authored marker)","Where exactly is RequestRegeneration/SetOnComplete implemented (RealTimePreview vs GenerationCoordinator) and what stages can be short-circuited for AxiomBounds?","Where is tool activation/registry owned (menu/panel) so new IViewportTool and tool stubs integrate without duplicating systems?"],"compression_loss_risks":[{"description":"Code snippets are illustrative; actual signatures (ApplyCityOutputToGlobalState, RequestRegeneration) may differ and require overloads/adapters.","line_range":"L6-L81","section_anchor":"p1_axiom_triggers_full_city","why_at_risk":"Direct copy/paste could cause compile breaks or mis-wire callbacks.","recovery_hint":"Codex should first search exact function declarations/definitions and implement minimal compatible overloads/default params."},{"description":"Preserve behavior references RoadFlags::UserPlaced, but context notes it may not exist; requires adding RoadSource/user_authored and stamping in RoadTool commit.","line_range":"L82-L158","section_anchor":"critical_missing_user_authored_flag","why_at_risk":"Without a real marker, applier cannot distinguish user vs generated; preservation logic becomes guesswork.","recovery_hint":"Add explicit field in Road struct; default Generated; set UserAuthored in RoadTool; preserve by filtering/merging in applier."},{"description":"Selection tolerance defined in ‘world units’ but should scale with zoom (screen-space feel).","line_range":"L116-L133","section_anchor":"p2_fix_b_pick_tolerance","why_at_risk":"Fixed world tolerance will feel wrong across zoom levels.","recovery_hint":"Compute tolerance_world = tolerance_pixels / camera.scale (or equivalent) and pass through PickFromViewportIndex."},{"description":"District fill/union details depend on polygon representation (holes, concavity, multiple rings) not specified here.","line_range":"L134-L156","section_anchor":"p2_fix_c_spline_polygon_contracts","why_at_risk":"May require additional geometry utilities or constraints beyond ‘one-function fix’.","recovery_hint":"Inspect existing Boost adapters and polygon storage; handle multipolygon by selecting largest shell or extending model if already supports multiple."},{"description":"Tool scope includes advanced RoadSpline tooling (Illustrator parity) but only the missing visualization/handles are explicitly required now.","line_range":"L134-L142","section_anchor":"p2_fix_c_spline_polygon_contracts","why_at_risk":"Codex may over-implement large toolset instead of minimal unblockers.","recovery_hint":"Constrain implementation to: IViewportTool wiring, tolerant picking, vertex handles + MoveRoadVertex, and minimal tool stubs; defer full spline suite."}]}
```

Two Separate but Related Problem Clusters
Your issues split cleanly into the Axiom/Generation pipeline bug and the tool feel/wiring gap. They share a root: the architecture is defined but not connected end-to-end.

Problem 1: Axiom Placement Triggers Full City Generation
Why It Happens
In rc_panel_axiom_editor.cpp, DrawContent() fires RequestRegeneration(...) on any axiom change, and the SetOnComplete callback immediately calls:

cpp
RogueCity::App::ApplyCityOutputToGlobalState(output, gs);
ApplyCityOutputToGlobalState (in CityOutputApplier.cpp) applies roads + districts + blocks + lots + buildings unconditionally every time — there is no scope gate. The StreetSweeper protocol runs the full pipeline to completion and the applier dumps everything. That's confirmed in your repo at CityOutputApplier.cpp.

The Fix: Three-Layer Scope Gate
Layer A — Add GenerationScope to CityOutputApplier.hpp

cpp
enum class GenerationScope : uint8_t {
    RoadsOnly      = 0,  // Axiom live preview: tensor field + road skeleton only
    RoadsAndBounds = 1,  // Adds district boundary outlines, no zoning fill
    FullCity       = 2,  // All layers: districts, lots, buildings
};

struct CityOutputApplyOptions {
    bool rebuild_viewport_index    { true  };
    bool mark_dirty_layers_clean   { true  };
    bool preserve_locked_user_entities { true };  // ← CRITICAL for user-placed roads
    GenerationScope scope          { GenerationScope::FullCity };
};
Layer B — Gate the applier body in CityOutputApplier.cpp

cpp
void ApplyCityOutputToGlobalState(
    const CityOutput& output, GlobalState& gs,
    const CityOutputApplyOptions& options)
{
    // Always apply roads (all scopes)
    ApplyRoads(output, gs, options.preserve_locked_user_entities);

    if (options.scope >= GenerationScope::RoadsAndBounds)
        ApplyDistrictBoundaries(output, gs);  // outlines only

    if (options.scope == GenerationScope::FullCity) {
        ApplyDistricts(output, gs);
        ApplyLots(output, gs);
        ApplyBuildings(output, gs);
        ApplyBlocks(output, gs);
    }

    if (options.rebuild_viewport_index)
        ViewportIndexBuilder::Build(gs);
}
Layer C — Add GenerationDepth to RealTimePreview / GenerationCoordinator

cpp
enum class GenerationDepth : uint8_t {
    AxiomBounds  = 0,  // Stops after tensor field → road skeleton
    FullPipeline = 1,  // Runs all generator stages
};

// In RealTimePreview.hpp:
void request_regeneration(
    const AxiomInputs& axioms,
    const Config& config,
    GenerationDepth depth = GenerationDepth::FullPipeline);
Then in rc_panel_axiom_editor.cpp, change the live preview call:

cpp
// Live axiom preview — skeleton only
s_generation_coordinator->RequestRegeneration(
    axiom_inputs, config,
    GenerationDepth::AxiomBounds);   // ← was: no depth, caused full apply

// OnComplete callback scoped accordingly:
s_generation_coordinator->SetOnComplete([](const CityOutput& output) {
    auto& gs = GetGlobalState();
    CityOutputApplyOptions opts;
    opts.scope = GenerationScope::RoadsOnly;           // ← key change
    opts.preserve_locked_user_entities = true;         // ← protects user roads
    ApplyCityOutputToGlobalState(output, gs, opts);
});
The "Generate City" button uses GenerationDepth::FullPipeline + GenerationScope::FullCity.

Preserving User-Placed Roads and Districts
The preserve_locked_user_entities flag in CityOutputApplyOptions is the hook for this. In the apply function, before overwriting gs.roads, skip any road where road.flags & RoadFlags::UserPlaced. The generator's output should blend around user geometry, not replace it. This needs a corresponding contract in RoadGenerator — user roads should be treated as fixed anchors in the tensor field, not overwritten.
​

Problem 2: Tools Feel Unwieldy / Selection Too Exact
Root Cause: Missing Wiring and Interface
Your audit confirms:
​

No IViewportTool.hpp canonical interface exists

PrimaryViewport::update() is an empty stub — no input is dispatched to tools

Only AxiomPlacementTool is fully implemented; RoadTool, DistrictTool, LotTool, BuildingTool, WaterTool have no .cpp files

Selection uses PickFromViewportIndex with no hit tolerance — it's pixel-exact

Fix A: Canonical IViewportTool Interface
Create app/include/RogueCity/App/Tools/IViewportTool.hpp:

cpp
struct IViewportTool {
    virtual ~IViewportTool() = default;
    virtual void on_activate()   {}
    virtual void on_deactivate() {}
    // Returns true if input was consumed
    virtual bool on_mouse_down(ImVec2 world_pos, int button)  { return false; }
    virtual bool on_mouse_up  (ImVec2 world_pos, int button)  { return false; }
    virtual bool on_mouse_move(ImVec2 world_pos)              { return false; }
    virtual bool on_scroll    (float delta)                   { return false; }
    virtual void on_key       (int key, bool ctrl, bool shift) {}
    virtual void on_render    (ImDrawList* dl, const Camera2D&) {}
    virtual const char* name  () const = 0;
};
Wire PrimaryViewport to hold IViewportTool* active_tool_ and call through in update(). The AxiomPlacementTool already uses this exact shape — copy its dispatch pattern.

Fix B: Selection Tolerance (the "too exact" problem)
The Illustrator feel comes from proximity snapping, not pixel-hit-testing. In rc_viewport_handler_common.cpp, PickFromViewportIndex currently tests exact containment. Add a world-space tolerance:

cpp
// Instead of exact point-in-polygon:
// Use a snapping radius that scales with zoom
constexpr float kPickToleranceWorld = 8.0f;  // world units

VpProbeData* PickFromViewportIndex(
    GlobalState& gs, ImVec2 world_pos,
    float tolerance = kPickToleranceWorld)
{
    // For roads: pick nearest segment within tolerance
    // For vertices: pick nearest vertex within tolerance/2
    // For areas (districts/lots): containment check still valid
}
For road vertex drag specifically, draw vertex handles as 6px squares when a road is selected. The drag threshold to distinguish click-from-drag should be ~3px screen-space (matches Illustrator's default). Wire this through EditorManipulation::MoveRoadVertex which already exists.
​

Fix C: Spline and Polygon Tool Contracts (What's Actually Missing)
Your audit doc defines full specs for these:
​

RoadSpline tools (Selection, DirectSelect, Pen, ConvertAnchor, AddRemoveAnchor, HandleTangents, SnapAlign, JoinSplit, Simplify) — these map directly to Illustrator's pen/direct selection tools. The key missing piece is the handle tangent display: you need to render Bezier control handles when a road spline vertex is selected. The geometry is in your spline data; you just need to draw it.

District polygon tools have a critical bug: Merge concatenates two polygon boundary arrays, which creates a figure-8, not a union. Use Boost.Geometry (already in your stack), replace the merge with boost::geometry::union_(). This is a one-function fix in EditorManipulation.

Zone fill uses AddConvexPolyFilled — this breaks on any concave district. Replace with AddConcavePolyFilled via ear-clipping, or triangulate the polygon before filling.

Priority Order (Do These In Sequence)
#	Task	File	Why Now
1	GenerationScope enum + gate applier	CityOutputApplier.hpp/.cpp	Fixes axiom bug, enables partial generation
2	GenerationDepth in RealTimePreview + GenerationCoordinator	RealTimePreview.hpp/.cpp, GenerationCoordinator.cpp	Connects depth to scope
3	Update axiom panel to pass AxiomBounds	rc_panel_axiom_editor.cpp	Stops full-city-on-axiom-place
4	preserve_locked_user_entities in applier	CityOutputApplier.cpp	Lets users start with own roads/districts
5	IViewportTool.hpp + wire PrimaryViewport::update()	New file + PrimaryViewport.cpp	Enables all tools to work
6	Add pick tolerance to PickFromViewportIndex	rc_viewport_handler_common.cpp	Fixes "too exact" selection
7	Road vertex handles + MoveRoadVertex wiring	rc_viewport_non_axiom_pipeline.cpp	Illustrator-style vertex dragging
8	Replace district Merge with Boost.Geometry union_	EditorManipulation.cpp	Fixes topological merge bug
9	Replace AddConvexPolyFilled with ear-clip	PrimaryViewport.cpp or overlays	Fixes concave district fills
10	Stub RoadTool, DistrictTool, WaterTool	app/src/Tools/	Lets tools register and activate
Steps 1–4 are the StreetSweeper/axiom fix and should ship as one PR since they're tightly coupled. Steps 5–7 are the tool feel fix and unblock all remaining tool work. Steps 8–10 are correctness polish.
​

One Critical Thing Your Plan Doesn't Fully Address
preserve_locked_user_entities needs a source-of-truth flag on the entity itself. Right now there's no RoadFlags::UserPlaced bit on road structs. Before the scope gate works correctly, you need to stamp any road created by RoadTool (user-placed) vs. generated by StreetSweeper (generator-placed) — otherwise the applier can't distinguish what to preserve. Add a bool user_authored or RoadSource enum to your Road struct in GlobalState, set it in RoadTool::commit(), and check it in the applier. This is the contract between your manual tools and the generator that the current spec doesn't explicitly define