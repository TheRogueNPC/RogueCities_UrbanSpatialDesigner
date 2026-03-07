# RogueCities ImGui Coding Standard for AI Agents

**Date:** 2026-03-06 (updated with full FAQ rules)
**Author:** Claude (claude-sonnet-4-6)
**Source:** `docs/30_architecture/Imgui_QA.md` (Dear ImGui official FAQ)
**Status:** ACTIVE MANDATE — all agents must comply

---

## Why This Document Exists

AI models default to retained-mode patterns (Qt/WPF/C#). In this codebase every UI
function is immediate-mode: a stateless visualizer called every frame. Violating this
creates silent bugs that are extremely hard to trace.

This document consolidates both the **architectural rules** (how to structure UI code)
and the **technical rules** (how Dear ImGui's API actually works) extracted from the
official FAQ.

---

## Part 1: The Three Zero-OOP Architecture Rules

### Rule 1: Pass-by-Reference State

Never create C++ classes or structs that represent UI elements. UI functions must be pure
visualizers that take application state by reference or pointer.

```cpp
// BAD — retained-mode: state duplication, own lifecycle
struct RoadGeneratorUI {
    int currentRoadLength = 10;
    void Draw();
};

// GOOD — immediate-mode: stateless visualizer
void DrawRoadGeneratorPanel(UrbanSpatialState& state) {
    ImGui::SliderInt("Road Length", &state.roadLength, 1, 100);
}
```

The application data struct is the single source of truth. The panel function is a lens —
nothing more.

### Rule 2: Immediate Conditional

No callbacks, no listeners, no event bindings. All UI interactions are handled
synchronously with `if()` on ImGui return values.

```cpp
// BAD — retained-mode event binding
m_GenerateButton->OnClicked = [this]() { GenerateRoads(state); };

// GOOD — immediate-mode
if (ImGui::Button("Generate Road Network")) {
    GenerateRoads(state.gridDimensions);
}
```

The `if`-statement IS the event system. There is no other event system.

### Rule 3: Static Scoping for Transients

If UI display needs localized transient state that does not belong in the engine data
model (a debug toggle, a text buffer), use function-local `static` variables.

```cpp
void DrawDebugOverlay() {
    static bool showHexGrid = false;
    static char searchBuf[256] = {};

    ImGui::Checkbox("Show Hex Grid Boundaries", &showHexGrid);
    ImGui::InputText("Filter", searchBuf, sizeof(searchBuf));

    if (showHexGrid) {
        RenderHexDebugLines();
    }
}
```

`static` locals persist across frames without polluting any struct. Never use a member
variable or a global to store UI-only transient state.

### RogueCities Panel Contract (compliant form)

```cpp
// rc_panel_foo.h
namespace RC_UI::Panels::Foo {
    bool IsOpen();
    void Toggle();
    void DrawContent(float dt);
    void Draw(float dt);
}

// rc_panel_foo.cpp
namespace RC_UI::Panels::Foo {
    static bool s_open = false;

    bool IsOpen() { return s_open; }
    void Toggle() { s_open = !s_open; }

    void DrawContent(float dt) {
        // All widgets here. Use static for UI-only transients.
        // Access engine data via globals/singletons or passed reference.
    }

    void Draw(float dt) {
        if (!s_open) return;
        auto& uiint = RC_UI::UiIntrospector::Get();
        if (uiint.BeginPanel("Foo", &s_open)) {
            DrawContent(dt);
        }
        uiint.EndPanel();
    }
}
```

`s_open` is the only class-level static a panel may own. Everything else is passed in or
is function-local static.

### Five Failure Modes — Reject Immediately

If you are about to write any of these, stop and restructure:

1. A struct/class with `Draw()` that holds widget-related member variables
2. A `std::function` or lambda stored as a UI "action" to be called later
3. A `UIManager` or `PanelManager` that tracks widget state centrally
4. An `OnChanged` / `OnClicked` / `SetEnabled()` pattern
5. Synchronization code that copies engine data into a UI struct

---

## Part 1.5: Canonical RC Wrapper API

For panel-level UI, prefer the canonical wrapper layer:

- `visualizer/src/ui/api/rc_imgui_api.h`
- Namespace: `RC_UI::API`

Rules:

1. Use `RC_UI::API` wrappers for standard panel controls (`Button`, `DragFloat`, `SectionHeader`, etc.).
2. Use `RC_UI::API::Mutate` only when raw ImGui access is truly needed (custom draw lists, child-region escapes, explicit layout claims).
3. Do not open top-level windows in `DrawContent()` via any path (`ImGui::Begin`, `Components::BeginTokenPanel`, `RC_UI::BeginDockableWindow`, or `API::BeginPanel`).
4. Raw panel widget calls like `ImGui::Button`/`ImGui::DragFloat` are contract violations across panel sources; use `RC_UI::API` wrappers.

---

## Part 2: ID Stack — The Most Common Bug

**Source:** FAQ "About the ID Stack system"

**THE MOST COMMON MISTAKE: using the same label in the same location.**

Dear ImGui identifies interactive widgets by hashing the path of labels leading to them.
Two widgets with the same label in the same scope get the SAME ID → only the first one
ever reacts to clicks.

### The Rules

**Rule: Every interactive widget in a loop MUST have `PushID` / `PopID` around it.**

```cpp
// BAD — all buttons get the same ID, only first works
for (int i = 0; i < count; ++i) {
    ImGui::Button("Delete");   // ID collision!
}

// GOOD — PushID scopes the label
for (int i = 0; i < count; ++i) {
    ImGui::PushID(i);
    ImGui::Button("Delete");   // unique per iteration
    ImGui::PopID();
}
```

**Rule: Use `##` to add invisible ID disambiguation to a visible label.**

```cpp
ImGui::Button("Play");         // ID = hash("MyWindow", "Play")
ImGui::Button("Play##road");   // ID = hash("MyWindow", "Play##road") — different!
ImGui::Button("Play##zone");   // ID = hash("MyWindow", "Play##zone") — different!
ImGui::Checkbox("##On", &b);   // No visible label, unique ID from "##On"
```

**Rule: Use `###` when the visible label changes but the ID must stay stable.**
(e.g., animated window titles, FPS counter in title bar)

```cpp
// Window ID is stable "MyGame" even though title changes every frame
sprintf(buf, "My City (%d fps)###MainGame", fps);
ImGui::Begin(buf);
```

**Rule: PushID can take a pointer, string, or int — all produce unique hashes.**

```cpp
for (auto& road : gs.roads) {
    ImGui::PushID(&road);          // pointer as ID (stable if container doesn't move)
    // or
    ImGui::PushID(road.id);        // integer ID (stable)
    // or
    ImGui::PushID(road.name.c_str()); // string (stable if unique)
    ImGui::Button("Edit");
    ImGui::PopID();
}
```

**Rule: Inside `BeginCombo`/`EndCombo`, enum names are unique so no `PushID` is needed.**
The combo popup creates its own ID scope. `Selectable` items inside are scoped to it.

### How to Debug ID Collisions

Use `ImGui::ShowIDStackToolWindow()` or `Demo > Tools > ID Stack Tool` at runtime.
It shows the hash path to each widget, making collisions obvious immediately.

### RogueCities Pattern: No Redundant Label Suffix When PushID Is Active

```cpp
// BAD — allocates std::string per row per frame, suffix is redundant
ImGui::PushID(static_cast<int>(idx));
const std::string label = "##row_" + std::to_string(idx);  // WRONG
ImGui::Selectable(label.c_str(), selected, ...);

// GOOD — PushID(idx) already scopes "##row" uniquely
ImGui::PushID(static_cast<int>(idx));
ImGui::Selectable("##row", selected, ...);  // unique because of PushID
```

---

## Part 3: Input Dispatch

**Source:** FAQ "How can I tell whether to dispatch mouse/keyboard to Dear ImGui or my application?"

**Rule: ALWAYS feed input to ImGui first, THEN gate on `WantCaptureMouse`.**

Never check "is the mouse over a window" manually — use `io.WantCaptureMouse`.

```cpp
void MyMouseButtonHandler(int button, bool down) {
    // Step 1: ALWAYS forward to ImGui first
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseButtonEvent(button, down);

    // Step 2: Only send to game/engine if ImGui doesn't want it
    if (!io.WantCaptureMouse)
        my_engine->HandleMouseInput(button, down);
}
```

**Exception (RogueCities-specific):** The viewport canvas is itself an ImGui widget,
so `WantCaptureMouse` is always true inside it. The `rc_ui_input_gate.cpp` handles this
correctly by using canvas hover state as the local gate instead. Do NOT change this logic.

**Rule: Enable keyboard/gamepad navigation via config flags, not manual polling.**

```cpp
io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // keyboard nav
io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // controller nav
```

---

## Part 4: DPI

**Source:** FAQ "How should I handle DPI in my application?" (Dear ImGui 1.92+)

The project uses GLFW — DPI awareness is handled automatically by GLFW.
`io.ConfigDpiScaleFonts` and `io.ConfigDpiScaleViewports` are already set in `main_gui.cpp`
from `gs.config`.

**Rule: Never hardcode pixel sizes. Express sizes as multiples of reference values.**

```cpp
// BAD — breaks on high-DPI or different font sizes
ImVec2(500, 300)
ImGui::SetNextWindowSize(ImVec2(600, 400));

// GOOD — scales with font and DPI
float fh = ImGui::GetFrameHeight();       // font + padding
float fs = ImGui::GetFontSize();          // raw font height
ImGui::SetNextWindowSize(ImVec2(40 * fs, 20 * fh));
```

**Rule: To change font size per-scope, use `PushFont(NULL, size)` (1.92+).**

```cpp
ImGui::PushFont(NULL, 18.0f);     // use current font at 18px (scaled by FontScaleDpi)
ImGui::Text("Section Header");
ImGui::PopFont();
```

**Rule: `style.ScaleAllSizes(factor)` must only be called ONCE after style setup.**
To change the scale factor later, reset the style and call it again.

---

## Part 5: ImDrawList Custom Rendering

**Source:** FAQ "How can I display custom shapes?"

**Rule: Always advance the ImGui cursor after custom draw calls with `Dummy()`.**

```cpp
void DrawViewportOverlay() {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();

    dl->AddCircleFilled(ImVec2(p.x + 50, p.y + 50), 30.0f,
                        IM_COL32(0, 200, 255, 200));
    dl->AddLine(p, ImVec2(p.x + 100, p.y + 100), UITokens::CyanAccent, 2.0f);

    // REQUIRED: advance cursor so the window doesn't collapse
    ImGui::Dummy(ImVec2(200, 200));
}
```

**Useful draw list accessors:**

```cpp
ImGui::GetWindowDrawList()      // draws inside current window (clips to window)
ImGui::GetBackgroundDrawList()  // draws behind all windows (no clipping)
ImGui::GetForegroundDrawList()  // draws in front of all windows (no clipping)
```

**Rule: Use `ImGui::GetColorU32(ImVec4(...))` when color must respect `style.Alpha`.**
Use `IM_COL32(r,g,b,a)` for compile-time constants that ignore global alpha.

**Rule: ClipRect in custom backends is `(x1, y1, x2, y2)` — NOT `(x, y, width, height)`.**
This is a backend-level concern; panel authors don't need to worry about it. Backend
authors: see `ImDrawCmd->ClipRect` and the DX11 backend reference.

---

## Part 6: std::string Performance in Hot UI Paths

**Source:** FAQ "How can I interact with standard C++ types?"

Dear ImGui takes `const char*`. Passing `std::string::c_str()` is fine. But:

**Rule: Never construct `std::string` in a per-frame, per-row loop just to pass a label.**

```cpp
// BAD — heap allocation every frame for every row
for (auto& road : roads) {
    ImGui::PushID(road.id);
    const std::string label = "Road #" + std::to_string(road.id);  // heap alloc!
    ImGui::Selectable(label.c_str(), selected);
    ImGui::PopID();
}

// GOOD — stack buffer, no allocation
char buf[64];
for (auto& road : roads) {
    ImGui::PushID(road.id);
    snprintf(buf, sizeof(buf), "Road #%u", road.id);
    ImGui::Selectable(buf, selected);
    ImGui::PopID();
}

// ALSO GOOD — use the label only for display, ID from PushID
for (auto& road : roads) {
    ImGui::PushID(road.id);
    ImGui::Selectable("##road", selected);   // display elsewhere, ID from PushID
    ImGui::SameLine();
    ImGui::Text("Road #%u", road.id);        // display text separately (no ID)
    ImGui::PopID();
}
```

**Rule: Use `misc/cpp/imgui_stdlib.h` for `InputText` with `std::string`.**

```cpp
#include <misc/cpp/imgui_stdlib.h>
std::string road_name = road.name;
if (ImGui::InputText("Name", &road_name)) {
    road.name = road_name;
}
```

---

## Part 7: ImTextureRef (Dear ImGui 1.92+)

**Source:** FAQ "What are ImTextureID/ImTextureRef?"

`ImTextureRef` was introduced in 1.92 (June 2025). All `Image()` / `AddImage()` functions
now take `ImTextureRef` instead of `ImTextureID`.

**Rule: For user-created textures, continue to manage `ImTextureID` and cast to `ImTextureRef`.**

```cpp
// GL texture
GLuint my_tex = LoadTexture("icon.png");
ImGui::Image((ImTextureID)(intptr_t)my_tex, ImVec2(64, 64));
// ImTextureRef is constructed implicitly from ImTextureID cast
```

**Rule: Never implicitly cast `ImTextureRef → ImTextureID` — the cast is intentionally absent.**

---

## Quick Reference: ID Stack Cheat Sheet

| Situation | Solution |
|---|---|
| Two buttons with same label in same window | Add `##suffix`: `"Delete##road"`, `"Delete##zone"` |
| Loop of widgets | `PushID(i)` / `PopID()` around each iteration |
| Label that changes but ID must be stable | Use `###id`: `"My City (60fps)###MainWindow"` |
| Widget with no visible label | Use `##` prefix only: `"##toggle"` |
| Loop with PushID already called | Plain label `"##row"` is unique, no suffix needed |
| Debug ID collisions at runtime | `ImGui::ShowIDStackToolWindow()` |

---

## References

- Full FAQ: `docs/30_architecture/Imgui_QA.md`
- Structured reference for agents: `docs/60_operations/imgui-faq-reference.md`
- Panel contract details: `AI/collaboration/claude_2026-03-01_ui_panel_pattern_contract.md`
- Token source of truth: `visualizer/src/ui/rc_ui_tokens.h`
- Input gate: `visualizer/src/ui/rc_ui_input_gate.cpp`

---

## CHANGELOG

- 2026-03-06: Initial document (Zero-OOP rules from user TLDR + manifesto)
- 2026-03-06: Major expansion — added ID Stack, Input Dispatch, DPI, ImDrawList,
  std::string perf, ImTextureRef rules from full FAQ audit.
  Fixed `rc_ui_data_index_panel.h` redundant string allocation.
