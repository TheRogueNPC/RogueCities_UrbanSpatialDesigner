#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace RogueCity::AI {

/// AI system configuration loaded from JSON
struct AiConfig {
    std::string startScript = "tools/Start_Ai_Bridge_Fixed.ps1";
    std::string stopScript = "tools/Stop_Ai_Bridge_Fixed.ps1";
    std::string uiAgentModel = "deepseek-coder-v2:16b";
    std::string citySpecModel = "deepseek-coder-v2:16b";
    std::string codeAssistantModel = "deepseek-coder-v2:16b";
    std::string namingModel = "deepseek-coder-v2:16b";
    bool preferPwsh = true;
    int healthCheckTimeoutSec = 30;
    std::string bridgeBaseUrl = "http://127.0.0.1:7077";

    // Debug / diagnostics
    bool debugLogHttp = false;              // Log HTTP request/response summaries to stdout/stderr
    bool debugWriteRoundtrips = false;      // Write request/response payloads to disk (AI/logs)
    std::string debugRoundtripDir = "AI/logs";
};

/// Singleton configuration manager
class AiConfigManager {
public:
    static AiConfigManager& Instance();
    
    bool LoadFromFile(const std::string& path);
    const AiConfig& GetConfig() const { return m_config; }
    
    // Convenience getters
    std::string GetUiAgentModel() const { return m_config.uiAgentModel; }
    std::string GetCitySpecModel() const { return m_config.citySpecModel; }
    std::string GetStartScriptPath() const { return m_config.startScript; }
    std::string GetStopScriptPath() const { return m_config.stopScript; }
    std::string GetBridgeBaseUrl() const { return m_config.bridgeBaseUrl; }
    
private:
    AiConfigManager() = default;
    AiConfig m_config;
};

} // namespace RogueCity::AI
