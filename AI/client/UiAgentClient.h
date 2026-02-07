#pragma once
#include "protocol/UiAgentProtocol.h"
#include <vector>
#include <string>

namespace RogueCity::AI {

/// Client for querying the UI Agent on the toolserver
class UiAgentClient {
public:
    /// Query the UI agent with a snapshot and goal
    /// Returns list of commands to apply
    static std::vector<UiCommand> QueryAgent(
        const UiSnapshot& snapshot,
        const std::string& goal
    );
};

} // namespace RogueCity::AI
