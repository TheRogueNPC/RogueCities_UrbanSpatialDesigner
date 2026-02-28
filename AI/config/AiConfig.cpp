#include "AiConfig.h"
#include <fstream>
#include <iostream>

namespace RogueCity::AI {

AiConfigManager& AiConfigManager::Instance() {
    static AiConfigManager instance;
    return instance;
}

bool AiConfigManager::LoadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[AI] Failed to open config: " << path << std::endl;
        std::cerr << "[AI] Using default configuration" << std::endl;
        return false;
    }
    
    try {
        nlohmann::json j;
        file >> j;
        
        m_config.startScript = j.value("start_script", "tools/Start_Ai_Bridge_Fixed.ps1");
        m_config.stopScript = j.value("stop_script", "tools/Stop_Ai_Bridge_Fixed.ps1");
        m_config.uiAgentModel = j.value("ui_agent_model", "gemma3:4b");
        m_config.citySpecModel = j.value("city_spec_model", "gemma3:4b");
        m_config.codeAssistantModel = j.value("code_assistant_model", "codegemma:2b");
        m_config.namingModel = j.value("naming_model", "gemma3:4b");
        m_config.controllerModel = j.value("controller_model", "functiongemma");
        m_config.triageModel = j.value("triage_model", "codegemma:2b");
        m_config.synthFastModel = j.value("synth_fast_model", "gemma3:4b");
        m_config.synthEscalationModel = j.value("synth_escalation_model", "gemma3:12b");
        m_config.embeddingModel = j.value("embedding_model", "embeddinggemma");
        m_config.visionModel = j.value("vision_model", "granite3.2-vision");
        m_config.ocrModel = j.value("ocr_model", "glm-ocr");
        m_config.pipelineV2Enabled = j.value("pipeline_v2_enabled", true);
        m_config.auditStrictEnabled = j.value("audit_strict_enabled", false);
        m_config.embeddingDimensions = j.value("embedding_dimensions", 512);
        m_config.preferPwsh = j.value("prefer_pwsh", true);
        m_config.healthCheckTimeoutSec = j.value("health_check_timeout_sec", 30);
        m_config.bridgeBaseUrl = j.value("bridge_base_url", "http://127.0.0.1:7077");

        // Debug / diagnostics (optional)
        m_config.debugLogHttp = j.value("debug_log_http", false);
        m_config.debugWriteRoundtrips = j.value("debug_write_roundtrips", false);
        m_config.debugRoundtripDir = j.value("debug_roundtrip_dir", "AI/logs");
        
        std::cout << "[AI] Config loaded successfully from " << path << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[AI] Config parse error: " << e.what() << std::endl;
        std::cerr << "[AI] Using default configuration" << std::endl;
        return false;
    }
}

} // namespace RogueCity::AI
