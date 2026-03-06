// FILE: app/include/RogueCity/App/Tools/AxiomTypeRegistry.hpp
// PURPOSE: Central registration point for all axiom type metadata.
//
// Adding a new axiom type after this refactor requires touching 4 files:
//   1. generators/include/.../CityGenerator.hpp          — add enum value
//   2. generators/include/.../AxiomTerminalFeatures.hpp  — add 4 features
//   3. app/src/Tools/AxiomTypeRegistry.cpp               — Register() call + icon fn
//   4. visualizer/src/ui/tools/rc_tool_contract.h        — ToolActionId enum value

#pragma once

#include "RogueCity/App/Tools/AxiomIcon.hpp"  // AxiomType, ImDrawList, ImU32, ImVec2

#include <array>
#include <span>
#include <vector>

namespace RogueCity::App {

using AxiomType   = RogueCity::App::AxiomType;  // = Generators::CityGenerator::AxiomInput::Type
using AxiomIconFn = void(*)(ImDrawList*, ImVec2 center, float radius, ImU32 color);

/// All per-type metadata bundled in one struct.
/// Create one of these per axiom type and pass to AxiomTypeRegistry::Register().
struct AxiomTypeDescriptor {
    AxiomType               type;
    const char*             name;
    ImU32                   primary_color;
    const char*             terminal_cue;       // guidance text for the Terminal panel section
    std::array<AxiomType,3> alternates;          // 3 compatible sibling types shown as quick-switch buttons
    AxiomIconFn             draw_icon;           // icon geometry — never null after Register()
};

/// Singleton registry. All built-in types are registered via RegisterBuiltInAxiomTypes().
/// Downstream code reads from this instead of scattered switch statements.
class AxiomTypeRegistry {
public:
    [[nodiscard]] static AxiomTypeRegistry& Instance();

    /// Register a descriptor. Descriptors are stored in insertion order, which
    /// determines the order of the icon grid in DrawAxiomLibraryContent().
    void Register(AxiomTypeDescriptor desc);

    /// Returns nullptr if the type is not registered.
    [[nodiscard]] const AxiomTypeDescriptor* Get(AxiomType type) const;

    /// All registered descriptors in insertion order.
    [[nodiscard]] std::span<const AxiomTypeDescriptor> All() const;

private:
    AxiomTypeRegistry() = default;
    std::vector<AxiomTypeDescriptor> descriptors_;
};

/// Registers all 10 built-in axiom types. Idempotent — safe to call multiple times.
/// Must be called before any DrawAxiomLibraryContent() or AxiomTypeRegistry::Get() call.
void RegisterBuiltInAxiomTypes();

} // namespace RogueCity::App
