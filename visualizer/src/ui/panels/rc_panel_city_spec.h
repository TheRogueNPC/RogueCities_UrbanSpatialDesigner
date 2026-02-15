#pragma once
#include "RogueCity/Core/Data/CitySpec.hpp"
#include <string>
#include <atomic>
#include <mutex>

namespace RogueCity::UI {

/// CitySpec Generator panel for AI-driven city design
class CitySpecPanel {
public:
    void Render();
    void RenderContent();
    
private:
    char m_descBuffer[512] = "A coastal tech city with dense downtown and residential suburbs";
    int m_scaleIndex = 2; // 0=hamlet, 1=town, 2=city, 3=metro
    Core::CitySpec m_currentSpec;
    bool m_hasSpec = false;
    std::atomic<bool> m_processing{false};
    float m_busyTime = 0.0f;
    std::mutex m_specMutex;
    std::string m_applyResult;
    bool m_applyError = false;
    std::string m_generationStatus;
    bool m_generationStatusError = false;
};

} // namespace RogueCity::UI
