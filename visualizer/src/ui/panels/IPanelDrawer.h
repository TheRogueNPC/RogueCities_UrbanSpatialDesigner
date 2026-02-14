// FILE: IPanelDrawer.h
// PURPOSE: Core interface for panel content drawers in Master Panel architecture
// PATTERN: Registry-based panel routing with state-reactive visibility

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace RogueCity::Core::Editor {
    class GlobalState;
    class EditorHFSM;
}

namespace RogueCity::UIInt {
    class UiIntrospector;
}

namespace RC_UI::Panels {

// Panel categories for tab grouping
enum class PanelCategory : uint8_t {
    Indices,   // Data index tables (roads, districts, lots, etc.)
    Controls,  // Generator control panels (zoning, building, water)
    Tools,     // Tool libraries and action panels
    System,    // Telemetry, log, system map, inspector
    AI,        // AI integration panels (feature-gated)
    Hidden     // Hidden panels that should not appear in tabs
};

// Unique panel type identifiers
enum class PanelType : uint8_t {
    // Indices (5)
    RoadIndex,
    DistrictIndex,
    LotIndex,
    RiverIndex,
    BuildingIndex,
    
    // Controls (4)
    ZoningControl,
    LotControl,
    BuildingControl,
    WaterControl,
    
    // Tools (2)
    AxiomBar,
    AxiomEditor,
    
    // System (6)
    Telemetry,
    Log,
    Tools,
    Inspector,
    SystemMap,
    DevShell,
    
    // AI (3) - feature-gated
    AiConsole,
    UiAgent,
    CitySpec,
    
    COUNT
};

// Context passed to drawers (avoids parameter bloat)
struct DrawContext {
    RogueCity::Core::Editor::GlobalState& global_state;
    RogueCity::Core::Editor::EditorHFSM& hfsm;
    RogueCity::UIInt::UiIntrospector& introspector;
    float dt;
    
    // Popout state (managed by MasterPanel)
    bool is_floating_window = false;
};

// Base interface for panel content drawers
// Drawers render content only - no ImGui::Begin/End window creation
class IPanelDrawer {
public:
    virtual ~IPanelDrawer() = default;
    
    // === REQUIRED OVERRIDES ===
    
    // Unique identifier for routing
    [[nodiscard]] virtual PanelType type() const = 0;
    
    // Display name for tabs/search
    [[nodiscard]] virtual const char* display_name() const = 0;
    
    // Category for tab grouping
    [[nodiscard]] virtual PanelCategory category() const = 0;
    
    // Main draw function - renders content within existing ImGui window
    // NO ImGui::Begin/End here - Master Panel owns the window
    virtual void draw(DrawContext& ctx) = 0;
    
    // === OPTIONAL OVERRIDES ===
    
    // Can this panel be popped out into floating window?
    [[nodiscard]] virtual bool can_popout() const { return true; }
    
    // Is panel visible in current editor state?
    // Checked before rendering to implement state-reactive visibility
    [[nodiscard]] virtual bool is_visible(DrawContext& ctx) const { return true; }
    
    // Source file for introspection metadata
    [[nodiscard]] virtual const char* source_file() const { return "unknown"; }
    
    // Introspection tags for AI pattern analysis
    [[nodiscard]] virtual std::vector<std::string> tags() const { return {}; }
    
    // Lifecycle hooks
    virtual void on_activated() {}    // Called when panel becomes visible/focused
    virtual void on_deactivated() {}  // Called when panel loses focus
    
    // Inter-panel messaging (future extension)
    virtual void on_message(const char* event_type, void* data) {}
};

// Helper functions for enums
[[nodiscard]] const char* PanelCategoryName(PanelCategory cat);
[[nodiscard]] const char* PanelTypeName(PanelType type);

} // namespace RC_UI::Panels
