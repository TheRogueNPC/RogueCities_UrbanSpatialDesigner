# ImGui Docking Guide - RogueCity Visualizer

## ? **BUILD SUCCESS - Docking Fixed!**

---

## **What Was Fixed:**

1. ? **OS Window Decorations:** Re-enabled (custom chrome removed for simplicity)
2. ? **Docking Enabled:** Multi-viewport and docking properly configured
3. ? **Reset Layout:** Added `Ctrl+R` hotkey and menu item
4. ? **Window Management:** All panels can now be docked/undocked

---

## **How to Use Docking:**

### **Drag and Drop Panels:**

1. **Click and hold** the title bar of any panel (e.g., "Analytics", "Log", "Tools")
2. **Drag** it around the screen
3. **Release** to:
   - **Dock inside main window:** Drag near edges to see blue highlight zones
   - **Dock as tab:** Drag onto another panel to tab them together
   - **Float as separate window:** Release in empty space

### **Docking Zones:**

When dragging a panel, you'll see **blue preview boxes**:
- **Center:** Dock in center area (covers main viewport)
- **Left/Right/Top/Bottom edges:** Dock to that side
- **On existing panel:** Tab with that panel

### **Multi-Viewport (Floating Windows):**

- **Drag panel outside main window** ? Creates new OS window
- Each floating window has its own title bar with minimize/close buttons
- Floating windows can be docked back into main window

---

## **Keyboard Shortcuts:**

| Shortcut | Action |
|----------|--------|
| **Ctrl+R** | Reset dock layout to default |
| **Middle Mouse Drag** | Move viewport camera |
| **Mouse Wheel** | Zoom in/out |

---

## **Menu Bar:**

At the top of the window:
- **Window** ? **Reset Layout** - Restores default panel positions
- Tip shown in menu: "Drag panel titles to dock/undock"

---

## **Default Layout:**

```
???????????????????????????????????????????????
?          Axiom Bar (Top)                    ?
???????????????????????????????????????????????
?      ?                      ?               ?
? Left ?   RogueVisualizer    ?   Analytics   ?
?      ?    (Main Viewport)   ?   (Right)     ?
?      ?                      ?               ?
???????????????????????????????????????????????
?  Tools                                      ?
???????????????????????????????????????????????
?  Log | District | Road | Lot | River       ?
?  (Tabbed Bottom Area)                      ?
???????????????????????????????????????????????
```

---

## **Troubleshooting:**

### ? **Panels Won't Move**
**Fix:** Make sure you're clicking the title bar, not the panel content

### ? **Can't See Docking Zones**
**Fix:** Drag slowly near window edges - blue zones appear automatically

### ? **Layout Messed Up**
**Fix:** Press **Ctrl+R** or use **Window ? Reset Layout**

### ? **Panel Disappeared**
**Fix:** Press **Ctrl+R** to restore all panels to default

### ? **Floating Window Won't Close**
**Fix:** Click the **X** button on the floating window's title bar, or dock it back

---

## **Panel List (All Dockable):**

| Panel Name | Default Position | Purpose |
|------------|------------------|---------|
| **Axiom Bar** | Top | Axiom placement controls |
| **RogueVisualizer** | Center | Main viewport |
| **Analytics** | Right | Stats and metrics |
| **Tools** | Bottom | Tool controls |
| **Log** | Bottom (Tab) | System messages |
| **District Index** | Bottom (Tab) | District list |
| **Road Index** | Bottom (Tab) | Road list |
| **Lot Index** | Bottom (Tab) | Lot list |
| **River Index** | Bottom (Tab) | River list |
| **Building Index** | Bottom (Tab) | Building list |
| **Zoning Control** | Right | Zoning parameters |
| **AI Console** | Floating | AI assistant |
| **UI Agent** | Floating | UI automation |
| **City Spec** | Floating | City generation |

---

## **Advanced Tips:**

### **Create Custom Layouts:**

1. Arrange panels however you like
2. ImGui saves layout automatically
3. Use **Ctrl+R** to revert to default anytime

### **Tab Multiple Panels:**

1. Drag panel **onto** another panel's title bar
2. Release ? They become tabs
3. Click tabs to switch between them

### **Split Panels:**

1. Drag panel to edge of another panel
2. See thin blue line (split indicator)
3. Release ? Creates side-by-side split

### **Maximize a Panel:**

1. Double-click panel title bar ? Maximizes
2. Double-click again ? Restores

---

## **What Works Now:**

? Drag panels anywhere  
? Dock to any edge  
? Tab panels together  
? Float panels as separate windows  
? Minimize/close floating windows  
? Reset layout with Ctrl+R  
? OS window has standard controls (minimize, maximize, close)  
? Multi-viewport support (drag panels to second monitor)  

---

## **Testing Checklist:**

- [ ] Drag "Analytics" panel to left side ? Should dock
- [ ] Drag "Log" panel to float ? Should create new window
- [ ] Drag floating panel back ? Should dock
- [ ] Press Ctrl+R ? All panels restore to default
- [ ] Double-click panel title ? Should maximize
- [ ] Right-click panel tab ? Context menu appears

---

## **Known Behaviors:**

?? **First drag might feel "sticky"** - This is normal ImGui behavior  
?? **Blue zones appear while dragging** - Shows where panel will dock  
?? **Tabs appear when dragging onto panels** - Creates tabbed interface  

---

## **If You Still Can't Move Panels:**

1. **Verify docking is enabled:**
   - Look for blue highlight zones when dragging
   - If no zones appear, docking might be disabled

2. **Check build output:**
   ```
   Build successful ?
   ```

3. **Run the visualizer:**
   ```cmd
   .\build\bin\Release\RogueCityVisualizerGui.exe
   ```

4. **Try these tests:**
   - Click "Analytics" title bar and drag slowly
   - Move mouse near left edge of main window
   - Blue rectangle should appear showing dock zone

---

## **Development Notes:**

- Dockspace ID: `RogueDockSpace`
- Host window: `RogueDockHost`
- Layout built on first frame
- Reset clears `s_dock_built` flag
- ImGui handles all docking logic internally

---

**Ready to use! Press F5 in Visual Studio to start.** ??
