#include "UiDesignAssistant.h"
#include "config/AiConfig.h"
#include "tools/HttpClient.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

// Suppress warnings from third-party headers
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 26819) // Unannotated fallthrough
#endif

using json = nlohmann::json;

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace RogueCity::AI {

nlohmann::json UiDesignAssistant::LoadPatternCatalog() {
    std::ifstream file("AI/docs/ui/ui_patterns.json");
    if (!file.is_open()) {
        std::cerr << "[UiDesignAssistant] Failed to open ui_patterns.json" << std::endl;
        return json::object();
    }
    
    try {
        json catalog;
        file >> catalog;
        return catalog;
    } catch (const std::exception& e) {
        std::cerr << "[UiDesignAssistant] Failed to parse catalog: " << e.what() << std::endl;
        return json::object();
    }
}

UiDesignPlan UiDesignAssistant::GenerateDesignPlan(
    const UiSnapshot& snapshot,
    const std::string& goal
) {
    UiDesignPlan plan;

#ifndef RC_FEATURE_AI_BRIDGE
    (void)snapshot;
    (void)goal;
    return plan;
#else
    auto& config = AiConfigManager::Instance().GetConfig();
    std::string url = config.bridgeBaseUrl + "/ui_design_assistant";

    // Load pattern catalog
    json catalog = LoadPatternCatalog();

    // Build request payload
    json requestBody;
    requestBody["snapshot"] = json::parse(UiAgentJson::SnapshotToJson(snapshot));
    requestBody["pattern_catalog"] = catalog;
    requestBody["goal"] = goal.empty() ? "Analyze UI for refactoring opportunities" : goal;
    requestBody["model"] = config.uiAgentModel;

    std::cout << "[UiDesignAssistant] Generating design plan..." << std::endl;
    if (!goal.empty()) {
        std::cout << "[UiDesignAssistant] Goal: " << goal << std::endl;
    }

    // Call toolserver
    std::string responseStr = HttpClient::PostJson(url, requestBody.dump());

    if (responseStr.empty() || responseStr == "[]") {
        std::cerr << "[UiDesignAssistant] Empty response from design assistant" << std::endl;
        return plan;
    }
    
    try {
        json response = json::parse(responseStr);
        
        if (response.contains("error")) {
            std::cerr << "[UiDesignAssistant] Error: " << response["error"] << std::endl;
            return plan;
        }
        
        // Parse component patterns
        if (response.contains("component_patterns")) {
            for (const auto& p : response["component_patterns"]) {
                UiComponentPattern pattern;
                pattern.name = p.value("name", "");
                pattern.template_name = p.value("template", "");
                pattern.description = p.value("description", "");
                
                if (p.contains("applies_to")) {
                    for (const auto& id : p["applies_to"]) {
                        pattern.applies_to.push_back(id.get<std::string>());
                    }
                }
                
                if (p.contains("props")) {
                    for (const auto& prop : p["props"]) {
                        pattern.props.push_back(prop.get<std::string>());
                    }
                }
                
                plan.component_patterns.push_back(pattern);
            }
        }
        
        // Parse refactoring opportunities
        if (response.contains("refactoring_opportunities")) {
            for (const auto& r : response["refactoring_opportunities"]) {
                UiDesignSuggestion suggestion;
                suggestion.name = r.value("name", "");
                suggestion.description = r.value("description", "");
                suggestion.priority = r.value("priority", "medium");
                suggestion.suggested_action = r.value("suggested_action", "");
                suggestion.rationale = r.value("rationale", "");
                
                if (r.contains("affected_panels")) {
                    for (const auto& panel : r["affected_panels"]) {
                        suggestion.affected_panels.push_back(panel.get<std::string>());
                    }
                }
                
                plan.refactoring_opportunities.push_back(suggestion);
            }
        }
        
        // Parse suggested files
        if (response.contains("suggested_files")) {
            for (const auto& file : response["suggested_files"]) {
                plan.suggested_files.push_back(file.get<std::string>());
            }
        }
        
        plan.summary = response.value("summary", "");
        
        std::cout << "[UiDesignAssistant] Generated " << plan.component_patterns.size() 
                  << " patterns, " << plan.refactoring_opportunities.size() 
                  << " refactoring opportunities" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "[UiDesignAssistant] Failed to parse response: " << e.what() << std::endl;
    }

    return plan;
#endif // RC_FEATURE_AI_BRIDGE
}

UiDesignPlan UiDesignAssistant::GenerateDesignPlan(
    const nlohmann::json& introspectionSnapshot,
    const std::string& goal
) {
    UiDesignPlan plan;

#ifndef RC_FEATURE_AI_BRIDGE
    (void)introspectionSnapshot;
    (void)goal;
    return plan;
#else
    auto& config = AiConfigManager::Instance().GetConfig();
    std::string url = config.bridgeBaseUrl + "/ui_design_assistant";

    // Load pattern catalog
    json catalog = LoadPatternCatalog();

    // Build request payload
    json requestBody;
    requestBody["snapshot"] = introspectionSnapshot;
    requestBody["introspection_snapshot"] = introspectionSnapshot;
    requestBody["pattern_catalog"] = catalog;
    requestBody["goal"] = goal.empty() ? "Analyze UI for refactoring opportunities" : goal;
    requestBody["model"] = config.uiAgentModel;

    std::cout << "[UiDesignAssistant] Generating design plan from introspection snapshot..." << std::endl;
    if (!goal.empty()) {
        std::cout << "[UiDesignAssistant] Goal: " << goal << std::endl;
    }

    std::string responseStr = HttpClient::PostJson(url, requestBody.dump());

    if (responseStr.empty() || responseStr == "[]") {
        std::cerr << "[UiDesignAssistant] Empty response from design assistant" << std::endl;
        return plan;
    }

    try {
        json response = json::parse(responseStr);

        if (response.contains("error")) {
            std::cerr << "[UiDesignAssistant] Error: " << response["error"] << std::endl;
            return plan;
        }

        if (response.contains("component_patterns")) {
            for (const auto& p : response["component_patterns"]) {
                UiComponentPattern pattern;
                pattern.name = p.value("name", "");
                pattern.template_name = p.value("template", "");
                pattern.description = p.value("description", "");

                if (p.contains("applies_to")) {
                    for (const auto& id : p["applies_to"]) {
                        pattern.applies_to.push_back(id.get<std::string>());
                    }
                }

                if (p.contains("props")) {
                    for (const auto& prop : p["props"]) {
                        pattern.props.push_back(prop.get<std::string>());
                    }
                }

                plan.component_patterns.push_back(pattern);
            }
        }

        if (response.contains("refactoring_opportunities")) {
            for (const auto& r : response["refactoring_opportunities"]) {
                UiDesignSuggestion suggestion;
                suggestion.name = r.value("name", "");
                suggestion.description = r.value("description", "");
                suggestion.priority = r.value("priority", "medium");
                suggestion.suggested_action = r.value("suggested_action", "");
                suggestion.rationale = r.value("rationale", "");

                if (r.contains("affected_panels")) {
                    for (const auto& panel : r["affected_panels"]) {
                        suggestion.affected_panels.push_back(panel.get<std::string>());
                    }
                }

                plan.refactoring_opportunities.push_back(suggestion);
            }
        }

        if (response.contains("suggested_files")) {
            for (const auto& file : response["suggested_files"]) {
                plan.suggested_files.push_back(file.get<std::string>());
            }
        }

        plan.summary = response.value("summary", "");

        std::cout << "[UiDesignAssistant] Generated " << plan.component_patterns.size()
                  << " patterns, " << plan.refactoring_opportunities.size()
                  << " refactoring opportunities" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[UiDesignAssistant] Failed to parse response: " << e.what() << std::endl;
    }

    return plan;
#endif // RC_FEATURE_AI_BRIDGE
}

bool UiDesignAssistant::SaveDesignPlan(
    const UiDesignPlan& plan,
    const std::string& filename
) {
    json j;
    
    // Serialize component patterns
    json patterns = json::array();
    for (const auto& p : plan.component_patterns) {
        json pattern;
        pattern["name"] = p.name;
        pattern["template"] = p.template_name;
        pattern["description"] = p.description;
        pattern["applies_to"] = p.applies_to;
        pattern["props"] = p.props;
        patterns.push_back(pattern);
    }
    j["component_patterns"] = patterns;
    
    // Serialize refactoring opportunities
    json refactorings = json::array();
    for (const auto& r : plan.refactoring_opportunities) {
        json refactor;
        refactor["name"] = r.name;
        refactor["description"] = r.description;
        refactor["priority"] = r.priority;
        refactor["affected_panels"] = r.affected_panels;
        refactor["suggested_action"] = r.suggested_action;
        refactor["rationale"] = r.rationale;
        refactorings.push_back(refactor);
    }
    j["refactoring_opportunities"] = refactorings;
    
    j["suggested_files"] = plan.suggested_files;
    j["summary"] = plan.summary;
    
    // Add timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    std::tm tm_buf{};
#if defined(_MSC_VER)
    localtime_s(&tm_buf, &time_t);
#else
    tm_buf = *std::localtime(&time_t);
#endif
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    j["generated_at"] = ss.str();
    
    // Write to file
    try {
        std::filesystem::path path(filename);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }
    } catch (...) {
        // best-effort; directory creation failure will be handled by file open below
    }

    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[UiDesignAssistant] Failed to open file for writing: " << filename << std::endl;
        return false;
    }
    
    file << j.dump(2);  // Pretty print with 2-space indent
    
    std::cout << "[UiDesignAssistant] Saved design plan to " << filename << std::endl;
    return true;
}

} // namespace RogueCity::AI
