#pragma once
#include <string>

namespace RogueCity::UI {

/// UI Agent Assistant panel for interactive layout optimization AND design/refactor planning
class UiAgentPanel {
public:
    void Render();
    
private:
    // Layout commands mode
    char m_goalBuffer[512] = "Optimize layout for road editing";
    std::string m_lastResult;
    bool m_processing = false;
    
    // Design/refactor mode
    char m_designGoalBuffer[512] = "Analyze UI for refactoring opportunities";
    std::string m_lastDesignResult;
    bool m_designProcessing = false;
};

} // namespace RogueCity::UI
