#pragma once
#include <string>
#include <atomic>
#include <mutex>

namespace RogueCity::UI {

/// UI Agent Assistant panel for interactive layout optimization AND design/refactor planning
class UiAgentPanel {
public:
    void Render();
    
private:
    // Layout commands mode
    char m_goalBuffer[512] = "Optimize layout for road editing";
    std::string m_lastResult;
    std::atomic<bool> m_processing{false};
    float m_busyTime = 0.0f;
    std::mutex m_resultMutex;
    
    // Design/refactor mode
    char m_designGoalBuffer[512] = "Analyze UI for refactoring opportunities";
    std::string m_lastDesignResult;
    std::atomic<bool> m_designProcessing{false};
    float m_designBusyTime = 0.0f;
    std::mutex m_designResultMutex;
};

} // namespace RogueCity::UI
