// FILE: rc_property_editor.h
// PURPOSE: Context-sensitive property editor for inspector panel.
#pragma once

#include "RogueCity/Core/Editor/GlobalState.hpp"

namespace RC_UI::Panels {

class PropertyEditor {
public:
    void Draw(RogueCity::Core::Editor::GlobalState& gs);
};

} // namespace RC_UI::Panels
