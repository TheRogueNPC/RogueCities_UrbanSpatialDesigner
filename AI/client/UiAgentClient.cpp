#include "UiAgentClient.h"
#include "config/AiConfig.h"
#include "tools/HttpClient.h"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

namespace RogueCity::AI {

std::vector<UiCommand> UiAgentClient::QueryAgent(
    const UiSnapshot& snapshot,
    const std::string& goal
) {
    auto& config = AiConfigManager::Instance().GetConfig();
    std::string url = config.bridgeBaseUrl + "/ui_agent";
    
    // Build request payload
    json requestBody;
    requestBody["snapshot"] = json::parse(UiAgentJson::SnapshotToJson(snapshot));
    requestBody["goal"] = goal;
    requestBody["model"] = config.uiAgentModel;
    
    std::cout << "[AI] Querying UI Agent with goal: " << goal << std::endl;
    
    // Call toolserver
    std::string responseStr = HttpClient::PostJson(url, requestBody.dump());
    
    if (responseStr.empty() || responseStr == "[]") {
        std::cerr << "[AI] Empty or error response from UI Agent" << std::endl;
        return {};
    }
    
    try {
        json response = json::parse(responseStr);
        
        if (response.contains("error")) {
            std::cerr << "[AI] UI Agent error: " << response["error"] << std::endl;
            return {};
        }
        
        if (response.contains("commands")) {
            return UiAgentJson::CommandsFromJson(response["commands"].dump());
        }
        
        // If response is directly an array of commands
        if (response.is_array()) {
            return UiAgentJson::CommandsFromJson(responseStr);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[AI] Failed to parse UI Agent response: " << e.what() << std::endl;
    }
    
    return {};
}

} // namespace RogueCity::AI
