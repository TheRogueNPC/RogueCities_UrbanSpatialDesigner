# Claude — Lucide SVG Icon System — 2026-03-02

## Objective
Integrate the Lucide open-source icon set into the ImGui / OpenGL3 visualizer
as rasterized SVG textures, providing a named icon API usable from any panel.

## Layer Ownership
**visualizer** (Layer 2) — UI assets and runtime texture management only.
No generator, app, or core layer files were touched.

## Files Created
| File | Purpose |
|------|---------|
| `3rdparty/nanosvg/nanosvg.h` | Header-only SVG parser (vendored from memononen/nanosvg) |
| `3rdparty/nanosvg/nanosvgrast.h` | Header-only SVG rasterizer (same repo) |
| `visualizer/assets/icons/lucide/*.svg` | 1950 Lucide SVG icons from `lucide-static` npm package |
| `visualizer/include/RogueCity/Visualizer/SvgTextureCache.hpp` | Singleton: SVG path + size → cached `ImTextureID` |
| `visualizer/src/ui/SvgTextureCache.cpp` | nanosvg parse → rasterize → `glTexImage2D` → cache |
| `visualizer/include/RogueCity/Visualizer/LucideIcons.hpp` | 33 named path constants in `namespace LC` |

## Files Modified
| File | Change |
|------|--------|
| `visualizer/CMakeLists.txt` | Added `SvgTextureCache.cpp` + `3rdparty/nanosvg` include path to both `RogueCityVisualizerHeadless` and `RogueCityVisualizerGui` |
| `visualizer/src/main.cpp` | Include + `RC::SvgTextureCache::Get().Clear()` before `ShutdownScreenshotRuntime` |
| `visualizer/src/main_gui.cpp` | Include + `RC::SvgTextureCache::Get().Clear()` before `ImGui_ImplOpenGL3_Shutdown` |

## Files Intentionally Not Touched
- No generator, app, or core headers modified.
- No existing panel files modified (zero-friction opt-in).
- No theme tokens changed.

## Usage Pattern (any panel)
```cpp
#include <RogueCity/Visualizer/SvgTextureCache.hpp>
#include <RogueCity/Visualizer/LucideIcons.hpp>

ImTextureID ico = RC::SvgTextureCache::Get().Load(LC::Settings, 18.f);
if (ico) { ImGui::Image(ico, ImVec2(18, 18)); ImGui::SameLine(); }
ImGui::Text("Settings");
```

To add icons beyond the 33 pre-named: use the full path directly,
e.g. `"visualizer/assets/icons/lucide/<name>.svg"`.

## Validation
- Build not yet run this session (user to trigger cmake + build).
- Validation gate: compile both targets, run app, call `SvgTextureCache::Get().Load(LC::Home, 24.f)` in any panel's `DrawContent()` and confirm texture renders.
- Teardown confirmed safe: `Clear()` fires before GL context destruction in both executables.

## Risks / Notes
- **Working directory**: icons resolve relative to repo root (same convention as `visualizer/RC_UI_Mockup.html`). If the binary is launched from a different CWD, paths will not resolve — consistent with existing project convention.
- **nanosvg limitations**: complex SVG features (filters, masks, `<use>` elements) may not render. Lucide icons are simple stroked paths and are fully supported.
- **No PCH conflict**: `NANOSVG_IMPLEMENTATION` / `NANOSVGRAST_IMPLEMENTATION` are defined only in `SvgTextureCache.cpp`, isolated from the precompiled headers.

## CHANGELOG updated: yes
