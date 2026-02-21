## Task 1 — Core State: Scale Policy + World Bounds + TextureSpace Rebuild

**Feed these files:** `core/include/RogueCity/Core/Editor/GlobalState.hpp` + `core/src/Core/Editor/GlobalState.cpp`[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/tree/main/core/src/Core/Editor)​

**Prompt:**

> I'm adding texture size and scale policy state to my city editor. Here are the two files. Make these exact changes and return the full modified files:
> 
> **In `GlobalState.hpp`:**
> 
> 1. Inside `namespace RogueCity::Core::Editor`, before `struct EditorConfig`, add:
>     
>     cpp
>     
>     ```
>     enum class ScalePolicy : uint8_t { FixedMetersPerPixel = 0, FixedWorldExtent = 1 };
>     ```
>     
> 2. At the end of `struct EditorConfig` (after `feature_city_boundary_hull`), add:
>     
>     cpp
>     
>     ```
>     ScalePolicy scale_policy{ ScalePolicy::FixedMetersPerPixel };
>     ```
>     
> 3. At the end of `struct GlobalState` (before `[[nodiscard]] static uint64_t MakeEntityKey`), add:
>     
>     cpp
>     
>     ```
>     int city_texture_size{ 2048 };
>     double city_meters_per_pixel{ 2.0 };
>     bool texture_space_dirty{ true };
>     ```
>     
> 4. After `InitializeTextureSpace` declaration, add:
>     
>     cpp
>     
>     ```
>     void EnsureTextureSpaceUpToDate();
>     ```
>     
> 5. After the closing brill of `struct GlobalState`, add this inline helper in the same namespace:
>     
>     cpp
>     
>     ```
>     inline Bounds ComputeWorldBounds(int tex_size, double meters_per_pixel) {
>         const double hw = (tex_size * meters_per_pixel) * 0.5;
>         return { {-hw, -hw}, {+hw, +hw} };
>     }
>     ```
>     
> 
> **In `GlobalState.cpp`:**  
> Add this function body. `texture_space_resolution` and `texture_space_bounds` already exist as members; `InitializeTextureSpace`, `MarkAllTextureLayersDirty` already exist as methods:
> 
> cpp
> 
> ```
> void GlobalState::EnsureTextureSpaceUpToDate() {
>     if (!texture_space_dirty && HasTextureSpace() && texture_space_resolution == city_texture_size)
>         return;
>     const Bounds b = ComputeWorldBounds(city_texture_size, city_meters_per_pixel);
>     InitializeTextureSpace(b, city_texture_size);
>     texture_space_bounds = b;
>     texture_space_resolution = city_texture_size;
>     texture_space_dirty = false;
>     MarkAllTextureLayersDirty();
> }
> ```
> 
> **DO NOT** rename or touch any existing fields (`texture_space_bounds`, `texture_space_resolution`, `HasTextureSpace`, `InitializeTextureSpace`, `MarkAllTextureLayersDirty`). **DO NOT** add any new includes. Return only the changed blocks with surrounding context, not the entire files.

---

## Task 2 — UI Dropdown + Viewport Clamping + City Boundary

**Feed these files:** `visualizer/src/ui/panels/rc_panel_tools.cpp` + `visualizer/src/ui/viewport/rc_viewport_interaction.cpp`

**Prompt:**

> I need to wire a new texture size dropdown and scale policy toggle into my ImGui tools panel, and update my viewport clamping to use the new bounds. Here are the two files. Make these exact changes:
> 
> **In `rc_panel_tools.cpp`:**  
> Find the main `Draw(GlobalState& gs)` method and add this block (pick a logical spot near other generation params):
> 
> cpp
> 
> ```
> // --- Texture Size + Scale Policy ---
> static const char* kTexSizes[] = { "512", "1024", "2048", "4096", "8192" };
> static const int kTexSizeVals[] = { 512, 1024, 2048, 4096, 8192 };
> int tex_idx = 2; // default 2048
> for (int i = 0; i < 5; ++i) { if (kTexSizeVals[i] == gs.city_texture_size) { tex_idx = i; break; } }
> if (ImGui::Combo("Texture Size", &tex_idx, kTexSizes, 5)) {
>     gs.city_texture_size = kTexSizeVals[tex_idx];
>     gs.texture_space_dirty = true;
>     gs.dirty_layers.MarkDirty(DirtyLayer::Tensor);
>     gs.dirty_layers.MarkDirty(DirtyLayer::ViewportIndex);
> }
> ImGui::SameLine();
> if (ImGui::SmallButton("?")) ImGui::OpenPopup("ScalePolicyHelp");
> if (ImGui::BeginPopup("ScalePolicyHelp")) {
>     ImGui::TextUnformatted("Policy A (active): Fixed m/px. World grows with texture size.");
>     ImGui::TextUnformatted("Policy B (stub): Fixed world extent. Not yet active.");
>     ImGui::EndPopup();
> }
> using SP = RogueCity::Core::Editor::ScalePolicy;
> int pol = gs.config.scale_policy == SP::FixedMetersPerPixel ? 0 : 1;
> if (ImGui::RadioButton("Policy A", pol == 0)) gs.config.scale_policy = SP::FixedMetersPerPixel;
> ImGui::SameLine();
> if (ImGui::RadioButton("Policy B (stub)", pol == 1)) gs.config.scale_policy = SP::FixedWorldExtent;
> ```
> 
> **In `rc_viewport_interaction.cpp`:**  
> Find the function `TryCityBoundaryBounds` (or the equivalent function that provides world-space min/max to the spring clamp logic). Replace its body so it always derives bounds from the new fields instead of iterating `gs.city_boundary` points:
> 
> cpp
> 
> ```
> const RogueCity::Core::Bounds b = RogueCity::Core::Editor::ComputeWorldBounds(
>     gs.city_texture_size, gs.city_meters_per_pixel);
> out_min = b.min;
> out_max = b.max;
> return true;
> ```
> 
> Also find wherever `gs.city_boundary` is populated with 4 corner points and update it to:
> 
> cpp
> 
> ```
> const auto b = RogueCity::Core::Editor::ComputeWorldBounds(gs.city_texture_size, gs.city_meters_per_pixel);
> gs.city_boundary = { {b.min.x,b.min.y},{b.max.x,b.min.y},{b.max.x,b.max.y},{b.min.x,b.max.y} };
> ```
> 
> **DO NOT** remove the spring clamp logic or the existing overscroll/velocity fields in `ToolRuntimeState` (`viewport_clamp_overscroll`, `viewport_clamp_velocity`, `viewport_clamp_active`). **DO NOT** store size only in local UI state. Return only the changed blocks with surrounding context.

---

## Task 3 — Overlay HUD: Scale Ruler + Compass Gimbal

**Feed this file:** `visualizer/src/ui/viewport/rc_viewport_overlays.h` + `visualizer/src/ui/viewport/rc_viewport_overlays.cpp`

**Prompt:**

> I need to add a scale ruler and compass gimbal HUD to my viewport overlay system. The namespace is `RC_UI::Viewport`. The class is `ViewportOverlays`. `ViewTransform` already has `zoom`, `yaw`, `viewport_pos`, `viewport_size`. `OverlayConfig` already has many `show_*` bools. Here are the two files. Make these exact changes:
> 
> **In `rc_viewport_overlays.h`:**
> 
> 1. At the end of `struct OverlayConfig` (before the closing `};`), add:
>     
>     cpp
>     
>     ```
>     bool show_scale_ruler = true;
>     bool show_compass_gimbal = true;
>     ```
>     
> 2. In the `public:` section of `class ViewportOverlays`, after `RenderConnectorGraph`, add:
>     
>     cpp
>     
>     ```
>     void RenderScaleRulerHUD(const RogueCity::Core::Editor::GlobalState& gs);
>     void RenderCompassGimbalHUD();
>     std::optional<float> requested_yaw_{};
>     ```
>     
> 
> **In `rc_viewport_overlays.cpp`:**
> 
> 1. In `ViewportOverlays::Render(...)`, before `RenderSelectionOutlines(gs)`, add:
>     
>     cpp
>     
>     ```
>     if (config.show_scale_ruler) RenderScaleRulerHUD(gs);
>     if (config.show_compass_gimbal) RenderCompassGimbalHUD();
>     ```
>     
> 2. Implement `RenderScaleRulerHUD`: placed bottom-left using `view_transform_.viewport_pos` + `viewport_size`. Read `mpp = gs.HasTextureSpace() ? gs.TextureSpaceRef().coordinateSystem().metersPerPixel() : gs.city_meters_per_pixel`. Compute `ppm = max(1e-4f, view_transform_.zoom / float(mpp))`. Target 140px. Snap to `1/2/5 * 10^n`. Draw via `ImGui::GetWindowDrawList()->AddLine` + `AddText`. Format as `"X m"` or `"X.XX km"`.
>     
> 3. Implement `RenderCompassGimbalHUD`: placed top-right. Draw a filled dark circle + white ring via `AddCircleFilled`/`AddCircle`. Rotate N/E/S/W labels by `-view_transform_.yaw` using `cos/sin`. Draw a red north needle. On `ImGui::IsMouseClicked(ImGuiMouseButton_Left)` inside the circle radius: if near the N label, set `requested_yaw_ = 0.0f`; otherwise set `requested_yaw_ = atan2(dy, dx) + π/2`.
>     
> 
> **DO NOT** add `#include <optional>` if it's already transitively included via `GlobalState.hpp`. **DO NOT** make `RenderCompassGimbalHUD` mutate `view_transform_` directly — write to `requested_yaw_` only so the viewport controller consumes it next frame. **DO NOT** make the ruler depend on `gs` for anything other than `city_meters_per_pixel` / `TextureSpaceRef`. Return only the new function bodies and the modified `Render` call.

---

## Task 4 — Tensor Heading from Linear/Stem Axiom Road Direction

**Feed these files:** `generators/src/Generators/Tensors/TensorFieldGenerator.cpp` + `generators/include/RogueCity/Generators/Tensors/TensorFieldGenerator.hpp`

**Prompt:**

> I need the tensor field for `Linear` and `Stem` axioms to align with the actual road heading instead of using the raw `axiom.theta` default. The axiom struct lives in `RogueCity::Core::Editor::EditorAxiom`. The type enum values are `EditorAxiom::Type::Linear` and `EditorAxiom::Type::Stem`. `addGridField` already exists and takes `(center, radius, theta, decay)`. Here is the generator file. Make this exact change:
> 
> Find the loop or section in `TensorFieldGenerator.cpp` that iterates axioms and calls `addGridField`. For each axiom of type `Linear` or `Stem`, before calling `addGridField`, check if the axiom has a `main_road_points` vector (or equivalent road-axis point list) with at least 2 points. If yes, compute:
> 
> cpp
> 
> ```
> const auto& pa = main_road_points.front();
> const auto& pb = main_road_points.back();
> const double heading = std::atan2(pb.y - pa.y, pb.x - pa.x);
> axiom.theta = heading; // or pass heading directly to addGridField
> ```
> 
> Then pass `heading` (or the updated `axiom.theta`) as the theta argument to `addGridField`.
> 
> If `main_road_points` is not directly accessible at that call site, tell me what the variable is called where the road axis points are built for the axiom in this file, and I'll clarify.
> 
> **DO NOT** touch `Organic`, `Grid`, `Radial`, `Hexagonal`, or any other axiom type. **DO NOT** modify `BasisFields.cpp` or `TerminalFeatureApplier.cpp`. **DO NOT** try to read heading from the overlay renderer. Return only the modified loop/if-block with surrounding context so I can locate it precisely.

---

## File Map Summary (for your own reference)

|Task|Header(s)|Implementation(s)|
|---|---|---|
|1 – Core State|`core/include/RogueCity/Core/Editor/GlobalState.hpp`|`core/src/Core/Editor/GlobalState.cpp`|
|2 – UI + Clamping|`(none new)`|`visualizer/src/ui/panels/rc_panel_tools.cpp` + `visualizer/src/ui/viewport/rc_viewport_interaction.cpp`|
|3 – HUD Overlays|`visualizer/src/ui/viewport/rc_viewport_overlays.h`|`visualizer/src/ui/viewport/rc_viewport_overlays.cpp`|
|4 – Tensor Heading|`generators/include/RogueCity/Generators/Tensors/TensorFieldGenerator.hpp`|`generators/src/Generators/Tensors/TensorFieldGenerator.cpp`|

Do them in order — Task 2 depends on the `ComputeWorldBounds` inline from Task 1, and Task 3 depends on `gs.city_meters_per_pixel` + `HasTextureSpace()`/`TextureSpaceRef()` being in the GlobalState signature which Task 1 establishes.