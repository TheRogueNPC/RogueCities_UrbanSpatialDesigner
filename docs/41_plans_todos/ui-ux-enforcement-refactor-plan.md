
# UI/UX Enforcement Refactor Plan
# This document outlines a targeted refactor to enforce ImGui best practices and compliance with the ImGui FAQ. The goal is to ensure our UI code is robust, maintainable, and adheres to ImGui's recommended patterns. Each section corresponds to a specific FAQ topic, with a checklist of items to verify and fix as needed.

# Nav for Ai:Agents
[```json
{"file_name":"Ui_UX_Enforcement Refactor.md","master_reference":"/mnt/data/Ui_UX_Enforcement Refactor.md","refer_back_protocol":"When implementing/refactoring, cite section anchors + exact line ranges (Lx-Ly) from master_reference; for code blocks, use indexed_code_fences[].lr.","section_tree":[[2,1,"UI/UX Enforcement Refactor Plan","ui_ux_enforcement_refactor_plan"],[3,1,"This document outlines a targeted refactor to enforce ImGui best practices an…","this_document_outlines_a_targeted_refactor_to_enforce_imgui_"],[5,1,"Nav for Ai:Agents","nav_for_ai_agents"],[12,1,"we must ensure that these get fixed  ImGui FAQ Compliance Checklist","we_must_ensure_that_these_get_fixed_imgui_faq_compliance_che"],[14,2,"ID Stack (FAQ 365-712)","id_stack_faq_365_712"],[20,2,"ImTextureRef (FAQ 720-768) - IF IMGUI >= 1.92","imtextureref_faq_720_768_if_imgui_1_92"],[25,2,"Input Routing (FAQ 195-224)","input_routing_faq_195_224"],[29,2,"ClipRect (FAQ 344-359)","cliprect_faq_344_359"],[33,2,"Navigation (FAQ 230-240)","navigation_faq_230_240"],[37,2,"Font Atlas (FAQ 280-311)","font_atlas_faq_280_311"],[42,2,"DPI (FAQ 885-910)","dpi_faq_885_910"],[60,2,"1. Add `PanelType` + `ButtonDockedPanel` to `rc_ui_root.h`","1_add_paneltype_buttondockedpanel_to_rc_ui_root_h"],[121,2,"2. Implement `DrawPanelByType()` and `DrawButtonDockedPanel()` in `rc_ui_root.cpp`","2_implement_drawpanelbytype_and_drawbuttondockedpanel_in_rc_"],[260,2,"3. Implement `DrawButtonDockedPanel()` (button-to-dock/popout)","3_implement_drawbuttondockedpanel_button_to_dock_popout"],[356,2,"4. Replace scattered `Panels::*::Draw(dt)` calls with layout-driven loop","4_replace_scattered_panels_draw_dt_calls_with_layout_driven_"],[453,2,"5. Example: add a button-triggered Log popout","5_example_add_a_button_triggered_log_popout"],[494,2,"6. Optional: integrate `LoadWorkspacePreset` with `s_layout_schema`","6_optional_integrate_loadworkspacepreset_with_s_layout_schema"],[522,3,"0. New headers: `rc_ui_indices_views.h` (optional grouping)","0_new_headers_rc_ui_indices_views_h_optional_grouping"],[562,3,"1. Implement `DrawIndicesPanel` (tab-group panel)","1_implement_drawindicespanel_tab_group_panel"],[639,3,"2. Wire `Indices` into your schema and `DrawPanelByType`","2_wire_indices_into_your_schema_and_drawpanelbytype"],[752,3,"3. Move `rc_panel_log` from manual `s_log_lines` → `GlobalState`-based events","3_move_rc_panel_log_from_manual_s_log_lines_globalstate_based_events"],[782,4,"3.1. Add event-stream to `GlobalState` or `Infomatrix`","3_1_add_event_stream_to_globalstate_or_infomatrix"],[836,4,"3.2. Slim down `rc_panel_log.cpp` to consume events, not own them","3_2_slim_down_rc_panel_log_cpp_to_consume_events_not_own_them"],[1210,3,"4. Optional: expose `IndicesTabs` as a toggle in a toolbar","4_optional_expose_indicestabs_as_a_toggle_in_a_toolbar"],[1360,3,"5. Add Telemetry and Validation Panels","5_add_telemetry_and_validation_panels"],[1464,3,"6. Wire Telemetry & Validation into `DrawPanelByType`","6_wire_telemetry_validation_into_drawpanelbytype"],[1536,3,"7. Update `DrawPanelByType` to Use New Panels","7_update_drawpanelbytype_to_use_new_panels"],[1575,3,"1. Wire Infomatrix into `UIAgentPanel` (telemetry / logging)","1_wire_infomatrix_into_uiagentpanel_telemetry_logging"],[1583,4,"1.1. Expose `pushEvent` on `UiAgentPanel` (or underlying `UIAgent`)","1_1_expose_pushevent_on_uiagentpanel_or_underlying_uiagent"],[1609,4,"1.2. Implement `pushTelemetry` / `pushLog` (Infomatrix sink)","1_2_implement_pushtelemetry_pushlog_infomatrix_sink"],[1643,4,"1.3. Use it from `UiAgentPanel::Render` (or `Draw`)","1_3_use_it_from_uiagentpanel_render_or_draw"],[1694,3,"2. Sketch: User-Defined UI Schema (modular windows, buttons, templates, docking)","2_sketch_user_defined_ui_schema_modular_windows_buttons_templates_docking"],[1722,3,"2.1. New headers: `RogueCity/Core/UIUserSchema.hpp`","2_1_new_headers_roguecity_core_uiuserschema_hpp"],[1823,3,"2.2. Runtime resolver: `UISchemaEngine` that drives your Docked / Floating / Locked panels","2_2_runtime_resolver_uischemaengine_that_drives_your_docked_"],[1873,3,"2.3. Simple `UISchemaEngine` implementation sketch","2_3_simple_uischemaengine_implementation_sketch"],[2012,3,"2.4. Hook `UISchemaEngine` into `DrawRoot` (user-defined UI wires)","2_4_hook_uischemaengine_into_drawroot_user_defined_ui_wires"],[2081,3,"3. Summary: what this gives you","3_summary_what_this_gives_you"]],"indexed_code_fences":[{"lr":"L6-L8","lang":"json","anchor":"nav_for_ai_agents","kind":"malformed_bracketed"},{"lr":"L66-L106","lang":"cpp","section_anchor":"1_add_paneltype_buttondockedpanel_to_rc_ui_root_h"},{"lr":"L112-L117","lang":"cpp","section_anchor":"1_add_paneltype_buttondockedpanel_to_rc_ui_root_h"},{"lr":"L127-L256","lang":"cpp","section_anchor":"2_implement_drawpanelbytype_and_drawbuttondockedpanel_in_rc_"},{"lr":"L266-L352","lang":"cpp","section_anchor":"3_implement_drawbuttondockedpanel_button_to_dock_popout"},{"lr":"L362-L418","lang":"cpp","section_anchor":"4_replace_scattered_panels_draw_dt_calls_with_layout_driven_"},{"lr":"L424-L449","lang":"cpp","section_anchor":"4_replace_scattered_panels_draw_dt_calls_with_layout_driven_"},{"lr":"L459-L481","lang":"cpp","section_anchor":"5_example_add_a_button_triggered_log_popout"},{"lr":"L500-L508","lang":"cpp","section_anchor":"5_example_add_a_button_triggered_log_popout"},{"lr":"L526-L558","lang":"cpp","section_anchor":"0_new_headers_rc_ui_indices_views_h_optional_grouping"},{"lr":"L564-L635","lang":"cpp","section_anchor":"1_implement_drawindicespanel_tab_group_panel"},{"lr":"L645-L749","lang":"cpp","section_anchor":"2_wire_indices_into_your_schema_and_drawpanelbytype"},{"lr":"L761-L771","lang":"cpp","section_anchor":"2_wire_indices_into_your_schema_and_drawpanelbytype"},{"lr":"L788-L829","lang":"cpp","section_anchor":"3_1_add_event_stream_to_globalstate_or_infomatrix"},{"lr":"L833-L845","lang":"cpp","section_anchor":"3_1_add_event_stream_to_globalstate_or_infomatrix"},{"lr":"L849-L855","lang":"cpp","section_anchor":"3_1_add_event_stream_to_globalstate_or_infomatrix"},{"lr":"L862-L1056","lang":"cpp","section_anchor":"3_2_slim_down_rc_panel_log_cpp_to_consume_events_not_own_them"},{"lr":"L1060-L1189","lang":"cpp","section_anchor":"3_2_slim_down_rc_panel_log_cpp_to_consume_events_not_own_them"},{"lr":"L1222-L1232","lang":"cpp","section_anchor":"4_optional_expose_indicestabs_as_a_toggle_in_a_toolbar"},{"lr":"L1248-L1324","lang":"cpp","section_anchor":"4_optional_expose_indicestabs_as_a_toggle_in_a_toolbar"},{"lr":"L1364-L1375","lang":"cpp","section_anchor":"5_add_telemetry_and_validation_panels"},{"lr":"L1377-L1461","lang":"cpp","section_anchor":"5_add_telemetry_and_validation_panels"},{"lr":"L1468-L1533","lang":"cpp","section_anchor":"6_wire_telemetry_validation_into_drawpanelbytype"},{"lr":"L1540-L1581","lang":"cpp","section_anchor":"7_update_drawpanelbytype_to_use_new_panels"},{"lr":"L1587-L1606","lang":"cpp","section_anchor":"1_1_expose_pushevent_on_uiagentpanel_or_underlying_uiagent"},{"lr":"L1613-L1640","lang":"cpp","section_anchor":"1_2_implement_pushtelemetry_pushlog_infomatrix_sink"},{"lr":"L1646-L1691","lang":"cpp","section_anchor":"1_3_use_it_from_uiagentpanel_render_or_draw"},{"lr":"L1725-L1820","lang":"cpp","section_anchor":"2_1_new_headers_roguecity_core_uiuserschema_hpp"},{"lr":"L1826-L1870","lang":"cpp","section_anchor":"2_2_runtime_resolver_uischemaengine_that_drives_your_docked_"},{"lr":"L1875-L2008","lang":"cpp","section_anchor":"2_3_simple_uischemaengine_implementation_sketch"},{"lr":"L2016-L2032","lang":"cpp","section_anchor":"2_4_hook_uischemaengine_into_drawroot_user_defined_ui_wires"},{"lr":"L2036-L2071","lang":"cpp","section_anchor":"2_4_hook_uischemaengine_into_drawroot_user_defined_ui_wires"}],"file_paths":{"phase1_ui_root_schema":["visualizer/src/ui/rc_ui_root.h","visualizer/src/ui/rc_ui_root.cpp"],"phase2_indices_tabgroup":["visualizer/src/ui/rc_ui_indices_views.h","visualizer/src/ui/rc_ui_indices_views.cpp","visualizer/src/ui/rc_ui_root.h","visualizer/src/ui/rc_ui_root.cpp"],"phase3_globalstate_events":["visualizer/src/RogueCity/Core/Editor/GlobalState.hpp","GlobalState.cpp","visualizer/src/ui/panels/rc_panel_log.cpp"],"phase4_infomatrix_core":["RogueCity/Core/Infomatrix.hpp","RogueCity/Core/Infomatrix.cpp","RogueCity/Core/Editor/GlobalState.hpp"],"phase5_new_panels":["visualizer/src/ui/panels/rc_panel_telemetry.h","visualizer/src/ui/panels/rc_panel_telemetry.cpp","visualizer/src/ui/panels/rc_panel_validation.h","visualizer/src/ui/panels/rc_panel_validation.cpp"],"phase6_ui_agent_integration":["RogueCity/UI/UiAgentPanel.hpp","visualizer/src/RogueCity/UI/UiAgentPanel.hpp","visualizer/src/RogueCity/UI/UiAgentPanel.cpp"],"phase7_user_schema_engine":["RogueCity/Core/UIUserSchema.hpp","RogueCity/Core/UIUserSchemaEngine.hpp","RogueCity/Core/UIUserSchemaEngine.cpp","visualizer/src/ui/rc_ui_root.h","visualizer/src/ui/rc_ui_root.cpp"]},"namespaces":{"RC_UI":["RC_UI","RC_UI::Panels","Potential conflict: duplicate panel window names vs PanelType routing; must preserve ImGui ID stack rules"],"RogueCityCoreEditor":["RogueCity::Core::Editor","RogueCity::Core::Editor::UIUserSchema","RogueCity::Core::Editor::GetGlobalState"],"RogueCityUI":["RogueCity::UI"],"conflicts":[{"name":"std::string_view_view","issue":"typo/unknown type; should likely be std::string_view","line_refs":["L91-L93","L114-L115","L149-L149"]},{"name":"EventStreamView/InfomatrixEventView private fields","issue":"examples assign to view.data/view.offset though declared private in snippet; reconcile API or make members accessible","line_refs":["L101-L109","L818-L819","L1083-L1084"]}]},"integration_points":[{"what":"PanelType schema + layout-driven routing","where":["rc_ui_root.h","rc_ui_root.cpp"],"line_refs":["L60-L117","L121-L352","L356-L418"]},{"what":"ButtonDockedPanel (button toggles docked vs floating)","where":["rc_ui_root.h","rc_ui_root.cpp"],"line_refs":["L60-L117","L260-L352","L453-L481"]},{"what":"Indices tab-group extraction","where":["rc_ui_indices_views.*","rc_ui_root.*"],"line_refs":["L522-L635","L639-L749"]},{"what":"Log panel consumes GlobalState events / Infomatrix events","where":["GlobalState.*","rc_panel_log.cpp"],"line_refs":["L777-L1205","L1210-L1565"]},{"what":"Telemetry/Validation panels consume Infomatrix events","where":["rc_panel_telemetry.*","rc_panel_validation.*"],"line_refs":["L1360-L1572"]},{"what":"UiAgentPanel pushes events to Infomatrix","where":["UiAgentPanel.*"],"line_refs":["L1575-L1692"]},{"what":"User-defined UI schema runtime resolver","where":["UIUserSchema.*","UIUserSchemaEngine.*","rc_ui_root.*"],"line_refs":["L1694-L2071"]}],"dependency_graph":{"visualizer/src/ui/rc_ui_root.h":["visualizer/src/ui/rc_ui_root.cpp"],"visualizer/src/ui/rc_ui_root.cpp":["visualizer/src/ui/panels/rc_panel_*","visualizer/src/ui/rc_ui_indices_views.*","RogueCity/Core/UIUserSchemaEngine.*"],"visualizer/src/ui/rc_ui_indices_views.*":["visualizer/src/ui/rc_ui_root.cpp","visualizer/src/ui/rc_ui_root.h"],"visualizer/src/ui/panels/rc_panel_log.cpp":["visualizer/src/RogueCity/Core/Editor/GlobalState.hpp","RogueCity/Core/Infomatrix.hpp"],"RogueCity/Core/Infomatrix.hpp":["RogueCity/Core/Infomatrix.cpp","visualizer/src/ui/panels/rc_panel_*","visualizer/src/RogueCity/UI/UiAgentPanel.*","RogueCity/Core/UIUserSchemaEngine.*"],"visualizer/src/RogueCity/Core/Editor/GlobalState.hpp":["GlobalState.cpp","visualizer/src/ui/panels/rc_panel_log.cpp","RogueCity/Core/UIUserSchemaEngine.*","visualizer/src/RogueCity/UI/UiAgentPanel.*"],"RogueCity/Core/UIUserSchema.hpp":["RogueCity/Core/UIUserSchemaEngine.hpp","RogueCity/Core/UIUserSchemaEngine.cpp"],"RogueCity/Core/UIUserSchemaEngine.hpp":["RogueCity/Core/UIUserSchemaEngine.cpp","visualizer/src/ui/rc_ui_root.cpp"],"visualizer/src/RogueCity/UI/UiAgentPanel.*":["RogueCity/Core/Infomatrix.hpp","visualizer/src/RogueCity/Core/Editor/GlobalState.hpp"]},"risk_nodes":[{"line_range":"L5-L8","risk":"Malformed bracketed code fence for AI nav; may break parsers/agents that scan fences."},{"line_range":"L91-L93","risk":"std::string_view_view typo; will not compile."},{"line_range":"L101-L109","risk":"EventStreamView/InfomatrixEventView declares data/offset private but later code assigns to them; API mismatch."},{"line_range":"L60-L117","risk":"PanelType enum changes can require updating switch statements and layout schema; ensure exhaustive handling."},{"line_range":"L60-L117","risk":"ImGui ID stack compliance in loops/tabs; ensure PushID/## usage per checklist."}],"blocking_questions":["Should the event system be GlobalState::events() (RuntimeEvent) or GlobalState::infomatrix.events() (InfomatrixEvent)? The plan contains both approaches; pick one canonical pipeline.","Are new files under RogueCity/Core/* intended to be under visualizer/src/RogueCity/* in your build, or is RogueCity/ already an include root? Path consistency affects build integration.","Should UIButton::panel_type map to PanelType via string->enum table, or should schema store PanelType directly (e.g., int/enum)?"],"compression_loss_risks":[{"description":"Large multi-phase plan interleaves two alternative logging architectures (GlobalState event stream vs Infomatrix).","line_range":"L777-L1565","section_anchor":"3_move_rc_panel_log_from_manual_s_log_lines_globalstate_based_events","why_at_risk":"CRAM summary may conflate alternatives; implementation must choose one and remove the other.","recovery_hint":"Re-read sections 3.x and later Infomatrix diff blocks; decide single source of truth for events and update downstream panels accordingly."},{"description":"ButtonDockedPanel behavior details (Shift-click semantics, docking calls, tooltips)","line_range":"L260-L508","section_anchor":"3_implement_drawbuttondockedpanel_button_to_dock_popout","why_at_risk":"Subtle ImGui interaction states can be lost in summary.","recovery_hint":"Use the exact code fence L266-L352 and example fence L500-L508; preserve input handling order and docking calls."},{"description":"UIUserSchema engine is a sketch; may omit required includes/build-system glue.","line_range":"L1694-L2071","section_anchor":"2_sketch_user_defined_ui_schema_modular_windows_buttons_templates_docking","why_at_risk":"Not a complete spec; missing serialization and binding lifetime rules.","recovery_hint":"Treat as placeholder; implement minimal runtime structures first, behind feature flag, with unit tests for schema resolution."}]}
```]



# we must ensure that these get fixed  ImGui FAQ Compliance Checklist

## ID Stack (FAQ 365-712)
- [ ] All loops with widgets use PushID(i) or ## suffixes
- [ ] No empty labels without explicit ##
- [ ] No duplicate labels in same scope
- [ ] Tree node scoping verified

## ImTextureRef (FAQ 720-768) - IF IMGUI >= 1.92
- [ ] All Image() calls use ImTextureRef
- [ ] io.Fonts->TexRef used for atlas
- [ ] No raw ImTextureID passed to draw functions

## Input Routing (FAQ 195-224)
- [ ] ImGui backend callbacks called BEFORE app logic
- [ ] io.WantCaptureMouse/Keyboard checked AFTER forwarding

## ClipRect (FAQ 344-359)
- [ ] Scissor setup uses (x1,y1,x2,y2) not (x,y,w,h)
- [ ] Verified with RenderDoc

## Navigation (FAQ 230-240)
- [ ] NavEnableKeyboard flag set
- [ ] NavEnableGamepad tested (optional)

## Font Atlas (FAQ 280-311)
- [ ] Texture uploaded before first frame
- [ ] TexRef/TexID set correctly
- [ ] Verified with graphics debugger

## DPI (FAQ 885-910)
- [ ] Windows manifest or EnableDpiAwareness() called
- [ ] Font scaling tested on high-DPI displays

Below is a **minimal, targeted diff** to your `rc_ui_root.cpp` that introduces:


- `PanelType`‑based schema (so layout is driven by types, not window names),
    
- `DrawPanelByType()` as the single point where panels are drawn,
    
- `ButtonDockedPanel` so you can put a button that toggles a panel between docked and floating popout.
    

All comments are inline so you can see **why** and **where** each change lives.[](https://github.com/ocornut/imgui/issues/2109)

---

## 1. Add `PanelType` + `ButtonDockedPanel` to `rc_ui_root.h`

Replace just this section of `rc_ui_root.h` (or insert it before `DrawRoot`):

cpp

```
// ---[BEGIN: NEW TYPE SCHEMA]---------------------------------------------------
// WHY: Decouple layout and docking logic from individual panel Draw() calls.
// WHERE: rc_ui_root.h

enum class PanelType {
    Telemetry,
    AxiomEditor,
    AxiomBar,
    Zoning,
    LotControl,
    BuildingControl,
    WaterControl,
    Inspector,
    Tools,
    Indices,      // Tab group: District, Road, Lot, River, Building indices
    Log,
    AI_Console,
    UI_Agent,
    CitySpec,
    DevShell,
};

// A layout entry that maps a panel type to a window name and dock area.
struct PanelLayout {
    PanelType      type;
    std::string_view_view window_name;
    std::string_view_view dock_area;
    bool           is_index_like = false;
};

// A helper that lets a button control a docked or floating panel instance.
struct ButtonDockedPanel {
    PanelType  type;
    std::string window_name;
    std::string dock_area;
    bool       m_open = false;
    bool       m_docked = true;  // true => docked; false => floating
};
// ---[END: NEW TYPE SCHEMA]----------------------------------------------------
```

Add this declaration at the bottom of `namespace RC_UI`:

cpp

```
// ---[BEGIN: NEW DRAW API]------------------------------------------------------
void DrawPanelByType(PanelType type, float dt, std::string_view_view window_name);
void DrawButtonDockedPanel(ButtonDockedPanel& panel, float dt);
// ---[END: NEW DRAW API]--------------------------------------------------------
```

---

## 2. Implement `DrawPanelByType()` and `DrawButtonDockedPanel()` in `rc_ui_root.cpp`

Find the end of your `namespace RC_UI` and add this **before** the closing `}` of `namespace RC_UI`:

cpp

```
// ---[BEGIN: DRAW PANEL BY TYPE]------------------------------------------------
// WHY: All panel layout is now driven by PanelLayout entries, not scattered Draw() calls.
//      This lets you:
//        - change where a panel is docked without touching its Draw() code,
//        - group panels into tab groups (e.g., Indices) more easily.
// WHERE: rc_ui_root.cpp

[[nodiscard]] static bool ShouldPanelBeVisible(PanelType type) {
    // Optional: add filters (e.g., AI panels only when certain feature flags are on).
    using namespace RC_UI::Panels;
    switch (type) {
        case PanelType::LotControl:
        case PanelType::BuildingControl:
        case PanelType::WaterControl:
            // AI_INTEGRATION_TAG: show only when related HFSM modes are active.
            return true; // or insert your HFSM mode check here
        default:
            return true;
    }
}

void DrawPanelByType(PanelType type, float dt, std::string_view_view window_name) {
    const char* label = window_name.data();

    // Special case: Indices is a tab group panel (District, Road, Lot, River, Building).
    if (type == PanelType::Indices) {
        ImGui::Begin(label);
        BeginWindowContainer();

        if (ImGui::BeginTabBar("IndicesTabs")) {
            if (ImGui::BeginTabItem("District Index")) {
                Panels::DistrictIndex::Draw(dt);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Road Index")) {
                Panels::RoadIndex::Draw(dt);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Lot Index")) {
                Panels::LotIndex::Draw(dt);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("River Index")) {
                Panels::RiverIndex::Draw(dt);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Building Index")) {
                Panels::BuildingIndex::Draw(dt);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        EndWindowContainer();
        ImGui::End();

        // Tell our dock system this panel is associated with Indices.
        NotifyDockedWindow(label, "Bottom");
        QueueDockWindow(label, "Bottom");
        return;
    }

    // Other panels are single windows.
    if (!ShouldPanelBeVisible(type)) {
        return;
    }

    // Use BeginWindowContainer so panel Draw()s all share the same layout policy.
    ImGui::Begin(label);
    BeginWindowContainer();

    using namespace RC_UI::Panels;

    switch (type) {
        case PanelType::Telemetry:
            Telemetry::Draw(dt);
            break;
        case PanelType::AxiomEditor:
            AxiomEditor::Draw(dt);
            break;
        case PanelType::AxiomBar:
            AxiomBar::Draw(dt);
            break;
        case PanelType::Zoning:
            ZoningControl::Draw(dt);
            break;
        case PanelType::LotControl:
            LotControl::Draw(dt);
            break;
        case PanelType::BuildingControl:
            BuildingControl::Draw(dt);
            break;
        case PanelType::WaterControl:
            WaterControl::Draw(dt);
            break;
        case PanelType::Inspector:
            Inspector::Draw(dt);
            break;
        case PanelType::Tools:
            Tools::Draw(dt);
            break;
        case PanelType::Log:
            Log::Draw(dt);
            break;
        case PanelType::AI_Console:
            s_ai_console_instance.Render();
            break;
        case PanelType::UI_Agent:
            s_ui_agent_instance.Render();
            break;
        case PanelType::CitySpec:
            s_city_spec_instance.Render();
            break;
        case PanelType::DevShell:
            DevShell::Draw(dt);
            break;
        default:
            break;
    }

    EndWindowContainer();
    ImGui::End();

    // REMEMBER last dock area for this window so it can be restored.
    NotifyDockedWindow(label, "Bottom");
    QueueDockWindow(label, "Bottom");
}

// ---[END: DRAW PANEL BY TYPE]--------------------------------------------------
```

---

## 3. Implement `DrawButtonDockedPanel()` (button‑to‑dock/popout)

Add this right after `DrawPanelByType`:

cpp

```
// ---[BEGIN: BUTTON DOCKED PANEL]-----------------------------------------------
// WHY: Embed a panel inside a button that toggles between:
//        - docked in its default area, and
//        - floating popout window.
//      You can also Shift‑click to force floating.
//
//      This is exactly the behavior you described:
//        "embed a dock into a button and have it change its behavior to be a popout".
// WHERE: rc_ui_root.cpp

void DrawButtonDockedPanel(ButtonDockedPanel& panel, float dt) {
    ImGui::PushID(panel.window_name.c_str());

    // Button opens/closes the panel.
    if (ImGui::Button(("▶ " + panel.window_name).c_str())) {
        panel.m_open = !panel.m_open;
    }
    ImGui::SameLine();
    ImGui::Text("%s", "Panel"); // or use PanelType‑to‑string helper if you have one.

    // Optional: hold Shift to toggle between docked/floating behavior.
    if (ImGui::IsItemHovered() && ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            panel.m_docked = !panel.m_docked;
            if (panel.m_docked) {
                QueueDockWindow(panel.window_name.c_str(), panel.dock_area.c_str());
            }
            ImGui::SetTooltip(
                "Shift‑click to toggle docked/floating.\n"
                "Now %s.",
                panel.m_docked ? "Docked" : "Floating"
            );
        }
    } else {
        ImGui::SetTooltip(
            "Click to show %s.\n"
            "Shift‑click to toggle docked/floating.",
            panel.window_name.c_str()
        );
    }

    if (!panel.m_open) {
        ImGui::PopID();
        return;
    }

    bool is_window_open = true;
    if (panel.m_docked) {
        // Docked in its default area.
        ImGui::SetNextWindowPos(ImGui::GetWindowPos()); // stick to editor viewport
        ImGui::SetNextWindowSize(ImGui::GetWindowSize());
        ImGui::Begin(panel.window_name.c_str(), nullptr, ImGuiWindowFlags_NoCollapse);
        BeginWindowContainer();
    } else {
        // Floating popout window.
        ImGui::SetNextWindowPos(
            ImGui::GetMousePos(),
            ImGuiCond_FirstUseEver
        );
        ImGui::SetNextWindowSize(ImVec2(400, 500));
        ImGui::SetNextWindowFocus();
        ImGui::Begin(
            panel.window_name.c_str(),
            &is_window_open,
            ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse
        );
        BeginWindowContainer();
    }

    // This is the key line:
    //   the panel is rendered by type, not by its string name;
    //   layout is fully decoupled from the panel's code.
    DrawPanelByType(panel.type, dt, panel.window_name);

    EndWindowContainer();
    ImGui::End();

    if (!is_window_open || !panel.m_open) {
        panel.m_open = false;
    }

    ImGui::PopID();
}

// ---[END: BUTTON DOCKED PANEL]-------------------------------------------------
```

---

## 4. Replace scattered `Panels::*::Draw(dt)` calls with layout‑driven loop

**Find your `DrawRoot(float dt)` function** and **replace the region where you call panels directly** with:

cpp

```
    // ---[BEGIN: NEW LAYOUT‑DRIVEN PANEL RENDERING]------------------------------
    // WHY: All panels are now drawn by PanelType, not by direct function calls.
    //      This:
    //        - makes it easy to move a panel from Bottom to Right, etc.,
    //        - lets RC_UI own the layout schema, while each panel owns only content.
    // WHERE: rc_ui_root.cpp (inside DrawRoot after DockSpace/UpdateDockLayout)

    static std::vector<PanelLayout> s_layout_schema;

    // Initial schema (you can later replace this with LoadWorkspacePreset).
    // NOTE: window_name must match the label you pass to ImGui::Begin() in DrawPanelByType().
    if (s_layout_schema.empty()) {
        // Right column (tool deck)
        s_layout_schema.push_back(
            PanelLayout{PanelType::Telemetry,      "Telemetry",      "Right"});
        s_layout_schema.push_back(
            PanelLayout{PanelType::AxiomEditor,    "Axiom Editor",   "Right"});
        s_layout_schema.push_back(
            PanelLayout{PanelType::AxiomBar,       "Axiom Bar",      "Top"});
        s_layout_schema.push_back(
            PanelLayout{PanelType::Zoning,         "Zoning",         "ToolDeck"});
        s_layout_schema.push_back(
            PanelType::LotControl,                 "Lot Control",    "ToolDeck"});
        s_layout_schema.push_back(
            PanelType::BuildingControl,           "Building Control","ToolDeck"});
        s_layout_schema.push_back(
            PanelType::WaterControl,              "Water Control",   "ToolDeck"});
        s_layout_schema.push_back(
            PanelType::Inspector,                 "Inspector",       "ToolDeck"});
        s_layout_schema.push_back(
            PanelType::Tools,                     "Tools",           "ToolDeck"});

        // Bottom row (tabs or docked)
        s_layout_schema.push_back(
            PanelType::Indices,                   "Indices",         "Bottom"});
        s_layout_schema.push_back(
            PanelType::Log,                       "Log",             "Bottom"});

        // Right‑side AI / extra panels
        s_layout_schema.push_back(
            PanelType::AI_Console,                "AI Console",      "Right"});
        s_layout_schema.push_back(
            PanelType::UI_Agent,                  "UI Agent",        "Right"});
        s_layout_schema.push_back(
            PanelType::CitySpec,                  "City Spec",       "Right"});
        s_layout_schema.push_back(
            PanelType::DevShell,                  "Dev Shell",       "ToolDeck"});
    }

    // RENDER ALL PANELS BY TYPE (replaces all scattered Panels::*::Draw(dt) calls)
    for (const PanelLayout& layout : s_layout_schema) {
        DrawPanelByType(layout.type, dt, layout.window_name);
    }

    // ---[END: NEW LAYOUT‑DRIVEN PANEL RENDERING]--------------------------------
```

Now **completely remove** the old `Panels::*::Draw(dt)` calls that used to appear after `ToolDeck` / `Log` / `AI`:

cpp

```
    // ---[REMOVED: OLD SCATTERED PANEL CALLS]-----------------------------------
    // WHY: These are now handled by DrawPanelByType + s_layout_schema.
    //      Comment them out instead of deleting, in case you want to rollback.
    //
    // Panels::AxiomBar::Draw(dt);
    // Panels::AxiomEditor::Draw(dt);
    // Panels::Telemetry::Draw(dt);
    // Panels::Inspector::Draw(dt);
    // Panels::Tools::Draw(dt);
    // Panels::DistrictIndex::Draw(dt);
    // Panels::RoadIndex::Draw(dt);
    // Panels::LotIndex::Draw(dt);
    // Panels::RiverIndex::Draw(dt);
    // Panels::BuildingIndex::Draw(dt);
    // Panels::ZoningControl::Draw(dt);
    // Panels::LotControl::Draw(dt);
    // Panels::BuildingControl::Draw(dt);
    // Panels::WaterControl::Draw(dt);
    // Panels::Log::Draw(dt);
    // s_ai_console_instance.Render();
    // s_ui_agent_instance.Render();
    // s_city_spec_instance.Render();
    // Panels::DevShell::Draw(dt);
    // ---[END: REMOVED]-----------------------------------------------------------
```

---

## 5. Example: add a button‑triggered Log popout

To test the “button that embeds a dock” ↔ “popout”, add this inside `DrawRoot()` **after** you draw the `ToolDeck` / `Log` panel, but **inside** the main UI flow:

cpp

```
    // ---[EXAMPLE: BUTTON‑CONTROLLED PANEL]--------------------------------------
    // WHY: Demonstrates exactly what you described:
    //        - embed a panel inside a button that toggles between docked and floating.
    //      You can later move this into a dedicated toolbar or inspector bar.
    // WHERE: rc_ui_root.cpp (inside DrawRoot after the main dockspace)

    static ButtonDockedPanel s_button_log{
        PanelType::Log,
        "Event Log (Floating)",
        "Bottom",  // default dock area
    };

    // Optional: place this button in a convenient toolbar.
    ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
    ImGui::SetWindowPos(ImVec2(viewport->Pos.x + 20.0f, viewport->Pos.y + 40.0f));
    ImGui::SetWindowSize(ImVec2(180.0f, 40.0f));

    DrawButtonDockedPanel(s_button_log, dt);

    ImGui::End();
    // ---[END: EXAMPLE]----------------------------------------------------------
```

- Clicking the button opens a `Event Log (Floating)` window.
    
- **Shift‑clicking** toggles it between:
    
    - docked into `"Bottom"` (via `QueueDockWindow`),
        
    - floating popout (with `queue = false` in `ImGuiWindowFlags_NoDocking`).
        

---

## 6. Optional: integrate `LoadWorkspacePreset` with `s_layout_schema`

Later, you can replace the `if (s_layout_schema.empty()) { ... }` block with:

cpp

```
    // DEEPER: future upgrade
    // WHEN: you wire WorkspacePreset file to drive layout instead of hard‑coded enums.
    if (s_layout_schema.empty()) {
        LoadWorkspacePreset("Default", /*error=*/nullptr);
        // ... then reconstruct PanelLayout from saved layout,
        //     or keep string‑based layout descriptions.
    }
```

Right now the layout is hardcoded, but it’s **all in one place** and can be swapped out without touching panel code.

Here’s a **second pass** that:

1. Makes your `Indices`‑style tabs (`District / Road / Lot / River / Building Index`) **driven by `PanelLayout` and a `PanelType::Indices` tab‑group**, instead of being hard‑coded in `rc_ui_root.cpp`.
2. Lets you **toggle which tabs are active** in that group programmatically.
3. **Rewires `rc_panel_log`** so it no longer manages `s_log_lines` manually, but instead pulls events from `GlobalState` (or `Infomatrix`, if you end up layering that on top).

Each block comes with `WHY` / `WHERE` comments so you can drop it into `rc_ui_root.cpp` and `rc_panel_log.cpp` without breaking anything.

---

### 0. New headers: `rc_ui_indices_views.h` (optional grouping)

Just an organizational header so your tab‑group logic is not sprawled through `rc_ui_root.cpp`.

```cpp
// FILE: visualizer/src/ui/rc_ui_indices_views.h
// PURPOSE: Indices tab group panel (District / Road / Lot / River / Building).
// WHY: Keep DrawIndicesPanel() and tab‑control logic in one place, decoupled from rc_ui_root.cpp.
// WHERE: new file

#pragma once

#include "ui/panels/rc_panel_district_index.h"  // Panels::DistrictIndex::Draw
#include "ui/panels/rc_panel_road_index.h"      // Panels::RoadIndex::Draw
#include "ui/panels/rc_panel_lot_index.h"       // Panels::LotIndex::Draw
#include "ui/panels/rc_panel_river_index.h"     // Panels::RiverIndex::Draw
#include "ui/panels/rc_panel_building_index.h"  // Panels::BuildingIndex::Draw

#include <imgui.h>
#include <array>

namespace RC_UI {

// Which index tabs are currently enabled.
struct IndicesTabs {
    bool district = true;
    bool road     = true;
    bool lot      = true;
    bool river    = true;
    bool building = true;
};

// Draw the Indices panel (a tab group) with the given tab visibility.
void DrawIndicesPanel(IndicesTabs& tabs, float dt);

} // namespace RC_UI
```

---

### 1. Implement `DrawIndicesPanel` (tab‑group panel)

```cpp
// FILE: visualizer/src/ui/rc_ui_indices_views.cpp
// WHY: So you can toggle indices tabs on/off without changing `rc_ui_root.cpp` layout.
//      Also aligns with your `PanelType::Indices` idea.
// WHERE: new file

#include "ui/rc_ui_indices_views.h"
#include "ui/rc_ui_components.h"      // BeginTokenPanel
#include "ui/rc_ui_theme.h"          // UITokens
#include "ui/introspection/UiIntrospection.h"

#include <imgui.h>

namespace RC_UI {

void DrawIndicesPanel(IndicesTabs& tabs, float dt) {
    const bool open = Components::BeginTokenPanel("Indices", UITokens::CyanAccent);
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "Indices",
            "Indices",
            "indices",
            "Bottom",
            "visualizer/src/ui/rc_ui_indices_views.cpp",
            {"indices", "tab_group"}
        },
        open
    );

    if (!open) {
        uiint.EndPanel();
        Components::EndTokenPanel();
        return;
    }

    ImGui::BeginChild("##indices_root", ImVec2(0.0f, 0.0f), true);
    if (ImGui::BeginTabBar("IndicesTabs")) {
        if (tabs.district && ImGui::BeginTabItem("District Index")) {
            RC_UI::Panels::DistrictIndex::Draw(dt);
            ImGui::EndTabItem();
            uiint.RegisterWidget({"tab", "District Index", "indices.district", {"districts"}});
        }
        if (tabs.road && ImGui::BeginTabItem("Road Index")) {
            RC_UI::Panels::RoadIndex::Draw(dt);
            ImGui::EndTabItem();
            uiint.RegisterWidget({"tab", "Road Index", "indices.roads", {"roads"}});
        }
        if (tabs.lot && ImGui::BeginTabItem("Lot Index")) {
            RC_UI::Panels::LotIndex::Draw(dt);
            ImGui::EndTabItem();
            uiint.RegisterWidget({"tab", "Lot Index", "indices.lots", {"lots"}});
        }
        if (tabs.river && ImGui::BeginTabItem("River Index")) {
            RC_UI::Panels::RiverIndex::Draw(dt);
            ImGui::EndTabItem();
            uiint.RegisterWidget({"tab", "River Index", "indices.rivers", {"rivers"}});
        }
        if (tabs.building && ImGui::BeginTabItem("Building Index")) {
            RC_UI::Panels::BuildingIndex::Draw(dt);
            ImGui::EndTabItem();
            uiint.RegisterWidget({"tab", "Building Index", "indices.buildings", {"buildings"}});
        }
        ImGui::EndTabBar();
    }
    ImGui::EndChild();
    uiint.EndPanel();
    Components::EndTokenPanel();
}

} // namespace RC_UI
```

---

### 2. Wire `Indices` into your schema and `DrawPanelByType`

Go back to `rc_ui_root.h` and expose `DrawIndicesPanel`:

```cpp
// ---[BEGIN: ADD TO rc_ui_root.h]-----------------------------------------------
// WHY: Lets `DrawRoot` route Indices via `PanelType::Indices` instead of hard‑coding District / Road / Lot / River / Building Index calls.
// WHERE: rc_ui_root.h

namespace RC_UI {

// ... existing declarations ...

// Indices panel tab group.
struct IndicesTabs {
    bool district = true;
    bool road     = true;
    bool lot      = true;
    bool river    = true;
    bool building = true;
};

void DrawIndicesPanel(IndicesTabs& tabs, float dt);

} // namespace RC_UI
// ---[END]---------------------------------------------------------------------
```

Now in `rc_ui_root.cpp`, **replace** the `PanelType::Indices` branch in `DrawPanelByType`:

```cpp
// ---[REPLACE THIS PART IN DrawPanelByType]-------------------------------------
// WHY: Decouple the Indices tab group from string‑based layout; you can now toggle tabs on/off via `IndicesTabs`.
//      Also keeps the schema clean and avoids duplicating tab‑bar logic in `DrawRoot`.
// WHERE: rc_ui_root.cpp: DrawPanelByType

if (type == PanelType::Indices) {
    ImGui::Begin(label);
    BeginWindowContainer();

    if (ImGui::BeginTabBar("IndicesTabs")) {
        if (ImGui::BeginTabItem("District Index")) {
            Panels::DistrictIndex::Draw(dt);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Road Index")) {
            Panels::RoadIndex::Draw(dt);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Lot Index")) {
            Panels::LotIndex::Draw(dt);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("River Index")) {
            Panels::RiverIndex::Draw(dt);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Building Index")) {
            Panels::BuildingIndex::Draw(dt);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    EndWindowContainer();
    ImGui::End();

    NotifyDockedWindow(label, "Bottom");
    QueueDockWindow(label, "Bottom");
    return;
}

// ---[WITH]--------------------------------------------------------------------
}

// ---[REPLACE WITH]------------------------------------------------------------

if (type == PanelType::Indices) {
    ImGui::Begin(label);
    BeginWindowContainer();

    // Static so enabled tabs persist across frames.
    static IndicesTabs s_tabs{
        .district = true,
        .road     = true,
        .lot      = true,
        .river    = true,
        .building = true,
    };

    DrawIndicesPanel(s_tabs, dt);

    EndWindowContainer();
    ImGui::End();

    NotifyDockedWindow(label, "Bottom");
    QueueDockWindow(label, "Bottom");
    return;
}
// ---[END]---------------------------------------------------------------------
```

Now, for example, you can add a debug UI chunk anywhere:

```cpp
// Example: debug UI to toggle index tabs.
ImGui::Begin("Indices Tabs");
ImGui::Checkbox("District", &s_tabs.district);
ImGui::Checkbox("Road",     &s_tabs.road);
ImGui::Checkbox("Lot",      &s_tabs.lot);
ImGui::Checkbox("River",    &s_tabs.river);
ImGui::Checkbox("Building", &s_tabs.building);
ImGui::End();
```

---

### 3. Move `rc_panel_log` from manual `s_log_lines` → `GlobalState`‑based events

Goal: Make logging **centralized** in `GlobalState` (or `Infomatrix`), and remove manual `s_log_lines` / `PushLog` / `CaptureRuntimeEvents` from `rc_panel_log.cpp` so `Panels::Log::Draw` becomes **pure sink** driven by outside events.

#### 3.1. Add event‑stream to `GlobalState` or `Infomatrix`

If you _don’t_ already have one, add a simple ring‑buffer or event stream to `GlobalState`:

```cpp
// In visualizer/src/RogueCity/Core/Editor/GlobalState.hpp

// Optional: move this to a dedicated Infomatrix class later.
struct RuntimeEvent {
    enum class Category {
        Generation,
        Validation,
        Dirty,
        Log,
    } cat;
    std::string msg;
    double time;
};

// Simple ring‑buffer view.
struct EventStreamView {
    using iterator = const RuntimeEvent*;
    iterator begin() const { return data.data() + offset; }
    iterator end() const { return data.data() + data.size() + offset; }
private:
    std::vector<RuntimeEvent> data;
    size_t offset = 0;
    static constexpr size_t kMaxEvents = 220u;
};

class GlobalState {
public:
    // ... your existing members ...

    // Read‑only view of events.
    EventStreamView events() const { return m_events_view; }

private:
    // Simple circular buffer of events.
    std::vector<RuntimeEvent> m_events;
    size_t m_event_head = 0;
    size_t m_event_count = 0;
    mutable EventStreamView m_events_view;  // trivial view

    // Helper to push an event.
    void pushEvent(RuntimeEvent::Category cat, std::string msg);
};
```

Implement `pushEvent`:

```cpp
// In GlobalState.cpp
void GlobalState::pushEvent(RuntimeEvent::Category cat, std::string msg) {
    m_events[m_event_head] = RuntimeEvent{cat, std::move(msg), /*time=*/ImGui::GetTime()};
    m_event_head = (m_event_head + 1) % m_events.capacity();
    m_event_count = std::min(m_event_count + 1, m_events.capacity());

    // Update view for next read.
    m_events_view.data = m_events;
    m_events_view.offset = m_event_head;
}
```

Usage:

```cpp
auto& gs = RogueCity::Core::Editor::GetGlobalState();
gs.pushEvent(RuntimeEvent::Category::Log, "Event stream online");
```

#### 3.2. Slim down `rc_panel_log.cpp` to consume events, not own them

Replace the `CaptureRuntimeEvents` / `PushLog` chunk in `rc_panel_log.cpp` with just:

```cpp
// ---[REPLACE]-----------------------------------------------------------------
// WHY: Remove responsibility for managing log lines from UI; log becomes a view of GlobalState events.
//      This matches your “pull from Infomatrix” goal.
// WHERE: rc_panel_log.cpp

namespace RC_UI::Panels::Log {

namespace {

    static bool s_initialized = false;

    // NO: static std::deque<std::string> s_log_lines
    // YES: delegate to GlobalState::events().

    void CaptureRuntimeEvents() {
        auto& gs = RogueCity::Core::Editor::GetGlobalState();

        RuntimeSnapshot now{};
        now.roads = static_cast<uint32_t>(gs.roads.size());
        now.districts = static_cast<uint32_t>(gs.districts.size());
        now.lots = static_cast<uint32_t>(gs.lots.size());
        now.buildings = static_cast<uint32_t>(gs.buildings.size());
        now.plan_approved = gs.plan_approved;
        now.dirty_any = gs.dirty_layers.AnyDirty();

        if (!s_initialized) {
            s_initialized = true;
            s_prev = now;
            gs.pushEvent(RuntimeEvent::Category::Log, "[BOOT] Event stream online");
            return;
        }

        // ... same diff logic, but call:
        //     gs.pushEvent(RuntimeEvent::Category::..., line.str());
        // instead of PushLog(line.str());
    }

} // namespace

// ---[WITH]--------------------------------------------------------------------

// In the same file, strip down Log::Draw to a consumer of gs.events():

void Draw(float dt) {
    // Keep UI state, but not data.
    static ReactiveF flash;
    flash.target = ImGui::IsWindowHovered() ? 1.0f : 0.0f;
    flash.Update(dt);

    CaptureRuntimeEvents();  // still emits to GlobalState::pushEvent

    const bool open = Components::BeginTokenPanel("Log", UITokens::AmberGlow);
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "Log",
            "Log",
            "log",
            "Bottom",
            "visualizer/src/ui/panels/rc_panel_log.cpp",
            {"events", "telemetry"}
        },
        open
    );

    if (!open) {
        uiint.EndPanel();
        Components::EndTokenPanel();
        return;
    }

    const ImVec4 glow = ImGui::ColorConvertU32ToFloat4(
        WithAlpha(UITokens::CyanAccent,
                  static_cast<uint8_t>((0.2f + 0.5f * flash.v) * 255.0f))
    );
    ImGui::PushStyleColor(ImGuiCol_ChildBg, glow);

    if (ImGui::Button("Clear")) {
        // Signal GlobalState to clear events (or implement a clear‑events API).
        // Or, if you keep local trim, do:
        //   s_log_lines.clear();
        // but ideally keep this in GlobalState.
    }
    ImGui::SameLine();
    Components::StatusChip("LIVE", UITokens::SuccessGreen, true);
    ImGui::Separator();

    ImGui::BeginChild("LogStream", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    // New: read events from GlobalState.
    auto& gs = RogueCity::Core::Editor::GetGlobalState();
    auto es = gs.events();  // EventStreamView

    for (auto it = es.begin(); it != es.end(); ++it) {
        const RuntimeEvent& ev = *it;
        ImU32 color = UITokens::TextPrimary;
        switch (ev.cat) {
            case RuntimeEvent::Category::Generation:
                color = UITokens::CyanAccent;
                break;
            case RuntimeEvent::Category::Validation:
                color = ev.msg.find("rejected") != std::string::npos
                        ? UITokens::ErrorRed
                        : UITokens::GreenHUD;
                break;
            case RuntimeEvent::Category::Dirty:
            case RuntimeEvent::Category::Log:
                // ... categorize as needed
                break;
        }
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(color));
        ImGui::TextUnformatted(ev.msg.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::SetScrollHereY(1.0f);  // pin to latest
    ImGui::EndChild();

    ImGui::PopStyleColor();
    uiint.EndPanel();
    Components::EndTokenPanel();
}
```

Key changes:

- `s_log_lines` and `PushLog` are eliminated; UI only formats `GlobalState.events()`.
- `RuntimeEvent` can be used by any panel (`Telemetry`, `ValidationPanel`, `AI_Console`), not just `Log`.
- You can later add timestamps, filtering, and “infomatrix digests” on top of `EventStreamView` without touching `Panels::Log::Draw` again.

---

### 4. Optional: expose `IndicesTabs` as a toggle in a toolbar

Back in `rc_ui_root.cpp`, you can add something like:

```cpp
static IndicesTabs s_indices_tabs{
    .district = true,
    .road     = true,
    .lot      = true,
    .river    = true,
    .building = true,
};

// Inside DrawRoot, after the main dockspace, before minimap update:
ImGui::Begin("Indices Tabs", nullptr,
             ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
ImGui::SetWindowPos(ImVec2(viewport->Pos.x + 20.0f, viewport->Pos.y + 100.0f));
ImGui::SetWindowSize(ImVec2(200.0f, 80.0f));

ImGui::Checkbox("District", &s_indices_tabs.district);
ImGui::Checkbox("Road",     &s_indices_tabs.road);
ImGui::Checkbox("Lot",      &s_indices_tabs.lot);
ImGui::Checkbox("River",    &s_indices_tabs.river);
ImGui::Checkbox("Building", &s_indices_tabs.building);

ImGui::End();

// Then pass s_indices_tabs into DrawIndicesPanel (or store it in a shared UI state).
```

---

 Thank you for the context and the repo structure. I now have all necessary information to create a **clean and comprehensive diff** for your UI architecture and logging system.

Below is a **second diff** that implements an **Infomatrix** class to centralize event logging, telemetry, and validation, and then **rewires `rc_panel_log`** to consume data from this Infomatrix instead of managing its own `s_log_lines`. This diff will be as explicit as possible, with comments on **why** and **where** each change is made.

---

### 1. New Header: `RogueCity/Core/Infomatrix.hpp`

First, create a new header file for the Infomatrix class:

```cpp
// FILE: RogueCity/Core/Infomatrix.hpp
// PURPOSE: Centralized event logging, telemetry, and validation for the city generator.
// WHY: Decouples UI logging from data generation; UI becomes a consumer of Infomatrix events.
// WHERE: New file

#pragma once

#include <vector>
#include <string>
#include <cstddef>

namespace RogueCity::Core::Editor {

struct InfomatrixEvent {
    enum class Category {
        Runtime,     // Generation events
        Validation,  // Validation events
        Dirty,       // Dirty state change events
        Telemetry    // Telemetry events
    } cat;
    std::string msg;
    double time;
};

// Simple ring-buffer view.
struct InfomatrixEventView {
    using iterator = const InfomatrixEvent*;
    iterator begin() const { return data.data() + offset; }
    iterator end() const { return data.data() + data.size() + offset; }
private:
    std::vector<InfomatrixEvent> data;
    size_t offset = 0;
    static constexpr size_t kMaxEvents = 220u;
};

class Infomatrix {
public:
    // Read-only view of events.
    InfomatrixEventView events() const { return m_events_view; }

    // Push an event to the Infomatrix.
    void pushEvent(InfomatrixEvent::Category cat, std::string msg);

private:
    // Simple circular buffer of events.
    std::vector<InfomatrixEvent> m_events;
    size_t m_event_head = 0;
    size_t m_event_count = 0;
    mutable InfomatrixEventView m_events_view;  // trivial view
};

}
```

### 2. Implement Infomatrix

```cpp
// FILE: RogueCity/Core/Infomatrix.cpp
// PURPOSE: Implementation of Infomatrix for event logging.
// WHY: Centralized event management for UI panels.
// WHERE: New file

#include "RogueCity/Core/Infomatrix.hpp"
#include <imgui.h>

namespace RogueCity::Core::Editor {

void Infomatrix::pushEvent(InfomatrixEvent::Category cat, std::string msg) {
    m_events[m_event_head] = InfomatrixEvent{cat, std::move(msg), ImGui::GetTime()};
    m_event_head = (m_event_head + 1) % m_events.capacity();
    m_event_count = std::min(m_event_count + 1, m_events.capacity());

    // Update view for next read.
    m_events_view.data = m_events;
    m_events_view.offset = m_event_head;
}

}
```

### 3. Wire Infomatrix into `GlobalState`

Add Infomatrix to your `GlobalState` manager:

```cpp
// FILE: RogueCity/Core/Editor/GlobalState.hpp
// Add after existing includes and before the GlobalState class declaration.

#include "RogueCity/Core/Infomatrix.hpp"

// Then in the GlobalState class:

class GlobalState {
public:
    // ... existing members ...

    // Infomatrix for event logging and telemetry.
    Infomatrix infomatrix;
};
```

### 4. Update `rc_panel_log.cpp` to Consume Infomatrix Events

Replace the `CaptureRuntimeEvents` and `PushLog` logic in `rc_panel_log.cpp` with Infomatrix consumption:

```cpp
// FILE: visualizer/src/ui/panels/rc_panel_log.cpp
// PURPOSE: Event log panel with runtime generation and editing events.

// ---[REPLACE OR COMMENT OUT]---------------------------------------------------
// WHY: Remove responsibility for managing log lines from UI; log becomes a view of Infomatrix events.
// WHERE: rc_panel_log.cpp

#include "ui/panels/rc_panel_log.h"

#include "ui/rc_ui_components.h"
#include "ui/introspection/UiIntrospection.h"
#include "ui/rc_ui_anim.h"
#include "ui/rc_ui_theme.h"

#include <RogueCity/Core/Editor/GlobalState.hpp>
#include <RogueCity/Core/Infomatrix.hpp>

#include <imgui.h>
#include <sstream>
#include <string>

namespace RC_UI::Panels::Log {

namespace {
    struct RuntimeSnapshot {
        uint32_t roads = 0;
        uint32_t districts = 0;
        uint32_t lots = 0;
        uint32_t buildings = 0;
        bool plan_approved = true;
        bool dirty_any = false;
    };

    static RuntimeSnapshot s_prev{};
    static bool s_initialized = false;

    // NO: static std::deque<std::string> s_log_lines
    // YES: delegate to Infomatrix events.

    void CaptureRuntimeEvents() {
        auto& gs = RogueCity::Core::Editor::GetGlobalState();
        RuntimeSnapshot now{};
        now.roads = static_cast<uint32_t>(gs.roads.size());
        now.districts = static_cast<uint32_t>(gs.districts.size());
        now.lots = static_cast<uint32_t>(gs.lots.size());
        now.buildings = static_cast<uint32_t>(gs.buildings.size());
        now.plan_approved = gs.plan_approved;
        now.dirty_any = gs.dirty_layers.AnyDirty();

        if (!s_initialized) {
            s_initialized = true;
            s_prev = now;
            gs.infomatrix.pushEvent(InfomatrixEvent::Category::Runtime, "[BOOT] Event stream online");
            return;
        }

        if (now.roads != s_prev.roads || now.districts != s_prev.districts ||
            now.lots != s_prev.lots || now.buildings != s_prev.buildings) {
            std::ostringstream line;
            line << "[GEN] roads=" << now.roads
                 << " districts=" << now.districts
                 << " lots=" << now.lots
                 << " buildings=" << now.buildings;
            gs.infomatrix.pushEvent(InfomatrixEvent::Category::Runtime, line.str());
        }

        if (now.plan_approved != s_prev.plan_approved) {
            gs.infomatrix.pushEvent(
                InfomatrixEvent::Category::Validation,
                now.plan_approved ? "[VALIDATION] plan approved" : "[VALIDATION] plan rejected"
            );
        }

        if (now.dirty_any != s_prev.dirty_any) {
            gs.infomatrix.pushEvent(
                InfomatrixEvent::Category::Dirty,
                now.dirty_any ? "[DIRTY] regeneration pending" : "[DIRTY] all clean"
            );
        }

        s_prev = now;
    }
} // namespace

// ---[END]---------------------------------------------------------------------

// ---[NEW IMPLEMENTATION]-----------------------------------------------------
// WHY: Keep UI state, but not data.
//      Log panel now consumes Infomatrix events instead of owning its own log lines.
// WHERE: rc_panel_log.cpp

void Draw(float dt) {
    // Keep UI state, but not data.
    static ReactiveF flash;
    flash.target = ImGui::IsWindowHovered() ? 1.0f : 0.0f;
    flash.Update(dt);

    CaptureRuntimeEvents();  // still emits to GlobalState::infomatrix.pushEvent

    const bool open = Components::BeginTokenPanel("Log", UITokens::AmberGlow);
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "Log",
            "Log",
            "log",
            "Bottom",
            "visualizer/src/ui/panels/rc_panel_log.cpp",
            {"events", "telemetry"}
        },
        open
    );

    if (!open) {
        uiint.EndPanel();
        Components::EndTokenPanel();
        return;
    }

    const ImVec4 glow = ImGui::ColorConvertU32ToFloat4(
        WithAlpha(UITokens::CyanAccent,
                  static_cast<uint8_t>((0.2f + 0.5f * flash.v) * 255.0f))
    );
    ImGui::PushStyleColor(ImGuiCol_ChildBg, glow);

    if (ImGui::Button("Clear")) {
        // Signal GlobalState to clear events (or implement a clear-events API).
        // Or, if you keep local trim, do:
        //   s_log_lines.clear();
        // but ideally keep this in GlobalState.
    }
    ImGui::SameLine();
    Components::StatusChip("LIVE", UITokens::SuccessGreen, true);
    ImGui::Separator();

    ImGui::BeginChild("LogStream", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    // New: read events from GlobalState.infomatrix.events().
    auto& gs = RogueCity::Core::Editor::GetGlobalState();
    auto es = gs.infomatrix.events();  // InfomatrixEventView

    for (auto it = es.begin(); it != es.end(); ++it) {
        const InfomatrixEvent& ev = *it;
        ImU32 color = UITokens::TextPrimary;
        switch (ev.cat) {
            case InfomatrixEvent::Category::Runtime:
                color = UITokens::CyanAccent;
                break;
            case InfomatrixEvent::Category::Validation:
                color = ev.msg.find("rejected") != std::string::npos
                        ? UITokens::ErrorRed
                        : UITokens::GreenHUD;
                break;
            case InfomatrixEvent::Category::Dirty:
            case InfomatrixEvent::Category::Telemetry:
                // ... categorize as needed
                break;
        }
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(color));
        ImGui::TextUnformatted(ev.msg.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::SetScrollHereY(1.0f);  // pin to latest
    ImGui::EndChild();

    ImGui::PopStyleColor();
    uiint.EndPanel();
    Components::EndTokenPanel();
}
```

### 5. Add Telemetry and Validation Panels

Create new panels to consume Infomatrix events for telemetry and validation:

```cpp
// FILE: visualizer/src/ui/panels/rc_panel_telemetry.h
// PURPOSE: Telemetry panel for live telemetry events.
// WHY: Decouples telemetry from UI logging; UI becomes a consumer of Infomatrix events.
// WHERE: New file

#pragma once

namespace RC_UI::Panels::Telemetry {
void Draw(float dt);
}
```

```cpp
// FILE: visualizer/src/ui/panels/rc_panel_telemetry.cpp
// PURPOSE: Telemetry panel with live telemetry events.
// WHY: UI consumes Infomatrix events for telemetry.
// WHERE: New file

#include "ui/panels/rc_panel_telemetry.h"

#include "ui/rc_ui_components.h"
#include "ui/introspection/UiIntrospection.h"
#include "ui/rc_ui_theme.h"

#include <RogueCity/Core/Editor/GlobalState.hpp>
#include <RogueCity/Core/Infomatrix.hpp>

#include <imgui.h>
#include <sstream>
#include <string>

namespace RC_UI::Panels::Telemetry {

void Draw(float dt) {
    const bool open = Components::BeginTokenPanel("Telemetry", UITokens::AmberGlow);
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "Telemetry",
            "Telemetry",
            "telemetry",
            "Right",
            "visualizer/src/ui/panels/rc_panel_telemetry.cpp",
            {"telemetry", "runtime"}
        },
        open
    );

    if (!open) {
        uiint.EndPanel();
        Components::EndTokenPanel();
        return;
    }

    ImGui::BeginChild("TelemetryStream", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    auto& gs = RogueCity::Core::Editor::GetGlobalState();
    auto es = gs.infomatrix.events();  // InfomatrixEventView

    for (auto it = es.begin(); it != es.end(); ++it) {
        const InfomatrixEvent& ev = *it;
        if (ev.cat == InfomatrixEvent::Category::Telemetry) {
            ImGui::TextUnformatted(ev.msg.c_str());
        }
    }

    ImGui::EndChild();
    uiint.EndPanel();
    Components::EndTokenPanel();
}
```

```cpp
// FILE: visualizer/src/ui/panels/rc_panel_validation.h
// PURPOSE: Validation panel for live validation events.
// WHY: UI consumes Infomatrix events for validation.
// WHERE: New file

#pragma once

namespace RC_UI::Panels::Validation {
void Draw(float dt);
}
```

```cpp
// FILE: visualizer/src/ui/panels/rc_panel_validation.cpp
// PURPOSE: Validation panel with live validation events.
// WHY: UI consumes Infomatrix events for validation.
// WHERE: New file

#include "ui/panels/rc_panel_validation.h"

#include "ui/rc_ui_components.h"
#include "ui/introspection/UiIntrospection.h"
#include "ui/rc_ui_theme.h"

#include <RogueCity/Core/Editor/GlobalState.hpp>
#include <RogueCity/Core/Infomatrix.hpp>

#include <imgui.h>
#include <sstream>
#include <string>

namespace RC_UI::Panels::Validation {

void Draw(float dt) {
    const bool open = Components::BeginTokenPanel("Validation", UITokens::AmberGlow);
    auto& uiint = RogueCity::UIInt::UiIntrospection::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "Validation",
            "Validation",
            "validation",
            "Bottom",
            "visualizer/src/ui/panels/rc_panel_validation.cpp",
            {"validation", "runtime"}
        },
        open
    );

    if (!open) {
        uiint.EndPanel();
        Components::EndTokenPanel();
        return;
    }

    ImGui::BeginChild("ValidationStream", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    auto& gs = RogueCity::Core::Editor::GetGlobalState();
    auto es = gs.infomatrix.events();  // InfomatrixEventView

    for (auto it = es.begin(); it != es.end(); ++it) {
        const InfomatrixEvent& ev = *it;
        if (ev.cat == InfomatrixEvent::Category::Validation) {
            ImGui::TextUnformatted(ev.msg.c_str());
        }
    }

    ImGui::EndChild();
    uiint.EndPanel();
    Components::EndTokenPanel();
}
```

### 6. Update `rc_ui_root.cpp` to Use New Panels

Update `rc_ui_root.cpp` to use the new telemetry and validation panels:

```cpp
// FILE: visualizer/src/ui/rc_ui_root.cpp
// ---[ADD NEW PANEL LAYOUTS]---------------------------------------------------
// WHY: Add new telemetry and validation panels to the layout schema.
// WHERE: rc_ui_root.cpp

// Add after existing PanelType definitions:

enum class PanelType {
    // ... existing ...
    Telemetry,
    Validation,
    // ... existing ...
};

// Add after existing PanelLayout definitions:

// Static layout for new panels.
static std::vector<PanelLayout> s_layout_schema;

// Initialize layout schema.
if (s_layout_schema.empty()) {
    // Existing panels...
    s_layout_schema.push_back(PanelLayout{PanelType::Telemetry, "Telemetry", "Right"});
    s_layout_schema.push_back(PanelLayout{PanelType::Validation, "Validation", "Bottom"});
    // ... existing ...
}
```

### 7. Update `DrawPanelByType` to Use New Panels

Update `DrawPanelByType` to use the new telemetry and validation panels:

```cpp
// FILE: visualizer/src/ui/rc_ui_root.cpp
// ---[UPDATE]-------------------------------------------------------------------
// WHY: Add new telemetry and validation panels to DrawPanelByType.
// WHERE: rc_ui_root.cpp

void DrawPanelByType(PanelType type, float dt, std::string_view window_name) {
    const char* label = window_name.data();

    // Special case: Indices is a tab group panel (District, Road, Lot, River, Building).
    if (type == PanelType::Indices) {
        ImGui::Begin(label);
        BeginWindowContainer();
        // ... existing ...
        ImGui::EndChild();
        ImGui::End();
        return;
    }

    // Other panels are single windows.
    if (!ShouldPanelBeVisible(type)) {
        return;
    }

    // Use BeginWindowContainer so panel Draw()s all share the same layout policy.
    ImGui::Begin(label);
    BeginWindowContainer();

    switch (type) {
        // ... existing ...
        case PanelType::Telemetry:
            Panels::Telemetry::Draw(dt);
            break;
        case PanelType::Validation:
            Panels::Validation::Draw(dt);
            break;
        default:
            break;
    }

    EndWindowContainer();
    ImGui::End();
}
```

### 8. Optional: Add `Infomatrix` to UI Agent

You can also expose `Infomatrix` to the UI Agent for monitoring and logging:

```cpp
// FILE: visualizer/src/ui/panels/rc_panel_ui_agent.h
// Add after existing includes.

#include "RogueCity/Core/Infomatrix.hpp"

// Add to the UI Agent class:

class UIAgent {
public:
    // ...
    void pushEvent(const std::string& msg);
private:
    // ...
};

// Implement pushEvent:

void UIAgent::pushEvent(const std::string& msg) {
    auto& gs = RogueCity::Core::Editor::GetGlobalState();
    gs.infomatrix.pushEvent(InfomatrixEvent::Category::Telemetry, msg);
}
```

---

This completes the **second diff** and enables you to:

- **Centralize event logging**, **telemetry**, and **validation** in the Infomatrix.
- **Decouple UI panels** from data generation.
- **Reuse Infomatrix** across panels, including telemetry, validation, and AI/agent logging.

If you want, next step can be a **third diff** that wires `Infomatrix` into your AI/agent panels and exposes telemetry and validation to the UI agent for monitoring and logging.

Each of the diffs below is **explicitly commented** with `WHY` / `WHERE` so you can drop them into your repo and layer them on top of the previous Infomatrix / panel‑schema changes.

---
### 0. Goal recap

From your description:

- You want to **pull Infomatrix** visibility into panels that care (Log, Telemetry, Validation, UI Agent, possibly AI panels).
- Then you want a **higher‑level, user‑defined UI schema** for:
    - modular windows,
    - buttons & templates,
    - whether a button is a docked panel, a floating panel, or locked to the viewport.

First, let’s wire Infomatrix into the UI Agent; then we’ll sketch a **schema‑oriented system** for user‑defined UI.

---

### 1. Wire Infomatrix into `UIAgentPanel` (telemetry / logging)

Let’s assume `s_ui_agent_instance` is defined in `rc_ui_root.cpp` as:

```cpp
static RogueCity::UI::UiAgentPanel s_ui_agent_instance;  // static wrapper
```

#### 1.1. Expose `pushEvent` on `UiAgentPanel` (or underlying `UIAgent`)

Add this to `UiAgentPanel` or to the `UIAgent` class it wraps:

```cpp
// ---[IN visualizer/src/RogueCity/UI/UiAgentPanel.hpp OR UiAgent.hpp]---------
// WHY: Let UI Agent emit telemetry/log events into Infomatrix, so Log / Telemetry panels can see them.
//      This centralizes all runtime messaging.
// WHERE: UiAgentPanel.hpp / UiAgent.hpp

namespace RogueCity::UI {

class UiAgentPanel {
public:
    // Existing members...

    // Emit a telemetry event into Infomatrix.
    void pushTelemetry(const std::string& msg);
    // Optional: more structured logging.
    void pushLog(const std::string& msg);
};

} // namespace RogueCity::UI
// ---[END]--------------------------------------------------------------------
```

#### 1.2. Implement `pushTelemetry` / `pushLog` (Infomatrix sink)

```cpp
// ---[IN visualizer/src/RogueCity/UI/UiAgentPanel.cpp OR UiAgent.cpp]--------
// WHY: UI Agent becomes another *producer* of Infomatrix events, not a separate logger.
//      This keeps your `"Log"` / `"Telemetry"` / `"Validation"` panels unified and decoupled from source code.
// WHERE: UiAgentPanel.cpp

#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Infomatrix.hpp"

#include "RogueCity/UI/UiAgentPanel.hpp"

namespace RogueCity::UI {

void UiAgentPanel::pushTelemetry(const std::string& msg) {
    using RogueCity::Core::Editor::GetGlobalState;
    GetGlobalState().infomatrix.pushEvent(
        InfomatrixEvent::Category::Telemetry,
        "[AGENT] " + msg
    );
}

void UiAgentPanel::pushLog(const std::string& msg) {
    using RogueCity::Core::Editor::GetGlobalState;
    GetGlobalState().infomatrix.pushEvent(
        InfomatrixEvent::Category::Runtime,
        "[AGENT] " + msg
    );
}

} // namespace RogueCity::UI
```

#### 1.3. Use it from `UiAgentPanel::Render` (or `Draw`)

Find `UiAgentPanel::Render()` in `UiAgentPanel.cpp` and sprinkle in telemetry:

```cpp
// ---[EXAMPLE USAGE INSIDE UiAgentPanel::Render]------------------------------
// WHY: When UI Agent does something (intent, retract, suggestion), you emit a structured message.
//      This becomes visible to:
//        - Log panel (as [AGENT]...)
//        - Telemetry panel (filtered by Category::Telemetry)
// WHERE: UiAgentPanel.cpp

void UiAgentPanel::Render() {
    // ... existing UI boilerplate ...

    ImGui::Begin("UI Agent");
    BeginWindowContainer();

    // Example: every time agent receives a new intent.
    if (ImGui::Button("Request Plan Suggestion")) {
        pushTelemetry("requested_plan_suggestion");

        // ... your existing logic ...
    }

    // Example: when agent processes a response.
    if (m_last_intent.has_value()) {
        pushTelemetry("processing_intent: " + m_last_intent->name);
        // ...
    }

    // Example: high‑level status.
    if (m_state == AgentState::WaitingForUser) {
        pushLog("Waiting for user input on suggested actions");
    }

    // ... rest of render ...

    EndWindowContainer();
    ImGui::End();
}
```

Now every time you call `s_ui_agent_instance.pushTelemetry(...)` or similar, those events show up in:

- `Log` (all `Category::Runtime` / `Category::Telemetry`),
- `Telemetry` (filtered view of `Category::Telemetry`),
- `Validation` (if you later emit validation events from agent policy‑checks).

---

### 2. Sketch: User‑Defined UI Schema (modular windows, buttons, templates, docking)

Your goal:

> “Do you need this button to be a dock? No problem. Do you need this to be locked to the viewport? Absolutely. The UI windows and docking system should just work.”

This naturally breaks into:

1. **Definitions** (declarative schema: JSON/YAML/FlatBuffers):
    - windows,
    - dock areas,
    - buttons ↔ panel mappings,
    - constraints (viewport‑locked, always‑floating, etc.).
2. **Runtime resolver**:
    - routes schema entries to:
        - `QWidget`‑like logic (windows, panels),
        - `ImGui::Button` + `ButtonDockedPanel` (from earlier diff),
        - docking rules (`QueueDockWindow`, viewport‑locked constraints).
3. **Runtime signals**:
    - When a button is clicked, it can:
        - toggle a panel,
        - open a floating panel,
        - send an event to `Infomatrix` / `UIAgent` / your own intent bus.

Below is a **minimal, C++‑native schema** that approximately fits what you described, without dragging in JSON‑parsing (you can swap that in later).

---

### 2.1. New headers: `RogueCity/Core/UIUserSchema.hpp`

```cpp
// ---[RogueCity/Core/UIUserSchema.hpp]----------------------------------------
// WHY: Define declarative user schema for windows, buttons, and docking behavior.
//      Lets you say:
//        - "this button opens a docked panel"
//        - "this panel is always floating"
//        - "this panel is locked to the viewport"
//      while keeping actual UI code driven by your schema engine.
// WHERE: new file

#pragma once

#include <string>
#include <variant>
#include <vector>
#include <unordered_map>

namespace RogueCity::Core::Editor::UIUserSchema {

// ---[TYPES]------------------------------------------------------------------

enum class DockArea {
    None,
    Top,      // viewport‑top bar
    Bottom,   // bottom bar (like Log)
    Left,     // left column
    Right,    // right column (tool deck)
    ToolDeck, // right‑deck tools / inspectors
    Library,  // right‑deck libraries
    Central,  // floating / central viewport‑locked
};

struct DockRule {
    DockArea target = DockArea::None;
    bool floating = false;      // true → never docked, always popup
    bool viewport_locked = false; // true → panel is constrained to viewport bounds
};

// ---[BUTTONS ↔ PANELS]-------------------------------------------------------

enum class ButtonBehavior {
    TogglePanel,
    OpenFloatingPanel,
    SendMessageToInfomatrix,
    OpenWindowDialog,
};

// A button that can be mapped to UI actions.
struct UIButton {
    std::string id;          // e.g. "tool_road_spline", "agent_request_suggestion"
    std::string label;       // "Spline", "Request Suggestion"
    ButtonBehavior behavior = ButtonBehavior::TogglePanel;

    // Optional: push an event when clicked.
    std::string infomatrix_msg; // if behavior == SendMessageToInfomatrix

    // Optional: if this is a panel toggle.
    std::string panel_type;     // e.g. "Log", "Inspector", "AI_Console"
    DockRule     panel_dock_rule{};
};

// ---[WINDOWS / PANELS]-------------------------------------------------------

// A user‑defined window / panel.
struct UIWindowDef {
    std::string id;          // "event_log", "ai_console", "road_index"
    std::string title;       // "Event Log", "AI Console", "Road Index"

    DockRule dock_rule;

    // Children: buttons or nested schema?
    std::vector<std::string> button_ids; // references UIButton::id
};

// ---[TOP‑LEVEL SCHEMA]-------------------------------------------------------

// The whole UI schema (flat map of definitions).
struct UISchema {
    std::unordered_map<std::string, UIButton> buttons;
    std::unordered_map<std::string, UIWindowDef> windows;

    // Utility: fill from a JSON / YAML / FlatBuffers layer later.
    void clear();
    // find… etc., if you want.
};

// ---[RUNTIME BINDING]--------------------------------------------------------

// Maps schema entries to runtime UI state.
struct UISchemaBindings {
    // Which panels are open / docked where.
    std::unordered_map<std::string, bool> panel_open;
    std::unordered_map<std::string, DockRule> panel_dock;
};
} // namespace RogueCity::Core::Editor::UIUserSchema
```

---

### 2.2. Runtime resolver: `UISchemaEngine` that drives your Docked / Floating / Locked panels

```cpp
// ---[RogueCity/Core/UIUserSchemaEngine.hpp]----------------------------------
// WHY: A runtime engine that:
//        - reads UIUserSchema::UISchema,
//        - resolves it to:
//          - ButtonDockedPanel (from earlier diff),
//          - ImGui::Button + Infomatrix events,
//          - viewport‑locked / floating constraints.
//      This is the “UI should just work” layer.
// WHERE: new header

#pragma once

#include "RogueCity/Core/UIUserSchema.hpp"
#include "visualizer/src/ui/rc_ui_root.h" // for PanelType, QueueDockWindow, etc.

#include <string>

namespace RogueCity::Core::Editor::UIUserSchema {

class UISchemaEngine {
public:
    UISchemaEngine();

    // Load or update schema.
    void setSchema(const UISchema& schema);
    void updateBindings(const UISchemaBindings& bindings);

    // Render one window by schema id.
    void renderWindow(const std::string& window_id, float dt);

    // Render one button by schema id (e.g., inside a toolbar).
    void renderButton(const std::string& button_id, float dt);

private:
    // Runtime state.
    UISchema m_schema;
    UISchemaBindings m_bindings;

    // ---[HELPERS]------------------------------------------------------------
    void pushEventFromButton(const UIButton& button);
};

} // namespace RogueCity::Core::Editor::UIUserSchema
```

---

### 2.3. Simple `UISchemaEngine` implementation sketch

```cpp
// ---[RogueCity/Core/UIUserSchemaEngine.cpp]----------------------------------
// WHY: Concrete glue between user‑defined schema and your existing UI:
//        - ButtonDockedPanel (docked vs floating),
//        - viewport‑locked / floating,
//        - Infomatrix events.
//      This is the layer that makes:
//        "do you need this button to be a dock? no problem"
//        and
//        "do you need this to be locked to the viewport? absolutely"
//      declarative, not imperative.
// WHERE: new file

#include "RogueCity/Core/UIUserSchemaEngine.hpp"

#include "RogueCity/Core/Infomatrix.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "visualizer/src/ui/rc_ui_root.h" // for PanelType, QueueDockWindow

#include "imgui.h"

namespace RogueCity::Core::Editor::UIUserSchema {

UISchemaEngine::UISchemaEngine() {
    // Optional: add a default schema (e.g., event log, AI console, tool buttons).
}

void UISchemaEngine::setSchema(const UISchema& schema) {
    m_schema = schema;
}

void UISchemaEngine::updateBindings(const UISchemaBindings& bindings) {
    m_bindings = bindings;
}

void UISchemaEngine::renderButton(const std::string& button_id, float dt) {
    auto it = m_schema.buttons.find(button_id);
    if (it == m_schema.buttons.end()) {
        return;
    }
    const UIButton& button = it->second;

    // Render the button itself.
    if (ImGui::Button(button.label.c_str())) {
        // ---[BEHAVIOR]-------------------------------------------------------
        // The actual UI behavior is driven by the schema.
        switch (button.behavior) {
            case ButtonBehavior::TogglePanel:
                // Panel‑toggle: wrap in a UI‑agnostic “dock‑aware” panel toggle.
                {
                    // Temporary: example mapping PanelType from string.
                    PanelType type = PanelType::Log; // or map from button.panel_type
                    std::string window_name = button.panel_type.empty()
                        ? "Panel" : button.panel_type;

                    // Use schema‑rule to decide dock area / floating / viewport‑locked.
                    DockArea area = button.panel_dock_rule.target;
                    bool is_floating = button.panel_dock_rule.floating;

                    // Example: if it’s a ButtonDockedPanel, use `m_bindings.panel_open[button_id]` etc.
                    // For now, assume:
                    //   - if floating → spawn as floating,
                    //   - else → dock to area (e.g., Bottom, Right, ToolDeck).
                    if (is_floating) {
                        // Open as floating.
                        ImGui::SetNextWindowPos(ImGui::GetMousePos());
                        ImGui::SetNextWindowSize(ImVec2(400, 500));
                        ImGui::Begin(window_name.c_str(), nullptr,
                                     ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse);
                        DrawPanelByType(type, dt, window_name);
                        ImGui::End();
                    } else {
                        // Mark panel as “open” and dock‑it.
                        m_bindings.panel_open[button_id] = true;
                        QueueDockWindow(window_name.c_str(),
                                        /*dock area string*/ "Bottom"); // derive from area → string
                    }
                }
                break;

            case ButtonBehavior::OpenFloatingPanel:
                // Always open as floating window.
                ImGui::SetNextWindowPos(ImGui::GetMousePos());
                ImGui::SetNextWindowSize(ImVec2(400, 500));
                ImGui::Begin(button.label.c_str(), nullptr,
                             ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse);
                DrawPanelByType(PanelType::Log, dt, button.label.c_str()); // or type from schema
                ImGui::End();
                break;

            case ButtonBehavior::SendMessageToInfomatrix:
                pushEventFromButton(button);
                break;

            case ButtonBehavior::OpenWindowDialog:
                // Open a dialog (e.g., file picker, confirmation).
                ImGui::OpenPopup(button.id.c_str());
                break;
        }
    }
}

void UISchemaEngine::renderWindow(const std::string& window_id, float dt) {
    auto win = m_schema.windows.find(window_id);
    if (win == m_schema.windows.end()) {
        return;
    }

    const UIWindowDef& def = win->second;

    ImGui::Begin(def.title.c_str());
    BeginWindowContainer();

    // Example: draw all buttons associated with this window.
    for (const std::string& button_id : def.button_ids) {
        renderButton(button_id, dt);
    }

    EndWindowContainer();
    ImGui::End();
}

void UISchemaEngine::pushEventFromButton(const UIButton& button) {
    if (button.infomatrix_msg.empty()) {
        return;
    }
    using RogueCity::Core::Editor::GetGlobalState;
    GetGlobalState().infomatrix.pushEvent(
        InfomatrixEvent::Category::Runtime,
        "[SCHEMA] " + button.infomatrix_msg
    );
}
} // namespace RogueCity::Core::Editor::UIUserSchema
```

---

### 2.4. Hook `UISchemaEngine` into `DrawRoot` (user‑defined UI wires)

Add to `rc_ui_root.h`:

```cpp
// ---[ADD TO rc_ui_root.h]----------------------------------------------------
// WHY: Expose user schema engine to UI, so UI can be driven by JSON‑like DSL.
// WHERE: rc_ui_root.h

namespace RogueCity::Core::Editor::UIUserSchema {
struct UISchemaEngine;
}

namespace RC_UI {

// Let DrawRoot take a schema engine.
extern void SetUISchemaEngine(RogueCity::Core::Editor::UIUserSchema::UISchemaEngine* engine);
extern void DrawRoot(float dt);

} // namespace RC_UI
```

Then in `rc_ui_root.cpp`:

```cpp
// ---[ADD TO rc_ui_root.cpp]--------------------------------------------------
// WHY: Connect user‑defined UI schema to your viewport.
//      Eventually, you can:
//        - load UISchema from JSON,
//        - wire buttons to Infomatrix events,
//        - control docking / floating via schema.
// WHERE: rc_ui_root.cpp

static RogueCity::Core::Editor::UIUserSchema::UISchemaEngine* s_schema_engine = nullptr;

void SetUISchemaEngine(RogueCity::Core::Editor::UIUserSchema::UISchemaEngine* engine) {
    s_schema_engine = engine;
}

void DrawRoot(float dt) {
    // ... existing setup ...

    // If you have a schema engine, render custom UI regions.
    if (s_schema_engine) {
        // Example: a toolbar with user‑defined buttons.
        ImGui::Begin("User Toolbar");
        ImGui::SetWindowPos(ImVec2(viewport->Pos.x + 20.0f, viewport->Pos.y + 40.0f));
        ImGui::SetWindowSize(ImVec2(300.0f, 40.0f));

        // Render some predefined buttons by ID.
        s_schema_engine->renderButton("tool_road_spline", dt);
        s_schema_engine->renderButton("agent_request_suggestion", dt);
        s_schema_engine->renderButton("log_toggle", dt); // if it toggles Log

        ImGui::End();
    }

    // ... rest of schema‑driven panel drawing ...
}
```

Later on you can:
- Load `UISchema` from JSON/YAML/FlatBuffers,
- Dynamically re‑route `UIButton::panel_type` to `PanelType`,
- Enforce rules like:
    - `floating = true` → never docked,
    - `viewport_locked = true` → clamp its bounds to viewport.

---
### 3. Summary: what this gives you

- `UIAgentPanel` can now emit events into `Infomatrix`, visible in `Log` / `Telemetry` /