#include "config/AiConfig.h"
#include "protocol/UiAgentProtocol.h"
#include "client/CitySpecClient.h"
#include "tools/Url.h"

#include <nlohmann/json.hpp>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>

using namespace RogueCity;

struct TestResult {
    bool passed{true};
    std::string name;
    std::string error_message;
    double execution_time_ms{0.0};
};

class TestRunner {
public:
    static void runTest(const std::string& test_name, const std::function<void()>& test_func) {
        TestResult result;
        result.name = test_name;

        auto start = std::chrono::high_resolution_clock::now();
        try {
            test_func();
            result.passed = true;
        } catch (const std::exception& e) {
            result.passed = false;
            result.error_message = e.what();
        }
        auto end = std::chrono::high_resolution_clock::now();
        result.execution_time_ms =
            std::chrono::duration<double, std::milli>(end - start).count();

        printResult(result);
        results_.push_back(result);
    }

    static int printSummaryAndExitCode() {
        std::cout << "\n========================================\n";
        std::cout << "TEST SUMMARY\n";
        std::cout << "========================================\n";

        int passed = 0;
        int failed = 0;
        for (const auto& r : results_) {
            if (r.passed) passed++;
            else failed++;
        }

        std::cout << "Total tests: " << results_.size() << "\n";
        std::cout << "Passed: " << passed << "\n";
        std::cout << "Failed: " << failed << "\n";
        std::cout << "========================================\n";
        return failed == 0 ? 0 : 1;
    }

private:
    static void printResult(const TestResult& result) {
        if (result.passed) {
            std::cout << "[PASS] " << result.name << " (" << result.execution_time_ms << " ms)\n";
        } else {
            std::cout << "[FAIL] " << result.name << " - " << result.error_message << " ("
                      << result.execution_time_ms << " ms)\n";
        }
    }

    static std::vector<TestResult> results_;
};

std::vector<TestResult> TestRunner::results_;

#define TEST(name) TestRunner::runTest(#name, name)

#define ASSERT_TRUE(condition) \
    if (!(condition)) throw std::runtime_error("Assertion failed: " #condition)

#define ASSERT_EQUAL(a, b) \
    if (!((a) == (b))) throw std::runtime_error("Assertion failed: " #a " == " #b)

static void test_url_parse_http_with_port() {
    AI::ParsedUrl u = AI::ParseUrl("http://127.0.0.1:7077/ui_agent");
    ASSERT_TRUE(u.valid);
    ASSERT_TRUE(!u.secure);
    ASSERT_EQUAL(u.host, "127.0.0.1");
    ASSERT_EQUAL(u.port, 7077);
    ASSERT_EQUAL(u.path, "/ui_agent");
}

static void test_url_join_path() {
    ASSERT_EQUAL(AI::JoinUrlPath("http://127.0.0.1:7077", "/health"),
                 "http://127.0.0.1:7077/health");
    ASSERT_EQUAL(AI::JoinUrlPath("http://127.0.0.1:7077/", "/health"),
                 "http://127.0.0.1:7077/health");
    ASSERT_EQUAL(AI::JoinUrlPath("http://127.0.0.1:7077", "health"),
                 "http://127.0.0.1:7077/health");
}

static void test_cityspec_json_roundtrip() {
    Core::CitySpec spec;
    spec.intent.description = "Test City";
    spec.intent.scale = "city";
    spec.intent.climate = "temperate";
    spec.intent.styleTags = {"modern", "industrial"};
    spec.districts.push_back(Core::DistrictHint{"residential", 0.6f});
    spec.districts.push_back(Core::DistrictHint{"downtown", 0.9f});
    spec.seed = 42;
    spec.roadDensity = 0.75f;

    nlohmann::json j = AI::CitySpecClient::ToJson(spec);
    Core::CitySpec parsed = AI::CitySpecClient::FromJson(j);

    ASSERT_EQUAL(parsed.intent.description, spec.intent.description);
    ASSERT_EQUAL(parsed.intent.scale, spec.intent.scale);
    ASSERT_EQUAL(parsed.intent.climate, spec.intent.climate);
    ASSERT_EQUAL(parsed.intent.styleTags.size(), spec.intent.styleTags.size());
    ASSERT_EQUAL(parsed.districts.size(), spec.districts.size());
    ASSERT_EQUAL(parsed.seed, spec.seed);
}

static void test_uiagent_snapshot_json_contains_fields() {
    AI::UiSnapshot s;
    s.app = "TestApp";
    s.header.left = "LEFT";
    s.header.mode = "MODE";
    s.header.filter = "FILTER";
    s.dockingEnabled = true;
    s.panels.push_back({"Tools", "Left", true, "toolbox", "rc_panel_tools", {"tool.active"}, {"toolbar+canvas"}});
    s.state.flowRate = 1.0;
    s.state.livePreview = true;
    s.state.debounceSec = 0.2;
    s.state.seed = 123;
    s.state.activeTool = "AXIOM_MODE";
    s.state.state_model["axiom.selected_id"] = "A123";

    auto jsonStr = AI::UiAgentJson::SnapshotToJson(s);
    auto root = nlohmann::json::parse(jsonStr);
    ASSERT_EQUAL(root["app"].get<std::string>(), "TestApp");
    ASSERT_EQUAL(root["header"]["left"].get<std::string>(), "LEFT");
    ASSERT_TRUE(root.contains("layout"));
    ASSERT_TRUE(root["layout"].contains("panels"));
    ASSERT_TRUE(root.contains("state"));
    ASSERT_TRUE(root["state"].contains("state_model"));
}

static void test_aiconfig_load_debug_flags() {
    namespace fs = std::filesystem;
    fs::path temp = fs::temp_directory_path() / "roguecity_ai_config_test.json";
    {
        std::ofstream f(temp.string(), std::ios::binary);
        f << R"({
  "bridge_base_url": "http://localhost:7077",
  "debug_log_http": true,
  "debug_write_roundtrips": true,
  "debug_roundtrip_dir": "AI/logs_test"
})";
    }

    AI::AiConfigManager::Instance().LoadFromFile(temp.string());
    const auto& cfg = AI::AiConfigManager::Instance().GetConfig();
    ASSERT_TRUE(cfg.debugLogHttp);
    ASSERT_TRUE(cfg.debugWriteRoundtrips);
    ASSERT_EQUAL(cfg.debugRoundtripDir, std::string("AI/logs_test"));
}

// ============================================================================
// GROUP A: Core-layer vocabulary
// ============================================================================

static void test_core_cityspec_full_district_types() {
    Core::CitySpec spec;
    spec.intent.scale = "city";
    spec.districts.push_back({"residential", 0.6f});
    spec.districts.push_back({"commercial",  0.4f});
    spec.districts.push_back({"industrial",  0.3f});
    spec.districts.push_back({"civic",       0.2f});
    spec.districts.push_back({"luxury",      0.1f});

    auto j = AI::CitySpecClient::ToJson(spec);
    ASSERT_EQUAL(j["districts"].size(), std::size_t(5));

    Core::CitySpec parsed = AI::CitySpecClient::FromJson(j);
    ASSERT_EQUAL(parsed.districts.size(), std::size_t(5));
    ASSERT_EQUAL(parsed.districts[0].type, std::string("residential"));
    ASSERT_EQUAL(parsed.districts[1].type, std::string("commercial"));
    ASSERT_EQUAL(parsed.districts[2].type, std::string("industrial"));
    ASSERT_EQUAL(parsed.districts[3].type, std::string("civic"));
    ASSERT_EQUAL(parsed.districts[4].type, std::string("luxury"));
}

static void test_core_cityspec_style_tags_axiom_mapping() {
    Core::CitySpec spec;
    spec.intent.styleTags = {"organic", "grid", "radial", "hexagonal",
                             "stem", "dendritic", "spine", "pinwheel"};

    auto j = AI::CitySpecClient::ToJson(spec);
    ASSERT_EQUAL(j["intent"]["style_tags"].size(), std::size_t(8));

    Core::CitySpec parsed = AI::CitySpecClient::FromJson(j);
    ASSERT_EQUAL(parsed.intent.styleTags.size(), std::size_t(8));
    ASSERT_EQUAL(parsed.intent.styleTags[0], std::string("organic"));
    ASSERT_EQUAL(parsed.intent.styleTags[3], std::string("hexagonal"));
    ASSERT_EQUAL(parsed.intent.styleTags[7], std::string("pinwheel"));
}

static void test_core_cityspec_boundary_seed_and_density() {
    // seed = 0 must not be treated as "unset" by Gemma
    Core::CitySpec specZero;
    specZero.seed = 0;
    specZero.roadDensity = 0.0f;
    auto j0 = AI::CitySpecClient::ToJson(specZero);
    Core::CitySpec p0 = AI::CitySpecClient::FromJson(j0);
    ASSERT_EQUAL(p0.seed, uint64_t(0));
    ASSERT_TRUE(j0.contains("seed"));
    ASSERT_TRUE(j0.contains("road_density"));

    // seed = UINT64_MAX (boundary check)
    Core::CitySpec specMax;
    specMax.seed = UINT64_MAX;
    specMax.roadDensity = 1.0f;
    auto jMax = AI::CitySpecClient::ToJson(specMax);
    Core::CitySpec pMax = AI::CitySpecClient::FromJson(jMax);
    ASSERT_EQUAL(pMax.seed, UINT64_MAX);
}

// ============================================================================
// GROUP B: Generators-layer — Pipeline V2 model routing via AiConfig
// ============================================================================

static void test_generators_aiconfig_pipeline_v2_models() {
    namespace fs = std::filesystem;
    fs::path temp = fs::temp_directory_path() / "rc_ai_test_pv2_models.json";
    {
        std::ofstream f(temp.string(), std::ios::binary);
        f << R"({
  "controller_model":       "functiongemma",
  "triage_model":           "codegemma:2b",
  "synth_fast_model":       "gemma3:4b",
  "synth_escalation_model": "gemma3:12b",
  "embedding_model":        "embeddinggemma",
  "vision_model":           "granite3.2-vision",
  "ocr_model":              "glm-ocr"
})";
    }
    AI::AiConfigManager::Instance().LoadFromFile(temp.string());
    const auto& cfg = AI::AiConfigManager::Instance().GetConfig();
    ASSERT_EQUAL(cfg.controllerModel,      std::string("functiongemma"));
    ASSERT_EQUAL(cfg.triageModel,          std::string("codegemma:2b"));
    ASSERT_EQUAL(cfg.synthFastModel,       std::string("gemma3:4b"));
    ASSERT_EQUAL(cfg.synthEscalationModel, std::string("gemma3:12b"));
    ASSERT_EQUAL(cfg.embeddingModel,       std::string("embeddinggemma"));
    ASSERT_EQUAL(cfg.visionModel,          std::string("granite3.2-vision"));
    ASSERT_EQUAL(cfg.ocrModel,             std::string("glm-ocr"));
}

static void test_generators_aiconfig_pipeline_v2_flags() {
    namespace fs = std::filesystem;
    fs::path temp = fs::temp_directory_path() / "rc_ai_test_pv2_flags.json";
    {
        std::ofstream f(temp.string(), std::ios::binary);
        f << R"({
  "pipeline_v2_enabled":  true,
  "audit_strict_enabled": true,
  "embedding_dimensions": 768
})";
    }
    AI::AiConfigManager::Instance().LoadFromFile(temp.string());
    const auto& cfg = AI::AiConfigManager::Instance().GetConfig();
    ASSERT_TRUE(cfg.pipelineV2Enabled);
    ASSERT_TRUE(cfg.auditStrictEnabled);
    ASSERT_EQUAL(cfg.embeddingDimensions, 768);
}

static void test_generators_cityspec_all_scale_values() {
    const std::vector<std::string> scales = {"hamlet", "town", "city", "metro"};
    for (const auto& scale : scales) {
        Core::CitySpec spec;
        spec.intent.scale = scale;
        auto j = AI::CitySpecClient::ToJson(spec);
        Core::CitySpec parsed = AI::CitySpecClient::FromJson(j);
        ASSERT_EQUAL(parsed.intent.scale, scale);
        ASSERT_EQUAL(j["intent"]["scale"].get<std::string>(), scale);
    }
}

// ============================================================================
// GROUP C: App-layer — model vocabulary and UI state
// ============================================================================

static void test_app_aiconfig_all_model_fields() {
    namespace fs = std::filesystem;
    fs::path temp = fs::temp_directory_path() / "rc_ai_test_app_models.json";
    {
        std::ofstream f(temp.string(), std::ios::binary);
        f << R"({
  "ui_agent_model":        "gemma3:4b",
  "city_spec_model":       "gemma3:4b",
  "code_assistant_model":  "codegemma:2b",
  "naming_model":          "gemma3:4b",
  "bridge_base_url":       "http://127.0.0.1:7077"
})";
    }
    AI::AiConfigManager::Instance().LoadFromFile(temp.string());
    const auto& cfg = AI::AiConfigManager::Instance().GetConfig();
    ASSERT_EQUAL(cfg.uiAgentModel,       std::string("gemma3:4b"));
    ASSERT_EQUAL(cfg.citySpecModel,      std::string("gemma3:4b"));
    ASSERT_EQUAL(cfg.codeAssistantModel, std::string("codegemma:2b"));
    ASSERT_EQUAL(cfg.namingModel,        std::string("gemma3:4b"));
    ASSERT_EQUAL(cfg.bridgeBaseUrl,      std::string("http://127.0.0.1:7077"));
}

static void test_app_uisnapshot_state_model_cross_layer_keys() {
    AI::UiSnapshot s;
    s.app = "RogueCity Visualizer";
    // One key per major layer/subsystem — vocabulary Gemma reads for editor state
    s.state.state_model["axiom.selected_id"]        = "A007";
    s.state.state_model["road.brush_radius"]         = "15.0";
    s.state.state_model["district.selected_type"]    = "commercial";
    s.state.state_model["zoning.budget_per_capita"]  = "1200";
    s.state.state_model["simulation.seed"]           = "42";
    s.state.state_model["theme.active_name"]         = "CyberPunk";
    s.state.state_model["workspace.persona"]         = "Enterprise";

    auto root = nlohmann::json::parse(AI::UiAgentJson::SnapshotToJson(s));
    ASSERT_TRUE(root["state"].contains("state_model"));
    const auto& sm = root["state"]["state_model"];
    ASSERT_TRUE(sm.contains("axiom.selected_id"));
    ASSERT_TRUE(sm.contains("road.brush_radius"));
    ASSERT_TRUE(sm.contains("district.selected_type"));
    ASSERT_TRUE(sm.contains("zoning.budget_per_capita"));
    ASSERT_TRUE(sm.contains("simulation.seed"));
    ASSERT_TRUE(sm.contains("theme.active_name"));
    ASSERT_TRUE(sm.contains("workspace.persona"));
    ASSERT_EQUAL(sm.size(), std::size_t(7));
}

static void test_app_uisnapshot_header_modes_and_filters() {
    const std::vector<std::string> modes   = {"SOLITON", "REACTIVE", "SATELLITE"};
    const std::vector<std::string> filters = {"NORMAL", "CAUTION", "EVASION", "ALERT"};

    for (const auto& mode : modes) {
        AI::UiSnapshot s;
        s.header.mode = mode;
        auto root = nlohmann::json::parse(AI::UiAgentJson::SnapshotToJson(s));
        ASSERT_EQUAL(root["header"]["mode"].get<std::string>(), mode);
    }
    for (const auto& filter : filters) {
        AI::UiSnapshot s;
        s.header.filter = filter;
        auto root = nlohmann::json::parse(AI::UiAgentJson::SnapshotToJson(s));
        ASSERT_EQUAL(root["header"]["filter"].get<std::string>(), filter);
    }
}

// ============================================================================
// GROUP D: Visualizer-layer — panels, commands, dock positions
// ============================================================================

static void test_vis_uisnapshot_all_real_panels_and_roles() {
    AI::UiSnapshot s;
    s.app = "RogueCity Visualizer";
    // 12 canonical panels with their roles (matches rc_ui_root panel map)
    s.panels.push_back({"AxiomEditor",     "Left",   true, "toolbox",     "rc_panel_axiom_editor",    {}, {}});
    s.panels.push_back({"Inspector",       "Right",  true, "inspector",   "rc_panel_inspector",       {}, {}});
    s.panels.push_back({"SystemMap",       "Right",  true, "nav",         "rc_panel_system_map",      {}, {}});
    s.panels.push_back({"ZoningControl",   "Left",   true, "control",     "rc_panel_zoning_control",  {}, {}});
    s.panels.push_back({"RoadEditor",      "Left",   true, "control",     "rc_panel_road_editor",     {}, {}});
    s.panels.push_back({"LotControl",      "Left",   true, "control",     "rc_panel_lot_control",     {}, {}});
    s.panels.push_back({"BuildingControl", "Left",   true, "control",     "rc_panel_building_control",{}, {}});
    s.panels.push_back({"WaterControl",    "Left",   true, "control",     "rc_panel_water_control",   {}, {}});
    s.panels.push_back({"Tools",           "Top",    true, "toolbox",     "rc_panel_tools",           {}, {}});
    s.panels.push_back({"DevShell",        "Bottom", true, "diagnostics", "rc_panel_dev_shell",       {}, {}});
    s.panels.push_back({"AiConsole",       "Bottom", true, "diagnostics", "rc_panel_ai_console",      {}, {}});
    s.panels.push_back({"Workspace",       "Right",  true, "control",     "rc_panel_workspace",       {}, {}});

    auto root = nlohmann::json::parse(AI::UiAgentJson::SnapshotToJson(s));
    const auto& panels = root["layout"]["panels"];
    ASSERT_EQUAL(panels.size(), std::size_t(12));

    const std::vector<std::string> expectedRoles = {
        "toolbox", "inspector", "nav",
        "control", "control", "control", "control", "control",
        "toolbox", "diagnostics", "diagnostics", "control"
    };
    for (std::size_t i = 0; i < panels.size(); ++i) {
        ASSERT_TRUE(panels[i].contains("role"));
        ASSERT_EQUAL(panels[i]["role"].get<std::string>(), expectedRoles[i]);
    }
}

static void test_vis_uicommand_all_cmd_types_roundtrip() {
    // All 5 command types in a single JSON array
    const std::string jsonStr = R"([
      {"cmd": "SetHeader",   "mode": "REACTIVE",     "filter": "CAUTION"},
      {"cmd": "RenamePanel", "from": "OldInspector", "to": "Inspector"},
      {"cmd": "DockPanel",   "panel": "Inspector",   "targetDock": "Right", "ownDockNode": true},
      {"cmd": "SetState",    "key": "road.brush_radius", "valueNumber": 12.5},
      {"cmd": "Request",     "fields": ["axiom.selected_id","simulation.seed"]}
    ])";

    auto cmds = AI::UiAgentJson::CommandsFromJson(jsonStr);
    ASSERT_EQUAL(cmds.size(), std::size_t(5));

    ASSERT_EQUAL(cmds[0].cmd, std::string("SetHeader"));
    ASSERT_TRUE(cmds[0].mode.has_value());
    ASSERT_EQUAL(cmds[0].mode.value(), std::string("REACTIVE"));
    ASSERT_TRUE(cmds[0].filter.has_value());

    ASSERT_EQUAL(cmds[1].cmd, std::string("RenamePanel"));
    ASSERT_TRUE(cmds[1].from.has_value());
    ASSERT_EQUAL(cmds[1].from.value(), std::string("OldInspector"));
    ASSERT_TRUE(cmds[1].to.has_value());

    ASSERT_EQUAL(cmds[2].cmd, std::string("DockPanel"));
    ASSERT_TRUE(cmds[2].panel.has_value());
    ASSERT_TRUE(cmds[2].targetDock.has_value());
    ASSERT_TRUE(cmds[2].ownDockNode.has_value());
    ASSERT_TRUE(cmds[2].ownDockNode.value());

    ASSERT_EQUAL(cmds[3].cmd, std::string("SetState"));
    ASSERT_TRUE(cmds[3].key.has_value());
    ASSERT_TRUE(cmds[3].valueNumber.has_value());

    ASSERT_EQUAL(cmds[4].cmd, std::string("Request"));
    ASSERT_EQUAL(cmds[4].requestFields.size(), std::size_t(2));
    ASSERT_EQUAL(cmds[4].requestFields[0], std::string("axiom.selected_id"));
}

static void test_vis_uisnapshot_all_dock_positions() {
    AI::UiSnapshot s;
    s.panels.push_back({"PanelLeft",   "Left",   true, "", "", {}, {}});
    s.panels.push_back({"PanelRight",  "Right",  true, "", "", {}, {}});
    s.panels.push_back({"PanelBottom", "Bottom", true, "", "", {}, {}});
    s.panels.push_back({"PanelTop",    "Top",    true, "", "", {}, {}});
    s.panels.push_back({"PanelCenter", "Center", true, "", "", {}, {}});

    auto root = nlohmann::json::parse(AI::UiAgentJson::SnapshotToJson(s));
    const auto& panels = root["layout"]["panels"];
    ASSERT_EQUAL(panels.size(), std::size_t(5));

    const std::vector<std::string> expectedDocks = {"Left","Right","Bottom","Top","Center"};
    for (std::size_t i = 0; i < panels.size(); ++i) {
        ASSERT_EQUAL(panels[i]["dock"].get<std::string>(), expectedDocks[i]);
    }
}

int main() {
    // Original 5
    TEST(test_url_parse_http_with_port);
    TEST(test_url_join_path);
    TEST(test_cityspec_json_roundtrip);
    TEST(test_uiagent_snapshot_json_contains_fields);
    TEST(test_aiconfig_load_debug_flags);
    // Group A: Core-layer vocabulary
    TEST(test_core_cityspec_full_district_types);
    TEST(test_core_cityspec_style_tags_axiom_mapping);
    TEST(test_core_cityspec_boundary_seed_and_density);
    // Group B: Generators-layer — Pipeline V2 model routing
    TEST(test_generators_aiconfig_pipeline_v2_models);
    TEST(test_generators_aiconfig_pipeline_v2_flags);
    TEST(test_generators_cityspec_all_scale_values);
    // Group C: App-layer — model vocabulary and UI state
    TEST(test_app_aiconfig_all_model_fields);
    TEST(test_app_uisnapshot_state_model_cross_layer_keys);
    TEST(test_app_uisnapshot_header_modes_and_filters);
    // Group D: Visualizer-layer — panels, commands, dock positions
    TEST(test_vis_uisnapshot_all_real_panels_and_roles);
    TEST(test_vis_uicommand_all_cmd_types_roundtrip);
    TEST(test_vis_uisnapshot_all_dock_positions);
    return TestRunner::printSummaryAndExitCode();
}

