#pragma once

#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"

#include <span>

#include <imgui.h>

namespace RogueCity::App {

using AxiomType = Generators::CityGenerator::AxiomInput::Type;

struct AxiomTypeInfo {
    AxiomType type;
    const char* name;
    ImU32 primary_color;
};

[[nodiscard]] std::span<const AxiomTypeInfo> GetAxiomTypeInfos();
[[nodiscard]] const AxiomTypeInfo& GetAxiomTypeInfo(AxiomType type);

void DrawAxiomIcon(ImDrawList* draw_list, ImVec2 center, float radius, AxiomType type, ImU32 color);

} // namespace RogueCity::App

