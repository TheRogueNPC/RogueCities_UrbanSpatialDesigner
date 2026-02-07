#pragma once

namespace RogueCity::UI {

/// AI Console panel for bridge control and status
class AiConsolePanel {
public:
    void Render();
    
private:
    bool m_showWindow = true;
};

} // namespace RogueCity::UI
