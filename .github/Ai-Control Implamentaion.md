Available Tools & Constraints for This Task
Language / Runtime

C++17 or later

Target: desktop app with ImGui + docking

Build system: existing project (do not change global settings unless necessary)

Project Structure

Root repo: RogueCities_UrbanSpatialDesigner

New AI-specific code must live under root AI/ directory:

AI/protocol/ for C++ protocol types (snapshot, commands, JSON helpers)

AI/tools/ for any HTTP helper stubs (if needed)

Do not modify core/ or generator modules; keep changes in app/editor layer

UI / Editor Layer

Existing ImGui-based editor with:

Main dockspace

Panels/windows like Analytics, Inspector, Tools, AxiomLibrary, RogueNavViewport

You may:

Include AI/protocol/UiAgentProtocol.h in the main editor/dockspace file

Read editor/app state (mode, filter, panel visibility, ImGui flags, generation settings)

Call existing functions that:

Configure dockspace and panel docking

Toggle panel visibility

Update editor configuration/state (flowRate, livePreview, debounce, seed, active tool, selections)

JSON & HTTP

JSON: use existing JSON library (assume nlohmann::json is available or easy to add)

HTTP:

Preferred: reuse any existing HTTP client in the project

Otherwise: create a small HttpPostJson stub in AI/tools/ and mark clearly as TODO for later wiring

AI Backend (Already Provided)

Local Ollama server on http://127.0.0.1:11434

FastAPI toolserver on http://127.0.0.1:7077 (launched via Start_Ai_Bridge.ps1)

Toolserver will be extended to expose /ui_agent and an OpenAI-compatible /v1/chat/completions endpoint

You do not need to talk to any external cloud AI (OpenAI, etc.)

Safety / Scope

All new behavior must be:

Opt-in (triggered by an “AI Assist” button or similar)

Safe and reversible (do not persist destructive layout changes without user interaction)

If AI response is invalid or unparseable, the app should:

Ignore commands gracefully

Optionally surface a simple error/log message

Do not introduce new global state if existing editor/app state objects can be extended instead.

You are updating the RogueCities_UrbanSpatialDesigner C++/ImGui app to support an AI-driven UI protocol. Implement all of the following in one cohesive change.

Goal:
Add a minimal, app-layer-only “UI Snapshot → AI → Commands” protocol, with all AI-facing code under a new AI/ root directory.

1) Create AI root directory and protocol files
At the repo root, create:

text
AI/
  protocol/
    UiAgentProtocol.h
    UiAgentProtocol.cpp
1.1 UiAgentProtocol.h
Create AI/protocol/UiAgentProtocol.h with:

cpp
#pragma once
#include <string>
#include <vector>
#include <optional>

// ---- Snapshot types ----

struct UiPanelInfo {
    std::string id;       // e.g. "Analytics", "Inspector"
    std::string dock;     // "Left","Right","Bottom","Top","Center"
    bool visible = true;
};

struct UiHeaderInfo {
    std::string left;     // e.g. "ROGUENAV"
    std::string mode;     // e.g. "SOLITON"
    std::string filter;   // e.g. "NORMAL"
};

struct UiStateInfo {
    double flowRate = 1.0;
    bool livePreview = false;
    double debounceSec = 0.0;
    uint64_t seed = 0;
    std::string activeTool;                // enum name / id
    std::vector<std::string> selectedAxioms;
};

struct UiSnapshot {
    std::string app;                       // "RogueCity Visualizer"
    UiHeaderInfo header;
    bool dockingEnabled = true;
    bool multiViewportEnabled = false;
    std::vector<UiPanelInfo> panels;
    UiStateInfo state;
    std::vector<std::string> logTail;      // last ~20 lines
};

// ---- Command type ----

struct UiCommand {
    std::string cmd; // "SetHeader","RenamePanel","DockPanel","SetState","Request"

    // Optional fields used by various commands:
    std::optional<std::string> mode;
    std::optional<std::string> filter;

    std::optional<std::string> from;
    std::optional<std::string> to;

    std::optional<std::string> panel;
    std::optional<std::string> targetDock;
    std::optional<bool> ownDockNode;

    std::optional<std::string> key;        // for SetState
    std::optional<std::string> valueStr;
    std::optional<double> valueNumber;
    std::optional<bool> valueBool;

    std::vector<std::string> requestFields; // for {"cmd":"Request","fields":[...]}
};

// JSON helpers (implemented in .cpp)
struct UiAgentJson {
    static std::string SnapshotToJson(const UiSnapshot& s);
    static std::vector<UiCommand> CommandsFromJson(const std::string& jsonStr);
};
1.2 UiAgentProtocol.cpp
Create AI/protocol/UiAgentProtocol.cpp using nlohmann::json:

cpp
#include "UiAgentProtocol.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static json ToJson(const UiSnapshot& snap) {
    json j;
    j["app"] = snap.app;
    j["header"] = {
        {"left",   snap.header.left},
        {"mode",   snap.header.mode},
        {"filter", snap.header.filter},
    };
    j["layout"] = {
        {"dockingEnabled",       snap.dockingEnabled},
        {"multiViewportEnabled", snap.multiViewportEnabled},
    };
    json panels = json::array();
    for (const auto& p : snap.panels) {
        panels.push_back({
            {"id",      p.id},
            {"dock",    p.dock},
            {"visible", p.visible},
        });
    }
    j["layout"]["panels"] = panels;

    json state;
    state["flowRate"]       = snap.state.flowRate;
    state["livePreview"]    = snap.state.livePreview;
    state["debounceSec"]    = snap.state.debounceSec;
    state["seed"]           = snap.state.seed;
    state["activeTool"]     = snap.state.activeTool;
    state["selectedAxioms"] = snap.state.selectedAxioms;
    j["state"] = state;

    j["logTail"] = snap.logTail;
    return j;
}

static std::vector<UiCommand> UiCommandsFromJsonImpl(const json& root) {
    std::vector<UiCommand> out;
    if (!root.is_array()) return out;
    for (const auto& item : root) {
        UiCommand c;
        if (!item.contains("cmd")) continue;
        c.cmd = item["cmd"].get<std::string>();

        auto setOptStr = [&](const char* key, std::optional<std::string>& dst) {
            if (item.contains(key) && !item[key].is_null())
                dst = item[key].get<std::string>();
        };
        auto setOptBool = [&](const char* key, std::optional<bool>& dst) {
            if (item.contains(key) && !item[key].is_null())
                dst = item[key].get<bool>();
        };
        auto setOptDouble = [&](const char* key, std::optional<double>& dst) {
            if (item.contains(key) && !item[key].is_null())
                dst = item[key].get<double>();
        };

        setOptStr("mode",        c.mode);
        setOptStr("filter",      c.filter);
        setOptStr("from",        c.from);
        setOptStr("to",          c.to);
        setOptStr("panel",       c.panel);
        setOptStr("targetDock",  c.targetDock);
        setOptBool("ownDockNode", c.ownDockNode);
        setOptStr("key",         c.key);
        setOptStr("valueStr",    c.valueStr);
        setOptDouble("valueNumber", c.valueNumber);
        setOptBool("valueBool",  c.valueBool);

        if (item.contains("fields") && item["fields"].is_array()) {
            for (auto& f : item["fields"])
                c.requestFields.push_back(f.get<std::string>());
        }

        out.push_back(std::move(c));
    }
    return out;
}

// Public API

std::string UiAgentJson::SnapshotToJson(const UiSnapshot& s) {
    return ToJson(s).dump();
}

std::vector<UiCommand> UiAgentJson::CommandsFromJson(const std::string& jsonStr) {
    try {
        json root = json::parse(jsonStr);
        return UiCommandsFromJsonImpl(root);
    } catch (...) {
        return {};
    }
}
Add this directory to your build system’s include paths and sources.

2) Hook into the main ImGui editor (app layer)
Locate the C++ file that:

Builds the main dockspace

Draws windows like: Analytics, Inspector, Tools, AxiomLibrary, RogueNavViewport

In that file:

2.1 Include the protocol
cpp
#include "AI/protocol/UiAgentProtocol.h"
2.2 Wire real state into a snapshot builder
Add a function:

cpp
static UiSnapshot BuildUiSnapshot() {
    UiSnapshot s;
    s.app = "RogueCity Visualizer";

    // TODO: replace with your real mode/filter state
    s.header.left   = "ROGUENAV";
    s.header.mode   = /* current mode string */ "SOLITON";
    s.header.filter = /* current filter */ "NORMAL";

    // TODO: wire these from your actual ImGui config / app config
    s.dockingEnabled       = /* bool */ true;
    s.multiViewportEnabled = /* bool */ false;

    // TODO: Replace with real panel visibility + dock position info
    s.panels.push_back({"Analytics",        "Left",   true});
    s.panels.push_back({"Inspector",        "Left",   true});
    s.panels.push_back({"Tools",            "Bottom", true});
    s.panels.push_back({"AxiomBar",         "Right",  true});
    s.panels.push_back({"AxiomLibrary",     "Right",  true});
    s.panels.push_back({"RogueNavViewport", "Center", true});

    // TODO: wire from your real state structs
    s.state.flowRate       = /* double */ 1.0;
    s.state.livePreview    = /* bool */ true;
    s.state.debounceSec    = /* double */ 0.2;
    s.state.seed           = /* uint64_t */ 123456;
    s.state.activeTool     = /* string id */ "AXIOM_MODE_ACTIVE";
    s.state.selectedAxioms = {/* fill from selection */};

    // TODO: hook into your logging system; last ~20 lines
    s.logTail = {/* "Generator: Ready", ... */};

    return s;
}
Fill all TODOs with real state from your existing app structures, not globals if you can avoid them.

2.3 Implement command application
Add:

cpp
static void ApplyUiCommand(const UiCommand& c, const UiSnapshot& snap) {
    if (c.cmd == "SetHeader") {
        if (c.mode) {
            // TODO: set your current mode state from *c.mode
        }
        if (c.filter) {
            // TODO: set your current filter state from *c.filter
        }
    }
    else if (c.cmd == "RenamePanel") {
        if (c.from && c.to) {
            // TODO: update your internal panel/window title/ID registry
            // from *c.from to *c.to
        }
    }
    else if (c.cmd == "DockPanel") {
        if (c.panel && c.targetDock) {
            // TODO: map targetDock ("Left","Right","Bottom","Top","Center")
            // to your dockspace node IDs and move the panel accordingly.
            // Use c.ownDockNode.value_or(false) to decide if it should be its own dock node.
        }
    }
    else if (c.cmd == "SetState") {
        if (!c.key) return;
        const std::string& key = *c.key;

        if (key == "flowRate" && c.valueNumber) {
            // TODO: write into your real flowRate
        }
        else if (key == "livePreview" && c.valueBool) {
            // TODO: write into your livePreview flag
        }
        else if (key == "debounceSec" && c.valueNumber) {
            // TODO: write into your debounce
        }
        else if (key == "seed" && c.valueNumber) {
            // TODO: write into your seed
        }
        // Extend with other keys as needed.
    }
    else if (c.cmd == "Request") {
        // Optional: surface a message in UI that AI requested more info
        // based on c.requestFields
    }
    // Unknown commands: ignore safely.
}

static void ApplyUiCommands(const std::vector<UiCommand>& cmds, const UiSnapshot& snap) {
    for (const auto& c : cmds) {
        ApplyUiCommand(c, snap);
    }
}
Wire these into your existing layout/state management code (dockspace API, app config, tool selection, etc.).

3) C++ → toolserver call stub
Add a small helper in the same app file (or in AI/tools/ if you prefer):

cpp
#include <string>
#include "AI/protocol/UiAgentProtocol.h"

// TODO: implement this using your existing HTTP client library (or add one)
static std::string HttpPostJson(const std::string& url, const std::string& bodyJson);

static std::vector<UiCommand> QueryUiAgent(const UiSnapshot& snap,
                                           const std::string& goal) {
    const std::string url = "http://127.0.0.1:7077/ui_agent";

    nlohmann::json payload;
    payload["snapshot"] = nlohmann::json::parse(UiAgentJson::SnapshotToJson(snap));
    payload["goal"] = goal;

    const std::string response = HttpPostJson(url, payload.dump());
    return UiAgentJson::CommandsFromJson(response);
}
Implement HttpPostJson using whatever HTTP mechanism the project already uses (cpr, libcurl, WinHTTP, etc.). If nothing exists yet, stub it clearly and mark TODO.

4) Add /ui_agent endpoint to the existing FastAPI toolserver
Open your toolserver.py (the one already used by Start_Ai_Bridge.ps1) and:

4.1 Add imports / model
Near the top:

python
from pydantic import BaseModel
from typing import Any, Dict
import json
Add:

python
class UiAgentIn(BaseModel):
    snapshot: Dict[str, Any]
    goal: str
4.2 Add /ui_agent route
Add this FastAPI route:

python
@app.post("/ui_agent")
async def ui_agent(payload: UiAgentIn):
    """
    UI agent endpoint:
    - Receives snapshot + goal
    - Calls local model via OpenAI-compatible /v1/chat/completions
    - Returns JSON array of commands
    """
    system_msg = (
        "You are assisting a C++/ImGui city generator editor.\n"
        "You NEVER assume UI state. You ONLY use the provided UI Snapshot JSON.\n"
        "You respond with a JSON array of commands only (no extra text).\n"
        "If information is missing, emit a single command: "
        '{"cmd":"Request","fields":[...]}.\n"
        "Constraints:\n"
        "- Commands must be safe and reversible.\n"
        "- Do not invent panels, ids, or fields that are not in the snapshot.\n"
        "- Prefer minimal changes.\n"
    )

    snapshot_str = json.dumps(payload.snapshot, indent=2)
    user_msg = (
        f"Snapshot:\n{snapshot_str}\n\n"
        f"Goal: {payload.goal}\n\n"
        "Respond with ONLY a JSON array of commands, no explanation."
    )

    req = {
        "model": DEFAULT_MODEL,  # e.g. "qwen2.5:latest"
        "messages": [
            {"role": "system", "content": system_msg},
            {"role": "user",   "content": user_msg},
        ],
        "temperature": 0.2,
        "stream": False,
    }

    # Assumes you already added /v1/chat/completions in toolserver for OpenAI-compatible calls.
    async with httpx.AsyncClient(timeout=300.0) as client:
        r = await client.post("http://127.0.0.1:11434/v1/chat/completions", json=req)
        r.raise_for_status()
        data = r.json()

    content = data["choices"][0]["message"]["content"]

    try:
        commands = json.loads(content)
        if isinstance(commands, list):
            return commands
    except Exception:
        return [{"cmd": "Request", "fields": ["invalid_response_format"]}]
Adjust the internal URL if your OpenAI-compatible endpoint is served differently (/v1/chat/completions path may be on the same FastAPI app instead; point this internal call accordingly).

5) Add “AI Assist” UI entry point
In your main editor UI, add a simple trigger:

cpp
void DrawAiAssistControls() {
    static char goalBuf[256] = "Make Inspector its own dock and fix viewport border issues.";

    ImGui::InputText("AI Goal", goalBuf, sizeof(goalBuf));
    if (ImGui::Button("AI Assist Layout")) {
        UiSnapshot snap = BuildUiSnapshot();
        auto cmds = QueryUiAgent(snap, std::string(goalBuf));
        ApplyUiCommands(cmds, snap);
    }
}
Call DrawAiAssistControls() from an appropriate place (e.g., a toolbar, “AI” menu, or debug panel).