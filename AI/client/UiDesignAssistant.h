#pragma once
#include "protocol/UiAgentProtocol.h"
#include <string>
#include <nlohmann/json.hpp>

namespace RogueCity::AI {

/// Design/refactoring suggestions from AI (not immediate commands)
struct UiDesignSuggestion {
    std::string name;
    std::string description;
    std::string priority;  // "high", "medium", "low"
    std::vector<std::string> affected_panels;
    std::string suggested_action;
    std::string rationale;
};

/// Component pattern recommendation
struct UiComponentPattern {
    std::string name;
    std::string template_name;
    std::vector<std::string> applies_to;
    std::vector<std::string> props;
    std::string description;
};

/// Full design/refactor plan
struct UiDesignPlan {
    std::vector<UiComponentPattern> component_patterns;
    std::vector<UiDesignSuggestion> refactoring_opportunities;
    std::vector<std::string> suggested_files;
    std::string summary;
};

/// Client for AI-driven UI design and refactoring assistance
class UiDesignAssistant {
public:
    /// Generate a design/refactor plan from current UI snapshot
    /// @param snapshot Current UI state with code-shape metadata
    /// @param goal Optional specific goal (e.g. "extract common inspector pattern")
    /// @return Design plan with suggested refactorings
    static UiDesignPlan GenerateDesignPlan(
        const UiSnapshot& snapshot,
        const std::string& goal = ""
    );
    
    /// Save design plan to JSON file
    static bool SaveDesignPlan(
        const UiDesignPlan& plan,
        const std::string& filename
    );
    
    /// Load UI pattern catalog
    static nlohmann::json LoadPatternCatalog();
};

} // namespace RogueCity::AI
