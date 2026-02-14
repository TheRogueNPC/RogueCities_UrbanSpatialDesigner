// FILE: PanelRegistry.cpp
// PURPOSE: Implementation of panel drawer registry

#include "PanelRegistry.h"
#include <algorithm>
#include <stdexcept>

namespace RC_UI::Panels {

PanelRegistry& PanelRegistry::Instance() {
    static PanelRegistry instance;
    return instance;
}

void PanelRegistry::Register(std::unique_ptr<IPanelDrawer> drawer) {
    if (!drawer) {
        return;
    }
    
    PanelType type = drawer->type();
    m_drawers[type] = std::move(drawer);
}

IPanelDrawer* PanelRegistry::GetDrawer(PanelType type) {
    auto it = m_drawers.find(type);
    return (it != m_drawers.end()) ? it->second.get() : nullptr;
}

std::vector<PanelType> PanelRegistry::GetPanelsInCategory(PanelCategory cat) const {
    std::vector<PanelType> result;
    result.reserve(8); // Most categories have < 8 panels
    
    for (const auto& [type, drawer] : m_drawers) {
        if (drawer && drawer->category() == cat) {
            result.push_back(type);
        }
    }
    
    // Sort by type enum value for consistent ordering
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<PanelType> PanelRegistry::GetAllPanelTypes() const {
    std::vector<PanelType> result;
    result.reserve(m_drawers.size());
    
    for (const auto& [type, drawer] : m_drawers) {
        if (drawer) {
            result.push_back(type);
        }
    }
    
    std::sort(result.begin(), result.end());
    return result;
}

void PanelRegistry::DrawByType(PanelType type, DrawContext& ctx) {
    IPanelDrawer* drawer = GetDrawer(type);
    if (!drawer) {
        return;
    }
    
    // Check state-reactive visibility
    if (!drawer->is_visible(ctx)) {
        return;
    }
    
    drawer->draw(ctx);
}

bool PanelRegistry::IsRegistered(PanelType type) const {
    return m_drawers.find(type) != m_drawers.end();
}

void PanelRegistry::Clear() {
    m_drawers.clear();
}

// Forward declarations for panel drawer constructors
// These will be defined in each panel's converted implementation
namespace RoadIndex { IPanelDrawer* CreateDrawer(); }
namespace DistrictIndex { IPanelDrawer* CreateDrawer(); }
namespace LotIndex { IPanelDrawer* CreateDrawer(); }
namespace RiverIndex { IPanelDrawer* CreateDrawer(); }
namespace BuildingIndex { IPanelDrawer* CreateDrawer(); }
namespace ZoningControl { IPanelDrawer* CreateDrawer(); }
namespace LotControl { IPanelDrawer* CreateDrawer(); }
namespace BuildingControl { IPanelDrawer* CreateDrawer(); }
namespace WaterControl { IPanelDrawer* CreateDrawer(); }
namespace AxiomBar { IPanelDrawer* CreateDrawer(); }
namespace AxiomEditor { IPanelDrawer* CreateDrawer(); }
namespace Telemetry { IPanelDrawer* CreateDrawer(); }
namespace Log { IPanelDrawer* CreateDrawer(); }
namespace Tools { IPanelDrawer* CreateDrawer(); }
namespace Inspector { IPanelDrawer* CreateDrawer(); }
namespace SystemMap { IPanelDrawer* CreateDrawer(); }
namespace DevShell { IPanelDrawer* CreateDrawer(); }

#if defined(ROGUE_AI_DLC_ENABLED)
namespace AiConsole { IPanelDrawer* CreateDrawer(); }
namespace UiAgent { IPanelDrawer* CreateDrawer(); }
namespace CitySpec { IPanelDrawer* CreateDrawer(); }
#endif

void InitializePanelRegistry() {
    auto& registry = PanelRegistry::Instance();
    
    // Register Indices
    registry.Register(std::unique_ptr<IPanelDrawer>(RoadIndex::CreateDrawer()));
    registry.Register(std::unique_ptr<IPanelDrawer>(DistrictIndex::CreateDrawer()));
    registry.Register(std::unique_ptr<IPanelDrawer>(LotIndex::CreateDrawer()));
    registry.Register(std::unique_ptr<IPanelDrawer>(RiverIndex::CreateDrawer()));
    registry.Register(std::unique_ptr<IPanelDrawer>(BuildingIndex::CreateDrawer()));
    
    // Register Controls
    registry.Register(std::unique_ptr<IPanelDrawer>(ZoningControl::CreateDrawer()));
    registry.Register(std::unique_ptr<IPanelDrawer>(LotControl::CreateDrawer()));
    registry.Register(std::unique_ptr<IPanelDrawer>(BuildingControl::CreateDrawer()));
    registry.Register(std::unique_ptr<IPanelDrawer>(WaterControl::CreateDrawer()));
    
    // Register Tools
    registry.Register(std::unique_ptr<IPanelDrawer>(AxiomBar::CreateDrawer()));
    registry.Register(std::unique_ptr<IPanelDrawer>(AxiomEditor::CreateDrawer()));
    
    // Register System
    registry.Register(std::unique_ptr<IPanelDrawer>(Telemetry::CreateDrawer()));
    registry.Register(std::unique_ptr<IPanelDrawer>(Log::CreateDrawer()));
    registry.Register(std::unique_ptr<IPanelDrawer>(Tools::CreateDrawer()));
    registry.Register(std::unique_ptr<IPanelDrawer>(Inspector::CreateDrawer()));
    registry.Register(std::unique_ptr<IPanelDrawer>(SystemMap::CreateDrawer()));
    registry.Register(std::unique_ptr<IPanelDrawer>(DevShell::CreateDrawer()));
    
    // Register AI panels (feature-gated)
#if defined(ROGUE_AI_DLC_ENABLED)
    registry.Register(std::unique_ptr<IPanelDrawer>(AiConsole::CreateDrawer()));
    registry.Register(std::unique_ptr<IPanelDrawer>(UiAgent::CreateDrawer()));
    registry.Register(std::unique_ptr<IPanelDrawer>(CitySpec::CreateDrawer()));
#endif
}

} // namespace RC_UI::Panels
