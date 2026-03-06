// FILE: RcPanelDrawers.cpp
// PURPOSE: Drawer wrapper implementations for all panels
// PATTERN: Each panel gets a drawer class that implements IPanelDrawer

#include "IPanelDrawer.h"
#include "PanelRegistry.h"
#include "RogueCity/Core/Editor/EditorState.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"

// Include all existing panel headers
#include "ui/panels/rc_panel_axiom_bar.h"
#include "ui/panels/rc_panel_axiom_editor.h"
#include "ui/panels/rc_panel_building_control.h"
#include "ui/panels/rc_panel_building_index.h"
#include "ui/panels/rc_panel_dev_shell.h"
#include "ui/panels/rc_panel_district_index.h"
#include "ui/panels/rc_panel_inspector.h"
#include "ui/panels/rc_panel_log.h"
#include "ui/panels/rc_panel_lot_control.h"
#include "ui/panels/rc_panel_lot_index.h"
#include "ui/panels/rc_panel_river_index.h"
#include "ui/panels/rc_panel_road_editor.h"
#include "ui/panels/rc_panel_road_index.h"
#include "ui/panels/rc_panel_system_map.h"
#include "ui/panels/rc_panel_telemetry.h"
#include "ui/panels/rc_panel_tools.h"
#include "ui/panels/rc_panel_ui_settings.h"
#include "ui/panels/rc_panel_validation.h"
#include "ui/panels/rc_panel_water_control.h"
#include "ui/panels/rc_panel_workspace.h"
#include "ui/panels/rc_panel_zoning_control.h"

#if defined(ROGUE_AI_DLC_ENABLED)
#include "ui/panels/rc_panel_ai_console.h"
#include "ui/panels/rc_panel_city_spec.h"
#include "ui/panels/rc_panel_ui_agent.h"
#endif

namespace RC_UI::Panels {

// ============================================================================
// DRAWER MACROS
// Collapse the 18-line boilerplate class+factory for the two common patterns.
// Complex drawers (state overrides, namespace mismatch) remain hand-written.
// ============================================================================

// RC_DEFINE_INDEX_DRAWER — for RcDataIndexPanel<T> panels:
//   draw() calls GetPanel().DrawContent(ctx.global_state, ctx.introspector)
#define RC_DEFINE_INDEX_DRAWER(NS, TYPE, DISPLAY, FILE, ...)                   \
  namespace NS {                                                               \
  class Drawer : public IPanelDrawer {                                         \
  public:                                                                      \
    PanelType type() const override { return PanelType::TYPE; }                \
    const char *display_name() const override { return DISPLAY; }              \
    PanelCategory category() const override { return PanelCategory::Indices; } \
    const char *source_file() const override { return FILE; }                  \
    std::vector<std::string> tags() const override { return {__VA_ARGS__}; }   \
    void draw(DrawContext &ctx) override {                                     \
      GetPanel().DrawContent(ctx.global_state, ctx.introspector);              \
    }                                                                          \
  };                                                                           \
  IPanelDrawer *CreateDrawer() { return new Drawer(); }                        \
  } /* namespace NS */

// RC_DEFINE_DRAWER — for panels with a plain DrawContent(float dt) function
//   and no state-reactive visibility or non-standard overrides.
#define RC_DEFINE_DRAWER(NS, TYPE, DISPLAY, CAT, FILE, ...)                    \
  namespace NS {                                                               \
  class Drawer : public IPanelDrawer {                                         \
  public:                                                                      \
    PanelType type() const override { return PanelType::TYPE; }                \
    const char *display_name() const override { return DISPLAY; }              \
    PanelCategory category() const override { return PanelCategory::CAT; }     \
    const char *source_file() const override { return FILE; }                  \
    std::vector<std::string> tags() const override { return {__VA_ARGS__}; }   \
    void draw(DrawContext &ctx) override { DrawContent(ctx.dt); }              \
  };                                                                           \
  IPanelDrawer *CreateDrawer() { return new Drawer(); }                        \
  } /* namespace NS */

// ============================================================================
// INDEX PANEL DRAWERS — 5 panels (RcDataIndexPanel<T> pattern)
// ============================================================================

RC_DEFINE_INDEX_DRAWER(RoadIndex, RoadIndex, "Road Index",
                       "visualizer/src/ui/panels/rc_panel_road_index.cpp",
                       "index", "road", "data")

RC_DEFINE_INDEX_DRAWER(DistrictIndex, DistrictIndex, "District Index",
                       "visualizer/src/ui/panels/rc_panel_district_index.cpp",
                       "index", "district", "data")

RC_DEFINE_INDEX_DRAWER(LotIndex, LotIndex, "Lot Index",
                       "visualizer/src/ui/panels/rc_panel_lot_index.cpp",
                       "index", "lot", "data")

RC_DEFINE_INDEX_DRAWER(RiverIndex, RiverIndex, "River Index",
                       "visualizer/src/ui/panels/rc_panel_river_index.cpp",
                       "index", "river", "water", "data")

RC_DEFINE_INDEX_DRAWER(BuildingIndex, BuildingIndex, "Building Index",
                       "visualizer/src/ui/panels/rc_panel_building_index.cpp",
                       "index", "building", "data")

// ============================================================================
// SIMPLE DRAWERS — 10 panels (plain DrawContent, no state overrides)
// ============================================================================

namespace LotControl {
class Drawer : public IPanelDrawer {
public:
  PanelType type() const override { return PanelType::LotControl; }
  const char *display_name() const override { return "Lot Control"; }
  PanelCategory category() const override { return PanelCategory::Hidden; }
  const char *source_file() const override {
    return "visualizer/src/ui/panels/rc_panel_lot_control.cpp";
  }
  std::vector<std::string> tags() const override {
    return {"control", "lot", "generator"};
  }
  bool is_visible(DrawContext &ctx) const override {
    return ctx.hfsm.state() ==
           RogueCity::Core::Editor::EditorState::Editing_Lots;
  }
  void draw(DrawContext &ctx) override {
    RC_UI::Panels::LotControl::DrawContent(ctx.dt);
  }
};
IPanelDrawer *CreateDrawer() { return new Drawer(); }
} // namespace LotControl

namespace BuildingControl {
class Drawer : public IPanelDrawer {
public:
  PanelType type() const override { return PanelType::BuildingControl; }
  const char *display_name() const override { return "Building Control"; }
  PanelCategory category() const override { return PanelCategory::Hidden; }
  const char *source_file() const override {
    return "visualizer/src/ui/panels/rc_panel_building_control.cpp";
  }
  std::vector<std::string> tags() const override {
    return {"control", "building", "generator"};
  }
  bool is_visible(DrawContext &ctx) const override {
    return ctx.hfsm.state() ==
           RogueCity::Core::Editor::EditorState::Editing_Buildings;
  }
  void draw(DrawContext &ctx) override {
    RC_UI::Panels::BuildingControl::DrawContent(ctx.dt);
  }
};
IPanelDrawer *CreateDrawer() { return new Drawer(); }
} // namespace BuildingControl

namespace WaterControl {
class Drawer : public IPanelDrawer {
public:
  PanelType type() const override { return PanelType::WaterControl; }
  const char *display_name() const override { return "Water Control"; }
  PanelCategory category() const override { return PanelCategory::Hidden; }
  const char *source_file() const override {
    return "visualizer/src/ui/panels/rc_panel_water_control.cpp";
  }
  std::vector<std::string> tags() const override {
    return {"control", "water", "generator"};
  }
  bool is_visible(DrawContext &ctx) const override {
    return ctx.hfsm.state() ==
           RogueCity::Core::Editor::EditorState::Editing_Water;
  }
  void draw(DrawContext &ctx) override {
    RC_UI::Panels::WaterControl::DrawContent(ctx.dt);
  }
};
IPanelDrawer *CreateDrawer() { return new Drawer(); }
} // namespace WaterControl

namespace AxiomEditor {
class Drawer : public IPanelDrawer {
public:
  PanelType type() const override { return PanelType::AxiomEditor; }
  const char *display_name() const override { return "Axiom Editor"; }
  PanelCategory category() const override { return PanelCategory::Hidden; }
  const char *source_file() const override {
    return "visualizer/src/ui/panels/rc_panel_axiom_editor.cpp";
  }
  std::vector<std::string> tags() const override {
    return {"tool", "axiom", "editor"};
  }
  // Axiom library content is now embedded in the Tools panel (rc_panel_tools.cpp)
  // when in Axiom mode. Returning false here ensures the Tools drawer always wins
  // in DrawActiveDrawers() and renders the full axiom context (sub-tools +
  // axiom library + actions) as one coherent panel.
  bool is_visible(DrawContext & /*ctx*/) const override { return false; }
  void draw(DrawContext & /*ctx*/) override {
    RC_UI::Panels::AxiomEditor::DrawSidebarContent();
  }
};
IPanelDrawer *CreateDrawer() { return new Drawer(); }
} // namespace AxiomEditor

RC_DEFINE_DRAWER(Telemetry, Telemetry, "Telemetry", Controls,
                 "visualizer/src/ui/panels/rc_panel_telemetry.cpp", "system",
                 "telemetry", "debug")

RC_DEFINE_DRAWER(Tools, Tools, "Tools", Tools,
                 "visualizer/src/ui/panels/rc_panel_tools.cpp", "tools",
                 "workflow", "generator")

RC_DEFINE_DRAWER(Inspector, Inspector, "Inspector", Hidden,
                 "visualizer/src/ui/panels/rc_panel_inspector.cpp", "system",
                 "inspector", "detail")

RC_DEFINE_DRAWER(SystemMap, SystemMap, "System Map", Hidden,
                 "visualizer/src/ui/panels/rc_panel_system_map.cpp", "system",
                 "map")

RC_DEFINE_DRAWER(Validation, Validation, "Validation", System,
                 "visualizer/src/ui/panels/rc_panel_validation.cpp", "system",
                 "validation", "errors")

RC_DEFINE_DRAWER(DevShell, DevShell, "Dev Shell", System,
                 "visualizer/src/ui/panels/rc_panel_dev_shell.cpp", "system",
                 "dev", "shell")

#undef RC_DEFINE_INDEX_DRAWER
#undef RC_DEFINE_DRAWER

// ============================================================================
// COMPLEX DRAWERS — state-reactive visibility, non-standard draw calls,
// feature gates, or namespace mismatches. Hand-written.
// ============================================================================

namespace ZoningControl {
class Drawer : public IPanelDrawer {
public:
  PanelType type() const override { return PanelType::ZoningControl; }
  const char *display_name() const override { return "Zoning Control"; }
  PanelCategory category() const override { return PanelCategory::Hidden; }
  const char *source_file() const override {
    return "visualizer/src/ui/panels/rc_panel_zoning_control.cpp";
  }
  std::vector<std::string> tags() const override {
    return {"control", "zoning", "generator"};
  }

  bool is_visible(DrawContext &ctx) const override {
    auto editor_state = ctx.hfsm.state();
    return (editor_state ==
            RogueCity::Core::Editor::EditorState::Editing_Districts);
  }

  void draw(DrawContext &ctx) override {
    RC_UI::Panels::ZoningControl::DrawContent(ctx.dt);
  }
};

IPanelDrawer *CreateDrawer() { return new Drawer(); }
} // namespace ZoningControl

namespace RoadEditor {
class Drawer : public IPanelDrawer {
public:
  PanelType type() const override { return PanelType::RoadEditor; }
  const char *display_name() const override { return "Road Editor"; }
  PanelCategory category() const override { return PanelCategory::Hidden; }
  const char *source_file() const override {
    return "visualizer/src/ui/panels/rc_panel_road_editor.cpp";
  }
  std::vector<std::string> tags() const override {
    return {"tool", "road", "editor"};
  }

  bool is_visible(DrawContext &ctx) const override {
    auto st = ctx.hfsm.state();
    return st == RogueCity::Core::Editor::EditorState::Editing_Roads ||
           st == RogueCity::Core::Editor::EditorState::Viewport_DrawRoad;
  }

  void draw(DrawContext &ctx) override {
    RC_UI::Panels::RoadEditor::Draw(ctx.dt);
  }
};

IPanelDrawer *CreateDrawer() { return new Drawer(); }
} // namespace RoadEditor

namespace AxiomBar {
class Drawer : public IPanelDrawer {
public:
  PanelType type() const override { return PanelType::AxiomBar; }
  const char *display_name() const override { return "Axiom Bar"; }
  PanelCategory category() const override { return PanelCategory::Hidden; }
  const char *source_file() const override {
    return "visualizer/src/ui/panels/rc_panel_axiom_bar.cpp";
  }
  std::vector<std::string> tags() const override {
    return {"tool", "axiom", "action"};
  }
  bool can_popout() const override { return false; }
  // Domain switching is handled by B1 ActivityBarLeft. Exclude from
  // DrawActiveDrawers() to prevent it preempting the Tools panel.
  bool is_visible(DrawContext & /*ctx*/) const override { return false; }

  void draw(DrawContext &ctx) override {
    RC_UI::Panels::AxiomBar::DrawContent(ctx.dt);
  }
};

IPanelDrawer *CreateDrawer() { return new Drawer(); }
} // namespace AxiomBar

namespace WorkspaceSettings {
class Drawer : public IPanelDrawer {
public:
  PanelType type() const override { return PanelType::UISettings; }
  const char *display_name() const override { return "UI Settings"; }
  PanelCategory category() const override { return PanelCategory::System; }
  const char *source_file() const override {
    return "visualizer/src/ui/panels/rc_panel_workspace.cpp";
  }
  std::vector<std::string> tags() const override {
    return {"system", "settings", "theme", "layout", "workspace"};
  }

  void draw(DrawContext &ctx) override {
    RC_UI::Panels::Workspace::DrawContent(ctx.dt);
  }
};

IPanelDrawer *CreateDrawer() { return new Drawer(); }
} // namespace WorkspaceSettings

namespace Log {
class Drawer : public IPanelDrawer {
public:
  PanelType type() const override { return PanelType::Log; }
  const char *display_name() const override { return "Log"; }
  PanelCategory category() const override { return PanelCategory::System; }
  const char *source_file() const override {
    return "visualizer/src/ui/panels/rc_panel_log.cpp";
  }
  std::vector<std::string> tags() const override {
    return {"system", "log", "debug"};
  }

  void draw(DrawContext &ctx) override { RC_UI::Panels::Log::Draw(ctx.dt); }
};

IPanelDrawer *CreateDrawer() { return new Drawer(); }
} // namespace Log

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
  const char *display_name() const override { return "AI Console"; }
  PanelCategory category() const override { return PanelCategory::AI; }
  const char *source_file() const override {
    return "visualizer/src/ui/panels/rc_panel_ai_console.cpp";
  }
  std::vector<std::string> tags() const override {
    return {"ai", "console", "bridge"};
  }

  bool is_visible(DrawContext &ctx) const override {
    return ctx.global_state.config.dev_mode_enabled;
  }

  void draw(DrawContext &ctx) override { DrawContent(ctx.dt); }
};

IPanelDrawer *CreateDrawer() { return new Drawer(); }
} // namespace AiConsole

namespace UiAgent {
// Forward declare the panel content function
void DrawContent(float dt);

class Drawer : public IPanelDrawer {
public:
  PanelType type() const override { return PanelType::UiAgent; }
  const char *display_name() const override { return "UI Agent"; }
  PanelCategory category() const override { return PanelCategory::AI; }
  const char *source_file() const override {
    return "visualizer/src/ui/panels/rc_panel_ui_agent.cpp";
  }
  std::vector<std::string> tags() const override {
    return {"ai", "ui", "assistant"};
  }

  bool is_visible(DrawContext &ctx) const override {
    return ctx.global_state.config.dev_mode_enabled;
  }

  void draw(DrawContext &ctx) override { DrawContent(ctx.dt); }
};

IPanelDrawer *CreateDrawer() { return new Drawer(); }
} // namespace UiAgent

namespace CitySpec {
// Forward declare the panel content function
void DrawContent(float dt);

class Drawer : public IPanelDrawer {
public:
  PanelType type() const override { return PanelType::CitySpec; }
  const char *display_name() const override { return "City Spec"; }
  PanelCategory category() const override { return PanelCategory::AI; }
  const char *source_file() const override {
    return "visualizer/src/ui/panels/rc_panel_city_spec.cpp";
  }
  std::vector<std::string> tags() const override {
    return {"ai", "cityspec", "generator"};
  }

  bool is_visible(DrawContext &ctx) const override {
    return ctx.global_state.config.dev_mode_enabled;
  }

  void draw(DrawContext &ctx) override { DrawContent(ctx.dt); }
};

IPanelDrawer *CreateDrawer() { return new Drawer(); }
} // namespace CitySpec

#endif // ROGUE_AI_DLC_ENABLED

} // namespace RC_UI::Panels
