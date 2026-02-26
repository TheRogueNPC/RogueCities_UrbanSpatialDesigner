#pragma once

#include "RogueCity/App/Tools/AxiomVisual.hpp"
#include <imgui.h>
#include <span>

namespace RogueCity::App {

using AxiomType = AxiomVisual::AxiomType;

struct AxiomTypeInfo {
    AxiomType type{ AxiomType::Grid };
    const char* name{ "" };
    ImU32 primary_color{ IM_COL32_WHITE };
};

[[nodiscard]] std::span<const AxiomTypeInfo> GetAxiomTypeInfos();
[[nodiscard]] const AxiomTypeInfo& GetAxiomTypeInfo(AxiomType type);

void DrawAxiomIcon(ImDrawList* draw_list, ImVec2 center, float radius, AxiomType type, ImU32 color);

} // namespace RogueCity::App
