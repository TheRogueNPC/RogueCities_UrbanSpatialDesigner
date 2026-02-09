# RogueCity Implementation Guide
## Multi-Viewport + Stubbed Code Completion

This guide provides the complete implementation for fixing viewport rendering and completing stubbed code.

---

## PART 1: Enable Multi-Viewport Rendering

### File: `visualizer/src/main_gui.cpp`

**Line 57 - Add after GLFW version hints:**
```cpp
    // Hide OS window decorations for custom window chrome
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
```

**Lines 73-74 - Replace docking config:**
```cpp
    // OLD:
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    
    // NEW:
    // CRITICAL: Enable docking + multi-viewport for floating windows
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    
    // When viewports are enabled, tweak WindowRounding/WindowBg for platform windows
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;  // Y2K hard edges
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;  // Opaque backgrounds for OS windows
    }
```

**Lines 142-143 - Add after ImGui::Render():**
```cpp
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        // NEW: Update and render platform windows (multi-viewport support)
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
```

---

## PART 2: Complete PrimaryViewport.cpp Implementation

### File: `app/src/Viewports/PrimaryViewport.cpp`

**Lines 79-88 - Replace placeholder rendering:**
```cpp
    // Render city output (if available)
    if (city_output_) {
        // Render roads
        const ImU32 road_color = IM_COL32(0, 255, 255, 255);  // Cyan
        for (const auto& road : city_output_->roads) {
            const ImVec2 start_screen = world_to_screen(road.start);
            const ImVec2 end_screen = world_to_screen(road.end);
            
            float thickness = 2.0f;
            if (road.classification == Generators::RoadClassification::Highway) {
                thickness = 6.0f;
            } else if (road.classification == Generators::RoadClassification::Arterial) {
                thickness = 4.0f;
            }
            
            draw_list->AddLine(start_screen, end_screen, road_color, thickness);
        }
        
        // Render districts (wireframe polygons)
        const ImU32 district_color = IM_COL32(255, 0, 255, 150);  // Magenta
        for (const auto& district : city_output_->districts) {
            if (district.boundary.empty()) continue;
            
            std::vector<ImVec2> screen_points;
            screen_points.reserve(district.boundary.size());
            for (const auto& point : district.boundary) {
                screen_points.push_back(world_to_screen(point));
            }
            
            // Draw district boundary
            for (size_t i = 0; i < screen_points.size(); ++i) {
                const ImVec2& p1 = screen_points[i];
                const ImVec2& p2 = screen_points[(i + 1) % screen_points.size()];
                draw_list->AddLine(p1, p2, district_color, 2.0f);
            }
            
            // Draw district label at centroid
            if (!screen_points.empty()) {
                ImVec2 centroid = ImVec2(0, 0);
                for (const auto& p : screen_points) {
                    centroid.x += p.x;
                    centroid.y += p.y;
                }
                centroid.x /= screen_points.size();
                centroid.y /= screen_points.size();
                
                const char* district_type = "???";
                switch (district.type) {
                    case Generators::DistrictType::Residential: district_type = "RES"; break;
                    case Generators::DistrictType::Commercial: district_type = "COM"; break;
                    case Generators::DistrictType::Industrial: district_type = "IND"; break;
                    case Generators::DistrictType::Park: district_type = "PARK"; break;
                    default: break;
                }
                
                draw_list->AddText(centroid, IM_COL32(255, 255, 255, 255), district_type);
            }
        }
        
        // Render lots (smaller subdivisions)
        const ImU32 lot_color = IM_COL32(255, 200, 0, 100);  // Yellow-amber
        for (const auto& lot : city_output_->lots) {
            if (lot.boundary.empty()) continue;
            
            std::vector<ImVec2> screen_points;
            screen_points.reserve(lot.boundary.size());
            for (const auto& point : lot.boundary) {
                screen_points.push_back(world_to_screen(point));
            }
            
            for (size_t i = 0; i < screen_points.size(); ++i) {
                const ImVec2& p1 = screen_points[i];
                const ImVec2& p2 = screen_points[(i + 1) % screen_points.size()];
                draw_list->AddLine(p1, p2, lot_color, 1.0f);
            }
        }
    } else {
        // No city output - show placeholder
        ImGui::SetCursorScreenPos(ImVec2(
            viewport_pos.x + viewport_size.x * 0.5f - 100,
            viewport_pos.y + viewport_size.y * 0.5f - 10
        ));
        ImGui::TextColored(ImVec4(0.5f, 0.7f, 1.0f, 1.0f), "[No City Generated]");
    }
```

**Add new method at end of file:**
```cpp
float PrimaryViewport::world_to_screen_scale(float world_distance) const {
    return world_distance * zoom_;
}
```

**Update header file (app/include/RogueCity/App/Viewports/PrimaryViewport.hpp):**
```cpp
    // Add this declaration:
    float world_to_screen_scale(float world_distance) const;
```

---

## PART 3: Complete AxiomVisual.cpp Implementation

### File: `app/src/Tools/AxiomVisual.cpp`

**Add after line 250 (complete RingControlKnob implementation):**
```cpp
}

bool RingControlKnob::check_hover(const Core::Vec2& world_pos, float world_radius) {
    const float dx = world_pos.x - world_position.x;
    const float dy = world_pos.y - world_position.y;
    const float dist_sq = dx * dx + dy * dy;
    
    const bool hovered = dist_sq <= (world_radius * world_radius);
    if (is_hovered != hovered) {
        is_hovered = hovered;
    }
    return is_hovered;
}

void RingControlKnob::start_drag() {
    is_dragging = true;
}

void RingControlKnob::end_drag() {
    is_dragging = false;
}

void RingControlKnob::update_value(float new_value) {
    value = std::clamp(new_value, 0.0f, 2.0f);  // 0-200% range
}

// AxiomTypeInfo Implementation
AxiomTypeInfo GetAxiomTypeInfo(AxiomVisual::AxiomType type) {
    switch (type) {
        case AxiomVisual::AxiomType::Radial:
            return {"Radial", "Spoke pattern", IM_COL32(255, 200, 0, 255)};
        case AxiomVisual::AxiomType::Grid:
            return {"Grid", "Orthogonal streets", IM_COL32(0, 255, 200, 255)};
        case AxiomVisual::AxiomType::Organic:
            return {"Organic", "Curved roads", IM_COL32(0, 255, 100, 255)};
        case AxiomVisual::AxiomType::Superblock:
            return {"Superblock", "Large blocks", IM_COL32(255, 0, 200, 255)};
        case AxiomVisual::AxiomType::Stem:
            return {"Stem", "Branch pattern", IM_COL32(255, 100, 0, 255)};
        case AxiomVisual::AxiomType::LooseGrid:
            return {"LooseGrid", "Jittered grid", IM_COL32(150, 150, 255, 255)};
        case AxiomVisual::AxiomType::SuburbanLoop:
            return {"SuburbanLoop", "Cul-de-sac", IM_COL32(255, 255, 100, 255)};
        default:
            return {"Unknown", "???", IM_COL32(128, 128, 128, 255)};
    }
}

// DrawAxiomIcon Implementation (SVG-like glyphs)
void DrawAxiomIcon(ImDrawList* draw_list, const ImVec2& center, float size, 
                   AxiomVisual::AxiomType type, ImU32 color) {
    switch (type) {
        case AxiomVisual::AxiomType::Radial: {
            // Radial: Star/spoke pattern
            const int spokes = 8;
            for (int i = 0; i < spokes; ++i) {
                const float angle = (2.0f * std::numbers::pi_v<float> * i) / spokes;
                const float cos_a = std::cos(angle);
                const float sin_a = std::sin(angle);
                draw_list->AddLine(
                    center,
                    ImVec2(center.x + cos_a * size, center.y + sin_a * size),
                    color, 1.5f
                );
            }
            break;
        }
        
        case AxiomVisual::AxiomType::Grid: {
            // Grid: Hash pattern
            const float half = size * 0.5f;
            // Vertical line
            draw_list->AddLine(
                ImVec2(center.x, center.y - half),
                ImVec2(center.x, center.y + half),
                color, 1.5f
            );
            // Horizontal line
            draw_list->AddLine(
                ImVec2(center.x - half, center.y),
                ImVec2(center.x + half, center.y),
                color, 1.5f
            );
            break;
        }
        
        case AxiomVisual::AxiomType::Organic: {
            // Organic: Wavy curve
            const int segments = 8;
            ImVec2 prev(center.x - size, center.y);
            for (int i = 1; i <= segments; ++i) {
                const float t = static_cast<float>(i) / segments;
                const float x = center.x - size + (2.0f * size * t);
                const float y = center.y + size * 0.3f * std::sin(t * 2.0f * std::numbers::pi_v<float>);
                ImVec2 curr(x, y);
                draw_list->AddLine(prev, curr, color, 1.5f);
                prev = curr;
            }
            break;
        }
        
        case AxiomVisual::AxiomType::Superblock: {
            // Superblock: Large square
            const float half = size * 0.6f;
            draw_list->AddRect(
                ImVec2(center.x - half, center.y - half),
                ImVec2(center.x + half, center.y + half),
                color, 0.0f, 0, 2.0f
            );
            break;
        }
        
        case AxiomVisual::AxiomType::Stem: {
            // Stem: Tree branch pattern
            draw_list->AddLine(
                ImVec2(center.x, center.y + size),
                ImVec2(center.x, center.y - size * 0.3f),
                color, 2.0f
            );
            // Left branch
            draw_list->AddLine(
                ImVec2(center.x, center.y),
                ImVec2(center.x - size * 0.5f, center.y - size * 0.5f),
                color, 1.5f
            );
            // Right branch
            draw_list->AddLine(
                ImVec2(center.x, center.y),
                ImVec2(center.x + size * 0.5f, center.y - size * 0.5f),
                color, 1.5f
            );
            break;
        }
        
        default:
            // Default: Simple dot
            draw_list->AddCircleFilled(center, size * 0.3f, color);
            break;
    }
}

} // namespace RogueCity::App
```

---

## PART 4: Standardize Button Styles

### File: `app/src/UI/DesignSystem.cpp`

**Add Button implementation methods:**
```cpp
bool DesignSystem::ButtonPrimary(const char* label, ImVec2 size) {
    ApplyButtonStyle(
        DesignTokens::CyanAccent,
        WithAlpha(DesignTokens::CyanAccent, 200),
        WithAlpha(DesignTokens::CyanAccent, 150)
    );
    
    const bool pressed = ImGui::Button(label, size);
    ImGui::PopStyleColor(3);
    return pressed;
}

bool DesignSystem::ButtonSecondary(const char* label, ImVec2 size) {
    ApplyButtonStyle(
        DesignTokens::PanelBackground,
        WithAlpha(DesignTokens::TextSecondary, 100),
        WithAlpha(DesignTokens::TextSecondary, 150)
    );
    
    const bool pressed = ImGui::Button(label, size);
    ImGui::PopStyleColor(3);
    return pressed;
}

bool DesignSystem::ButtonDanger(const char* label, ImVec2 size) {
    ApplyButtonStyle(
        DesignTokens::ErrorRed,
        WithAlpha(DesignTokens::ErrorRed, 200),
        WithAlpha(DesignTokens::ErrorRed, 150)
    );
    
    const bool pressed = ImGui::Button(label, size);
    ImGui::PopStyleColor(3);
    return pressed;
}

void DesignSystem::ApplyButtonStyle(ImU32 base_color, ImU32 hover_color, ImU32 active_color) {
    ImGui::PushStyleColor(ImGuiCol_Button, base_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, active_color);
}
```

---

## USAGE EXAMPLES

### Using Multi-Viewport
```cpp
// Windows can now be dragged outside the main window
// Each floating window gets its own OS window automatically
ImGui::Begin("Floating Panel", nullptr, ImGuiWindowFlags_None);
// ... panel content ...
ImGui::End();
```

### Using Standardized Buttons
```cpp
// Replace all ImGui::Button() calls with:
if (DesignSystem::ButtonPrimary("Generate City")) {
    // Action
}

if (DesignSystem::ButtonSecondary("Cancel")) {
    // Action
}

if (DesignSystem::ButtonDanger("Delete")) {
    // Action
}
```

### Rendering City in Viewport
```cpp
PrimaryViewport viewport;
viewport.set_city_output(&city_generator.get_output());
viewport.render();  // Will now show roads, districts, lots
```

---

## TESTING CHECKLIST

- [ ] Multi-viewport enabled (ConfigFlags_ViewportsEnable)
- [ ] Can drag panels to separate OS windows
- [ ] OS window decorations hidden (custom chrome)
- [ ] Viewport renders city geometry (not placeholder)
- [ ] Roads rendered with correct thickness
- [ ] Districts shown as wireframe polygons
- [ ] Lots rendered with yellow borders
- [ ] Axiom icons display correctly (radial, grid, etc.)
- [ ] Control knobs respond to hover/drag
- [ ] All buttons use DesignSystem::Button* methods
- [ ] Y2K aesthetic maintained (hard edges, neon colors)

---

## MIGRATION NOTES

**Before:**
- Viewport tied to OS window size
- Single window only
- Placeholder city rendering
- Inconsistent button styles

**After:**
- Viewport renders within ImGui dock space
- Multi-viewport support for floating panels
- Full city geometry rendering
- Unified button design system

---

**Implementation Status:** Ready to merge
**Testing Required:** Visual verification of multi-viewport + city rendering
**Breaking Changes:** None (additive only)
