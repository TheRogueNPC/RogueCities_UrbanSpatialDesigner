#pragma once
// LucideIcons.hpp — Named path constants for Lucide SVG icons.
// Icons live at visualizer/assets/icons/lucide/ (relative to repo root).
//
// Usage (from any panel):
//   #include <RogueCity/Visualizer/SvgTextureCache.hpp>
//   #include <RogueCity/Visualizer/LucideIcons.hpp>
//
//   ImTextureID ico = RC::SvgTextureCache::Get().Load(LC::Home, 18.f);
//   if (ico) { ImGui::Image(ico, ImVec2(18, 18)); ImGui::SameLine(); }
//   ImGui::Text("Home");
//
// To add more icons: ls visualizer/assets/icons/lucide/ and add a constant below.

namespace LC {

// ── Navigation & Layout ───────────────────────────────────────────────────────
static constexpr const char* Home         = "visualizer/assets/icons/lucide/home.svg";
static constexpr const char* Settings     = "visualizer/assets/icons/lucide/settings.svg";
static constexpr const char* Layers       = "visualizer/assets/icons/lucide/layers.svg";
static constexpr const char* Map          = "visualizer/assets/icons/lucide/map.svg";
static constexpr const char* MapPin       = "visualizer/assets/icons/lucide/map-pin.svg";
static constexpr const char* Grid         = "visualizer/assets/icons/lucide/grid-3x3.svg";
static constexpr const char* Layout       = "visualizer/assets/icons/lucide/layout-dashboard.svg";

// ── Tools & Actions ───────────────────────────────────────────────────────────
static constexpr const char* Pencil       = "visualizer/assets/icons/lucide/pencil.svg";
static constexpr const char* Trash        = "visualizer/assets/icons/lucide/trash-2.svg";
static constexpr const char* Plus         = "visualizer/assets/icons/lucide/plus.svg";
static constexpr const char* Minus        = "visualizer/assets/icons/lucide/minus.svg";
static constexpr const char* Search       = "visualizer/assets/icons/lucide/search.svg";
static constexpr const char* Filter       = "visualizer/assets/icons/lucide/filter.svg";
static constexpr const char* Sliders      = "visualizer/assets/icons/lucide/sliders-horizontal.svg";
static constexpr const char* Move         = "visualizer/assets/icons/lucide/move.svg";
static constexpr const char* Crosshair    = "visualizer/assets/icons/lucide/crosshair.svg";
static constexpr const char* Ruler        = "visualizer/assets/icons/lucide/ruler.svg";

// ── Status & Feedback ─────────────────────────────────────────────────────────
static constexpr const char* AlertTriangle = "visualizer/assets/icons/lucide/triangle-alert.svg";
static constexpr const char* CheckCircle   = "visualizer/assets/icons/lucide/check-circle.svg";
static constexpr const char* Info          = "visualizer/assets/icons/lucide/info.svg";
static constexpr const char* XCircle       = "visualizer/assets/icons/lucide/x-circle.svg";
static constexpr const char* ChevronRight  = "visualizer/assets/icons/lucide/chevron-right.svg";
static constexpr const char* ChevronDown   = "visualizer/assets/icons/lucide/chevron-down.svg";
static constexpr const char* RefreshCw     = "visualizer/assets/icons/lucide/refresh-cw.svg";

// ── City & Urban ──────────────────────────────────────────────────────────────
static constexpr const char* Building      = "visualizer/assets/icons/lucide/building-2.svg";
static constexpr const char* BuildingAlt   = "visualizer/assets/icons/lucide/building.svg";
static constexpr const char* Road          = "visualizer/assets/icons/lucide/route.svg";
static constexpr const char* Trees         = "visualizer/assets/icons/lucide/trees.svg";
static constexpr const char* Landmark      = "visualizer/assets/icons/lucide/landmark.svg";

// ── AI & Dev ──────────────────────────────────────────────────────────────────
static constexpr const char* Bot           = "visualizer/assets/icons/lucide/bot.svg";
static constexpr const char* Terminal      = "visualizer/assets/icons/lucide/terminal.svg";
static constexpr const char* Cpu           = "visualizer/assets/icons/lucide/cpu.svg";
static constexpr const char* Activity      = "visualizer/assets/icons/lucide/activity.svg";

// ── Expanded UI/Iconification set ───────────────────────────────────────────
static constexpr const char* Radius        = "visualizer/assets/icons/lucide/radius.svg";
static constexpr const char* Waves         = "visualizer/assets/icons/lucide/waves.svg";
static constexpr const char* Waypoints     = "visualizer/assets/icons/lucide/waypoints.svg";
static constexpr const char* ChartNetwork  = "visualizer/assets/icons/lucide/chart-network.svg";
static constexpr const char* MapPinHouse   = "visualizer/assets/icons/lucide/map-pin-house.svg";
static constexpr const char* Building2     = "visualizer/assets/icons/lucide/building-2.svg";
static constexpr const char* BookMarked    = "visualizer/assets/icons/lucide/book-marked.svg";
static constexpr const char* IdCard        = "visualizer/assets/icons/lucide/id-card.svg";
static constexpr const char* ToolCase      = "visualizer/assets/icons/lucide/tool-case.svg";
static constexpr const char* MonitorCog    = "visualizer/assets/icons/lucide/monitor-cog.svg";
static constexpr const char* ArrowDownUp   = "visualizer/assets/icons/lucide/arrow-down-up.svg";
static constexpr const char* ArrowUpDown   = "visualizer/assets/icons/lucide/arrow-up-down.svg";
static constexpr const char* ArrowDown01   = "visualizer/assets/icons/lucide/arrow-down-0-1.svg";
static constexpr const char* ArrowUp10     = "visualizer/assets/icons/lucide/arrow-up-1-0.svg";
static constexpr const char* ArrowDownAZ   = "visualizer/assets/icons/lucide/arrow-down-a-z.svg";
static constexpr const char* ArrowUpZA     = "visualizer/assets/icons/lucide/arrow-up-z-a.svg";
static constexpr const char* ClockArrowDown= "visualizer/assets/icons/lucide/clock-arrow-down.svg";
static constexpr const char* ClockArrowUp  = "visualizer/assets/icons/lucide/clock-arrow-up.svg";
static constexpr const char* PanelBottomClose = "visualizer/assets/icons/lucide/panel-bottom-close.svg";
static constexpr const char* PanelBottomOpen  = "visualizer/assets/icons/lucide/panel-bottom-open.svg";
static constexpr const char* PanelLeftClose   = "visualizer/assets/icons/lucide/panel-left-close.svg";
static constexpr const char* PanelLeftOpen    = "visualizer/assets/icons/lucide/panel-left-open.svg";
static constexpr const char* PanelRightClose  = "visualizer/assets/icons/lucide/panel-right-close.svg";
static constexpr const char* PanelRightOpen   = "visualizer/assets/icons/lucide/panel-right-open.svg";

} // namespace LC
