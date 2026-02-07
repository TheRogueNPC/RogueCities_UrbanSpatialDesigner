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
    ASSERT_EQUAL(cfg.debugRoundtripDir, "AI/logs_test");
}

int main() {
    TEST(test_url_parse_http_with_port);
    TEST(test_url_join_path);
    TEST(test_cityspec_json_roundtrip);
    TEST(test_uiagent_snapshot_json_contains_fields);
    TEST(test_aiconfig_load_debug_flags);
    return TestRunner::printSummaryAndExitCode();
}

