Plan: UI/UX Enforcement Refactor (2-Pass Integration)
Your codebase has 22 working panels with scattered Draw() calls, a solid docking infrastructure, but no centralized panel routing or event system. The plan document provides comprehensive diffs that migrate to a type-safe, schema-driven architecture.

TL;DR
Pass 1 builds the core infrastructure: PanelType routing, Infomatrix event system, and replaces 15+ scattered calls with a layout-driven loop. Pass 2 completes the integration: ButtonDockedPanel for dock/float toggling, Indices tab consolidation, AI telemetry wiring, and optional UIUserSchema engine for user-defined layouts.

Each pass is complete, compilable, and testable. Code blocks reference exact line ranges from your plan document (Ui_UX_Enforcement Refactor.md).

Each pass covers and utilizes the code found in @Ui_UX_Enforcement Refactor.md, with direct references to line numbers for easy implamentation. The plan also includes risk mitigation strategies for potential issues like typos, signature mismatches, and build system changes.


Pass 1: Core Infrastructure (Schema + Infomatrix)
✅ Must‑do (5)
Must: keep PanelType ↔ PanelLayout strictly read‑only in DrawRoot

Only DrawPanelByType is allowed to route to Panels::XXX::Draw or instance.Render() based on PanelType; no additional scattered Draw() calls are allowed anywhere outside the layout‑schema loop.

Why: This is the whole point of the schema; if you sneak one more Panels::Validator::Draw(dt) in DrawRoot, you’re already breaking the contract.

Must: all UI‑only state (e.g., s_log_lines, s_prev, etc.) moves out of panels into Infomatrix or GlobalState

Log::Draw must not own its own deque<string>; it must be a pure consumer of GS.infomatrix.events(). Other panels that currently log to local buffers should be refactored similarly as you touch them.

Why: Infomatrix is the canonical source of truth; UI should be transient.

Must: no magical dependencies on panel order or names in DrawPanelByType

switch (type) is allowed, but no strcmp(label, "Log") / strstr(window_name, "Indices") hacks. If you need to know behavior, derive it from PanelType or PanelLayout.is_index_like, not strings.

Why: This keeps the schema engine clean and testable.

Must: all UI‑only ImGui IDs avoid collisions by using PushID / suffixes in loops

Every for‑loop that creates identical‑looking buttons must have either PushID(i) / PushID(ptr) or ##index‑style suffix.

Why: ImGui’s ID‑stack rules are not optional; skipping them creates non‑deterministic widget state under multiple panels.

Must: keep Infomatrix as a ring‑buffer view with fixed capacity (kMaxEvents = 220)

Infomatrix::pushEvent() must not grow indefinitely; indexing must be mathematically correct (no offset OOB, no unchecked m_event_head wrap). Initialize m_events.reserve(220);.

Why: This is performance‑sensitive runtime telemetry; OOB heap growth slows iteration and can hide bugs.

🚫 Never‑do (5)
Never: expose InfomatrixEventView::data / offset from ImGui UI threads without guarantees

events() returns a view, but UI panels must not store pointers into data across frames or mutate Infomatrix internals. Only read‑once‑per‑frame iteration is allowed.

Why: Infomatrix may be updated from background jobs or async systems; holding raw pointers is a dangling‑reference trap.

Never: let any panel push directly to s_log_lines or its own deque after Infomatrix lands

Once gs.infomatrix.pushEvent(...) exists, all panels (including AI panels) must funnel messages through it. If you see push_back in Log::Draw, delete it.

Why: Otherwise you have two sources of truth for the same data.

Never: implement layout‑level behavior inside individual panel .Draw calls

A district‑index panel should not contain QueueDockWindow(...) or docking logic; that lives in DrawPanelByType / ButtonDockedPanel / UISchemaEngine.

Why: This breaks the separation of concerns; layout and docking become scattered and untestable.

Never: change calling signature of DrawPanelByType every time you add a new panel type

Keep DrawPanelByType(PanelType, dt, window_name) fixed and generic; new panels just add a case in the switch.

Why: This keeps the schema engine composable and hides complexity from users.

Never: ship ImGui FAQ violations (widget ID collisions, ClipRect misusage, input‑routing bugs)

Specifically:

No duplicate‑label button IDs without ##.

rc_viewport_renderer scissor must use (x1,y1,x2,y2) consistently, not (x,y,w,h).

io.WantCaptureMouse checks must come after ImGui backend callbacks.

Why: These are the classic “mystery UI bug” sources; fixing them once and guarding against them is part of enforcing UI/UX quality.

Pass 1: Core Infrastructure (Schema + Infomatrix)
Goal: Eliminate scattered panel calls, add centralized routing, introduce event-driven logging

Step 1.1: Add PanelType Enum + DrawPanelByType to rc_ui_root.h
Code: Plan doc L66-L117 (2 fences)

Actions:

Add PanelType enum (Telemetry, AxiomEditor, AxiomBar, Zoning, etc.) - 19 types total
Add PanelLayout struct (type, window_name, dock_area, is_index_like)
Add ButtonDockedPanel struct (type, window_name, dock_area, m_open, m_docked)
Declare DrawPanelByType(PanelType, dt, window_name) and DrawButtonDockedPanel(panel, dt)
Why: Decouples layout from panel implementation; enables type-safe routing

Risk: std::string_view_view typo exists in plan (line L91-L93) - change to std::string_view

Step 1.2: Implement DrawPanelByType() in rc_ui_root.cpp
Code: Plan doc L127-L256 (switch dispatcher)

Actions:

Add ShouldPanelBeVisible(PanelType) filter function
Implement DrawPanelByType() with switch statement routing to 19 panel Draw() calls
Special case: PanelType::Indices now draws all 5 index tabs in one panel (District/Road/Lot/River/Building)
Each case calls existing Panels::XXX::Draw(dt) or s_*_instance.Render()
Why: Centralizes all panel dispatching; makes layout changes trivial

Current: rc_ui_root.cpp lines 813-837 has 15+ scattered calls - all replaced by this

Step 1.3: Replace Scattered Calls with Layout Schema Loop in DrawRoot()
Code: Plan doc L362-L449 (layout schema + loop)

Actions:

Add static std::vector<PanelLayout> s_layout_schema in DrawRoot()
Initialize schema with 15+ entries mapping PanelType → window_name → dock_area
Replace all existing Panels::*::Draw(dt) calls with:

for (const PanelLayout& layout : s_layout_schema) {    DrawPanelByType(layout.type, dt, layout.window_name);}
Comment out old calls (for rollback safety)
Why: Single source of truth for layout; workspace presets can later mutate schema

Verification: Run app, confirm all 22 panels still render in correct dock areas

Step 1.4: Add Infomatrix Event System
Code: Plan doc L788-L855 (3 code fences)

Actions:

New file: core/include/RogueCity/Core/Infomatrix.hpp
InfomatrixEvent struct (Category: Runtime/Validation/Dirty/Telemetry, msg, time)
InfomatrixEventView iterator (ring buffer view, max 220 events)
Infomatrix class with pushEvent() and events() methods
New file: core/src/Core/Infomatrix.cpp
Implement ring-buffer logic in pushEvent()
Update: GlobalState.hpp
Add Infomatrix infomatrix; member to GlobalState
Why: Centralizes all event logging; UI panels become consumers, not producers

Risk: EventStreamView in plan has private fields but code assigns to them (L818-819) - make fields public or add setters

Step 1.5: Rewire Log Panel to Consume Infomatrix Events
Code: Plan doc L862-L1189 (2 large fences)

Actions:

Update: rc_panel_log.cpp
Remove static s_log_lines deque
Keep CaptureRuntimeEvents() but change all PushLog(msg) calls to:

gs.infomatrix.pushEvent(InfomatrixEvent::Category::Runtime, msg);
In Draw(), replace log rendering loop to iterate gs.infomatrix.events()
Color events by category (Runtime=Cyan, Validation=Red/Green, Dirty=Amber)
Why: Proves Infomatrix works; establishes pattern for other panels

Verification: Run app, open Log panel, trigger road generation → events appear with color coding

Step 1.6: Add ImGui FAQ Compliance Fixes (ID Stack Audit)
Code: Plan doc L14-L47 (checklist)

Actions:

Audit loops in all panels for missing ImGui::PushID(i) / ##suffix usage
Search pattern: for.*ImGui::(Button|Checkbox) without PushID
Fix: Wrap loop bodies with PushID(i) / PopID() or add ## suffixes
Check input routing in main.cpp:
Ensure ImGui backend callbacks fire before io.WantCaptureMouse checks
Verify ClipRect in visualizer/src/ui/rc_viewport_renderer.cpp:
Confirm scissor uses (x1,y1,x2,y2) not (x,y,w,h) semantics
Enable navigation in init:

io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
Why: Prevents widget ID collisions, input routing bugs, clipping artifacts

Verification: Run with multiple index panels open, verify no "same ID" warnings in ImGui metrics

Pass 1 Deliverables
✅ PanelType enum + DrawPanelByType dispatcher
✅ Infomatrix event system in GlobalState
✅ Log panel consumes events (proof of concept)
✅ Layout schema replaces 15+ scattered calls
✅ ImGui FAQ compliance (ID stack, input routing)
✅ Fully compilable and testable

Hand-off Question for Pass 2: "Should ButtonDockedPanel support Shift-click to toggle docked/floating, or prefer right-click context menu?"

Hand-off Answer for Pass 2: "should ButtonDockedPanel support Shift-click to toggle docked/floating, or prefer right-click context menu?" → Implement right-click menu with a toggling for simplicity and discoverability; Shift-click menu can be added later when we work on the keyboard shortcuts.


Pass 2: Advanced Features (Indices, AI Integration, Schema)
Goal: Consolidate index tabs, wire AI telemetry, add Telemetry/Validation panels, sketch UIUserSchema engine

Step 2.1: Extract Indices Tab Group to Separate Component
Code: Plan doc L526-L635, L645-L771 (3 fences)

Actions:

New file: visualizer/src/ui/rc_ui_indices_views.h
IndicesTabs struct (bool district/road/lot/river/building flags)
DrawIndicesPanel(IndicesTabs&, dt) declaration
New file: visualizer/src/ui/rc_ui_indices_views.cpp
Implement DrawIndicesPanel() with ImGui::BeginTabBar("IndicesTabs")
Conditionally render tabs based on IndicesTabs flags
Register widgets with UiIntrospector for AI visibility
Update: rc_ui_root.cpp DrawPanelByType():
Replace PanelType::Indices case to call DrawIndicesPanel(s_tabs, dt) instead of inline tab bar
Why: Enables programmatic tab visibility toggling; keeps schema clean

Verification: Add debug UI with checkboxes for s_tabs.district etc., confirm tabs toggle

Step 2.2: Implement ButtonDockedPanel (Dock/Float Toggling)
Code: Plan doc L266-L352, L459-L508 (2 fences)

Actions:

Update: rc_ui_root.cpp
Implement DrawButtonDockedPanel(ButtonDockedPanel& panel, dt):
Button click toggles panel.m_open
Shift-click toggles panel.m_docked flag
If docked: render via QueueDockWindow() in target dock area
If floating: render with ImGuiWindowFlags_NoDocking, positioned at mouse
Add example button for Log panel as toolbar test (L500-L508)
Why: Fulfills "embed a dock into a button" requirement; enables dynamic layout control

Verification: Shift-click button → Log panel pops out as floating, Shift-click again → docks back to Bottom

Step 2.3: Wire AI Panels to Infomatrix Telemetry
Code: Plan doc L1587-L1691 (3 fences)

Actions:

Update: rc_panel_ui_agent.h
Add methods: pushTelemetry(const std::string& msg) and pushLog(const std::string& msg)
Update: rc_panel_ui_agent.cpp
Implement push methods: call GetGlobalState().infomatrix.pushEvent() with [AGENT] prefix
In UiAgentPanel::Render(), sprinkle telemetry calls:
When user requests plan suggestion
When agent processes intent
When state transitions (e.g., WaitingForUser)
Repeat for: rc_panel_ai_console.cpp and rc_panel_city_spec.cpp
Why: AI panels now emit structured events visible in Log/Telemetry; centralizes monitoring

Verification: Click "Request Suggestion" in UI Agent → Log panel shows [AGENT] requested_plan_suggestion

Step 2.4: Add Telemetry and Validation Panels
Code: Plan doc L1364-L1533 (3 fences for both panels)

Actions:

New files:
visualizer/src/ui/panels/rc_panel_telemetry.{h,cpp}
visualizer/src/ui/panels/rc_panel_validation.{h,cpp}
Implement Telemetry::Draw(dt):
Filter Infomatrix.events() to Category::Telemetry only
Render in scrolling child window
Implement Validation::Draw(dt):
Filter to Category::Validation events
Color-code: approved=Green, rejected=Red
Update: rc_ui_root.cpp
Add PanelType::Telemetry and PanelType::Validation to enum
Add cases to DrawPanelByType() switch
Add layout entries to s_layout_schema: Telemetry → Right, Validation → Bottom
Why: Dedicates panels for real-time monitoring; Log panel no longer cluttered with all event types

Verification: Open Telemetry panel → only AI agent events appear; Validation panel → only approval/rejection events

Step 2.5: (Optional) Sketch UIUserSchema Engine
Code: Plan doc L1725-L2071 (4 large fences)

Actions:

New files:
core/include/RogueCity/Core/UIUserSchema.hpp (schema types)
core/include/RogueCity/Core/UIUserSchemaEngine.hpp (runtime resolver)
core/src/Core/UIUserSchemaEngine.cpp (implementation)
Define schema types:
DockArea enum, DockRule struct, ButtonBehavior enum
UIButton and UIWindowDef structs
UISchema and UISchemaBindings containers
Implement UISchemaEngine:
renderButton(button_id, dt) - handles TogglePanel/OpenFloating/SendMessage behaviors
renderWindow(window_id, dt) - draws window with child buttons
pushEventFromButton() - emits Infomatrix events
Update: rc_ui_root.cpp
Add SetUISchemaEngine(UISchemaEngine*) function
In DrawRoot(), if schema engine set, render custom toolbar with schema-defined buttons
Why: Enables JSON-driven UI layouts; fulfills "do you need this button to be a dock? no problem" vision

Defer to Phase 3? This is marked as "sketch" in the plan - consider gating behind #ifdef EXPERIMENTAL_UI_SCHEMA flag

Verification: Load JSON schema defining a "Log Toggle" button → button appears in toolbar, toggles Log panel

Pass 2 Deliverables
✅ Indices tab group extracted (toggleable tabs)
✅ ButtonDockedPanel working (Shift-click dock/float)
✅ AI panels emit to Infomatrix telemetry
✅ Telemetry + Validation panels operational
✅ (Optional) UIUserSchema engine prototype
✅ Fully compilable and testable

Verification Strategy
After Pass 1:

Run app → all 22 panels render correctly
Open Log panel → events appear with color coding
Trigger road generation → see [GEN] roads=X events
Check ImGui Metrics → no ID collision warnings
After Pass 2:

Open Indices panel → toggle tab checkboxes → tabs appear/disappear
Shift-click Log button → panel floats, Shift-click again → docks to Bottom
Click "Request Suggestion" in UI Agent → Telemetry panel shows [AGENT] event
Trigger validation → Validation panel shows green/red events
(If schema enabled) Load JSON → custom buttons appear in toolbar
Decisions
Decision 1: Unify Panel Signatures

Current: AI panels use Render(), others use Draw(float dt)
Chosen: Keep existing signatures, call via DrawPanelByType() dispatcher
Why: Non-breaking; refactoring 22 panels to unified signature is out-of-scope
Decision 2: Event System - GlobalState.infomatrix vs Separate Service

Current: Plan shows both approaches (GlobalState::events() vs Infomatrix)
Chosen: GlobalState.infomatrix as canonical pipeline
Why: Plan doc lines L788-1565 converge on this; GlobalState already singleton
Decision 3: UIUserSchema Scope

Current: Plan includes full schema engine (L1694-2071)
Chosen: Implement as optional Pass 2.5, behind feature flag
Why: Core value achieved by Pass 2.4; schema is future extensibility
Critical File Paths (Dependency Order)
Pass 1 Order:

core/include/RogueCity/Core/Infomatrix.hpp (new)
core/src/Core/Infomatrix.cpp (new)
GlobalState.hpp (add infomatrix member)
rc_ui_root.h (add PanelType + DrawPanelByType)
rc_ui_root.cpp (implement routing + schema)
rc_panel_log.cpp (consume events)
Pass 2 Order:

visualizer/src/ui/rc_ui_indices_views.h (new)
visualizer/src/ui/rc_ui_indices_views.cpp (new)
rc_ui_root.cpp (add ButtonDockedPanel impl)
rc_panel_ui_agent.cpp (add telemetry)
visualizer/src/ui/panels/rc_panel_telemetry.{h,cpp} (new)
visualizer/src/ui/panels/rc_panel_validation.{h,cpp} (new)
core/include/RogueCity/Core/UIUserSchema*.hpp (optional)
Risk Mitigation
Risk 1: Typos in Plan Document

std::string_view_view (L91-L93) → change to std::string_view
EventStreamView private fields accessed (L818-819) → make public or add accessors
Risk 2: AI Panel Signature Mismatch

AI panels use Render(), others Draw(dt)
Mitigation: DrawPanelByType() handles both (see L127-256)
Risk 3: Build System Changes

New files in Core require CMakeLists.txt updates
Mitigation: Add to CMakeLists.txt target sources after creating files
Risk 4: Infomatrix Ring Buffer Index

Plan shows offset access pattern but doesn't initialize capacity
Mitigation: Add m_events.reserve(220); in Infomatrix constructor
This plan references exact line ranges from your [Ui_UX_Enforcement Refactor.md](d:\Projects\RogueCities\RogueCities_UrbanSpatialDesigner\docs\Plans\Ui_UX_Enforcement Refactor.md) plan document. Each code block can be copied directly from the specified indexed_code_fences entries in your Nav JSON.