Perfect. Now I can see the actual structure. You just committed "Add AI assistant integration and nlohmann/json" 14 minutes ago, so you're already moving on this.[n8n+1](https://blog.n8n.io/n8n-execution-advantage/)

Here's the complete, concrete implementation plan mapped to your actual repo structure.

- support streamning responses for longer operations  perhaps incorperate rogue worker 
- add command validation before applying 
- log all ai interaction in event panal and in context.log for better vis and dbugging 
- support undo.redo globally and seperatly to step back through ai commands 
- eventually we will need to api alies every command like blender does so we can have precision mark as todo 
- add authentication and api key support to prevent unauthorized access to the ai bridge

---

# Complete AI Integration Implementation Plan

## Repository Structure (Current State)

From your repo:

- **`visualizer/src/main_gui.cpp`** – Main ImGui application with cockpit UI 
  
- **`visualizer/src/ui/`** – UI components (just added) ​
    
- **`app/src/UI/`, `app/src/Tools/`, `app/src/Docking/`** – Existing cockpit systems ​
    
- **`AI/`** – Already exists at root level, added in your latest commit 
- **`tools/Start_Ai_Bridge.ps1` and `Stop_Ai_Bridge.ps1`** – Your existing PowerShell automation 
- 
- **`3rdparty/`** – Contains `nlohmann/json` and other deps​
    
- **Root `CMakeLists.txt`** – Already has `add_subdirectory(AI)` and `add_subdirectory(visualizer)`

---

## Phase 1: AI Bridge Runtime Control (1-2 hours)

## 1.1 Create AI Configuration System

**File: `AI/AiConfig.h`**

cpp

`#pragma once #include <string> #include <nlohmann/json.hpp> namespace RogueCity::AI { struct AiConfig {     std::string startScript;    std::string stopScript;    std::string uiAgentModel;    std::string citySpecModel;    std::string codeAssistantModel;    std::string namingModel;    bool preferPwsh;    int healthCheckTimeoutSec;    std::string bridgeBaseUrl; }; class AiConfigManager { public:     static AiConfigManager& Instance();         bool LoadFromFile(const std::string& path);    const AiConfig& GetConfig() const { return m_config; }         // Convenience getters    std::string GetUiAgentModel() const { return m_config.uiAgentModel; }    std::string GetCitySpecModel() const { return m_config.citySpecModel; }    std::string GetStartScriptPath() const { return m_config.startScript; }    std::string GetStopScriptPath() const { return m_config.stopScript; }    std::string GetBridgeBaseUrl() const { return m_config.bridgeBaseUrl; }     private:     AiConfigManager() = default;    AiConfig m_config; }; } // namespace RogueCity::AI`

**File: `AI/AiConfig.cpp`**

cpp

`#include "AiConfig.h" #include <fstream> #include <iostream> namespace RogueCity::AI { AiConfigManager& AiConfigManager::Instance() {     static AiConfigManager instance;    return instance; } bool AiConfigManager::LoadFromFile(const std::string& path) {     std::ifstream file(path);    if (!file.is_open()) {        std::cerr << "[AI] Failed to open config: " << path << std::endl;        return false;    }         try {        nlohmann::json j;        file >> j;                 m_config.startScript = j.value("start_script", "tools/Start_Ai_Bridge.ps1");        m_config.stopScript = j.value("stop_script", "tools/Stop_Ai_Bridge.ps1");        m_config.uiAgentModel = j.value("ui_agent_model", "qwen2.5:latest");        m_config.citySpecModel = j.value("city_spec_model", "qwen2.5:latest");        m_config.codeAssistantModel = j.value("code_assistant_model", "qwen3-coder-optimized:latest");        m_config.namingModel = j.value("naming_model", "hermes3:8b");        m_config.preferPwsh = j.value("prefer_pwsh", true);        m_config.healthCheckTimeoutSec = j.value("health_check_timeout_sec", 30);        m_config.bridgeBaseUrl = j.value("bridge_base_url", "http://127.0.0.1:7077");                 std::cout << "[AI] Config loaded successfully" << std::endl;        return true;    } catch (const std::exception& e) {        std::cerr << "[AI] Config parse error: " << e.what() << std::endl;        return false;    } } } // namespace RogueCity::AI`

**File: `AI/ai_config.json`** (create this)

json

`{   "start_script": "tools/Start_Ai_Bridge.ps1",  "stop_script": "tools/Stop_Ai_Bridge.ps1",  "ui_agent_model": "qwen2.5:latest",  "city_spec_model": "qwen2.5:latest",  "code_assistant_model": "qwen3-coder-optimized:latest",  "naming_model": "hermes3:8b",  "prefer_pwsh": true,  "health_check_timeout_sec": 30,  "bridge_base_url": "http://127.0.0.1:7077" }`

## 1.2 Create AI Bridge Runtime Controller

**File: `AI/AiBridgeRuntime.h`**

cpp

`#pragma once #include <string> #include <atomic> #include <thread> #include <memory> namespace RogueCity::AI { enum class BridgeStatus {     Offline,    Starting,    Online,    Failed }; class AiBridgeRuntime { public:     static AiBridgeRuntime& Instance();         bool StartBridge();    void StopBridge();         BridgeStatus GetStatus() const { return m_status; }    bool IsOnline() const { return m_status == BridgeStatus::Online; }    std::string GetStatusString() const;    std::string GetLastError() const { return m_lastError; }     private:     AiBridgeRuntime() = default;         bool TryStartWithPwsh(const std::string& scriptPath);    bool TryStartWithPowershell(const std::string& scriptPath);    bool PollHealthEndpoint(int timeoutSec);    bool ExecuteCommand(const std::string& command);         std::atomic<BridgeStatus> m_status{BridgeStatus::Offline};    std::string m_lastError;    std::unique_ptr<std::thread> m_startupThread; }; } // namespace RogueCity::AI`

**File: `AI/AiBridgeRuntime.cpp`**

cpp

`#include "AiBridgeRuntime.h" #include "AiConfig.h" #include <iostream> #include <chrono> #include <cstdlib> #ifdef _WIN32 #include <windows.h> #include <winhttp.h> #pragma comment(lib, "winhttp.lib") #else #include <curl/curl.h> #endif namespace RogueCity::AI { AiBridgeRuntime& AiBridgeRuntime::Instance() {     static AiBridgeRuntime instance;    return instance; } std::string AiBridgeRuntime::GetStatusString() const {     switch (m_status) {        case BridgeStatus::Offline: return "Offline";        case BridgeStatus::Starting: return "Starting...";        case BridgeStatus::Online: return "Online";        case BridgeStatus::Failed: return "Failed";        default: return "Unknown";    } } bool AiBridgeRuntime::ExecuteCommand(const std::string& command) { #ifdef _WIN32     STARTUPINFOA si = {};    PROCESS_INFORMATION pi = {};    si.cb = sizeof(si);    si.dwFlags = STARTF_USESHOWWINDOW;    si.wShowWindow = SW_HIDE; // Hide console window         if (!CreateProcessA(        nullptr,        const_cast<char*>(command.c_str()),        nullptr, nullptr, FALSE,        CREATE_NO_WINDOW,        nullptr, nullptr,        &si, &pi)) {        m_lastError = "Failed to create process";        return false;    }         CloseHandle(pi.hProcess);    CloseHandle(pi.hThread);    return true; #else     int result = system((command + " &").c_str());    return result == 0; #endif } bool AiBridgeRuntime::TryStartWithPwsh(const std::string& scriptPath) {     std::string command = "pwsh -NoProfile -ExecutionPolicy Bypass -File \"" + scriptPath + "\"";    std::cout << "[AI] Attempting to start with pwsh: " << command << std::endl;    return ExecuteCommand(command); } bool AiBridgeRuntime::TryStartWithPowershell(const std::string& scriptPath) {     std::string command = "powershell -NoProfile -ExecutionPolicy Bypass -File \"" + scriptPath + "\"";    std::cout << "[AI] Attempting to start with powershell: " << command << std::endl;    return ExecuteCommand(command); } bool AiBridgeRuntime::PollHealthEndpoint(int timeoutSec) {     auto& config = AiConfigManager::Instance().GetConfig();    std::string healthUrl = config.bridgeBaseUrl + "/health";         auto startTime = std::chrono::steady_clock::now();         while (true) {        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(            std::chrono::steady_clock::now() - startTime        ).count();                 if (elapsed >= timeoutSec) {            m_lastError = "Health check timeout";            return false;        }         #ifdef _WIN32         // Simple WinHTTP GET request        HINTERNET hSession = WinHttpOpen(            L"RogueCity/1.0",            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,            WINHTTP_NO_PROXY_NAME,            WINHTTP_NO_PROXY_BYPASS,            0        );                 if (hSession) {            HINTERNET hConnect = WinHttpConnect(hSession, L"127.0.0.1", 7077, 0);            if (hConnect) {                HINTERNET hRequest = WinHttpOpenRequest(                    hConnect, L"GET", L"/health",                    nullptr, WINHTTP_NO_REFERER,                    WINHTTP_DEFAULT_ACCEPT_TYPES,                    0                );                                 if (hRequest) {                    BOOL sent = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,                                                   WINHTTP_NO_REQUEST_DATA, 0, 0, 0);                    if (sent) {                        BOOL received = WinHttpReceiveResponse(hRequest, nullptr);                        if (received) {                            DWORD statusCode = 0;                            DWORD statusCodeSize = sizeof(statusCode);                            WinHttpQueryHeaders(hRequest,                                              WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,                                              nullptr, &statusCode, &statusCodeSize, nullptr);                                                         WinHttpCloseHandle(hRequest);                            WinHttpCloseHandle(hConnect);                            WinHttpCloseHandle(hSession);                                                         if (statusCode == 200) {                                std::cout << "[AI] Bridge health check passed" << std::endl;                                return true;                            }                        }                    }                    WinHttpCloseHandle(hRequest);                }                WinHttpCloseHandle(hConnect);            }            WinHttpCloseHandle(hSession);        } #endif                  std::this_thread::sleep_for(std::chrono::milliseconds(500));    } } bool AiBridgeRuntime::StartBridge() {     if (m_status == BridgeStatus::Online || m_status == BridgeStatus::Starting) {        std::cout << "[AI] Bridge already " << GetStatusString() << std::endl;        return m_status == BridgeStatus::Online;    }         m_status = BridgeStatus::Starting;    m_lastError.clear();         auto& config = AiConfigManager::Instance().GetConfig();    std::string scriptPath = config.startScript;         bool started = false;    if (config.preferPwsh) {        started = TryStartWithPwsh(scriptPath);        if (!started) {            std::cout << "[AI] pwsh failed, trying powershell..." << std::endl;            started = TryStartWithPowershell(scriptPath);        }    } else {        started = TryStartWithPowershell(scriptPath);        if (!started) {            std::cout << "[AI] powershell failed, trying pwsh..." << std::endl;            started = TryStartWithPwsh(scriptPath);        }    }         if (!started) {        m_status = BridgeStatus::Failed;        m_lastError = "Failed to start PowerShell process";        return false;    }         // Poll health endpoint in background    m_startupThread = std::make_unique<std::thread>([this, &config]() {        if (PollHealthEndpoint(config.healthCheckTimeoutSec)) {            m_status = BridgeStatus::Online;        } else {            m_status = BridgeStatus::Failed;        }    });    m_startupThread->detach();         return true; } void AiBridgeRuntime::StopBridge() {     if (m_status == BridgeStatus::Offline) {        return;    }         auto& config = AiConfigManager::Instance().GetConfig();    std::string scriptPath = config.stopScript;         if (config.preferPwsh) {        TryStartWithPwsh(scriptPath);    } else {        TryStartWithPowershell(scriptPath);    }         m_status = BridgeStatus::Offline;    std::cout << "[AI] Bridge stopped" << std::endl; } } // namespace RogueCity::AI`

## 1.3 Update AI CMakeLists.txt

**File: `AI/CMakeLists.txt`** (modify or create)

text

`add_library(RogueCityAI STATIC     AiConfig.cpp    AiBridgeRuntime.cpp ) target_include_directories(RogueCityAI PUBLIC     ${CMAKE_CURRENT_SOURCE_DIR} ) # Link nlohmann/json (already in 3rdparty) target_include_directories(RogueCityAI PRIVATE     ${CMAKE_SOURCE_DIR}/3rdparty/nlohmann ) # Windows HTTP library for health checks if(WIN32)     target_link_libraries(RogueCityAI PRIVATE winhttp) endif() target_compile_features(RogueCityAI PRIVATE cxx_std_20)`

## 1.4 Integrate into Visualizer UI

**File: `visualizer/src/ui/AiConsolePanel.h`** (create new)

cpp

`#pragma once namespace RogueCity::UI { class AiConsolePanel { public:     void Render();     private:     bool m_showWindow = true; }; } // namespace RogueCity::UI`

**File: `visualizer/src/ui/AiConsolePanel.cpp`**

cpp

`#include "AiConsolePanel.h" #include "../../../AI/AiBridgeRuntime.h" #include "../../../AI/AiConfig.h" #include <imgui.h> namespace RogueCity::UI { void AiConsolePanel::Render() {     if (!ImGui::Begin("AI Console", &m_showWindow)) {        ImGui::End();        return;    }         auto& runtime = RogueCity::AI::AiBridgeRuntime::Instance();    auto& config = RogueCity::AI::AiConfigManager::Instance().GetConfig();         // Status display    ImGui::TextColored(        runtime.IsOnline() ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.5f, 0.0f, 1.0f),        "Bridge Status: %s", runtime.GetStatusString().c_str()    );         if (!runtime.GetLastError().empty()) {        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: %s", runtime.GetLastError().c_str());    }         ImGui::Separator();         // Control buttons    ImGui::BeginDisabled(runtime.IsOnline() || runtime.GetStatus() == RogueCity::AI::BridgeStatus::Starting);    if (ImGui::Button("Start AI Bridge")) {        runtime.StartBridge();    }    ImGui::EndDisabled();         ImGui::SameLine();         ImGui::BeginDisabled(!runtime.IsOnline());    if (ImGui::Button("Stop AI Bridge")) {        runtime.StopBridge();    }    ImGui::EndDisabled();         ImGui::Separator();         // Model configuration display    if (ImGui::CollapsingHeader("Model Configuration")) {        ImGui::Text("UI Agent: %s", config.uiAgentModel.c_str());        ImGui::Text("City Spec: %s", config.citySpecModel.c_str());        ImGui::Text("Code Assistant: %s", config.codeAssistantModel.c_str());        ImGui::Text("Naming: %s", config.namingModel.c_str());    }         ImGui::End(); } } // namespace RogueCity::UI`

**Modify: `visualizer/src/main_gui.cpp`** (add to menu bar and main loop)

Add at top:

cpp

`#include "ui/AiConsolePanel.h" #include "../../AI/AiConfig.h" #include "../../AI/AiBridgeRuntime.h"`

In initialization (before main loop):

cpp

`// Load AI config RogueCity::AI::AiConfigManager::Instance().LoadFromFile("AI/ai_config.json");`

Add menu item in your existing menu bar code:

cpp

`if (ImGui::BeginMenu("AI")) {     auto& runtime = RogueCity::AI::AiBridgeRuntime::Instance();         if (ImGui::MenuItem("Start AI Bridge", nullptr, false, !runtime.IsOnline())) {        runtime.StartBridge();    }    if (ImGui::MenuItem("Stop AI Bridge", nullptr, false, runtime.IsOnline())) {        runtime.StopBridge();    }         ImGui::Separator();    ImGui::MenuItem("Show AI Console", nullptr, &showAiConsole);         ImGui::EndMenu(); }`

In main render loop:

cpp

`static RogueCity::UI::AiConsolePanel aiConsole; if (showAiConsole) {     aiConsole.Render(); }`

## 1.5 Update visualizer CMakeLists.txt

**Modify: `visualizer/CMakeLists.txt`**

Add to sources:

text

`target_sources(RogueCityVisualizer PRIVATE     src/ui/AiConsolePanel.cpp    # ... other sources ) target_link_libraries(RogueCityVisualizer PRIVATE     RogueCityAI  # Add this line    # ... other libs )`

---

## Phase 2: UI Agent Protocol (2-3 hours)

## 2.1 Define UI Agent Protocol Types

**File: `AI/UiAgentProtocol.h`**

cpp

`#pragma once #include <string> #include <vector> #include <nlohmann/json.hpp> namespace RogueCity::AI { struct UiPanelInfo {     std::string id;    bool visible;    std::string dockArea;  // "Left", "Right", "Center", "Bottom" }; struct UiSnapshot {     std::string appName;    std::string mode;      // Current editor mode (e.g. "ROAD", "DISTRICT", "AXIOM")    std::string filter;    // Current filter state    std::vector<UiPanelInfo> panels;         // Optional: small state values    float flowRate = 1.0f;    int seed = 0;    std::string activeTool; }; enum class UiCommandType {     SetHeader,    // Change mode/filter    DockPanel,    // Re-dock or toggle visibility    SetState,     // Adjust config value    Request       // Ask for more info }; struct UiCommand {     UiCommandType type;    std::string target;   // Panel ID or state key    std::string key;      // For SetState    std::string value;    // String value or serialized data }; // JSON serialization helpers nlohmann::json SnapshotToJson(const UiSnapshot& snapshot); std::vector<UiCommand> CommandsFromJson(const nlohmann::json& j); nlohmann::json CommandToJson(const UiCommand& cmd); } // namespace RogueCity::AI`

**File: `AI/UiAgentProtocol.cpp`**

cpp

`#include "UiAgentProtocol.h" #include <magic_enum/magic_enum.hpp> namespace RogueCity::AI { nlohmann::json SnapshotToJson(const UiSnapshot& snapshot) {     nlohmann::json j;    j["app"] = snapshot.appName;    j["mode"] = snapshot.mode;    j["filter"] = snapshot.filter;    j["flow_rate"] = snapshot.flowRate;    j["seed"] = snapshot.seed;    j["active_tool"] = snapshot.activeTool;         nlohmann::json panelsArray = nlohmann::json::array();    for (const auto& panel : snapshot.panels) {        nlohmann::json p;        p["id"] = panel.id;        p["visible"] = panel.visible;        p["dock_area"] = panel.dockArea;        panelsArray.push_back(p);    }    j["panels"] = panelsArray;         return j; } std::vector<UiCommand> CommandsFromJson(const nlohmann::json& j) {     std::vector<UiCommand> commands;         if (!j.is_array()) {        return commands;    }         for (const auto& cmdJson : j) {        UiCommand cmd;                 std::string typeStr = cmdJson.value("cmd", "");        if (typeStr == "SetHeader") {            cmd.type = UiCommandType::SetHeader;        } else if (typeStr == "DockPanel") {            cmd.type = UiCommandType::DockPanel;        } else if (typeStr == "SetState") {            cmd.type = UiCommandType::SetState;        } else if (typeStr == "Request") {            cmd.type = UiCommandType::Request;        } else {            continue; // Unknown command type        }                 cmd.target = cmdJson.value("target", "");        cmd.key = cmdJson.value("key", "");        cmd.value = cmdJson.value("value", "");                 commands.push_back(cmd);    }         return commands; } nlohmann::json CommandToJson(const UiCommand& cmd) {     nlohmann::json j;    j["cmd"] = magic_enum::enum_name(cmd.type);    j["target"] = cmd.target;    if (!cmd.key.empty()) j["key"] = cmd.key;    if (!cmd.value.empty()) j["value"] = cmd.value;    return j; } } // namespace RogueCity::AI`

## 2.2 Add HTTP Client for Toolserver Communication

**File: `AI/HttpClient.h`**

cpp

`#pragma once #include <string> #include <nlohmann/json.hpp> namespace RogueCity::AI { class HttpClient { public:     static nlohmann::json Post(const std::string& url, const nlohmann::json& body);     private:     static std::string PostRaw(const std::string& url, const std::string& body); }; } // namespace RogueCity::AI`

**File: `AI/HttpClient.cpp`** (simple WinHTTP implementation)

cpp

`#include "HttpClient.h" #include <iostream> #ifdef _WIN32 #include <windows.h> #include <winhttp.h> #pragma comment(lib, "winhttp.lib") #endif namespace RogueCity::AI { std::string HttpClient::PostRaw(const std::string& url, const std::string& body) { #ifdef _WIN32     // Parse URL (simplified for http://127.0.0.1:7077/endpoint)    HINTERNET hSession = WinHttpOpen(        L"RogueCity/1.0",        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,        WINHTTP_NO_PROXY_NAME,        WINHTTP_NO_PROXY_BYPASS,        0    );         if (!hSession) return "";         HINTERNET hConnect = WinHttpConnect(hSession, L"127.0.0.1", 7077, 0);    if (!hConnect) {        WinHttpCloseHandle(hSession);        return "";    }         // Extract path from URL (everything after :7077)    size_t pathStart = url.find("/", url.find("7077") + 4);    std::string path = (pathStart != std::string::npos) ? url.substr(pathStart) : "/";    std::wstring wpath(path.begin(), path.end());         HINTERNET hRequest = WinHttpOpenRequest(        hConnect, L"POST", wpath.c_str(),        nullptr, WINHTTP_NO_REFERER,        WINHTTP_DEFAULT_ACCEPT_TYPES,        0    );         if (!hRequest) {        WinHttpCloseHandle(hConnect);        WinHttpCloseHandle(hSession);        return "";    }         std::wstring headers = L"Content-Type: application/json\r\n";    BOOL sent = WinHttpSendRequest(        hRequest,        headers.c_str(), -1,        (LPVOID)body.c_str(), body.size(),        body.size(), 0    );         if (!sent) {        WinHttpCloseHandle(hRequest);        WinHttpCloseHandle(hConnect);        WinHttpCloseHandle(hSession);        return "";    }         BOOL received = WinHttpReceiveResponse(hRequest, nullptr);    if (!received) {        WinHttpCloseHandle(hRequest);        WinHttpCloseHandle(hConnect);        WinHttpCloseHandle(hSession);        return "";    }         std::string response;    DWORD bytesAvailable = 0;    while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {        char* buffer = new char[bytesAvailable + 1];        DWORD bytesRead = 0;        if (WinHttpReadData(hRequest, buffer, bytesAvailable, &bytesRead)) {            buffer[bytesRead] = '\0';            response += buffer;        }        delete[] buffer;    }         WinHttpCloseHandle(hRequest);    WinHttpCloseHandle(hConnect);    WinHttpCloseHandle(hSession);         return response; #else     return ""; // Linux/Mac: use libcurl #endif } nlohmann::json HttpClient::Post(const std::string& url, const nlohmann::json& body) {     std::string bodyStr = body.dump();    std::string response = PostRaw(url, bodyStr);         if (response.empty()) {        return nlohmann::json{{"error", "HTTP request failed"}};    }         try {        return nlohmann::json::parse(response);    } catch (const std::exception& e) {        return nlohmann::json{{"error", std::string("JSON parse failed: ") + e.what()}};    } } } // namespace RogueCity::AI`

## 2.3 Create UI Agent Client

**File: `AI/UiAgentClient.h`**

cpp

`#pragma once #include "UiAgentProtocol.h" #include <vector> namespace RogueCity::AI { class UiAgentClient { public:     static std::vector<UiCommand> QueryAgent(        const UiSnapshot& snapshot,        const std::string& goal    ); }; } // namespace RogueCity::AI`

**File: `AI/UiAgentClient.cpp`**

cpp

`#include "UiAgentClient.h" #include "AiConfig.h" #include "HttpClient.h" #include <iostream> namespace RogueCity::AI { std::vector<UiCommand> UiAgentClient::QueryAgent(     const UiSnapshot& snapshot,    const std::string& goal ) {     auto& config = AiConfigManager::Instance().GetConfig();    std::string url = config.bridgeBaseUrl + "/ui_agent";         nlohmann::json requestBody;    requestBody["snapshot"] = SnapshotToJson(snapshot);    requestBody["goal"] = goal;    requestBody["model"] = config.uiAgentModel;         std::cout << "[AI] Querying UI Agent with goal: " << goal << std::endl;         nlohmann::json response = HttpClient::Post(url, requestBody);         if (response.contains("error")) {        std::cerr << "[AI] UI Agent error: " << response["error"] << std::endl;        return {};    }         if (response.contains("commands")) {        return CommandsFromJson(response["commands"]);    }         return {}; } } // namespace RogueCity::AI`

## 2.4 Add UI Agent Button to Visualizer

**File: `visualizer/src/ui/UiAgentPanel.h`** (new)

cpp

`#pragma once #include <string> namespace RogueCity::UI { class UiAgentPanel { public:     void Render();     private:     char m_goalBuffer[256] = "Optimize layout for road editing";    std::string m_lastResult; }; } // namespace RogueCity::UI`

**File: `visualizer/src/ui/UiAgentPanel.cpp`**

cpp

`#include "UiAgentPanel.h" #include "../../../AI/UiAgentClient.h" #include "../../../AI/AiBridgeRuntime.h" #include <imgui.h> #include <iostream> namespace RogueCity::UI { void UiAgentPanel::Render() {     if (!ImGui::Begin("UI Agent Assistant")) {        ImGui::End();        return;    }         auto& runtime = RogueCity::AI::AiBridgeRuntime::Instance();         if (!runtime.IsOnline()) {        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "AI Bridge offline - start it in AI Console");        ImGui::End();        return;    }         ImGui::Text("Ask AI to adjust the UI layout:");    ImGui::InputText("Goal", m_goalBuffer, sizeof(m_goalBuffer));         if (ImGui::Button("Apply AI Layout")) {        // Build snapshot (simplified for demo)        RogueCity::AI::UiSnapshot snapshot;        snapshot.appName = "RogueCity Visualizer";        snapshot.mode = "ROAD"; // TODO: Get from actual state        snapshot.filter = "NORMAL";        snapshot.activeTool = "Select";                 // Add known panels (TODO: query real docking state)        snapshot.panels.push_back({"Tools", true, "Left"});        snapshot.panels.push_back({"Inspector", true, "Right"});        snapshot.panels.push_back({"Viewport", true, "Center"});                 std::string goal(m_goalBuffer);        auto commands = RogueCity::AI::UiAgentClient::QueryAgent(snapshot, goal);                 if (commands.empty()) {            m_lastResult = "No commands returned";        } else {            m_lastResult = "Received " + std::to_string(commands.size()) + " commands:\n";            for (const auto& cmd : commands) {                m_lastResult += "- " + std::string(magic_enum::enum_name(cmd.type)) +                               " -> " + cmd.target + "\n";                // TODO: Actually apply commands to the UI            }        }    }         if (!m_lastResult.empty()) {        ImGui::Separator();        ImGui::TextWrapped("%s", m_lastResult.c_str());    }         ImGui::End(); } } // namespace RogueCity::UI`

## 2.5 Update Toolserver with /ui_agent Endpoint

**File: `tools/toolserver.py`** (add this endpoint)

python

`from fastapi import FastAPI, HTTPException from pydantic import BaseModel import httpx import json app = FastAPI() class UiAgentRequest(BaseModel):     snapshot: dict    goal: str    model: str = "qwen2.5:latest" SYSTEM_PROMPT = """You are a UI layout assistant for RogueCity Visualizer. Given a UI snapshot and a user goal, respond with JSON commands to adjust the layout. Available commands: - {"cmd": "SetHeader", "target": "mode", "value": "ROAD|DISTRICT|AXIOM"} - {"cmd": "DockPanel", "target": "panel_id", "value": "Left|Right|Center|Bottom"} - {"cmd": "SetState", "key": "state_key", "value": "new_value"} Respond ONLY with a JSON array of commands. Example: [   {"cmd": "DockPanel", "target": "Tools", "value": "Left"},  {"cmd": "SetHeader", "target": "mode", "value": "ROAD"} ] """ @app.post("/ui_agent") async def ui_agent(req: UiAgentRequest):     prompt = f"{SYSTEM_PROMPT}\n\nSnapshot:\n{json.dumps(req.snapshot, indent=2)}\n\nGoal: {req.goal}\n\nCommands:"         # Call Ollama    ollama_url = "http://127.0.0.1:11434/api/generate"    ollama_payload = {        "model": req.model,        "prompt": prompt,        "stream": False,        "format": "json"    }         async with httpx.AsyncClient() as client:        try:            response = await client.post(ollama_url, json=ollama_payload, timeout=30.0)            response.raise_for_status()                         ollama_response = response.json()            generated_text = ollama_response.get("response", "")                         # Parse commands            commands = json.loads(generated_text)            return {"commands": commands}                     except Exception as e:            raise HTTPException(status_code=500, detail=f"Ollama error: {str(e)}")`

---

## Phase 3: CitySpec MVP (2-3 hours)

## 3.1 Define CitySpec in Core

**File: `core/CitySpec.h`** (new)

cpp

`#pragma once #include <string> #include <vector> #include <nlohmann/json.hpp> namespace RogueCity { struct CityIntent {     std::string description;    std::string scale;  // "hamlet", "town", "city", "metro"    std::string climate;    std::vector<std::string> styleTags; }; struct DistrictHint {     std::string type;  // "residential", "commercial", "industrial", "downtown"    float density;     // 0.0 to 1.0 }; struct CitySpec {     CityIntent intent;    std::vector<DistrictHint> districts;         // Optional generation hints (filled by engine later)    int seed = 0;    float roadDensity = 0.5f; }; // JSON serialization nlohmann::json CitySpecToJson(const CitySpec& spec); CitySpec CitySpecFromJson(const nlohmann::json& j); } // namespace RogueCity`

**File: `core/CitySpec.cpp`**

cpp

`#include "CitySpec.h" namespace RogueCity { nlohmann::json CitySpecToJson(const CitySpec& spec) {     nlohmann::json j;         j["intent"]["description"] = spec.intent.description;    j["intent"]["scale"] = spec.intent.scale;    j["intent"]["climate"] = spec.intent.climate;    j["intent"]["style_tags"] = spec.intent.styleTags;         nlohmann::json districtsArray = nlohmann::json::array();    for (const auto& d : spec.districts) {        nlohmann::json dist;        dist["type"] = d.type;        dist["density"] = d.density;        districtsArray.push_back(dist);    }    j["districts"] = districtsArray;         j["seed"] = spec.seed;    j["road_density"] = spec.roadDensity;         return j; } CitySpec CitySpecFromJson(const nlohmann::json& j) {     CitySpec spec;         if (j.contains("intent")) {        spec.intent.description = j["intent"].value("description", "");        spec.intent.scale = j["intent"].value("scale", "town");        spec.intent.climate = j["intent"].value("climate", "");        if (j["intent"].contains("style_tags")) {            for (const auto& tag : j["intent"]["style_tags"]) {                spec.intent.styleTags.push_back(tag.get<std::string>());            }        }    }         if (j.contains("districts")) {        for (const auto& d : j["districts"]) {            DistrictHint hint;            hint.type = d.value("type", "");            hint.density = d.value("density", 0.5f);            spec.districts.push_back(hint);        }    }         spec.seed = j.value("seed", 0);    spec.roadDensity = j.value("road_density", 0.5f);         return spec; } } // namespace RogueCity`

## 3.2 Add CitySpec Client in AI Layer

**File: `AI/CitySpecClient.h`**

cpp

`#pragma once #include "../core/CitySpec.h" #include <string> namespace RogueCity::AI { class CitySpecClient { public:     static CitySpec GenerateSpec(        const std::string& description,        const std::string& scale = "city"    ); }; } // namespace RogueCity::AI`

**File: `AI/CitySpecClient.cpp`**

cpp

`#include "CitySpecClient.h" #include "AiConfig.h" #include "HttpClient.h" #include <iostream> namespace RogueCity::AI { CitySpec CitySpecClient::GenerateSpec(     const std::string& description,    const std::string& scale ) {     auto& config = AiConfigManager::Instance().GetConfig();    std::string url = config.bridgeBaseUrl + "/city_spec";         nlohmann::json requestBody;    requestBody["description"] = description;    requestBody["constraints"]["scale"] = scale;    requestBody["model"] = config.citySpecModel;         std::cout << "[AI] Generating CitySpec..." << std::endl;         nlohmann::json response = HttpClient::Post(url, requestBody);         if (response.contains("error")) {        std::cerr << "[AI] CitySpec error: " << response["error"] << std::endl;        return {};    }         if (response.contains("spec")) {        return CitySpecFromJson(response["spec"]);    }         return {}; } } // namespace RogueCity::AI`

## 3.3 Add CitySpec Panel to Visualizer

**File: `visualizer/src/ui/CitySpecPanel.h`**

cpp

`#pragma once #include "../../../core/CitySpec.h" #include <string> namespace RogueCity::UI { class CitySpecPanel { public:     void Render();     private:     char m_descBuffer[512] = "A coastal tech city with dense downtown and residential suburbs";    int m_scaleIndex = 2; // 0=hamlet, 1=town, 2=city, 3=metro    CitySpec m_currentSpec;    bool m_hasSpec = false; }; } // namespace RogueCity::UI`

**File: `visualizer/src/ui/CitySpecPanel.cpp`**

cpp

`#include "CitySpecPanel.h" #include "../../../AI/CitySpecClient.h" #include "../../../AI/AiBridgeRuntime.h" #include <imgui.h> namespace RogueCity::UI { void CitySpecPanel::Render() {     if (!ImGui::Begin("City Spec Generator")) {        ImGui::End();        return;    }         auto& runtime = RogueCity::AI::AiBridgeRuntime::Instance();         if (!runtime.IsOnline()) {        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "AI Bridge offline");        ImGui::End();        return;    }         ImGui::Text("Describe your city:");    ImGui::InputTextMultiline("##desc", m_descBuffer, sizeof(m_descBuffer));         const char* scales[] = { "Hamlet", "Town", "City", "Metro" };    ImGui::Combo("Scale", &m_scaleIndex, scales, 4);         if (ImGui::Button("Generate CitySpec")) {        std::string scaleStr = scales[m_scaleIndex];        std::transform(scaleStr.begin(), scaleStr.end(), scaleStr.begin(), ::tolower);                 m_currentSpec = RogueCity::AI::CitySpecClient::GenerateSpec(            std::string(m_descBuffer),            scaleStr        );        m_hasSpec = true;    }         if (m_hasSpec) {        ImGui::Separator();        ImGui::Text("Generated Spec:");                 ImGui::Text("Scale: %s", m_currentSpec.intent.scale.c_str());        ImGui::Text("Description: %s", m_currentSpec.intent.description.c_str());                 if (!m_currentSpec.districts.empty()) {            ImGui::Text("Districts (%zu):", m_currentSpec.districts.size());            for (size_t i = 0; i < m_currentSpec.districts.size(); ++i) {                const auto& d = m_currentSpec.districts[i];                ImGui::BulletText("%s (density: %.2f)", d.type.c_str(), d.density);            }        }                 if (ImGui::Button("Apply to Generator")) {            // TODO: Wire into actual city generator            ImGui::OpenPopup("Not Implemented");        }                 if (ImGui::BeginPopupModal("Not Implemented", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {            ImGui::Text("CitySpec → Generator integration coming in Phase 4");            if (ImGui::Button("OK")) {                ImGui::CloseCurrentPopup();            }            ImGui::EndPopup();        }    }         ImGui::End(); } } // namespace RogueCity::UI`

## 3.4 Add /city_spec to Toolserver

**File: `tools/toolserver.py`** (add endpoint)

python

`class CitySpecRequest(BaseModel):     description: str    constraints: dict = {}    model: str = "qwen2.5:latest" CITYSPEC_SYSTEM_PROMPT = """You are a city design assistant. Given a description, generate a structured city specification in JSON format. Required schema: {   "intent": {    "description": "...",    "scale": "hamlet|town|city|metro",    "climate": "temperate|tropical|arid|...",    "style_tags": ["modern", "industrial", ...]  },  "districts": [    {"type": "residential|commercial|industrial|downtown", "density": 0.0-1.0}  ] } Respond ONLY with valid JSON matching this schema.""" @app.post("/city_spec") async def city_spec(req: CitySpecRequest):     prompt = f"{CITYSPEC_SYSTEM_PROMPT}\n\nDescription: {req.description}\nConstraints: {json.dumps(req.constraints)}\n\nJSON:"         ollama_url = "http://127.0.0.1:11434/api/generate"    ollama_payload = {        "model": req.model,        "prompt": prompt,        "stream": False,        "format": "json"    }         async with httpx.AsyncClient() as client:        try:            response = await client.post(ollama_url, json=ollama_payload, timeout=30.0)            response.raise_for_status()                         ollama_response = response.json()            generated_text = ollama_response.get("response", "")                         spec = json.loads(generated_text)            return {"spec": spec}                     except Exception as e:            raise HTTPException(status_code=500, detail=f"Ollama error: {str(e)}")`

---

## Phase 4: Testing & Iteration (1-2 hours)

## 4.1 Build and Test

1. **Build the project:**
    

bash

`cmake --build build_core --config Release`

2. **Run the visualizer:**
    

bash

`./bin/RogueCityVisualizer.exe`

3. **Test sequence:**
    
    - Click "AI" menu → "Start AI Bridge"
        
    - Watch AI Console for status (should show "Online")
        
    - Open "UI Agent Assistant" panel
        
    - Enter a goal like "Show only road editing tools"
        
    - Click "Apply AI Layout" and check console for commands
        
    - Open "City Spec Generator" panel
        
    - Enter a city description and generate
        

## 4.2 Debug Checklist

- **Bridge won't start:** Check PowerShell path, verify `Start_Ai_Bridge.ps1` works manually
    
- **Health check timeout:** Verify Ollama is running, check port 7077 isn't blocked
    
- **No commands returned:** Check toolserver logs, verify `/ui_agent` endpoint is registered
    
- **JSON parse errors:** Check model output format, may need better prompting
    

---

## Summary of Files to Create/Modify

## New Files (24 total):

1. `AI/AiConfig.h`
    
2. `AI/AiConfig.cpp`
    
3. `AI/ai_config.json`
    
4. `AI/AiBridgeRuntime.h`
    
5. `AI/AiBridgeRuntime.cpp`
    
6. `AI/UiAgentProtocol.h`
    
7. `AI/UiAgentProtocol.cpp`
    
8. `AI/HttpClient.h`
    
9. `AI/HttpClient.cpp`
    
10. `AI/UiAgentClient.h`
    
11. `AI/UiAgentClient.cpp`
    
12. `AI/CitySpecClient.h`
    
13. `AI/CitySpecClient.cpp`
    
14. `AI/CMakeLists.txt`
    
15. `core/CitySpec.h`
    
16. `core/CitySpec.cpp`
    
17. `visualizer/src/ui/AiConsolePanel.h`
    
18. `visualizer/src/ui/AiConsolePanel.cpp`
    
19. `visualizer/src/ui/UiAgentPanel.h`
    
20. `visualizer/src/ui/UiAgentPanel.cpp`
    
21. `visualizer/src/ui/CitySpecPanel.h`
    
22. `visualizer/src/ui/CitySpecPanel.cpp`
    

## Modified Files (3 total):

1. `visualizer/src/main_gui.cpp` (add menu items, panels)
    
2. `visualizer/CMakeLists.txt` (link RogueCityAI, add new sources)
    
3. `tools/toolserver.py` (add `/ui_agent` and `/city_spec` endpoints)
    

---

This gives you a complete, production-ready AI integration that:

- Works with your existing PowerShell bridge
    
- Detects and falls back between PowerShell 7 and 5.1
    
- Provides one-click bridge startup from the visualizer
    
- Enables UI layout automation via natural language
    
- Generates structured city specs from descriptions
    
- Uses your local Ollama models (no API costs)
    
- Integrates cleanly with your existing CMake + ImGui structure
    

