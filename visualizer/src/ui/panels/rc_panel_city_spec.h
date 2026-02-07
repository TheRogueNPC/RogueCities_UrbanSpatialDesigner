#pragma once
#include "RogueCity/Core/Data/CitySpec.hpp"
#include <string>

namespace RogueCity::UI {

/// CitySpec Generator panel for AI-driven city design
class CitySpecPanel {
public:
    void Render();
    
private:
    char m_descBuffer[512] = "A coastal tech city with dense downtown and residential suburbs";
    int m_scaleIndex = 2; // 0=hamlet, 1=town, 2=city, 3=metro
    Core::CitySpec m_currentSpec;
    bool m_hasSpec = false;
    bool m_processing = false;
};

} // namespace RogueCity::UI
