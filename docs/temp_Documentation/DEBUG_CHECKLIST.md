# COMPREHENSIVE FIX FOR ALL THREE ISSUES

## Issue Diagnosis:

1. **Docking not working** - Likely because dockspace needs proper initialization
2. **Viewport tied to window** - ViewportOverlays using wrong coordinates  
3. **City not rendering** - Roads/districts not being generated or not in GlobalState

---

## Fix 1: Force Docking to Work

### Problem:
Panels might have invisible blocking flags

### Solution:
Add debug output to verify docking is enabled, then check each panel

**File: `visualizer/src/main_gui.cpp`** (line ~130, inside main loop before DrawRoot)

```cpp
// Debug: Verify docking is enabled
static bool printed_once = false;
if (!printed_once) {
    printf("[DEBUG] ConfigFlags: 0x%X\n", io.ConfigFlags);
    printf("[DEBUG] DockingEnable: %d\n", !!(io.ConfigFlags & ImGuiConfigFlags_DockingEnable));
    printf("[DEBUG] ViewportsEnable: %d\n", !!(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable));
    printed_once = true;
}
```

---

## Fix 2: Viewport Coordinate Issue

### Problem:
Viewport overlays render using window space instead of ImGui content space

### Test Code:
Add this to `visualizer/src/ui/panels/rc_panel_axiom_editor.cpp` around line 850:

```cpp
// DEBUG: Print transform values
static int frame_count = 0;
if (frame_count++ % 60 == 0) {  // Every 60 frames
    printf("[VIEWPORT] pos=(%.0f,%.0f) size=(%.0f,%.0f) zoom=%.2f\n",
        viewport_pos.x, viewport_pos.y,
        viewport_size.x, viewport_size.y,
        transform.zoom);
}
```

### Expected Output:
```
[VIEWPORT] pos=(200,100) size=(1000,600) zoom=1.00
```

If `pos=(0,0)` ? Viewport is using wrong coordinates!

---

## Fix 3: City Not Rendering

### Problem:
GlobalState might not have roads/districts OR they're rendering but invisible

### Test 1: Check if data exists

**Add to `visualizer/src/ui/panels/rc_panel_axiom_editor.cpp` around line 845:**

```cpp
// DEBUG: Check GlobalState data
static int debug_frame = 0;
if (debug_frame++ % 120 == 0) {  // Every 2 seconds
    printf("[CITY DATA] Roads: %llu, Districts: %llu, Lots: %llu\n",
        (unsigned long long)gs.roads.size(),
        (unsigned long long)gs.districts.size(),
        (unsigned long long)gs.lots.size());
}
```

### Test 2: Force draw a test road

**Add AFTER grid rendering (line ~622):**

```cpp
// DEBUG: Draw test road to verify rendering works
draw_list->AddLine(
    ImVec2(viewport_pos.x + 100, viewport_pos.y + 100),
    ImVec2(viewport_pos.x + 300, viewport_pos.y + 300),
    IM_COL32(255, 0, 255, 255),  // Bright magenta
    5.0f
);
ImGui::SetCursorScreenPos(ImVec2(viewport_pos.x + 110, viewport_pos.y + 110));
ImGui::TextColored(ImVec4(1, 0, 1, 1), "TEST ROAD");
```

**Expected:** Magenta diagonal line from top-left

---

## Quick Actions to Try:

### Action 1: Verify Docking Works
1. Run the program
2. Try to drag "Log" panel title bar
3. Look for blue docking zones
4. If NO zones appear ? Docking disabled
5. If zones appear but panel snaps back ? Window has NoDocking flag

### Action 2: Generate a City
1. Open "Axiom Editor" panel
2. Click to place an axiom in viewport
3. Click "Generate" button (if exists)
4. Check console output for city data

### Action 3: Check Console Output
Look for these messages when program starts:
```
[DEBUG] DockingEnable: 1
[DEBUG] ViewportsEnable: 1
[VIEWPORT] pos=(X,Y) size=(W,H)  // Should NOT be (0,0)
[CITY DATA] Roads: N  // Should be >0 after generation
```

---

## If Still Broken:

### Docking Still Locked?
**Check if BeginPanel has NoDocking:**

Search `rc_panel_axiom_editor.cpp` for:
```cpp
ImGuiWindowFlags_NoDocking
```

If found ? Remove it!

### Viewport Still Wrong?
**The issue is coordinate transform:**

In `rc_viewport_overlays.cpp`, `WorldToScreen()` might be broken.

Check line ~113:
```cpp
ImVec2 WorldToScreen(const Core::Vec2& world_pos) const {
    // Should use view_transform_.viewport_pos + offset
    // NOT just offset!
}
```

### City Still Not Showing?
**Roads might be outside viewport bounds:**

Camera might be at (0,0) but roads at (1000, 1000).

Check camera position:
```cpp
printf("Camera: (%.0f, %.0f)\n",
    s_primary_viewport->get_camera_xy().x,
    s_primary_viewport->get_camera_xy().y);
```

---

## Next Steps:

1. Add ALL debug printfs above
2. Run program
3. Paste console output here
4. We'll fix based on actual data

---

**Save this file as `DEBUG_CHECKLIST.md`**
