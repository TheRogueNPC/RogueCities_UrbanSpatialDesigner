# UI Patterns

## Panel Shell
Use token wrapper:

```cpp
const bool open = RC_UI::Components::BeginTokenPanel(
    "Panel Name",
    RC_UI::UITokens::CyanAccent,
    nullptr,
    ImGuiWindowFlags_NoCollapse);
if (!open) {
    RC_UI::Components::EndTokenPanel();
    return;
}

// content...

RC_UI::Components::EndTokenPanel();
```

## Dockable Window
Use dock wrapper for windows that can be closed/recovered:

```cpp
static RC_UI::DockableWindowState s_window;
if (!RC_UI::BeginDockableWindow("Tools", s_window, "Bottom", ImGuiWindowFlags_NoCollapse)) {
    return;
}
// content...
RC_UI::EndDockableWindow();
```

## Semantic Text
Use token-aware helper:

```cpp
RC_UI::Components::TextToken(RC_UI::UITokens::ErrorRed, "Validation failed: %s", message.c_str());
```

## Workspace Presets
- Save: `RC_UI::SaveWorkspacePreset("my-layout", &error)`
- Load: `RC_UI::LoadWorkspacePreset("my-layout", &error)`
- List: `RC_UI::ListWorkspacePresets()`
