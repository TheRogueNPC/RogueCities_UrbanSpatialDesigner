// FILE: RcPanelDrawers.cpp
// PURPOSE: Drawer wrapper implementations for all panels
// PATTERN: Each panel gets a drawer class that implements IPanelDrawer

#include "IPanelDrawer.h"
#include "PanelRegistry.h"
#include "RogueCity/Core/Editor/EditorState.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"

// Include all existing panel headers
#include "ui/panels/rc_panel_road_index.h"
#include "ui/panels/rc_panel_district_index.h"
#include "ui/panels/rc_panel_lot_index.h"
#include "ui/panels/rc_panel_river_index.h"
#include "ui/panels/rc_panel_building_index.h"
#include "ui/panels/rc_panel_zoning_control.h"
#include "ui/panels/rc_panel_lot_control.h"
#include "ui/panels/rc_panel_building_control.h"
#include "ui/panels/rc_panel_water_control.h"
#include "ui/panels/rc_panel_axiom_bar.h"
#include "ui/panels/rc_panel_axiom_editor.h"
#include "ui/panels/rc_panel_telemetry.h"
#include "ui/panels/rc_panel_log.h"
#include "ui/panels/rc_panel_tools.h"
#include "ui/panels/rc_panel_inspector.h"
#include "ui/panels/rc_panel_system_map.h"
#include "ui/panels/rc_panel_dev_shell.h"
#include "ui/panels/rc_panel_ui_settings.h"
#include "ui/panels/rc_panel_road_editor.h"

#if defined(ROGUE_AI_DLC_ENABLED)
#include "ui/panels/rc_panel_ai_console.h"
#include "ui/panels/rc_panel_ui_agent.h"
#include "ui/panels/rc_panel_city_spec.h"
#endif

namespace RC_UI::Panels {

// ============================================================================
// INDEX PANEL DRAWERS (Template-based)
// ============================================================================

namespace RoadIndex {
    class Drawer : public IPanelDrawer {
    public:
        PanelType type() const override { return PanelType::RoadIndex; }
        const char* display_name() const override { return "Road Index"; }
        PanelCategory category() const override { return PanelCategory::Indices; }
        const char* source_file() const override { return "visualizer/src/ui/panels/rc_panel_road_index.cpp"; }
        std::vector<std::string> tags() const override { return {"index", "road", "data"}; }
        
        void draw(DrawContext& ctx) override {
            auto& panel = RC_UI::Panels::RoadIndex::GetPanel();
            panel.DrawContent(ctx.global_state, ctx.introspector);
        }
    };
    
    IPanelDrawer* CreateDrawer() { return new Drawer(); }
}

namespace DistrictIndex {
    class Drawer : public IPanelDrawer {
    public:
        PanelType type() const override { return PanelType::DistrictIndex; }
        const char* display_name() const override { return "District Index"; }
        PanelCategory category() const override { return PanelCategory::Indices; }
        const char* source_file() const override { return "visualizer/src/ui/panels/rc_panel_district_index.cpp"; }
        std::vector<std::string> tags() const override { return {"index", "district", "data"}; }
        
        void draw(DrawContext& ctx) override {
            auto& panel = RC_UI::Panels::DistrictIndex::GetPanel();
            panel.DrawContent(ctx.global_state, ctx.introspector);
        }
    };
    
    IPanelDrawer* CreateDrawer() { return new Drawer(); }
}

namespace LotIndex {
    class Drawer : public IPanelDrawer {
    public:
        PanelType type() const override { return PanelType::LotIndex; }
        const char* display_name() const override { return "Lot Index"; }
        PanelCategory category() const override { return PanelCategory::Indices; }
        const char* source_file() const override { return "visualizer/src/ui/panels/rc_panel_lot_index.cpp"; }
        std::vector<std::string> tags() const override { return {"index", "lot", "data"}; }
        
        void draw(DrawContext& ctx) override {
            auto& panel = RC_UI::Panels::LotIndex::GetPanel();
            panel.DrawContent(ctx.global_state, ctx.introspector);
        }
    };
    
    IPanelDrawer* CreateDrawer() { return new Drawer(); }
}

namespace RiverIndex {
    class Drawer : public IPanelDrawer {
    public:
        PanelType type() const override { return PanelType::RiverIndex; }
        const char* display_name() const override { return "River Index"; }
        PanelCategory category() const override { return PanelCategory::Indices; }
        const char* source_file() const override { return "visualizer/src/ui/panels/rc_panel_river_index.cpp"; }
        std::vector<std::string> tags() const override { return {"index", "river", "water", "data"}; }
        
        void draw(DrawContext& ctx) override {
            RC_UI::Panels::RiverIndex::DrawContent(ctx.dt);
        }
    };
    
    IPanelDrawer* CreateDrawer() { return new Drawer(); }
}

namespace BuildingIndex {
    class Drawer : public IPanelDrawer {
    public:
        PanelType type() const override { return PanelType::BuildingIndex; }
        const char* display_name() const override { return "Building Index"; }
        PanelCategory category() const override { return PanelCategory::Indices; }
        const char* source_file() const override { return "visualizer/src/ui/panels/rc_panel_building_index.cpp"; }
        std::vector<std::string> tags() const override { return {"index", "building", "data"}; }
        
        void draw(DrawContext& ctx) override {
            auto& panel = RC_UI::Panels::BuildingIndex::GetPanel();
            panel.DrawContent(ctx.global_state, ctx.introspector);
        }
    };
    
    IPanelDrawer* CreateDrawer() { return new Drawer(); }
}

// ============================================================================
// CONTROL PANEL DRAWERS
// ============================================================================

namespace ZoningControl {
    class Drawer : public IPanelDrawer {
    public:
        PanelType type() const override { return PanelType::ZoningControl; }
        const char* display_name() const override { return "Zoning Control"; }
        PanelCategory category() const override { return PanelCategory::Hidden; }
        const char* source_file() const override { return "visualizer/src/ui/panels/rc_panel_zoning_control.cpp"; }
        std::vector<std::string> tags() const override { return {"control", "zoning", "generator"}; }
        
        bool is_visible(DrawContext& ctx) const override {
            auto editor_state = ctx.hfsm.state();
            return (editor_state == RogueCity::Core::Editor::EditorState::Editing_Districts ||
                    editor_state == RogueCity::Core::Editor::EditorState::Editing_Lots ||
                    editor_state == RogueCity::Core::Editor::EditorState::Editing_Buildings);
        }
        
        void draw(DrawContext& ctx) override {
            RC_UI::Panels::ZoningControl::DrawContent(ctx.dt);
        }
    };
    
    IPanelDrawer* CreateDrawer() { return new Drawer(); }
}

namespace LotControl {
    class Drawer : public IPanelDrawer {
    public:
        PanelType type() const override { return PanelType::LotControl; }
        const char* display_name() const override { return "Lot Control"; }
        PanelCategory category() const override { return PanelCategory::Hidden; }
        const char* source_file() const override { return "visualizer/src/ui/panels/rc_panel_lot_control.cpp"; }
        std::vector<std::string> tags() const override { return {"control", "lot", "generator"}; }
       
        void draw(DrawContext& ctx) override {
            RC_UI::Panels::LotControl::DrawContent(ctx.dt);
        }
    };
    
    IPanelDrawer* CreateDrawer() { return new Drawer(); }
}

namespace BuildingControl {
    class Drawer : public IPanelDrawer {
    public:
        PanelType type() const override { return PanelType::BuildingControl; }
        const char* display_name() const override { return "Building Control"; }
        PanelCategory category() const override { return PanelCategory::Hidden; }
        const char* source_file() const override { return "visualizer/src/ui/panels/rc_panel_building_control.cpp"; }
        std::vector<std::string> tags() const override { return {"control", "building", "generator"}; }
        
        void draw(DrawContext& ctx) override {
            RC_UI::Panels::BuildingControl::DrawContent(ctx.dt);
        }
    };
    
    IPanelDrawer* CreateDrawer() { return new Drawer(); }
}

namespace WaterControl {
    class Drawer : public IPanelDrawer {
    public:
        PanelType type() const override { return PanelType::WaterControl; }
        const char* display_name() const override { return "Water Control"; }
        PanelCategory category() const override { return PanelCategory::Hidden; }
        const char* source_file() const override { return "visualizer/src/ui/panels/rc_panel_water_control.cpp"; }
        std::vector<std::string> tags() const override { return {"control", "water", "generator"}; }
        
        void draw(DrawContext& ctx) override {
            RC_UI::Panels::WaterControl::DrawContent(ctx.dt);
        }
    };
    
    IPanelDrawer* CreateDrawer() { return new Drawer(); }
}

// ============================================================================
// TOOL PANEL DRAWERS
// ============================================================================

namespace AxiomBar {
    class Drawer : public IPanelDrawer {
    public:
        PanelType type() const override { return PanelType::AxiomBar; }
        const char* display_name() const override { return "Axiom Bar"; }
        PanelCategory category() const override { return PanelCategory::Tools; }
        const char* source_file() const override { return "visualizer/src/ui/panels/rc_panel_axiom_bar.cpp"; }
        std::vector<std::string> tags() const override { return {"tool", "axiom", "action"}; }
        bool can_popout() const override { return false; } // Should stay docked as toolbar
        
        void draw(DrawContext& ctx) override {
            RC_UI::Panels::AxiomBar::DrawContent(ctx.dt);
        }
    };
    
    IPanelDrawer* CreateDrawer() { return new Drawer(); }
}

namespace AxiomEditor {
    class Drawer : public IPanelDrawer {
    public:
        PanelType type() const override { return PanelType::AxiomEditor; }
        const char* display_name() const override { return "Axiom Editor"; }
        PanelCategory category() const override { return PanelCategory::Tools; }
        const char* source_file() const override { return "visualizer/src/ui/panels/rc_panel_axiom_editor.cpp"; }
        std::vector<std::string> tags() const override { return {"tool", "axiom", "editor"}; }
        
        void draw(DrawContext& ctx) override {
            RC_UI::Panels::AxiomEditor::DrawContent(ctx.dt);
        }
    };
    
    IPanelDrawer*CreateDrawer() { return new Drawer(); }
}

namespace RoadEditor {
    class Drawer : public IPanelDrawer {
    public:
        PanelType type() const override { return PanelType::RoadEditor; }
        const char* display_name() const override { return "Road Editor"; }
        PanelCategory category() const override { return PanelCategory::Tools; }
        const char* source_file() const override { return "visualizer/src/ui/panels/rc_panel_road_editor.cpp"; }
        std::vector<std::string> tags() const override { return {"tool", "road", "editor"}; }

        bool is_visible(DrawContext& ctx) const override {
            return ctx.hfsm.state() == RogueCity::Core::Editor::EditorState::Editing_Roads;
        }

        void draw(DrawContext& ctx) override {
            RC_UI::Panels::RoadEditor::Draw(ctx.dt);
        }
    };

    IPanelDrawer* CreateDrawer() { return new Drawer(); }
}

// ============================================================================
// SYSTEM PANEL DRAWERS
// ============================================================================

namespace Telemetry {
    class Drawer : public IPanelDrawer {
    public:
        PanelType type() const override { return PanelType::Telemetry; }
        const char* display_name() const override { return "Telemetry"; }
        PanelCategory category() const override { return PanelCategory::Controls; }
        const char* source_file() const override { return "visualizer/src/ui/panels/rc_panel_telemetry.cpp"; }
        std::vector<std::string> tags() const override { return {"system", "telemetry", "debug"}; }
        
        void draw(DrawContext& ctx) override {
            RC_UI::Panels::Telemetry::DrawContent(ctx.dt);
        }
    };
    
    IPanelDrawer* CreateDrawer() { return new Drawer(); }
}

namespace Log {
    class Drawer : public IPanelDrawer {
    public:
        PanelType type() const override { return PanelType::Log; }
        const char* display_name() const override { return "Log"; }
        PanelCategory category() const override { return PanelCategory::System; }
        const char* source_file() const override { return "visualizer/src/ui/panels/rc_panel_log.cpp"; }
        std::vector<std::string> tags() const override { return {"system", "log", "debug"}; }
        
        void draw(DrawContext& ctx) override {
            RC_UI::Panels::Log::DrawContent(ctx.dt);
        }
    };
    
    IPanelDrawer* CreateDrawer() { return new Drawer(); }
}

namespace Tools {
    class Drawer : public IPanelDrawer {
    public:
        PanelType type() const override { return PanelType::Tools; }
        const char* display_name() const override { return "Tools"; }
        PanelCategory category() const override { return PanelCategory::Tools; }
        const char* source_file() const override { return "visualizer/src/ui/panels/rc_panel_tools.cpp"; }
        std::vector<std::string> tags() const override { return {"tools", "workflow", "generator"}; }
        
        void draw(DrawContext& ctx) override {
            RC_UI::Panels::Tools::DrawContent(ctx.dt);
        }
    };
    
    IPanelDrawer* CreateDrawer() { return new Drawer(); }
}

namespace Inspector {
    class Drawer : public IPanelDrawer {
    public:
        PanelType type() const override { return PanelType::Inspector; }
        const char* display_name() const override { return "Inspector"; }
        PanelCategory category() const override { return PanelCategory::System; }
        const char* source_file() const override { return "visualizer/src/ui/panels/rc_panel_inspector.cpp"; }
        std::vector<std::string> tags() const override { return {"system", "inspector", "detail"}; }
        
        void draw(DrawContext& ctx) override {
            RC_UI::Panels::Inspector::DrawContent(ctx.dt);
        }
    };
    
    IPanelDrawer* CreateDrawer() { return new Drawer(); }
}

namespace SystemMap {
    class Drawer : public IPanelDrawer {
    public:
        PanelType type() const override { return PanelType::SystemMap; }
        const char* display_name() const override { return "System Map"; }
        PanelCategory category() const override { return PanelCategory::System; }
        const char* source_file() const override { return "visualizer/src/ui/panels/rc_panel_system_map.cpp"; }
        std::vector<std::string> tags() const override { return {"system", "map"}; }
        
        void draw(DrawContext& ctx) override {
            RC_UI::Panels::SystemMap::DrawContent(ctx.dt);
        }
    };
    
    IPanelDrawer* CreateDrawer() { return new Drawer(); }
}

namespace DevShell {
    class Drawer : public IPanelDrawer {
    public:
        PanelType type() const override { return PanelType::DevShell; }
        const char* display_name() const override { return "Dev Shell"; }
        PanelCategory category() const override { return PanelCategory::System; }
        const char* source_file() const override { return "visualizer/src/ui/panels/rc_panel_dev_shell.cpp"; }
        std::vector<std::string> tags() const override { return {"system", "dev", "shell"}; }
        
        void draw(DrawContext& ctx) override {
            RC_UI::Panels::DevShell::DrawContent(ctx.dt);
        }
    };
    
    IPanelDrawer* CreateDrawer() { return new Drawer(); }
}

namespace UISettings {
    class Drawer : public IPanelDrawer {
    public:
        PanelType type() const override { return PanelType::UISettings; }
        const char* display_name() const override { return "UI Settings"; }
        PanelCategory category() const override { return PanelCategory::System; }
        const char* source_file() const override { return "visualizer/src/ui/panels/rc_panel_ui_settings.cpp"; }
        std::vector<std::string> tags() const override { return {"system", "settings", "theme", "ui"}; }
        
        void draw(DrawContext& ctx) override {
            RC_UI::Panels::UiSettings::DrawContent(ctx.dt);
        }
    };
    
    IPanelDrawer* CreateDrawer() { return new Drawer(); }
}

// ============================================================================
// AI PANEL DRAWERS (Feature-gated)
// ============================================================================

#if defined(ROGUE_AI_DLC_ENABLED)

namespace AiConsole {
    // Forward declare the panel content function
    void DrawContent(float dt);
    
    class Drawer : public IPanelDrawer {
    public:
        PanelType type() const override { return PanelType::AiConsole; }
        const char* display_name() const override { return "AI Console"; }
        PanelCategory category() const override { return PanelCategory::AI; }
        const char* source_file() const override { return "visualizer/src/ui/panels/rc_panel_ai_console.cpp"; }
        std::vector<std::string> tags() const override { return {"ai", "console", "bridge"}; }

        bool is_visible(DrawContext& ctx) const override {
            return ctx.global_state.config.dev_mode_enabled;
        }
        
        void draw(DrawContext& ctx) override {
            DrawContent(ctx.dt);
        }
    };
    
    IPanelDrawer* CreateDrawer() { return new Drawer(); }
}

namespace UiAgent {
    // Forward declare the panel content function
    void DrawContent(float dt);
    
    class Drawer : public IPanelDrawer {
    public:
        PanelType type() const override { return PanelType::UiAgent; }
        const char* display_name() const override { return "UI Agent"; }
        PanelCategory category() const override { return PanelCategory::AI; }
        const char* source_file() const override { return "visualizer/src/ui/panels/rc_panel_ui_agent.cpp"; }
        std::vector<std::string> tags() const override { return {"ai", "ui", "assistant"}; }

        bool is_visible(DrawContext& ctx) const override {
            return ctx.global_state.config.dev_mode_enabled;
        }
        
        void draw(DrawContext& ctx) override {
            DrawContent(ctx.dt);
        }
    };
    
    IPanelDrawer* CreateDrawer() { return new Drawer(); }
}

namespace CitySpec {
    // Forward declare the panel content function
    void DrawContent(float dt);
    
    class Drawer : public IPanelDrawer {
    public:
        PanelType type() const override { return PanelType::CitySpec; }
        const char* display_name() const override { return "City Spec"; }
        PanelCategory category() const override { return PanelCategory::AI; }
        const char* source_file() const override { return "visualizer/src/ui/panels/rc_panel_city_spec.cpp"; }
        std::vector<std::string> tags() const override { return {"ai", "cityspec", "generator"}; }

        bool is_visible(DrawContext& ctx) const override {
            return ctx.global_state.config.dev_mode_enabled;
        }
        
        void draw(DrawContext& ctx) override {
            DrawContent(ctx.dt);
        }
    };
    
    IPanelDrawer* CreateDrawer() { return new Drawer(); }
}

#endif // ROGUE_AI_DLC_ENABLED

} // namespace RC_UI::Panels
