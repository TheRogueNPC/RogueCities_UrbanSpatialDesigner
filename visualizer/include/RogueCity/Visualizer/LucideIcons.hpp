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

} // namespace LC
