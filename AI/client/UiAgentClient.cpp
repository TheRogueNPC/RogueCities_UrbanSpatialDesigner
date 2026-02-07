#include "UiAgentClient.h"
#include "config/AiConfig.h"
#include "tools/HttpClient.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

using json = nlohmann::json;

namespace RogueCity::AI {

static std::string TimestampForFilename() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto t = system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    tm = *std::localtime(&t);
#endif
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S") << "_" << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

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

    if (config.debugWriteRoundtrips) {
        try {
            std::filesystem::create_directories(config.debugRoundtripDir);
            json log;
            log["endpoint"] = "/ui_agent";
            log["url"] = url;
            log["goal"] = goal;
            log["request"] = requestBody;
            log["response_raw"] = responseStr;
            std::string filename = config.debugRoundtripDir + "/ui_agent_" + TimestampForFilename() + ".json";
            std::ofstream f(filename, std::ios::binary);
            if (f.is_open()) f << log.dump(2);
        } catch (...) {
            // best-effort only
        }
    }
    
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
