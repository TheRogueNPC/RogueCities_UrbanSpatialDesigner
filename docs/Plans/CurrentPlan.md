Think of it as the **UI Scene Graph + Semantics**. Here’s what it should contain.

---
You can get to a powerful “god’s‑eye UI snapshot + AI‑friendly layout model” in two focused steps.

---
Here’s what I’d tell any “AI Coder” agent about the best tools and practices **for this repo specifically**.

---

## Core libraries to lean on

- **nlohmann_json** (3rdparty/nlohmann_json)
    
    - Use for all JSON config, AI protocol, CitySpec, UI layout, and introspection data.[](https://n8n.io/pricing/)​
        
    - Prefer `to_json/from_json` overloads per struct to keep things type‑safe and auto‑serializable.
        
- **magic_enum**
    
    - Use for enum ↔ string conversions (modes, roles, tags) instead of custom maps.[](https://n8n.io/pricing/)​
        
    - This is ideal for UI snapshot exports and AI‑facing protocols (e.g. `"mode": "AXIOM"`).
        
- **sol2 + Lua**
    
    - Best choice for hot‑reloadable scripting and future UI layout DSL.[](https://n8n.io/pricing/)​
        
    - Use Lua for:
        
        - Describing UI patterns/layouts at a high level.
            
        - Prototyping tools without recompiling C++.
            
- **Tabulate**
    
    - Use inside debug/Dev Shell panels to render rich tables in ImGui (e.g. AI logs, introspection summaries) before drawing them.[](https://n8n.io/pricing/)​
        
- **ImDesignManager + ImVue**
    
    - Use for higher‑level UI composition and design system enforcement in the ImGui layer.[](https://n8n.io/pricing/)​
        
    - Ideal place to integrate your “Cockpit Doctrine” patterns and Dev Shell tools.
        
- **LInput**
    
    - Use for consistent input handling across viewports and tools, instead of raw ImGui input.[](https://n8n.io/pricing/)​
        

---

## Best practices for this codebase

1. **Keep core and AI/UI separate**
    
    - `core/`: pure data + algorithms, no ImGui/AI deps.[](https://blog.n8n.io/n8n-execution-advantage/)​
        
    - `generators/`: algorithms that consume `CitySpec` and axioms.
        
    - `AI/`: bridge, protocols, HTTP/WinHTTP clients, no direct ImGui.
        
    - `visualizer/`: ImGui UI, Dev Shell, AI console, panel orchestration.
        
2. **All AI protocols go through `AI/`**
    
    - Define C++ structs and JSON mapping in `AI/protocol/`.
        
    - Use `AI/integration/` for clients (`CitySpecClient`, `UiAgentClient`, `UiDesignClient`).
        
    - Do not call HTTP or parse AI JSON directly from `visualizer/` or `core/`.
        
3. **Use enums + magic_enum for modes and roles**
    
    - Define enums for:
        
        - Editor modes (ROAD, AXIOM, DISTRICT…).
            
        - Panel roles (INSPECTOR, TOOLBOX, VIEWPORT, LOG).
            
    - Use `magic_enum::enum_name` / `magic_enum::enum_cast` in all JSON/AI protocols, not raw strings.
        
4. **Use nlohmann_json consistently**
    
    - One `to_json/from_json` pair per public struct that crosses boundaries:
        
        - `CitySpec`, `CityIntent`, `DistrictHint`.
            
        - `UiLayout`, `UiPanelDef`, `UiActionDef`.
            
        - `UiIntrospectionSnapshot`, `UiPanelInfo`, `UiWidgetInfo`.
            
    - Avoid ad‑hoc parsing; keep everything strongly typed.
        
5. **Keep toolserver simple, stateless, and text‑only**
    
    - FastAPI + httpx + Ollama in `tools/` is purely a translator:
        
        - JSON in → text LLM → JSON out.
            
    - No persistent state or business logic; that lives in C++.
        
6. **Tests for AI layer and protocols**
    
    - Use `tests/` (already present) to:
        
        - Validate JSON schemas (round‑trip layout/introspection).
            
        - Test AI client URL parsing, error handling, and async paths.[](https://blog.n8n.io/n8n-execution-advantage/)​
            
7. **Dev Shell Mode for experimental features**
    
    - All live UI editing, introspection, and AI design tools should be gated behind Dev Shell / “Development Feature” flags, never auto‑enabled in normal runs.
## Step 1 – Implement UiIntrospectionSnapshot in C++ and JSON export

**Goal:** From a running build, press a button and dump a full, structured JSON description of the current UI.

1. **Define introspection types**
    

Create `visualizer/src/ui/introspection/UiIntrospection.h`:

cpp

`#pragma once #include <string> #include <vector> #include <nlohmann/json.hpp> namespace RogueCity::UIInt { struct UiWidgetInfo {     std::string type;         // "button","slider","property_editor","table","tree",...    std::string label;        // Visible label    std::string binding;      // "axiom.metrics.complexity" or "action:axiom.recompute"    std::vector<std::string> tags; }; struct UiPanelInfo {     std::string id;           // "rc_axiom_inspector"    std::string title;    std::string role;         // "inspector","toolbox","viewport","nav","log","settings"    std::string dockArea;     // "Left","Right","Center","Bottom","Floating"    bool visible = true;    std::string ownerModule;  // source file or module hint    std::vector<std::string> tags;    std::vector<UiWidgetInfo> widgets; }; struct DockNodeInfo {     std::string id;    std::string orientation;             // "horizontal","vertical","tab"    std::vector<std::string> panelIds;   // panels in this node (tabs or stacked)    std::vector<DockNodeInfo> children;  // nested nodes }; struct UiIntrospectionSnapshot {     std::string appName = "RogueCity Visualizer";    std::string protocolVersion = "ui-introspect-v1";    std::string snapshotId;             // e.g. timestamp or incrementing counter    std::string activeMode;             // "ROAD","AXIOM",...    std::vector<std::string> contextTags;     std::vector<UiPanelInfo> panels;    std::vector<DockNodeInfo> dockTree; }; nlohmann::json ToJson(const UiIntrospectionSnapshot& s); } // namespace RogueCity::UIInt`

Implement `ToJson` in `UiIntrospection.cpp` using nlohmann_json (you already have it).

2. **Add a UiIntrospector helper**
    

Create `UiIntrospector` as a singleton that panels can call into:

cpp

`// visualizer/src/ui/introspection/UiIntrospector.h #pragma once #include "UiIntrospection.h" namespace RogueCity::UIInt { class UiIntrospector { public:     static UiIntrospector& Instance();     void BeginFrame(const std::string& activeMode,                    const std::vector<std::string>& contextTags);     void BeginPanel(const UiPanelInfo& panelMeta);    void RegisterWidget(const UiWidgetInfo& widget);    void EndPanel();     void SetDockTree(const DockNodeInfo& root);    UiIntrospectionSnapshot BuildSnapshot() const; private:     UiIntrospector() = default;    UiIntrospectionSnapshot m_snapshot;    int m_snapshotCounter = 0;    int m_currentPanelIndex = -1; }; } // namespace RogueCity::UIInt`

In `BeginFrame`, reset `m_snapshot`, set `activeMode/contextTags`, generate a new `snapshotId` (e.g. increment, or use time). Panels call `BeginPanel` / `RegisterWidget` / `EndPanel` around their draw code.

3. **Instrument panel rendering**
    

In each panel draw function (or wrapper):

cpp

`UiPanelInfo meta; meta.id = "rc_axiom_inspector"; meta.title = "Axiom Inspector"; meta.role = "inspector"; meta.dockArea = /* derive from your docking info */; meta.visible = ImGui::IsWindowAppearing() || ImGui::IsWindowHovered() || ImGui::IsWindowFocused(); meta.ownerModule = "visualizer/src/ui/panels/rc_ui_panel_axiom_editor.cpp"; meta.tags = {"axiom","detail","editor"}; auto& introspector = RogueCity::UIInt::UiIntrospector::Instance(); introspector.BeginPanel(meta); // When drawing widgets: UiWidgetInfo w; w.type = "property_editor"; w.label = "Complexity"; w.binding = "axiom.metrics.complexity"; w.tags = {"metrics"}; introspector.RegisterWidget(w); // After panel UI: introspector.EndPanel();`

You can progressively instrument panels; start with a couple (Axiom, Road) and expand.

4. **Export snapshot to JSON**
    

In Dev Shell / AI Console, add “Export UI Snapshot”:

cpp

`auto& introspector = RogueCity::UIInt::UiIntrospector::Instance(); auto snapshot = introspector.BuildSnapshot(); nlohmann::json j = RogueCity::UIInt::ToJson(snapshot); std::ofstream out("AI/ui_snapshot.json"); out << j.dump(2);`

Now any AI can be given `AI/ui_snapshot.json` for a faithful view of the UI.

---

## Step 2 – Add /ui_design_assistant for layout/design suggestions

**Goal:** Given `UiLayout` + `UiIntrospectionSnapshot` + a goal, get back a proposal for improved layout and patterns.

1. **Reuse UiLayout + JSON mapping from earlier**
    

Use the `UiLayout` structs we defined before and add a loader for `AI/ui_layout.json`. This is your **editable layout model** (what should exist), while `UiIntrospectionSnapshot` is **what currently exists**.

2. **Define UiDesignRequest/Response (FastAPI)**
    

In `tools/toolserver.py`:

python

`from pydantic import BaseModel from typing import List, Dict, Any class UiPanelDefModel(BaseModel):     id: str    title: str    role: str    dock_area: str    visible: bool = True    owner_module: str    data_bindings: List[str] = []    tags: List[str] = [] class UiActionDefModel(BaseModel):     id: str    label: str    context_tags: List[str] = [] class UiLayoutModel(BaseModel):     panels: List[UiPanelDefModel] = []    actions: List[UiActionDefModel] = []    global_tags: List[str] = [] class UiDesignRequest(BaseModel):     current_layout: UiLayoutModel    introspection_snapshot: Dict[str, Any]    goal: str    pattern_catalog: Dict[str, Any] = {}    model: str = "qwen2.5:latest" class ComponentPatternModel(BaseModel):     name: str    template: str    applies_to: List[str]    props: List[str]    description: str class RefactorOpportunityModel(BaseModel):     name: str    priority: str    affected_panels: List[str]    suggested_action: str    rationale: str class UiDesignResponse(BaseModel):     updated_layout: UiLayoutModel    component_patterns: List[ComponentPatternModel]    refactoring_opportunities: List[RefactorOpportunityModel]    suggested_files: List[str]    summary: str`

3. **Endpoint implementation**
    

python

`DESIGN_ASSISTANT_PROMPT = """You are a UI architecture assistant for RogueCity Visualizer. You receive: - current_layout: desired panel/action structure. - introspection_snapshot: actual runtime UI (panels, widgets, tags). - pattern_catalog: known reusable UI patterns. - goal: what the developer wants. Respond with VALID JSON matching UiDesignResponse. Rules: - Keep panel IDs stable; only add new ones when necessary. - Use roles and tags consistently. - Prefer proposing patterns that reduce duplication. - Explain major changes in refactoring_opportunities and summary. """ @app.post("/ui_design_assistant", response_model=UiDesignResponse) async def ui_design_assistant(req: UiDesignRequest):     prompt = (        f"{DESIGN_ASSISTANT_PROMPT}\n\n"        f"current_layout:\n{req.current_layout.json(indent=2)}\n\n"        f"introspection_snapshot:\n{json.dumps(req.introspection_snapshot, indent=2)}\n\n"        f"pattern_catalog:\n{json.dumps(req.pattern_catalog, indent=2)}\n\n"        f"goal:\n{req.goal}\n\n"        "JSON response:"    )     ollama_url = "http://127.0.0.1:11434/api/generate"    ollama_payload = {        "model": req.model,        "prompt": prompt,        "stream": False,        "format": "json"    }     async with httpx.AsyncClient(timeout=180.0) as client:        r = await client.post(ollama_url, json=ollama_payload)        r.raise_for_status()        text = r.json().get("response", "")        data = json.loads(text)        return UiDesignResponse.model_validate(data)`

4. **Call from Dev Shell**
    

In Dev Shell Mode:

- Load `UiLayout` (`AI/ui_layout.json`) and `UiIntrospectionSnapshot` (`UIIntrospector` → JSON).
    
- Call `/ui_design_assistant` with a goal like:
    
    - “Unify all inspector panels and propose a common pattern.”
        
- Save `updated_layout` into `AI/ui_layout_proposed.json` and show a diff in a panel.
    

Your existing stack (glm, gl3w, ImDesignManager, ImVue, llinput, Lua, magic_enum, nlohmann_json, sol2, Tabulate) is already enough to:

- Serialize/inspect (`nlohmann_json`, magic_enum).
    
- Build nice tables/diffs in‑editor (`Tabulate`, ImGui).
    
- Optionally script layout transforms in Lua (`sol2`) later.
    

These two steps give you:

- A stable, AI‑friendly **UI snapshot** of “what exists.”
    
- A structured endpoint for getting **layout and pattern suggestions** against that snapshot.
## 1) What the JSON export should capture

Aim for a single export endpoint/struct like `UiIntrospectionSnapshot` with these sections:

1. **Global context**
    
    - `app_name`: `"RogueCity Visualizer"`
        
    - `protocol_version`: `"ui-introspect-v1"`
        
    - `active_mode`: `"ROAD" | "DISTRICT" | "AXIOM" | ...`
        
    - `context_tags`: `["mode:AXIOM", "selection:none", "devshell:active"]`
        
2. **Panels (what the user sees)**  
    For each visible/known panel:
    
    - `id`: `"rc_axiom_inspector"`
        
    - `title`: `"Axiom Inspector"`
        
    - `role`: `"inspector" | "toolbox" | "viewport" | "nav" | "log" | "settings"`
        
    - `dock_area`: `"Left" | "Right" | "Center" | "Bottom" | "Floating"`
        
    - `visible`: `true/false`
        
    - `owner_module`: `"visualizer/src/ui/panels/rc_ui_panel_axiom_editor.cpp"`
        
    - `tags`: `["axiom", "detail", "editor"]`
        
    - **content summary** (no raw pixels, but structure):
        
        - `widgets`: array of:
            
            - `type`: `"button" | "checkbox" | "slider" | "table" | "tree" | "property_editor"`
                
            - `label`: `"Recompute Metrics"`, `"Complexity"`, etc.
                
            - `binding`: `"axiom.metrics.complexity"` (if present)
                
            - `tags`: `["action:recompute", "metrics"]`
                
3. **Actions (verbs the UI exposes)**
    
    - `id`: `"axiom.create"`
        
    - `label`: `"Create Axiom"`
        
    - `panel_id`: `"rc_axiom_toolbar"`
        
    - `context_tags`: `["mode:AXIOM", "selection:none"]`
        
    - `handler_symbol`: `"AxiomEditor::CreateNewAxiom"` (optional hint into code)
        
4. **Layout / docking structure**
    
    - High‑level dock tree:
        
        - `dock_nodes`: each with:
            
            - `id`
                
            - `orientation`: `"horizontal" | "vertical"`
                
            - `children`: panel IDs or nested nodes
                
    - This is optional but helpful for layout reasoning.
        
5. **Pattern hints (optional, but powerful)**
    
    - `patterns_detected`: e.g.
        
        - `"InspectorPanel"` applies to `["rc_axiom_inspector", "rc_road_inspector"]`
            
        - `"ToolStrip"` applies to `["rc_axiom_toolbar", "rc_road_toolbar"]`
            

This gives any AI a **structured, declarative snapshot** of “what exists right now,” without needing to parse C++.

---

## 2) Example JSON export shape

Something like:

json

`{   "app_name": "RogueCity Visualizer",  "protocol_version": "ui-introspect-v1",  "active_mode": "AXIOM",  "context_tags": ["mode:AXIOM", "selection:none", "devshell:inactive"],   "panels": [    {      "id": "rc_axiom_inspector",      "title": "Axiom Inspector",      "role": "inspector",      "dock_area": "Right",      "visible": true,      "owner_module": "visualizer/src/ui/panels/rc_ui_panel_axiom_editor.cpp",      "tags": ["axiom", "detail", "editor"],      "widgets": [        {          "type": "property_editor",          "label": "Complexity",          "binding": "axiom.metrics.complexity",          "tags": ["metrics"]        },        {          "type": "button",          "label": "Recompute Metrics",          "binding": "action:axiom.recompute_metrics",          "tags": ["action", "metrics"]        }      ]    }  ],   "actions": [    {      "id": "axiom.create",      "label": "Create Axiom",      "panel_id": "rc_axiom_toolbar",      "context_tags": ["mode:AXIOM", "selection:none"],      "handler_symbol": "AxiomEditor::CreateNewAxiom"    }  ],   "dock_tree": {    "id": "root",    "orientation": "horizontal",    "children": [      {"panel_id": "rc_scene_viewport"},      {        "id": "right_column",        "orientation": "vertical",        "children": [          {"panel_id": "rc_axiom_inspector"},          {"panel_id": "rc_logs"}        ]      }    ]  },   "patterns_detected": [    {      "name": "InspectorPanel",      "applies_to": ["rc_axiom_inspector", "rc_road_inspector"]    }  ] }`

You export this from the running app (e.g., “Export UI Snapshot” button), and feed it to any AI with a very clear prompt: “Here is the authoritative description of the current UI.”

---

## 3) How to generate this in your app

Inside RogueCities:

1. **Instrument your panel registry**
    
    - Every panel already has:
        
        - An ID, title, and draw function.
            
    - Add metadata:
        
        - `role`, `owner_module`, `tags`.
            
    - While drawing, optionally register widgets with a simple API:
        

cpp

`UiIntrospector::BeginPanel("rc_axiom_inspector", metadata); UiIntrospector::RegisterWidget({type, label, binding, tags}); UiIntrospector::EndPanel();`

2. **Maintain a global `UiIntrospectionSnapshot` struct**
    
    - Filled each frame (or on demand) from the introspector calls and docking state.
        
    - When you hit “Export UI JSON,” you just dump that struct via nlohmann::json.
        
3. **Dock tree**
    
    - You already manage docking via ImGui; you can walk the ImGui dock nodes or your own layout structure to build a simplified `dock_tree`.
        

---

## 4) How this helps any AI

Once this JSON exists, any AI you use (local, cloud, future tool) can:

- See what panels/actions/widgets exist, how they’re grouped, and what they bind to.
    
- Propose:
    
    - “Add a new panel with these widgets and tags.”
        
    - “Move metrics from panel A to panel B.”
        
    - “Unify these three inspector panels into one pattern.”
        

You no longer ask the model to _infer_ the UI from C++; you hand it a **canonical UI manifest** and ask for **changes to that manifest** (plus optional code skeletons), which you then apply.

---