# Changelog

## [Unreleased] - 2026-03-03 (ASAM OpenDRIVE Objects Full Spec)
- **Object Feature Completeness (ASAM OpenDRIVE 1.8 § 13)**: Extended `rc_opendrive` with full support for:
  - `RoadObject` extensions: `material`, `parkingSpace`, `markings`, `borders`, `skeleton`.
  - Road-level objects: `objectReference`, `tunnel`, `bridge`.
  - Multi-outline support (v1.45+) and lane validity for objects.
  - JSON serialization/deserialization for all new object types.
  - New `RoadObjectXmlWriter` for high-fidelity XML emission of ASAM 1.8 objects.
## [Unreleased] - 2026-03-03 (ASAM OpenDRIVE Common Junction Full Spec)
- **Junction Feature Completeness (ASAM OpenDRIVE 1.8 § 6)**: Extended `rc_opendrive` (`Junction.h/cpp`, `OpenDriveMap.cpp`, `JsonSerialization.h`) with full Common Junction topology — `type` attribute (§ 6.2), `JunctionCrossPath` for pedestrian crossings (§ 6.3.3 / Code 24), and `JunctionTrafficIsland` with `<cornerLocal>` polygon outlines (§ 6.5 / Code 32). Added `JunctionXmlWriter.h`, a header-only pugixml DOM writer that emits spec-compliant `<junction>` XML with deterministic element ordering; the JSON-first fast-generator path and the posterity XML path now share the same in-memory structs, enabling true spatial believability in procedurally generated cities.

## [Unreleased] - 2026-03-03 (OpenDRIVE Architecture Bridging)
- **Standalone `rc_opendrive` Library Integration**: Added JSON serialization and R-OADG coordinate definitions to `rc_opendrive`.
- **Core Architecture Bridge**: Created `OpenDriveBridge` module to map OpenDRIVE roads and topologies natively to `RogueCity::Core` structures, adhering to strict layer boundaries.

## [Unreleased] - 2026-03-02 (OpenDRIVE Lanes & Road Structure)
- **Implement Lane Specifications**: Implemented ASAM OpenDRIVE 1.7 Lane definitions (10.1 - 10.6) in `rc_opendrive/include/rc_opendrive/core/RoadLane.h`. This includes `LaneOffset`, `LaneWidth`, `LaneHeight`, `LaneBorder`, and `LaneSection` structures.
- **Top-Level Road Structure**: Created `rc_opendrive/include/rc_opendrive/core/Road.h` to aggregate `PlanView`, `ElevationProfile`, `LateralProfile`, and `Lanes` into a cohesive `Road` object.
- **Road Linkage & Type**: Added `RoadLink` (Chapter 6) and `RoadType` (Chapter 5) definitions within `Road.h` to complete the road network topology requirements.

## [Unreleased] - 2026-03-02 (OpenDRIVE Core Geometry)
- **Review & Extend Geometry**: Reviewed existing horizontal geometry definitions (`Line`, `Arc`, `Spiral`, etc.) in `RoadGeometry.h` and confirmed their correctness against the ASAM OpenDRIVE spec.
- **Implement Vertical/Lateral Profiles**: Added definitions for vertical and lateral road profiles by creating `rc_opendrive/include/rc_opendrive/core/RoadProfile.h`. This new header implements `ElevationProfile` (ASAM 8.3), `Superelevation`, and `Crossfall` (ASAM 8.5) to complete the road reference line geometry.

## [Unreleased] - 2026-03-02 (OpenDRIVE Serialization & Core)
- **Serialization Success**: Achieved complete success with serialization tests covering polymorphism, `std::unique_ptr`, `std::map`, and `std::vector`.
- **JSON Implementation**: Implemented `nlohmann/json` definitions in `JsonSerialization.h` for robust data handling.
- **Build Resolution**: Resolved multiple `rc_opendrive` build hurdles and ensured necessary default constructors are present.
- **Verification**: Verified all changes with a new, integrated test suite, confirming serialization is fully defined and functional.

## [Unreleased] - 2026-03-02 (Lucide SVG Icon System)
- Added `nanosvg` + `nanosvgrast` (header-only) to `3rdparty/nanosvg/` for runtime SVG parsing and RGBA rasterization with no external dependencies.
- Vendored 1950 Lucide SVG icons (`lucide-static` v0.x) into `visualizer/assets/icons/lucide/` as the canonical icon asset source.
- Introduced `RC::SvgTextureCache` (`visualizer/include/.../SvgTextureCache.hpp` + `visualizer/src/ui/SvgTextureCache.cpp`): a singleton that lazily loads and OpenGL-uploads SVG files keyed by `(path, size_px)`, returning `ImTextureID` for direct use with `ImGui::Image` / `ImageButton`.
- Added `LucideIcons.hpp` with 33 named path constants (`LC::Home`, `LC::Settings`, `LC::Building`, `LC::Bot`, etc.) grouped by Navigation, Tools, Status, City, and AI/Dev categories.
- Wired `SvgTextureCache.cpp` and the `3rdparty/nanosvg` include path into both `RogueCityVisualizerHeadless` and `RogueCityVisualizerGui` targets in `visualizer/CMakeLists.txt`.
- Added `RC::SvgTextureCache::Get().Clear()` teardown calls in `main.cpp` (before `ShutdownScreenshotRuntime`) and `main_gui.cpp` (before `ImGui_ImplOpenGL3_Shutdown`) to safely release GL textures before context destruction.

## [Unreleased] - 2026-03-01 (3D Viewport & Editor Architecture)
- **True 3D Viewport**: Modernized renderer backend and `WorldToScreen`/`ScreenToWorld` math utilizing `glm::perspective` and `glm::inverse` to support `pitch` and `is_3d` camera modes.
- **Authoritative Footprints**: Migrated footprint generation into `SiteGenerator` as explicit `Boost.Geometry` buffered polygon insets stored natively on `BuildingSite::outline`.
- **Planar Face Block Topology**: Rewrote `PolygonFinder::fromGraph` applying accurate Half-Edge Data Structure (DCEL) minimal-cycle polygon extraction for guaranteed logical blocks.
- **Building Tool Integration**: Hooked the `BuildingTool.cpp` placeholder logic via newly established footprints and `PolygonUtil::insidePolygon` raycasting to securely bind interactions.
- **Dynamic Scale Policy & Flow Module**: Tied `SP::FixedWorldExtent` to dynamic pixel scaling during texture resolution overrides and added a pedestrian/traffic density settings module to the `Flow` tool in `rc_panel_inspector.cpp`.
## [Unreleased] - 2026-03-01 (Viewport Command Cohesion Pass)
- Reworked the viewport `G` tool surface from an in-canvas child into a dedicated overlay window (`##global_tool_palette_overlay`) with eased width/alpha/slide animation, so the panel now layers cleanly on top of the viewport instead of feeling embedded/clipped.
- Added a visualizer-owned right-click popup (`##viewport_context_actions`) with shared global selection wiring: `Undo/Redo`, select modes, selection target filters, `Move/Rotate/Scale`, snap toggle, contextual topology actions (`Add/Split`, `Merge/Snap`, `Trim`), plus `Delete Selected` and `Clear Selection`.
- Added explicit suppression support for the default viewport right-click command menu in `ProcessViewportCommandTriggers(...)` and enabled it from the visualizer panel so the new contextual menu is the single right-click surface.
- Added viewport delete hotkeys (`Delete`/`Backspace`) to execute the undoable selection delete path and aligned non-axiom transform hotkeys to standard controls (`Q` select, `W` move, `E` rotate, `R` scale) while preserving legacy aliases (`G`, `S`) for continuity.
- Tightened input gating by treating palette hover and active context popup state as overlay-blocking interaction zones, preventing accidental viewport edits while menus are open/hovered.

## [Unreleased] - 2026-03-01 (Visualizer G-Menu Tooling + Persistence)
- Reworked the viewport `G` slideout into clustered visualizer tooling (`Selection`, `Transform`, `Edit/Topology`, `Attribute/View/Utility`) while keeping active runtime wiring for `Rectangle Select`, `Lasso Select`, `Move`, and `Handle Move` through the shared tool dispatcher and viewport interaction pipeline.
- Added direct `G`-tool hotkeys (`1-4`) for fast switching of the four active visualizer tools and updated in-panel guidance to reflect the new shortcut flow.
- Added workspace-preset persistence for `G` runtime state in `rc_ui_root.cpp` by serializing/restoring `viewport_selection_mode`, `viewport_edit_tool`, and `viewport_selection_target`, so saved workspace presets now restore last-used visualizer tool intent.
- **Visualizer Rendering Modernization**: Updated `RenderBuildingSites` to draw oriented, footprint-aware quads using `footprint_area` and `rotation_radians` instead of screen-space markers.
- **Road Layer Visualization**: Updated `RenderRoadNetwork` to visualize grade separation: bridges (`layer_id > 0`) now draw with a dark casing/shadow, and tunnels (`layer_id < 0`) draw with reduced opacity.
- **Terrain Visualization**: Updated `RenderZoneColors` to visualize terracing requirements and `RenderLotBoundaries` to show retaining walls using the new 3D-ready metadata.

## [Unreleased] - 2026-03-01 (Visualizer Selection + Utility Pass)
- Replaced spatial pick/query stubs in `rc_viewport_handler_common.cpp` with active behavior (`TryPickFromSpatialGrid`, `TryQueryRegionFromSpatialGrid`) and routed box/lasso query calls through bounded spatial-cell scanning via world-bounds hints.
- Updated non-axiom lasso behavior so `Shift` now correctly enters add-selection mode at lasso start (with existing `Ctrl` toggle preserved) and kept click/box/lasso on the shared accelerated selection path.
- Replaced `Layer Toggles (Planned)` in the `G` slideout with an active popup utility (all-on/all-off, per-layer visibility flags, and layer-manager dim/through-hidden toggles).
- Replaced `Select By Query (Planned)` in the `G` slideout with an active query popup supporting filterable entity selection (`Kind`, `ID Min/Max`, `District ID`, `User-Created`, `Visible Only`) and apply modes (`Replace`, `Add`, `Toggle`) across road/district/lot/building/water.

## [Unreleased] - 2026-03-01 (UI Code Reduction — Zones 1–4)
- Created `visualizer/src/ui/rc_ui_panel_macros.h` with `RC_PANEL_DRAW_IMPL` macro, collapsing the 17-line `Draw(float dt)` boilerplate in Variant A panels; applied to `rc_panel_validation.cpp`, `rc_panel_system_map.cpp`, and `rc_panel_telemetry.cpp` (~60 LOC removed).
- Rewrote `RcPanelDrawers.cpp` with two internal macros (`RC_DEFINE_INDEX_DRAWER`, `RC_DEFINE_DRAWER`) that collapse 18-line drawer class definitions; replaced 15 of 23 drawers with macro calls (582 → ~290 LOC, ~50% reduction).
- Extracted three shared helpers (`DrawUserFlagBlock<T>`, `DrawGenerationLocked<T>`, `DrawLayerAssignment`) in `rc_property_editor.cpp` to eliminate copy-pasted generation-flag and layer-assignment blocks across `DrawSingleRoad/District/Lot/Building` (~130 LOC removed).
- Extracted `ClearAllSelections` helper in `rc_panel_data_index_traits.h` and applied to all four entity `OnEntitySelected` functions, removing the repeated 4-clear block (~12 LOC removed).

## [Unreleased] - 2026-03-01 (Vision/OCR Pipeline Improvements)
- Replaced generic `num_predict: 64` cap with per-task limits: vision=384 tokens, OCR=256 tokens, allowing complete spatial descriptions and grouped text extraction.
- Replaced generic vision prompt with an RC-aware cockpit prompt naming all 12 cockpit panels by column (Left/Center/Right/Bottom) so `granite3.2-vision` produces structured, panel-referencing output.
- Replaced generic OCR prompt with a panel-grouped extraction prompt so `glm-ocr` returns text organized under known panel headers (AxiomEditor, Inspector, ZoningControl, etc.).
- Added `_parse_vision_regions()`: scans vision model output for 19 known RC panel keywords and populates `VisualEvidence.ui_regions` with `VisualRegion(label="PanelName@Dock")` entries (previously always empty).
- Added `_vision_confidence()`: scores vision output quality by response length and spatial-keyword density (mirrors existing `_ocr_confidence` pattern); emits `vision_low_confidence_output` warning on low-quality responses.
- Changed screenshot list from `screenshot_paths[:1]` to `screenshot_paths` so all captured frames are sent to both vision and OCR models when multiple screenshots are available.

## [Unreleased] - 2026-03-01 (Gemma Suite Expansion)
- Added 12 new test cases to `tests/test_ai.cpp` (17 total) covering all four codebase layers: core district/style-tag/seed vocabulary; generators Pipeline V2 model routing (controller/triage/synth_fast/synth_escalation/embedding/vision/ocr) and scale values; app cross-layer state_model keys and header mode/filter vocabulary; visualizer 12-panel role map, all 5 UiCommand types, and all 5 dock positions.
- Expanded `AI/docs/symbol_index.json` with 29 new entries: full AI layer (UiSnapshot, UiCommand, AiConfig, AiConfigManager, CitySpecClient, AiAssist, AiBridgeRuntime, AiAvailability, ParsedUrl — 9 files completely absent before) and 20 missing visualizer panel files (Inspector, InspectorSidebar, Workspace, DevShell, BuildingControl, LotControl, WaterControl, Tools, SystemMap, Log, Telemetry, AxiomBar, UiSettings, Validation, and all 5 index panels).

## [Unreleased] - 2026-03-01 (Core Audit Behavior Pass)
- Hardened core geometry operations in `PolygonOps` by adding polygon sanitization/validity guards and new boolean operations (`difference`, `union`) plus simplification/validation helpers, and added a dedicated `test_polygon_ops` suite for deterministic area/shape regression coverage.
- Implemented `Tensor2D::add(..., smooth=true)` smoothing behavior for near-opposing tensor blends (instead of ignoring the flag), preserving directional stability in cancellation-heavy blends while keeping weighted-add semantics for aligned fields.
- Upgraded `SimulationPipeline` from placeholder stage stubs to active runtime behavior: finite-coordinate physics validation, selection/hover pruning in agent step, validation-overlay refresh in systems step, and explicit non-deterministic mode stepping semantics.
- Reworked `EditorIntegrity` from no-op checks into executable integrity/spatial reporting with tagged plan-violation output (`[Integrity/Entity]`, `[Integrity/Spatial]`) and idempotent refresh semantics.
- Hardened `StableIDRegistry` persistence by implementing alias-aware reverse lookup and strict deserialize validation (reject malformed/ambiguous records) with new regression checks in `test_stable_id_registry`.
- Added and registered new core tests: `test_core`, `test_tensor2d_smoothing`, `test_polygon_ops`, and `test_editor_integrity`; expanded `test_simulation_pipeline` coverage for non-deterministic mode and runtime validation behavior.
- Added a Clipper2 source fallback in `core/CMakeLists.txt` when a `Clipper2` target is not present, ensuring `PolygonOps` links reliably in standalone core/test builds.

## [Unreleased] - 2026-03-01 (Phase 2 PolygonOps Generator Wiring)
- Wired `PolygonOps` directly into lot subdivision so each candidate lot is polygon-clipped against sanitized block shells (instead of center-point-only acceptance), and lot `boundary/area/centroid` now derive from clipped geometry.
- Added robust hole handling in lot subdivision by sanitizing block hole rings and rejecting hole-overlapping candidates, preventing lot geometry from crossing courtyard/no-build voids in downstream 3D paths.
- Upgraded zoning setback placement to polygon-aware clamping: AABB setback intent is preserved, then clamped to a computed setback region built from lot polygon clips/insets for irregular lot boundaries.
- Fixed `SiteGenerator` behavior so `randomize_sites=false` no longer jitters site positions, and randomization now respects polygon boundaries for non-rectangular lots.
- Expanded regression coverage:
  - `test_zoning_generator`: holed-block integration case validating lot validity and zero overlap with hole geometry.
  - `test_polygon_ops`: deterministic robustness sweep and holed-difference area invariant coverage, with always-on `RC_EXPECT` checks in Release test runs.

## [Unreleased] - 2026-03-01 (Phase 3 Polygon Region Ops + Tool Honor Pass)
- Added first-class `PolygonRegion` (`outer + holes`) support to `PolygonOps` with new region APIs:
  - `ClipPolygons(const Polygon&, const PolygonRegion&)`
  - `DifferencePolygons(const Polygon&, const PolygonRegion&)`
  - `SimplifyRegion(...)`
  - `IsValidRegion(...)`
- Implemented oriented clip-path execution for region operations so hole semantics are preserved as true no-build voids instead of relying on implicit flat-path winding behavior.
- Rewired `LotGenerator` block handling to build/simplify/validate a `PolygonRegion` and clip lot candidates against that region directly, removing duplicated manual hole-overlap logic.
- Updated generator-facing tool/runtime behavior to honor region semantics consistently in zoning subdivision paths, ensuring holed blocks remain hole-safe under production tool flows.
- Expanded `test_polygon_ops` with explicit region-operation invariants and invalid-region checks to lock the new API behavior.

## [Unreleased] - 2026-03-01 (Phase 4 Region Output Topology)
- Added topology-preserving output APIs to `PolygonOps`:
  - `ClipRegions(const PolygonRegion&, const PolygonRegion&)`
  - `DifferenceRegions(const PolygonRegion&, const PolygonRegion&)`
  - `UnionRegions(const std::vector<PolygonRegion>&)`
- Implemented PolyTree-based region extraction so output now preserves explicit `outer + holes` hierarchy instead of flattening to orientation-only path sets.
- Added region flatten bridge for legacy polygon-return APIs, keeping backward compatibility while exposing topology-safe outputs for 3D handoff paths.
- Updated `LotGenerator` clipping selection to consume `ClipRegions(...)` and accept only simple region outers for `LotToken` emission, preventing hole-bearing candidate outputs from being misinterpreted as single-shell lots.
- Expanded `test_polygon_ops` with explicit Phase 4 output-topology invariants for clip/difference/union region operations (including hole-count expectations and net-area checks).

## [Unreleased] - 2026-03-01 (Generator Audit Phases)
- Zoning budget accounting now uses the effective runtime config (including CitySpec overrides) and rescales per-lot allocations when type multipliers would oversubscribe city budget, keeping `totalBudgetUsed` strictly equal to summed lot allocations.
- Wired `ZoningBridge` building-coverage UI controls into zoning-domain densities (`residential/civic=min`, `industrial=mid`, `commercial=max`) so the coverage sliders now drive zoning output.
- Updated `ZoningGenerator` placement to apply deterministic density gating per lot program and clamp building sites to inset lot bounds from `frontSetback`, `sideSetback`, and `rearSetback`.
- Synced placement/population helper paths by stamping `BuildingSite::estimated_cost` from runtime cost config and using shared population-density helpers during resident/worker rollup.
- Added and registered `test_zoning_generator` coverage for budget invariants, zero-budget behavior, CitySpec budget override precedence, density sensitivity, setback enforcement, and CitySpec zoning override enforcement.

## [Unreleased] - 2026-03-01 (Tool Wiring E2E)
- Updated `Render2DCursorHUD` viewport behavior to hide the system cursor while hovering the visualizer viewport and removed the on-reticle `2D_Cursor` label, leaving only the precision 2D reticle.
- Changed non-axiom selection flow so unmodified `click` now resolves on mouse release and `click-and-hold` starts default box selection after drag threshold; box/lasso region finalize now support expected modifiers (`Shift` add, `Ctrl` toggle, plain drag replace).
- Added drag-selection preview color cues in the viewport (`cyan` replace, `green` add, `amber` toggle) to reflect active region-selection mode during box/lasso interaction.
- Restored root-level shared tool-library rendering in `rc_ui_root.cpp` by adding a dedicated `DrawToolLibraryWindows()` pass from `DrawRoot()`, so activated libraries now render as actual windows again (including popout instances).
- Rewired the Axiom library path to use `AxiomEditor::DrawAxiomLibraryContent()` in the shared library window flow, restoring per-axiom rich controls (type icons, terminal features, radial controls, and alternates) in the canonical panel route.
- Centralized axiom default-type synchronization in `DispatchToolAction()` and extended `DispatchContext` with `apply_axiom_default`, making command palette/smart menu/pie executions consistently update placement default type.
- Preserved Ctrl-based “apply to selected axiom” behavior by setting `apply_axiom_default=false` in axiom selection-only dispatch sites, preventing unintended default-type mutation during targeted edits.
- Refactored the `Tools` drawer `TOOL DECK` area into an embedded library host with per-library tabs and in-panel action rendering, so users can work tool libraries in the master panel without a second duplicate tool surface.
- Changed root library-window policy to embedded-by-default and popout-only for detached windows, so separate library windows appear only when explicitly requested via popout.
- Renamed the viewport cursor overlay to `Render2DCursorHUD` and tied it to the live mouse position when hovering the viewport, introducing the `2D_Cursor` precision reticle while preserving center-idle fallback when not hovered.

## [Unreleased] - 2026-02-28 (Phase 4 Updates)
- Added `RogueCityVisualizerHeadless` `--interactive` mode, spawning a resilient, thread-safe REPL console with cyberpunk ANSI styling and a text-driven command queue (`dump_ui`, `hfsm`, `state`, `quit`) for unbuffered AI/shell introspection and control.
- Upgraded the TUI console with a concurrent subprocess engine leveraging `_popen`, allowing native `dev-shell.ps1` tools (`build`, `ai_start`, `doctor`) to execute sequentially within the thread without freezing the main rendering loop.
- Modified `rc_ui_root.cpp` to recline ImGui dock nodes into a distinct 3-column topology mirroring the mockups (Master Panel | Viewport | Inspector).
- Restructured `DrawRoot` constraints restricting the global DockSpace to nest exactly within the pre-defined TitleBar / StatusBar top-bottom bounds.
- Re-wrote `rc_panel_inspector.cpp` replacing its mock Property Editor layout with styled LCARS expandable sections reflecting Tool Domains, Gizmos, and Editor Layers.
- Shifted the HUD compass overlay and bottom-scale ruler in `rc_viewport_overlays.cpp` to accurate `right: 90px` and `bottom/center` coordinates respecting the layout grid.

## [Unreleased] - 2026-02-28 (Phase 3 Updates)
- Added `RC_UI::Components::DrawNeonBoxShadow` utility to emulate CSS `box-shadow` layering for glowing components.
- Integrated `DrawPanelFrame` inside `BeginTokenPanel` natively across all major UI container surfaces, mimicking the CSS `.panel::before` + `.panel-corner` border geometry.
- Suppressed `RenderFlightDeckHUD` module permanently to prevent collisions with the `RenderToolBadgeHUD`.
- *Build Note:* Hard dependency inclusion of `<magic_enum.hpp>` introduced safely to sidestep potential MSVC `enum_name().c_str()` fallout, preserving C2228 regression protection.
- **UI Restoration (Regression Fix)**: Restored the primary viewport drawing call and re-enabled the Tool Library action icons by restoring their orphaned rendering loop. Corrected the Master Panel dock layout to include `Library` and `ToolDeck` nodes and categorized the `AxiomEditor` drawer as hidden to prevent UI tab redundancy.

### Added
- **Mockup HUD Effects (Phase 2)**: Added new screen-space and viewport HUD metrics reflecting the HTML/CSS mockup style.
  - Injected `DrawGlobalScanlines` into `rc_ui_root.cpp` background.
  - Added `RenderCenterCursorHUD` and `RenderToolBadgeHUD` calls in `rc_viewport_overlays.cpp`.
  - Refined `RenderScaleRulerHUD` and `RenderCompassGimbalHUD` styling with neon glows.
  - *Regression Fix Note*: In `RenderToolBadgeHUD`, `active_domain` (enum `ToolDomain`) was incorrectly treated as a string (`.c_str()`), causing C2228. Codex fix uses `magic_enum::enum_name(...)` + empty fallback + bounded `%.*s` formatting.
- **Build-Break Incident RCA Note (2026-02-28)**: Added `AI/collaboration/incident_tooldomain_cstr_buildbreak_2026-02-28.md` documenting that Gemini introduced a `ToolDomain` enum/string misuse in HUD code, Codex applied the compile fix, and shared enum-formatting edge-case prevention guidance for future agent work.
- **Compile-Fix Collaboration Brief (2026-02-28)**: Added `AI/collaboration/codex_buildfix_tooldomain_badge_2026-02-28.md` documenting root cause, visualizer-layer-only scope, and validation artifacts for the ToolDomain HUD badge build fix.
- **Cross-Agent Changelog Enforcement (2026-02-28)**: Added `AI/collaboration/CHANGELOG_MANDATE.md` and propagated explicit changelog-update requirements into collaboration handoff/SOP manifests so all agents in the shared workspace follow the same `CHANGELOG.md` discipline.
- **AI Collaboration SOP + Handoff Artifacts (2026-02-28)**: Added shared coordination documentation in `AI/collaboration/` including a Codex session brief, a collaboration SOP manifest, and a Gemini handoff brief to initiate the next UI implementation phase with explicit validation gates.
- **Architectural Refactor**: Migrated `GeometryPolicy` from the visualizer layer to the app layer (`app/src/Tools/GeometryPolicy.cpp`) to enforce strict layer boundaries. Updated `WaterTool` and `RoadTool` to utilize the dynamic `GeometryPolicy`, removing hardcoded geometric constants.
- **Core Generator Logic**: 
  - Implemented `FrontageProfiler.cpp` to procedurally trace building facades along edge splines using a discrete `TransitionMatrix` (Markov Chain), including adaptive scaling and truncation handling for terminal modules.
  - Implemented `Policies.cpp` enforcing the Road MDP logic (`GridPolicy`, `OrganicPolicy`) based on distance fields and local network density constraints.
- **Geometry Foundation (`PolygonOps.cpp`)**: Fully implemented polygon clipping and insetting utilizing the newly integrated `Clipper2` library. Fixed integer coordinate scaling (`kClipperScale = 1000.0`) is enforced per architectural specifications.
- **Dependency Cleanliness**: Added `Clipper2` directly to the `3rdparty/` directory for full debug visibility and stripped unneeded noise directories (`.git`, `test`) to ensure clean audits.
- **UI Framework & Panels (RC-0.12)**:
  - **Unified Design System**: Centralized all UI tokens (colors, spacings, rounding) into `rc_ui_theme.h` and refactored the theme implementation to enforce Cockpit Doctrine compliance.
  - **Multi-Column Data Panels**: Refactored `RcDataIndexPanel` to support generic multi-column traits. Updated all entity index panels (Roads, Districts, Lots, Buildings) and the River index to use this unified template.
  - **Building Search Overlay**: Implemented a viewport-contextual search overlay for buildings, activated by `Ctrl+F`. Supports real-time filtering and automatic selection sync.
  - **Viewport Integration**: Integrated search overlay rendering and hotkeys directly into the `AxiomEditorPanel` viewport chrome.
- **Workspace & Theme System**:
  - **WorkspaceProfile**: Implemented `WorkspacePersona` enum and `WorkspaceProfile` struct (`visualizer/include/RogueCity/Visualizer/WorkspaceProfile.hpp`).
  - **Themes**: Added `Enterprise`, `Planner`, `Tron`, and `RedlightDistrict` themes. Registered 8 built-in themes in `ThemeManager`.
  - **Presets**: Upgraded `WorkspacePresetStore` to schema 2 to support theme persistence alongside layout INI.
  - **UI**: Added `rc_panel_workspace` for persona switching and theme management.
- **AI Integration**:
  - **Feature Flag**: Added `RC_FEATURE_AI_BRIDGE` to `AI/CMakeLists.txt` to gate AI features.
  - **Gating**: Wrapped `UiAgentClient` and `UiDesignAssistant` calls with feature flag checks.
- **UI/UX Redesign (In Progress)**:
  - **Mockup**: Adopted `RC_UI_Mockup.html` as the design specification.
  - **Strategy**: Defined mapping from HTML/CSS tokens to ImGui/C++:
    - CSS Custom Properties -> `DesignTokens` namespace (`rc_ui_tokens.h`)
    - `div.panel` -> `UI::BeginPanel()` (`DesignSystem.cpp`)
    - Flexbox/Grid -> `ImGui::SameLine()`, `ImGui::BeginTable()`
    - CSS Classes -> ImGui Style Pushes / Helper functions

### Fixed
- **Visualizer Debug Build Fix (2026-02-28)**: Fixed `rc_viewport_overlays.cpp` HUD badge formatting to treat `tool_runtime.active_domain` as `ToolDomain` (enum) instead of a string, removing invalid `.c_str()` usage and restoring `RogueCityVisualizerGui` Debug compilation.
- **Gemma-First Pipeline V2 Staging (2026-02-28)**:
  - Added Toolserver pipeline endpoints: `POST /pipeline/query`, `POST /pipeline/eval`, and `POST /pipeline/index/build` with deterministic triage/retrieval/synthesis/verification flow.
  - Added pipeline schemas (`TriagedQueryPlan`, `ToolCall`, `VisualEvidence`, `EvidenceBundle`, `PipelineAnswer`) and audit-mode strict verification behavior (hard-fail in audit mode only).
  - Added feature-flag gating via `RC_AI_PIPELINE_V2` / `RC_AI_AUDIT_STRICT` with config fallback from `AI/ai_config.json`.
  - Added Gemma-first model role routing defaults for pipeline v2 (`functiongemma`, `codegemma`, `gemma3:4b`, `gemma3:12b`, `embeddinggemma`) while keeping legacy endpoints unchanged.
  - Added JSONL-based semantic index scaffolding under `tools/.ai-index/` (chunking, embed build, flat cosine retrieval).
  - Updated `rc-ai-query`/`rc-ai-eval` to call Toolserver pipeline endpoints when `RC_AI_PIPELINE_V2=on`, with fallback to legacy direct Ollama flow when off.
  - Added new AI config keys (`controller_model`, `triage_model`, `synth_fast_model`, `synth_escalation_model`, `embedding_model`, `vision_model`, `ocr_model`, `pipeline_v2_enabled`, `audit_strict_enabled`, `embedding_dimensions`) and C++ parser support.
  - Validation status: Python and PowerShell syntax checks pass for updated files; live HTTP endpoint smoke is pending in a Windows shell with `fastapi`/`uvicorn` installed.
  - Performance profile update: set `synth_escalation_model` to `gemma3:4b` in `AI/ai_config.json` to avoid automatic 12B latency on current hardware.
  - Bridge hardening update: `Start_Ai_Bridge_Fixed.ps1` now verifies pipeline-v2 endpoint availability when reusing an existing listener and warns/fails fast on legacy bridge processes that lack `/pipeline/query`.
  - Dev-shell fix: `rc-ai-query`/`rc-ai-eval` now only apply `-Model` overrides when explicitly passed (v2 no longer silently forces `deepseek-coder-v2:16b`).
  - Dev-shell UX hardening: improved pipeline error messaging for 404/legacy bridge scenarios with explicit restart command guidance.
  - Visualizer AI Console enhancement: added a safe terminal-like command launcher (allowlisted dev-shell commands) for runtime/build control from inside the app panel.
  - Dev-shell model defaults migrated to Gemma-first stack (`gemma3:4b` default for smoke/query/eval); `rc-ai-setup` now pulls Gemma-first models instead of DeepSeek.
  - Applied pre-tuned Gemma generation defaults for pipeline/dev-shell (`temperature=0.10`, `top_p=0.85`, `num_predict=256`) based on quick latency/quality sweep.
  - Full 6-case tuning sweep completed: near-fast profiles tied at `2/6` pass with stable required-path recall failures; `-IncludeRepoIndex` currently degrades to `0/6` and remains non-default.
  - Implemented deterministic required-path enforcement in pipeline post-processing; full 6-case eval now passes `6/6` with balanced profile (`temperature=0.10`, `top_p=0.85`, `num_predict=256`).
  - Added Program Perception endpoints in toolserver: `POST /perception/observe` and `POST /perception/audit` with capture, mockup contract checks, code-candidate mapping, and hybrid vision/OCR hooks (degrading safely when screenshots/models are unavailable).
  - Added perception shell commands: `rc-perceive-ui` and `rc-perception-audit`.
  - Added custom Python MCP server scaffold at `tools/mcp-server/roguecity-mcp/` with allowlisted tools for bridge/build/snapshot/perception/pipeline/report orchestration.
  - Extended AI Console allowlisted command launcher with `rc-perceive-ui` and `rc-perception-audit`.
  - Bridge startup endpoint listing now advertises perception endpoints when pipeline v2 is available.
  - Runtime compatibility hardening: migrated remaining legacy model defaults from DeepSeek to Gemma-first (`gemma3:4b` / `codegemma:2b`) across `AI/ai_config.json`, C++ AI config fallbacks, bridge startup mode probe, and toolserver legacy request defaults.
  - Bridge startup now initializes compatibility env defaults (`RC_AI_PIPELINE_V2=on`, model-role envs, bridge base URL) when unset for deterministic shell/runtime behavior.
  - Dev shell “hypercharge” commands added: `rc-ai-harden`, `rc-mcp-setup`, and deterministic `rc-mcp-smoke` readiness JSON for bridge + endpoint + MCP dependency checks.
  - Env doctor expanded with AI stack checks (ai_config alignment, bridge script presence, MCP server presence, Python AI module inventory, and Ollama model stack completeness).
  - RogueCity MCP server upgraded with: `--self-test`, strict build target/preset allowlists, repo-path sanitization for path-bearing tools, `rc_env_validate`, and composite `rc_observe_and_map`.
  - Added dual-runtime bridge strategy for broad compatibility:
    - Cross-host bind mode in `Start_Ai_Bridge_Fixed.ps1` via `-BindAll` / `-BindHost`, with WSL/Linux access hints in startup output.
    - Same-environment WSL bridge scripts: `tools/start_ai_bridge_wsl.sh` and `tools/stop_ai_bridge_wsl.sh`.
    - Dev-shell wrappers: `rc-ai-start-wsl` and `rc-ai-stop-wsl`.
    - MCP tools for same-runtime control: `rc_bridge_start_local` and `rc_bridge_stop_local`.
  - Applied Ollama tuning defaults in hardened env setup and bridge startup (`OLLAMA_FLASH_ATTENTION=1`, `OLLAMA_KV_CACHE_TYPE=f16`) when unset.
  - Cross-runtime Ollama compatibility hardening:
    - Toolserver Ollama calls now resolve candidate base URLs from `OLLAMA_BASE_URL`/`OLLAMA_HOST` with WSL-friendly fallbacks (`host.docker.internal`, `/etc/resolv.conf` nameserver) instead of hardcoded localhost.
    - `start_ai_bridge_wsl.sh` now supports `--ollama-base-url` and auto-selects a reachable Ollama base when unset.
    - Dev-shell Ollama-aware commands (`rc-ai-harden`, `rc-ai-smoke`, `rc-ai-query`) now use resolved Ollama base URLs.
    - MCP self-test/health probing now checks candidate Ollama endpoints and reports tried URLs for faster diagnostics.
    - Added short per-candidate connect timeouts for Toolserver Ollama fallback calls to prevent long stalls on blackhole hosts.
  - **PowerShell-Primary Stabilization Pass (2026-02-28)**:
    - Pipeline verifier now enforces answer quality (`answer_quality_ok`, `answer_quality_reason`) with retry/escalation; audit mode hard-fails on low-quality/empty answers and normal mode returns structured failure instead of silent empties.
    - Dev-shell strict query/eval paths now treat `answer_quality_ok=false` as a failure signal and expose answer-quality regression counts in eval summaries.
    - Added explicit runtime `Titlebar` and `Status Bar` introspection surfaces in `rc_ui_root.cpp` with live status counters (validation/log/dirty), closing missing-section contract gaps.
    - Added headless runtime screenshot export support via `--export-ui-screenshot <path>` in `RogueCityVisualizerHeadless` with hidden OpenGL render path and PNG framebuffer capture.
    - Updated headless CMake wiring to enable screenshot runtime when GLFW/OpenGL are available (including ImGui OpenGL backend and gl3w linkage).
    - Perception full mode now requires runtime screenshot artifacts for OCR/vision gate runs and no longer relies on seeded fixture screenshots in smoke flow.
    - Perception multimodal lane now runs vision/OCR concurrently with trimmed prompt/`num_predict` payloads for lower full-mode latency.
    - MCP self-test semantics now report `ok_core`, `ok_full`, and `runtime_recommendation`; top-level readiness maps to `ok_core` for PowerShell-primary operation.
    - Dev-shell `rc-mcp-smoke` now surfaces runtime recommendation and fallback commands; `rc-ai-start-wsl` warns and prints explicit PowerShell fallback commands on degraded WSL/Ollama routing.
    - Added commit-gate full smoke command path (`rc-full-smoke`) and upgraded `tools/.run/rc_full_smoke.ps1` to enforce gate checks (pipeline quality, required UI sections, p95 latency, runtime screenshot + OCR/vision evidence).
- **Dev Shell Headless + Local AI Command Surface (2026-02-28)**:
  - Added headless helpers in `tools/dev-shell.ps1`: `rc-bld-headless`, `rc-run-headless`, and `rc-smoke-headless`.
  - Added `rc-ai-query` to query local Ollama with workspace context from `.gemini/GEMINI.md` and `.agents/Agents.md`.
  - Updated `rc-help` command listing to document new headless and AI query workflows.
  - Added `--export-ui-snapshot` CLI support in `RogueCityVisualizerHeadless` to emit runtime UI introspection JSON for layout auditing.
  - Added `rc-ai-stop-admin [-Port]` to invoke elevated bridge stop when normal shell privileges cannot terminate inherited listener processes.
  - Hardened `rc-ai-query` with strict JSON contract, path existence validation, retry-on-invalid output, and deterministic sampling controls (`temperature`/`top_p`).
  - Added `rc-ai-eval` plus benchmark cases (`tools/ai_eval_cases.json`) to measure local-model grounding/compliance instead of ad-hoc prompts.
  - Added context file manifest injection to improve path-grounded answers from local model.
  - Added required-path enforcement (`RequiredPaths`) and correction retries so local AI can be pushed to include mandatory repo file references.
  - Added optional repo/search context injection (`-IncludeRepoIndex`, `-SearchPattern`) so local AI can ground answers using live file index/search evidence from the workspace.
- **AI Bridge Script Non-Interactive Hardening (2026-02-28)**:
  - Added `-NonInteractive` support to `tools/Start_Ai_Bridge_Fixed.ps1` and `tools/Stop_Ai_Bridge_Fixed.ps1` so bridge lifecycle can be automated from shell tooling and CI-like smoke flows.
  - Fixed PowerShell automatic variable collision in stop script by replacing local `$pid` usage with explicit target PID variables.
  - Added port-check fallback logic for environments where `netstat` is unavailable, using `Get-NetTCPConnection`.
  - Hardened stop behavior with explicit Windows `taskkill` fallback invocation path and clearer diagnostics when listener PID metadata is inaccessible.
  - Standardized restart semantics: `rc-ai-start` now force-recycles listener state; if termination is blocked and a healthy process remains, live-mode start now fails explicitly instead of silently reusing an unknown-mode bridge.
  - Added bridge health metadata (`mock`, `pid`) and local `/admin/shutdown` endpoint for graceful teardown flows.
  - Removed default `uvicorn --reload` from bridge startup to reduce orphan watcher edge cases during automation.
  - Added `-Port` support to `rc-ai-start`/`rc-ai-stop`/`rc-ai-restart` and bridge scripts for deterministic fallback when default listener ownership blocks termination.
  - Hardened `rc-ai-query` request serialization to UTF-8 JSON bytes and sanitized control characters from embedded context files.
- **AI Config DeepSeek Defaults + Fixed Script Paths (2026-02-28)**:
  - Updated AI config fallbacks (`AI/config/AiConfig.h/.cpp`) to use fixed bridge script names and standardized default models for code assistant/naming to `deepseek-coder-v2:16b`.
- **Build Warning Cleanup (2026-02-27)**:
  - Removed an unused `GlobalState` local in `visualizer/src/ui/panels/rc_panel_road_editor.cpp`.
  - Removed an unreferenced static helper `ToolLibraryPopoutWindowName(...)` in `visualizer/src/ui/rc_ui_root.cpp`.
  - Removed unused HUD color locals in `visualizer/src/ui/viewport/rc_viewport_overlays.cpp`.
  - Result: prior `C4189`/`C4505` warnings in the GUI target no longer reproduce.
- **Dev Shell Python Tooling Hardening (2026-02-28)**:
  - Refactored `Invoke-RCPythonTool` in `tools/dev-shell.ps1` to stop depending on `cmd` and instead resolve/execute `py`, `python`, or `python3` directly with safe argument forwarding.
  - Added launcher/path fallbacks and improved terminal error messaging so `rc-doctor`, `rc-problems`, and related `rc-*` Python helpers fail with actionable diagnostics instead of command-resolution crashes.
  - Normalized launcher success handling for environments where native process calls can leave `$LASTEXITCODE` unset despite successful execution.
  - Added de-duplicated launcher attempts, enforced execution from `$env:RC_ROOT`, and included per-attempt exit diagnostics to make cross-shell failures deterministic and debuggable.
- **Env Doctor + Compile Commands Workflow (2026-02-28)**:
  - Updated `tools/env_doctor.py` to treat Visual Studio generator workflows as valid when `build_vs/compile_commands.json` is absent, while still preferring and validating Ninja-style compile databases when available.
  - Generated `build_ninja/compile_commands.json` for editor tooling and verified `env_doctor` now reports `PASS` for compile commands in this workspace.
- **Toolchain + AI + Tooling Validation Sweep (2026-02-28)**:
  - Fixed `env_doctor` toolchain probing for `VSCMD_VER` in WSL-hosted cmd bootstrap flow and verified full doctor status reached `PASS=6 WARN=0 FAIL=0`.
  - Completed a full visualizer build + AI bridge mock connectivity smoke (`/health`, `/ui_agent`, `/city_spec`) and a practical `rc-*` tool suite run with all tested commands passing.
- **AI Base Model Standardization (2026-02-28)**:
  - Updated base defaults from `qwen2.5:latest` to `deepseek-coder-v2:16b` across shell smoke tooling, Python toolserver request models, and C++ AI config fallbacks.
  - Verified non-mock live AI smoke executes on `deepseek-coder-v2:16b` with healthy `/health`, valid `city_spec` response, and no transport/API errors.

### Done
- Implemented viewport render spatial indexing in `GlobalState` with CSR layers (`roads`, `districts`, `lots`, `water`, `buildings`) and per-entity ID-to-handle maps.
- Extended `fva::Container` API (`const operator[]`, `isValidIndex`, `indexCount`) to support safe render-time dereference from `const GlobalState`.
- Updated `ViewportIndexBuilder::Build` to rebuild both legacy `viewport_index` and new render spatial index/ID maps in one deterministic pass.
- Wired derived-index lifecycle through existing flows (`CityOutputApplier` and axiom editor viewport guard) without introducing manager classes.
- Added viewport LOD policy and active-domain visibility override in overlay/panel rendering paths.
- Refactored major overlay loops to visible-cell traversal with dedupe passes for roads/districts/lots/water/buildings.
- Centralized base road rendering into `rc_viewport_overlays.cpp` (`RenderRoadNetwork`) so core geometry and overlay rendering share one viewport-local pipeline.
- Added reusable scratch buffers in overlays to reduce per-frame allocations on static geometry draw paths.

### Need To Do
- Add missing tests `tests/test_viewport_spatial_grid.cpp` and `tests/test_viewport_lod_policy.cpp`, then register both in `CMakeLists.txt` with `add_test`.
- Run and document telemetry/perf acceptance at metro scale, including `< 2 ms` zoomed-out target on baseline machine profile.
- Decide and document whether minimap LOD policy remains intentionally separate or is unified with viewport LOD policy.
- Complete pass-3 research items (grid occupancy tuning, pathological geometry behavior, and GPU-forward data-layout notes).

### Verified
- CMake reconfiguration succeeded with `-DCMAKE_TOOLCHAIN_FILE=C:/Users/teamc/vcpkg/scripts/buildsystems/vcpkg.cmake`, `-DVCPKG_TARGET_TRIPLET=x64-windows`, and `-DROGUECITY_BUILD_VISUALIZER=ON`.
- Build succeeded for visualizer runtime target `RogueCityVisualizerHeadless`.
- Smoke run succeeded (exit code `0`) for `./bin/RogueCityVisualizerHeadless.exe --help` (under timeout).
- Full project build completed after integration changes with no new blocking compile/link errors in modified viewport and editor files.
- Visualizer build passed on 2026-02-27 via:
  - `"/mnt/c/Program Files/CMake/bin/cmake.exe" --build build_vs --config Release --target RogueCityVisualizerGui --parallel 4`

## [0.11.0] - 2026-02-24

### Added
- **Foundational Mandates (`.gemini/GEMINI.md`)**: Established core architectural and engineering standards, including layer ownership and mathematical reference tables for AESP and Grid Quality.
- **Urban Analytics Core (`GridMetrics.hpp`)**: Introduced `GridQualityReport` and `RoadStroke` data structures to the core layer for semantic road network analysis.
- **Grid Quality Engine (`GridAnalytics.cpp`)**: Developed a high-performance module to calculate Straightness ($\varsigma$), Orientation Order ($\Phi$), and 4-Way Proportion ($I$) through stroke-based extraction.
- **"Urban Hell" Diagnostics**: Implemented hardening checks for disconnected network islands, dead-end sprawl (Gamma Index), and micro-segment geometric health.
- **Spline Tool Architecture (`SplineManipulator.cpp`)**: Centralized interactive spline logic into a reusable manipulator in the `app` layer for consistent road and river editing.
- **Hydrated Viewport Tools**: Replaced `RoadTool` and `WaterTool` stubs with functional implementations that handle vertex dragging, pen-tool addition, and proportional editing.
- **Telemetry UI Integration**: Enhanced the `Analytics` panel with real-time visual feedback for grid quality metrics and structural integrity warnings.

### Build & UI Architecture Hardening
- **Centralized Event System (`Infomatrix`)**: Created a decoupled event logging and telemetry backbone in the `core` layer to separate generator data from UI sinks.
- **Type-Driven UI Layout**: Implemented a declarative panel schema in `rc_ui_root.cpp` that drives docking and visibility via `PanelType` routing, replacing scattered imperative calls.
- **Button-to-Dock Popouts**: Introduced `ButtonDockedPanel` functionality, allowing UI components to toggle between docked states and floating windows with Shift-click overrides.
- **Live Validation Panel**: Added a dedicated panel for real-time generator rule violations and constraint monitoring, powered by the new `Infomatrix` stream.
- **Build System Hardening**: Fixed `vcpkg` toolchain integration, resolved cross-layer namespace ambiguities (`Urban` vs `Roads`), and corrected multiple header/source linkage issues.
- **Core Utility Consolidation**: Centralized entity lookup helpers into `EditorUtils` to resolve unresolved external symbols and maintain strict layer boundaries.

### Changed
- **Pipeline Integration**: Updated `CityGenerator` to compute and cache grid analytics at the end of the road tracing stage for zero-latency UI updates.
- **Global State Wiring**: Refactored `CityOutputApplier` and `GlobalState` to persist and broadcast urban analytics across the application.
- **Build System**: Registered the new Scoring and Analytics modules in the `RogueCityGenerators` library via `CMakeLists.txt`.

## [0.10.0] - 2026-02-19

### Added
- Determinism hashing and baseline validation APIs:
  - `RogueCity/Core/Validation/DeterminismHash.hpp`
  - `ComputeDeterminismHash`, `SaveBaselineHash`, `ValidateAgainstBaseline`
- Fixed-step simulation pipeline API:
  - `RogueCity/Core/Simulation/SimulationPipeline.hpp`
- Stable viewport identity API:
  - `RogueCity/Core/Editor/StableIDRegistry.hpp`
- Incremental/cancelable generation APIs:
  - `RogueCity/Generators/Pipeline/GenerationStage.hpp`
  - `RogueCity/Generators/Pipeline/GenerationContext.hpp`
- Profile-driven AESP scoring API:
  - `RogueCity/Generators/Scoring/ScoringProfile.hpp`
- CMake package export for external consumers:
  - `RogueCitiesConfig.cmake`
  - `RogueCitiesTargets.cmake`

### Changed
- Generator pipeline now supports staged incremental regeneration and cancellation context propagation.
- Performance checks for generation latency and stage behavior are integrated into the test suite.
- Install rules now include generator headers and libraries for package consumers.

### Fixed
- Removed legacy, unreferenced compatibility include headers under:
  - `generators/include/Pipeline/`
  - `generators/include/Roads/`
  - `generators/include/Tensors/`
  - `generators/include/Districts/`
- Addressed test warning regressions in:
  - `tests/test_core.cpp`
  - `tests/test_viewport.cpp`

### Verification Highlights
- Clean build and full test suite pass (`26/26`).
- Determinism baseline hash regenerated from a clean state and validated.
- External package consumption validated via `find_package(RogueCities CONFIG REQUIRED)`.
