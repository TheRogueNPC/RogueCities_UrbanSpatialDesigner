#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace RogueCity::AI {

/// AI system configuration loaded from JSON
struct AiConfig {
    std::string startScript = "tools/Start_Ai_Bridge_Fixed.ps1";
    std::string stopScript = "tools/Stop_Ai_Bridge_Fixed.ps1";
    std::string uiAgentModel = "gemma3:4b";
    std::string citySpecModel = "gemma3:4b";
    std::string codeAssistantModel = "codegemma:2b";
    std::string namingModel = "gemma3:4b";
    // Pipeline v2 model roles
    std::string controllerModel = "functiongemma";
    std::string triageModel = "codegemma:2b";
    std::string synthFastModel = "gemma3:4b";
    std::string synthEscalationModel = "gemma3:12b";
    std::string embeddingModel = "embeddinggemma";
    std::string visionModel = "granite3.2-vision";
    std::string ocrModel = "glm-ocr";
    bool pipelineV2Enabled = true;
    bool auditStrictEnabled = false;
    int embeddingDimensions = 512;
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
