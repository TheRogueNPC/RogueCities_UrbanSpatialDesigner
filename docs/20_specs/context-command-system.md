# Viewport Context Command System

Purpose: unify viewport command invocation into one registry and one dispatch path with multiple UX surfaces.

## Command Surfaces

1. Smart List: hierarchical popup grouped by library.
2. Pie: radial-style quick command popup (8-slot ring layout).
3. Global Palette: searchable modal command list.

All surfaces execute the same command IDs via `RC_UI::Commands::ExecuteCommand(...)`.

## Registry and Dispatch

1. Registry source: `RC_UI::Tools::GetToolActionCatalog()` via `rc_context_command_registry.cpp`.
2. Tool execution path: `Commands::ExecuteCommand` -> `Tools::DispatchToolAction`.
3. Global command execution path: `Commands::ExecuteGlobalViewportCommand`.
4. No menu surface is allowed to call `DispatchToolAction` directly.

## Built-in Global Viewport Commands

1. `Toggle Minimap`
2. `Force Generate`
3. `Reset Dock Layout`

## Preferences and Defaults

Stored in `GlobalState::config` (`EditorConfig`):

1. `viewport_context_default_mode`
2. `viewport_hotkey_space_enabled`
3. `viewport_hotkey_slash_enabled`
4. `viewport_hotkey_grave_enabled`
5. `viewport_hotkey_p_enabled`

## Hotkeys

1. `Space`: Smart List
2. `/`: Pie
3. `` `~ ``: Pie
4. `P`: Global Palette (viewport focus only, no text input active)
5. `Ctrl+P`: remains Master Panel search (unchanged)

## Input Gate Rules

Commands may open only when:

1. `UiInputGate` allows viewport key/mouse actions.
2. Viewport is hovered/focused.
3. Minimap interaction region is not active.
4. `io.WantTextInput == false`.

This trigger path is centralized through viewport interaction utilities (`rc_viewport_interaction.cpp`) instead of panel-local key/mouse blocks.

## Enforcement

1. `python3 tools/check_context_command_contract.py`
2. Included transitively by `python3 tools/check_ui_compliance.py`
