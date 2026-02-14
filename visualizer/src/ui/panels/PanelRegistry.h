// FILE: PanelRegistry.h
// PURPOSE: Global registry for panel drawer lookup and routing
// PATTERN: Singleton registry with category-based grouping

#pragma once

#include "IPanelDrawer.h"
#include <memory>
#include <unordered_map>
#include <vector>

namespace RC_UI::Panels {

// Singleton registry for panel drawer management
class PanelRegistry {
public:
    // Get singleton instance
    [[nodiscard]] static PanelRegistry& Instance();
    
    // Register a drawer (called during initialization)
    // Takes ownership of the drawer
    void Register(std::unique_ptr<IPanelDrawer> drawer);
    
    // Get drawer by type (returns nullptr if not found)
    [[nodiscard]] IPanelDrawer* GetDrawer(PanelType type);
    
    // Get all drawers in a category (useful for tab rendering)
    [[nodiscard]] std::vector<PanelType> GetPanelsInCategory(PanelCategory cat) const;
    
    // Get all registered drawer types
    [[nodiscard]] std::vector<PanelType> GetAllPanelTypes() const;
    
    // Draw by type (convenience wrapper)
    void DrawByType(PanelType type, DrawContext& ctx);
    
    // Check if a panel is registered
    [[nodiscard]] bool IsRegistered(PanelType type) const;
    
    // Clear all registrations (for testing/cleanup)
    void Clear();
    
private:
    PanelRegistry() = default;
    ~PanelRegistry() = default;
    PanelRegistry(const PanelRegistry&) = delete;
    PanelRegistry& operator=(const PanelRegistry&) = delete;
    
    std::unordered_map<PanelType, std::unique_ptr<IPanelDrawer>> m_drawers;
};

// Initialize all panel registrations
// Called once during UI system startup
void InitializePanelRegistry();

} // namespace RC_UI::Panels
