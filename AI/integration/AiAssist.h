#pragma once
#include "protocol/UiAgentProtocol.h"
#include <string>
#include <vector>

namespace RogueCity::UI {

/// AI Assist integration for editor automation
/// Builds UI snapshots, queries AI agent, applies commands
class AiAssist {
public:
    /// Build current UI snapshot from editor state
    /// @return Snapshot of current UI state for AI analysis
    static AI::UiSnapshot BuildSnapshot();
    
    /// Apply AI commands to editor
    /// @param commands List of commands from AI
    /// @param snap Original snapshot (for context)
    static void ApplyCommands(const std::vector<AI::UiCommand>& commands, 
                             const AI::UiSnapshot& snap);
    
    /// Query AI agent with goal
    /// @param snap Current UI snapshot
    /// @param goal User's request (e.g. "Fix Inspector dock position")
    /// @return List of commands from AI
    static std::vector<AI::UiCommand> QueryAgent(const AI::UiSnapshot& snap,
                                                  const std::string& goal);
    
    /// Render "AI Assist" UI controls
    /// @param dt Delta time
    static void DrawControls(float dt);
    
private:
    static void ApplyCommand(const AI::UiCommand& c, const AI::UiSnapshot& snap);
};

} // namespace RogueCity::UI
