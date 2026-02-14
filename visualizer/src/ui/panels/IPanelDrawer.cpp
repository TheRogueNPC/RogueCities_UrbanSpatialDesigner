// FILE: IPanelDrawer.cpp
// PURPOSE: Implementation of panel drawer helper functions

#include "IPanelDrawer.h"
#include <magic_enum/magic_enum.hpp>

namespace RC_UI::Panels {

const char* PanelCategoryName(PanelCategory cat) {
    switch (cat) {
        case PanelCategory::Indices: return "Indices";
        case PanelCategory::Controls: return "Controls";
        case PanelCategory::Tools: return "Tools";
        case PanelCategory::System: return "System";
        case PanelCategory::AI: return "AI";
        case PanelCategory::Hidden: return "Hidden";
        default: return "Unknown";
    }
}

const char* PanelTypeName(PanelType type) {
    auto name = magic_enum::enum_name(type);
    return name.empty() ? "Unknown" : name.data();
}

} // namespace RC_UI::Panels
