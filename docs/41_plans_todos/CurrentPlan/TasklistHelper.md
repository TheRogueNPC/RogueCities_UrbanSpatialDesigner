## 0) Choose scale policy add a toggle with popup next to dropdown 

**Goal:** viewport scaling must match maximum city scale, where city scale bounds are `texture_size^2` (512²..8192²).


- Policy A (recommended now): fixed `meters_per_pixel` (world scale grows with texture size), because overlays already read `TextureSpace.coordinateSystem().metersPerPixel()` for world mapping.
  
  ##  Scale policy (Policy A)

Your overlay layer already has a single place where “screen scale” is defined: `WorldToScreenScale(world_distance)` is driven by `view_transform_.zoom`, and yaw is driven by `view_transform_.yaw`. That means Policy A boils down to: keep `meters_per_pixel` fixed in **GlobalState/TextureSpace**, and make the camera clamp + city boundary derive from `tex_size * meters_per_pixel` in world units (so overlays stay consistent).

Code sketch for the _world extent_ helper (new, but uses your established `ViewTransform`/world-units conventions):

## Policy B - Stub for now 
- (later): fixed world extent, and `meters_per_pixel` shrinks as texture size grows (better fidelity without changing city size). (Do not do this in the first pass unless you already have a clear “city extent meters” spec.)
    

**Recommendation:** Policy A with `meters_per_pixel = 2.0` (2m/texel) as a simple, consistent ruler-friendly baseline; with 2048 this yields ~4.1km across, 4096 ~8.2km, 8192 ~16.4km.

---
## 1) Texture Size dropdown (single source of truth)

**Add UI control** “Texture Size: {Current}” with options 512..8192, and make it drive _both_ viewport constraints and TextureSpace rebuild.

Where to implement (fastest / least invasive):

1. **State fields** in `RogueCity::Core::Editor::GlobalState`:
    

- Add `int city_texture_size = 2048;`
    
- Add `double city_meters_per_pixel = 2.0;` (Policy A)
    
- Add a “dirty” bit like `bool texture_space_dirty = true;` and/or reuse your existing dirty-layer system.
    

2. **TextureSpace rebuild entry point** (GlobalState-owned):
    

- Add `void EnsureTextureSpaceUpToDate();` or `RebuildTextureSpaceIfDirty();`
    
- It must create/resize the backing layers so that `gs.HasTextureSpace()` becomes true and `gs.TextureSpaceRef().coordinateSystem().metersPerPixel()` matches `city_meters_per_pixel`.
    

3. **UI location**:
    

- If you have a top “Visualizer Bar” widget already, add the combo there; otherwise, add it to your most central tools panel (commit history references `rc_panel_tools.cpp` as an established ImGui panel touchpoint).
    
- On change: set `gs.city_texture_size`, mark `texture_space_dirty`, and trigger an incremental regen (not a full export) so overlays like tensor/height update quickly.
    

**Do NOT**

- Do not allocate 8192 layers automatically on selection without a guard; require a confirm (“This may allocate hundreds of MB; continue?”) or auto-switch to “overlays off” for heavy layers.
    
- Do not store the size only in UI-local state; overlays pull from `gs` (`HasTextureSpace()`, `TextureSpaceRef()`) so the size must live in GlobalState.
    

## 2) Viewport clamping + city boundary tied to size

You already have `ViewportOverlays::RenderCityBoundary(gs)` drawing `gs.city_boundary`, and you have a world→screen projection that includes `view_transform_.yaw` and `zoom`. You also recently added spring-based clamping for viewport/minimap interactions (`rc_viewport_interaction.cpp` in your commit log).

Implementation steps:

1. Define world bounds rectangle from TextureSpace:
    

- Compute: `world_w = tex_size * meters_per_pixel`, `world_h = tex_size * meters_per_pixel`.
    
- Define world bounds centered at (0,0) or at your chosen “foundation origin” (use your existing convention; overlays assume world coords map consistently).
    

2. Clamp camera/minimap using that rectangle:
    

- In `rc_viewport_interaction.cpp` (mentioned in your recent viewport clamp commit), update the clamping call to use the computed bounds.
    
- Ensure yaw-rotated view still clamps correctly (either clamp in world-space AABB of the camera center, or clamp by transforming viewport corners back into world like your grid overlay does).
    

3. City boundary visualization:
    

- Generate `gs.city_boundary` from the same bounds (or from your “blue boundary buffer”) so `RenderCityBoundary` always matches the texture size selection.
    

## 3) Add ruler + compass gimbal (overlay HUD)

You already have an overlay system with toggles like `config.show_gizmos`, and you already carry `view_transform_.yaw` in overlays and use it in `WorldToScreen` / grid projection.

## Size ruler (bottom-left)

Where: `visualizer/src/ui/viewport/rc_viewport_overlays.cpp`, inside `ViewportOverlays::Render(...)` after the main overlays but before highlights (HUD should sit on top).

How:

- Compute pixels-per-meter: `ppm = view_transform_.zoom` (because `WorldToScreenScale(world_distance) = world_distance * zoom`).
    
- Pick a fixed screen length target (e.g., 140 px), convert to world meters (`meters = target_px / ppm`), snap to nice steps (1/2/5×10^n), then draw a line and label “{X} m” or “{X} km”.
    
- Add an option `config.show_scale_ruler`.
    

## Compass gimbal (top-right)

Where: same file (`rc_viewport_overlays.cpp`), as a HUD widget.

How:

- Draw a small circle with N/E/S/W marks, rotate it by `-view_transform_.yaw` so “N” always shows world-north relative to screen.
    
- Input: if mouse drags on the compass, set a desired yaw and write it back to the canonical camera/viewport yaw state (don’t keep yaw only inside overlays).
    
- Add a “Reset North” button (sets yaw = 0).
    

**Do NOT**

- Do not implement compass rotation as a post-process on overlays only; `WorldToScreen` already assumes yaw parity with `PrimaryViewport::world_to_screen`, so yaw must be shared by the real viewport camera.
    
## 1B) Add render hooks (base-style)

Edit: `visualizer/src/ui/viewport/rc_viewport_overlays.h`



```cpp
class ViewportOverlays {
public:
    // ... existing ...
    void RenderScaleRulerHUD();
    void RenderCompassGimbalHUD();
    // ... existing ...
};
```

Edit: `visualizer/src/ui/viewport/rc_viewport_overlays.cpp` — you already do ordered overlay passes inside `ViewportOverlays::Render(...)`, so add HUD near the end (after content overlays, before highlights is fine; grid currently renders late).



```cpp
void ViewportOverlays::Render(const RogueCity::Core::Editor::GlobalState& gs, const OverlayConfig& config) {
    // ... existing overlay calls ...

    if (config.show_scale_ruler) {
        RenderScaleRulerHUD();
    }
    if (config.show_compass_gimbal) {
        RenderCompassGimbalHUD();
    }

    RenderSelectionOutlines(gs);
    RenderHighlights();
}
```

## 2) Scale ruler (draw-list HUD)

This uses the exact style you already use elsewhere in overlays: `ImGui::GetWindowDrawList()->Add...` and viewport size inference from the current ImGui window.

Edit: `visualizer/src/ui/viewport/rc_viewport_overlays.cpp`



```cpp
namespace {
float SnapNiceMeters(float meters) {
    // Snap to 1/2/5 * 10^n
    if (meters <= 0.0f) return 1.0f;
    const float exp10 = std::pow(10.0f, std::floor(std::log10(meters)));
    const float m = meters / exp10;
    float nice = 1.0f;
    if (m >= 5.0f) nice = 5.0f;
    else if (m >= 2.0f) nice = 2.0f;
    return nice * exp10;
}

void FormatDistance(char* out, size_t out_sz, float meters) {
    if (meters >= 1000.0f) {
        std::snprintf(out, out_sz, "%.2f km", meters / 1000.0f);
    } else {
        std::snprintf(out, out_sz, "%.0f m", meters);
    }
}
}

void ViewportOverlays::RenderScaleRulerHUD() {
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Place it in bottom-left of the viewport window
    const ImVec2 vp0 = view_transform_.viewport_pos;
    const ImVec2 vps = view_transform_.viewport_size;

    const float pad = 12.0f;
    const ImVec2 origin = ImVec2(vp0.x + pad, vp0.y + vps.y - pad);

    // Your projection uses zoom as pixels-per-world-unit scale via WorldToScreenScale(). [cite:24]
    const float ppm = std::max(1e-4f, view_transform_.zoom); // pixels per meter (if 1 world unit == 1 meter)

    const float target_px = 140.0f;
    const float raw_m = target_px / ppm;
    const float nice_m = SnapNiceMeters(raw_m);
    const float bar_px = nice_m * ppm;

    const ImU32 col = IM_COL32(240, 240, 240, 220);
    const ImU32 col_shadow = IM_COL32(0, 0, 0, 140);

    // Bar + end ticks
    const ImVec2 a = ImVec2(origin.x, origin.y);
    const ImVec2 b = ImVec2(origin.x + bar_px, origin.y);

    dl->AddLine(ImVec2(a.x+1, a.y+1), ImVec2(b.x+1, b.y+1), col_shadow, 2.0f);
    dl->AddLine(a, b, col, 2.0f);

    dl->AddLine(ImVec2(a.x, a.y - 6), ImVec2(a.x, a.y + 2), col, 2.0f);
    dl->AddLine(ImVec2(b.x, b.y - 6), ImVec2(b.x, b.y + 2), col, 2.0f);

    char label[64];
    FormatDistance(label, sizeof(label), nice_m);
    dl->AddText(ImVec2(a.x, a.y - 22), col, label);
}
```


```cpp
// Somewhere shared (core or app/editor utilities)
struct WorldBounds {
    RogueCity::Core::Vec2 min;
    RogueCity::Core::Vec2 max;
};

inline WorldBounds ComputeWorldBounds(int tex_size, double meters_per_pixel) {
    const double w = static_cast<double>(tex_size) * meters_per_pixel;
    const double h = static_cast<double>(tex_size) * meters_per_pixel;
    const double hw = w * 0.5;
    const double hh = h * 0.5;
    return { {-hw, -hh}, {+hw, +hh} };
}
```

## 1) Overlay toggles (ruler + compass)

Your overlay visibility pattern is already “just add booleans to `OverlayConfig` and branch in `ViewportOverlays::Render()`.”

## 1A) Add config flags (base-style)

Edit: `visualizer/src/ui/viewport/rc_viewport_overlays.h`

```cpp
struct OverlayConfig {
    // ... existing flags ...

    // UI HUD helpers
    bool show_scale_ruler = true;
    bool show_compass_gimbal = true;
};
```
    
Notes (what to do / not do):

- Do: keep it HUD-only; it should not depend on `gs` at all, only `view_transform_`.
    
- Do not: assume `ppm == zoom` unless 1 world unit == 1 meter in your coordinate system; if your world units are not meters, replace `ppm` with a proper conversion factor (e.g., pull `metersPerPixel` from TextureSpace and convert).
    

## 3) Compass gimbal (draw-only here; input handled by viewport controller)

Your overlays currently receive view state via `SetViewTransform(...)` and do not mutate it (Render takes `const GlobalState&`, and the transform is an internal snapshot). So the cleanest pattern is: overlays _draw_ the compass, while `rc_viewport_interaction.cpp` (or whichever controller owns yaw) handles click/drag inside the compass rect and updates the canonical yaw, then the next frame you call `SetViewTransform` with the updated yaw.

Edit: `visualizer/src/ui/viewport/rc_viewport_overlays.cpp`



```cpp
void ViewportOverlays::RenderCompassGimbalHUD() {
    ImDrawList* dl = ImGui::GetWindowDrawList();

    const ImVec2 vp0 = view_transform_.viewport_pos;
    const ImVec2 vps = view_transform_.viewport_size;

    const float pad = 12.0f;
    const float r = 34.0f;

    const ImVec2 c = ImVec2(vp0.x + vps.x - pad - r, vp0.y + pad + r);

    const ImU32 ring = IM_COL32(240, 240, 240, 200);
    const ImU32 fill = IM_COL32(20, 20, 20, 90);
    dl->AddCircleFilled(c, r, fill, 32);
    dl->AddCircle(c, r, ring, 32, 2.0f);

    // Rotate marks by -yaw so the “N” marker indicates world-north relative to the view yaw. [cite:24]
    const float yaw = view_transform_.yaw;
    auto rot = [&](float ang) -> ImVec2 {
        const float a = ang - yaw;
        return ImVec2(std::cos(a), std::sin(a));
    };

    const ImVec2 n = rot(0.0f);
    const ImVec2 e = rot(1.5707963f);
    const ImVec2 s = rot(3.1415926f);
    const ImVec2 w = rot(4.7123890f);

    dl->AddText(ImVec2(c.x + n.x*(r-14) - 4, c.y + n.y*(r-14) - 6), ring, "N");
    dl->AddText(ImVec2(c.x + e.x*(r-14) - 4, c.y + e.y*(r-14) - 6), ring, "E");
    dl->AddText(ImVec2(c.x + s.x*(r-14) - 4, c.y + s.y*(r-14) - 6), ring, "S");
    dl->AddText(ImVec2(c.x + w.x*(r-14) - 4, c.y + w.y*(r-14) - 6), ring, "W");

    // Optional: a small “north needle”
    dl->AddLine(c, ImVec2(c.x + n.x*(r-18), c.y + n.y*(r-18)), IM_COL32(255, 80, 80, 220), 2.0f);
}
```

If you want overlays to _also_ report interaction (without directly owning yaw), add a “requested yaw” latch:

- Add `std::optional<float> requested_yaw_;` to `ViewportOverlays`, set it when dragging inside compass, and let the viewport controller read/consume it after Render and apply it to the canonical camera yaw.
## 4) Make tensors follow primary-road heading (Linear/Stem)

Your tensor visualization reads from `TextureSpace.tensorLayer()` and plots direction vectors directly, so if you align the generator tensor to the axiom’s “major road heading,” the overlay will automatically reflect it.

Fastest generator-side approach (fits your existing API):

- When building tensor fields for each axiom type, for **Linear** and **Stem**, compute `theta` from the axiom’s edited centerline direction (tangent of the primary segment, or average tangent).
    
- Use `TensorFieldGenerator::addGridField(center, radius, theta, decay)` (this function exists and is explicitly designed to orient a tensor grid with a heading).
    
- Blend it strongly near the centerline (higher weight), and let it decay toward the axiom boundary so it doesn’t override neighboring axioms too aggressively.
    

Where to hook it:

- In the same “axiom → generator basis fields” path you extended in your latest commit that “wired terminal-feature support through the app and generator pipeline” (commit message calls out `GeneratorBridge` and new generator appliers).
    
- Do **not** try to hack this in the overlay renderer; `RenderTensorField` is display-only and already trusts the layer data.
    

If you answer one clarification, I can tighten the plan into exact-function edits with zero ambiguity:  
Which object currently owns the canonical camera yaw (GlobalState, viewport state, or overlay view_transform), and where is it updated each frame?

---
Here is the complete, exact-file implementation plan. I've tailored it using your actual codebase structure and types (e.g. `RogueCity::Core::Editor::GlobalState`, `RogueCity::Core::Vec2`, `InitializeTextureSpace`, `DirtyLayer`, and `ViewportOverlays`).

## 0) Scale policy (Policy A) & Toggle

**Goal:** Viewport scaling matches maximum city scale (`texture_size^2`). Add a UI toggle to select the policy.

**Step 0.1: Add the policy enum and config to `GlobalState.hpp`** Edit `core/include/RogueCity/Core/Editor/GlobalState.hpp` inside `struct EditorConfig`:

```cpp
namespace RogueCity::Core::Editor {

enum class ScalePolicy : uint8_t {
    FixedMetersPerPixel = 0,  // Policy A
    FixedWorldExtent = 1      // Policy B (stub)
};

struct EditorConfig {
    // ... existing fields ...
    ScalePolicy scale_policy{ ScalePolicy::FixedMetersPerPixel };
};
```

**Step 0.2: Define the `WorldBounds` helper (used later for clamping)** Add to a utility header or top of `GlobalState.hpp`:

```cpp
inline RogueCity::Core::Bounds ComputeWorldBounds(int tex_size, double meters_per_pixel) {
    const double w = static_cast<double>(tex_size) * meters_per_pixel;
    const double h = static_cast<double>(tex_size) * meters_per_pixel;
    const double hw = w * 0.5;
    const double hh = h * 0.5;
    return { {-hw, -hh}, {+hw, +hh} };
}
```

---

## 1) Texture Size dropdown (single source of truth)

**Step 1.1: Add state fields to `GlobalState.hpp`** Edit `core/include/RogueCity/Core/Editor/GlobalState.hpp` inside `struct GlobalState`:

```cpp
    int city_texture_size{ 2048 };
    double city_meters_per_pixel{ 2.0 };
    bool texture_space_dirty{ true };
```

**Step 1.2: Add the rebuild entry point** Declare in `GlobalState.hpp` (inside `struct GlobalState`):

```cpp
    void EnsureTextureSpaceUpToDate();
```

Implement in `core/src/Core/Editor/GlobalState.cpp`:

```cpp
void GlobalState::EnsureTextureSpaceUpToDate() {
    if (!texture_space_dirty && HasTextureSpace() && texture_space_resolution == city_texture_size) {
        return;
    }

    // Use Policy A bounds
    const Bounds b = ComputeWorldBounds(city_texture_size, city_meters_per_pixel);
    
    InitializeTextureSpace(b, city_texture_size);
    texture_space_bounds = b;
    texture_space_resolution = city_texture_size;
    texture_space_dirty = false;

    MarkAllTextureLayersDirty();
}
```

**Step 1.3: Add the UI Control** In your tools panel (e.g., `rc_panel_tools.cpp`):

```cpp
const char* sizes[] = { "512", "1024", "2048", "4096", "8192" };
int current_idx = 0;
// find current_idx based on gs.city_texture_size...

if (ImGui::Combo("Texture Size", &current_idx, sizes, 5)) {
    gs.city_texture_size = std::stoi(sizes[current_idx]);
    gs.texture_space_dirty = true;
    gs.dirty_layers.MarkDirty(DirtyLayer::Tensor);
    gs.dirty_layers.MarkDirty(DirtyLayer::ViewportIndex);
}

ImGui::SameLine();
if (ImGui::SmallButton("?")) ImGui::OpenPopup("ScalePolicyHelp");
if (ImGui::BeginPopup("ScalePolicyHelp")) {
    ImGui::TextUnformatted("Policy A: Fixed meters-per-pixel (world grows with texture).");
    ImGui::TextUnformatted("Policy B: Fixed world extent (m/px shrinks as texture grows).");
    ImGui::EndPopup();
}

int pol = (gs.config.scale_policy == ScalePolicy::FixedMetersPerPixel) ? 0 : 1;
if (ImGui::RadioButton("Policy A", pol == 0)) gs.config.scale_policy = ScalePolicy::FixedMetersPerPixel;
ImGui::SameLine();
if (ImGui::RadioButton("Policy B", pol == 1)) gs.config.scale_policy = ScalePolicy::FixedWorldExtent;
```

---

## 2) Viewport clamping + city boundary tied to size

**Step 2.1: Update Viewport Clamping** Edit `visualizer/src/ui/viewport/rc_viewport_interaction.cpp`. Modify `TryCityBoundaryBounds` to use the new exact bounds rather than inferring from `gs.city_boundary` points.

```cpp
[[nodiscard]] bool TryCityBoundaryBounds(
    const RogueCity::Core::Editor::GlobalState& gs,
    RogueCity::Core::Vec2& out_min,
    RogueCity::Core::Vec2& out_max) {
    
    // Always use the canonical texture space bounds derived from our chosen scale
    const RogueCity::Core::Bounds b = ComputeWorldBounds(gs.city_texture_size, gs.city_meters_per_pixel);
    out_min = b.min;
    out_max = b.max;
    return true;
}
```

**Step 2.2: Ensure city boundary visual uses the exact bounds** Wherever you populate `gs.city_boundary` (likely where axioms are processed or during layout gen), generate exactly 4 corners of the same box:

```cpp
const auto b = ComputeWorldBounds(gs.city_texture_size, gs.city_meters_per_pixel);
gs.city_boundary = { 
    {b.min.x, b.min.y}, 
    {b.max.x, b.min.y}, 
    {b.max.x, b.max.y}, 
    {b.min.x, b.max.y} 
};
```

---

## 3) Add ruler + compass gimbal (overlay HUD)

**Step 3.1: Add flags to `OverlayConfig`** Edit `visualizer/src/ui/viewport/rc_viewport_overlays.h`:

```cpp
struct OverlayConfig {
    // ... existing flags ...
    bool show_scale_ruler = true;
    bool show_compass_gimbal = true;
};

class ViewportOverlays {
public:
    // ... existing functions ...
    void RenderScaleRulerHUD(const RogueCity::Core::Editor::GlobalState& gs);
    void RenderCompassGimbalHUD();
    std::optional<float> requested_yaw_{}; // For the viewport controller to consume [cite:24]
};
```

**Step 3.2: Hook into `Render(...)`** Edit `visualizer/src/ui/viewport/rc_viewport_overlays.cpp`:

```cpp
void ViewportOverlays::Render(const RogueCity::Core::Editor::GlobalState& gs, const OverlayConfig& config) {
    // ... existing ...
    if (config.show_scale_ruler) { RenderScaleRulerHUD(gs); }
    if (config.show_compass_gimbal) { RenderCompassGimbalHUD(); }
    
    RenderSelectionOutlines(gs);
    RenderHighlights();
}
```

**Step 3.3: Ruler Implementation** Add this to `rc_viewport_overlays.cpp`:

```cpp
void ViewportOverlays::RenderScaleRulerHUD(const RogueCity::Core::Editor::GlobalState& gs) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 vp0 = view_transform_.viewport_pos;
    const ImVec2 vps = view_transform_.viewport_size;
    const float pad = 12.0f;
    const ImVec2 origin = ImVec2(vp0.x + pad, vp0.y + vps.y - pad);

    // Use actual texture space scaling if possible, otherwise fallback to zoom
    double mpp = gs.city_meters_per_pixel; 
    if (gs.HasTextureSpace()) {
        mpp = gs.TextureSpaceRef().coordinateSystem().metersPerPixel();
    }
    
    // Zoom factor applies to base world scale
    const float ppm = std::max(1e-4f, view_transform_.zoom / static_cast<float>(mpp));

    const float target_px = 140.0f;
    const float raw_m = target_px / ppm;
    
    // Snap to nice round numbers
    const float exp10 = std::pow(10.0f, std::floor(std::log10(raw_m)));
    const float m = raw_m / exp10;
    float nice_m = (m >= 5.0f) ? 5.0f : (m >= 2.0f ? 2.0f : 1.0f);
    nice_m *= exp10;
    
    const float bar_px = nice_m * ppm;
    const ImU32 col = IM_COL32(240, 240, 240, 220);
    const ImU32 col_shadow = IM_COL32(0, 0, 0, 140);
    const ImVec2 a = origin;
    const ImVec2 b = ImVec2(origin.x + bar_px, origin.y);

    dl->AddLine(ImVec2(a.x+1, a.y+1), ImVec2(b.x+1, b.y+1), col_shadow, 2.0f);
    dl->AddLine(a, b, col, 2.0f);
    dl->AddLine(ImVec2(a.x, a.y - 6), ImVec2(a.x, a.y + 2), col, 2.0f);
    dl->AddLine(ImVec2(b.x, b.y - 6), ImVec2(b.x, b.y + 2), col, 2.0f);

    char label[64];
    if (nice_m >= 1000.0f) std::snprintf(label, sizeof(label), "%.2f km", nice_m / 1000.0f);
    else std::snprintf(label, sizeof(label), "%.0f m", nice_m);
    dl->AddText(ImVec2(a.x, a.y - 22), col, label);
}
```

**Step 3.4: Compass Gimbal Implementation** Add this to `rc_viewport_overlays.cpp`:

```cpp
void ViewportOverlays::RenderCompassGimbalHUD() {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 vp0 = view_transform_.viewport_pos;
    const ImVec2 vps = view_transform_.viewport_size;
    const float pad = 12.0f;
    const float r = 34.0f;
    const ImVec2 c = ImVec2(vp0.x + vps.x - pad - r, vp0.y + pad + r);

    const ImU32 ring = IM_COL32(240, 240, 240, 200);
    const ImU32 fill = IM_COL32(20, 20, 20, 90);
    dl->AddCircleFilled(c, r, fill, 32);
    dl->AddCircle(c, r, ring, 32, 2.0f);

    const float yaw = view_transform_.yaw;
    auto rot = [&](float ang) -> ImVec2 {
        const float a = ang - yaw;
        return ImVec2(std::cos(a), std::sin(a));
    };

    const ImVec2 n = rot(0.0f);
    const ImVec2 e = rot(1.5707963f);
    const ImVec2 s = rot(3.1415926f);
    const ImVec2 w = rot(4.7123890f);

    dl->AddText(ImVec2(c.x + n.x*(r-14) - 4, c.y + n.y*(r-14) - 6), ring, "N");
    dl->AddText(ImVec2(c.x + e.x*(r-14) - 4, c.y + e.y*(r-14) - 6), ring, "E");
    dl->AddText(ImVec2(c.x + s.x*(r-14) - 4, c.y + s.y*(r-14) - 6), ring, "S");
    dl->AddText(ImVec2(c.x + w.x*(r-14) - 4, c.y + w.y*(r-14) - 6), ring, "W");

    // Interaction handling (pass requested yaw back out)
    ImVec2 mouse_pos = ImGui::GetMousePos();
    float dist = std::hypot(mouse_pos.x - c.x, mouse_pos.y - c.y);
    if (dist <= r && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (std::hypot(mouse_pos.x - (c.x + n.x*(r-14)), mouse_pos.y - (c.y + n.y*(r-14))) < 10.0f) {
            requested_yaw_ = 0.0f; // Reset North clicked
        } else {
            requested_yaw_ = std::atan2(mouse_pos.y - c.y, mouse_pos.x - c.x) + 1.5707963f; 
        }
    }
}
```

_Note: The viewport controller (`rc_viewport_interaction.cpp` / `PrimaryViewport`) must read `GetViewportOverlays().requested_yaw_`, apply it to `set_camera_yaw()`, and clear the latch._

---

## 4) Make tensors follow primary-road heading (Linear/Stem)

Because TensorField processing happens before `ApplyCityOutputToGlobalState` (in `CityGenerator.cpp` or the generator modules), you must inject the axiom's heading property at the time the road polyline is laid down.

**Find where your Roads are constructed from Axioms** (likely inside a Generator Applier for `Linear` / `Stem`). When you create the road points for the core axis, update `axiom.theta` directly:

```cpp
// In your Generator logic where Linear/Stem axioms create their central spline:
if (axiom.type == RogueCity::Core::Editor::EditorAxiom::Type::Linear || 
    axiom.type == RogueCity::Core::Editor::EditorAxiom::Type::Stem) {
    
    // Assuming 'main_road_points' contains your primary axis RogueCity::Core::Vec2 points
    if (main_road_points.size() >= 2) {
        const auto& a = main_road_points.front();
        const auto& b = main_road_points.back();
        
        // Compute theta and store it back on the axiom so the TensorField generator sees it
        axiom.theta = std::atan2(b.y - a.y, b.x - a.x);
    }
}
```

Then, in your Tensor generator (wherever `addGridField` or equivalent is called for the tensor data layer):

```cpp
// In the Tensor Generator Pass:
if (axiom.type == EditorAxiom::Type::Linear || axiom.type == EditorAxiom::Type::Stem) {
    // Force the tensor field orientation to match the road's heading
    tensor_field.addGridField(
        axiom.position, 
        axiom.radius, 
        axiom.theta,  // Now correctly populated by the road heading!
        axiom.decay
    );
}
```