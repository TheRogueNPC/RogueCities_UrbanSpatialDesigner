# implot Rust Crate Documentation

implot 0.6.0 provides Rust bindings to the C++ ImPlot library for plotting with Dear ImGui. [docs](https://docs.rs/implot/latest/implot/)

## Overview
This crate offers idiomatic bindings to ImPlot via implot-sys, with self-contained documentation and examples in the implot-examples crate. Check the GitHub readme for implemented features; low-level bindings are available for others. [docs](https://docs.rs/implot/latest/implot/)

## Structs
- **AxisFlags**: Flags for axis customization (e.g., LOCK_MIN, LOCK_MAX). [docs](https://docs.rs/implot/latest/implot/)
- **Context**: Manages an ImPlot context. [docs](https://docs.rs/implot/latest/implot/)
- **ImPlotLimits**, **ImPlotPoint**, **ImPlotRange**: Plot-related data structures. [docs](https://docs.rs/implot/latest/implot/)
- **ImVec2**, **ImVec4**: Vector types for positions and colors. [docs](https://docs.rs/implot/latest/implot/)
- **Plot**: Core struct for containing plots. [docs](https://docs.rs/implot/latest/implot/)
- **PlotBars**, **PlotHeatmap**, **PlotLine**, **PlotScatter**, **PlotStairs**, **PlotStems**, **PlotText**: Specialized plot types. [docs](https://docs.rs/implot/latest/implot/)
- **PlotFlags**: Customizes plot behavior (e.g., NO_LEGEND). [docs](https://docs.rs/implot/latest/implot/)
- **PlotToken**, **PlotUi**: Utilities for plot management. [docs](https://docs.rs/implot/latest/implot/)
- **StyleColorToken**, **StyleVarToken**: Style stack trackers. [docs](https://docs.rs/implot/latest/implot/)

## Enums
- **Colormap**, **Condition**, **Marker**: Preset choices and styles. [docs](https://docs.rs/implot/latest/implot/)
- **PlotColorElement**, **PlotLocation**, **PlotOrientation**: Plot customization enums. [docs](https://docs.rs/implot/latest/implot/)
- **StyleVar**, **YAxisChoice**: Style variables and Y-axis selectors. [docs](https://docs.rs/implot/latest/implot/)

## Key Functions
- `get_plot_limits()`, `get_plot_mouse_position()`, `get_plot_query()`: Retrieve plot data. [docs](https://docs.rs/implot/latest/implot/)
- `is_plot_hovered()`, `is_legend_entry_hovered()`: Interaction checks. [docs](https://docs.rs/implot/latest/implot/)
- `pixels_to_plot_f32()`, `plot_to_pixels_f32()`: Coordinate conversions. [docs](https://docs.rs/implot/latest/implot/)
- `push_style_color()`, `push_style_var_*()`: Style modifications with tokens. [docs](https://docs.rs/implot/latest/implot/)
- `set_colormap_from_preset()`, `set_colormap_from_vec()`: Colormap setup. [docs](https://docs.rs/implot/latest/implot/)
- `show_demo_window()`: Displays ImPlot demo. [docs](https://docs.rs/implot/latest/implot/)

# implot Rust Crate Documentation (Page 2)

This page details the implot 0.6.0 crate, providing idiomatic Rust bindings to the C++ ImPlot library via implot-sys. [docs](https://docs.rs/implot/latest/implot/#rust-bindings-to-implot)

## Crate Overview
The crate includes self-contained documentation and examples via the implot-examples crate; refer to the GitHub readme for implemented features and low-level bindings for others. For full usage, see the demo code in implot's repository. [docs](https://docs.rs/implot/latest/implot/#rust-bindings-to-implot)

## Crate Structure
Access all items, structs, enums, and functions listed under sections like Rust bindings to ImPlot. [docs](https://docs.rs/implot/latest/implot/#rust-bindings-to-implot)

## Structs
Key structs include AxisFlags for axis customization, Context for ImPlot management, Plot for containing plots, and specialized ones like PlotBars, PlotHeatmap, PlotLine, PlotScatter, PlotStairs, PlotStems, and PlotText. Additional types: ImPlotLimits, ImPlotPoint, ImPlotRange, ImVec2, ImVec4, PlotFlags, PlotToken, PlotUi, StyleColorToken, StyleVarToken. [docs](https://docs.rs/implot/latest/implot/#rust-bindings-to-implot)

## Enums
Enums cover Colormap presets, Condition for settings, Marker styles, PlotColorElement, PlotLocation, PlotOrientation, StyleVar, and YAxisChoice for Y-axis selection. [docs](https://docs.rs/implot/latest/implot/#rust-bindings-to-implot)

## Functions
Utility functions like get_plot_limits(), get_plot_mouse_position(), pixels_to_plot_f32(), push_style_color(), set_colormap_from_preset(), and show_demo_window() for plot data, interaction, conversions, styling, and demos. Others include is_plot_hovered(), plot_to_pixels_vec2(), set_plot_y_axis(), etc. [docs](https://docs.rs/implot/latest/implot/#rust-bindings-to-implot)

Copy and paste into a `.md` file; this captures the full page content structure and key details. [docs](https://docs.rs/implot/latest/implot/#rust-bindings-to-implot)


# Structs
AxisFlags
Context
ImPlotLimits
ImPlotPoint
ImPlotRange
ImVec2
ImVec4
Plot
PlotBars
PlotFlags
PlotHeatmap
PlotLine
PlotScatter
PlotStairs
PlotStems
PlotText
PlotToken
PlotUi
StyleColorToken
StyleVarToken
# Enums
Colormap
Condition
Marker
PlotColorElement
PlotLocation
PlotOrientation
StyleVar
YAxisChoice
# Functions
get_plot_limits
get_plot_mouse_position
get_plot_query
is_legend_entry_hovered
is_plot_hovered
is_plot_queried
is_plot_x_axis_hovered
is_plot_y_axis_hovered
pixels_to_plot_f32
pixels_to_plot_vec2
plot_to_pixels_f32
plot_to_pixels_vec2
push_style_color
push_style_var_f32
push_style_var_i32
push_style_var_imvec2
set_colormap_from_preset
set_colormap_from_vec
set_plot_y_axis
show_demo_window


Dear ImPlot3D - Rust bindings (high level)

Safe wrapper over dear-implot3d-sys, designed to integrate with dear-imgui-rs. Mirrors dear-implot design: context + Ui facade, builder-style helpers, optional mint inputs.

# Quick Start
```
use dear_imgui_rs::*;
use dear_implot3d::*;

let mut imgui_ctx = Context::create();
let plot3d_ctx = Plot3DContext::create(&imgui_ctx);

// In your main loop:
let ui = imgui_ctx.frame();
let plot_ui = plot3d_ctx.get_plot_ui(&ui);

if let Some(_token) = plot_ui.begin_plot("3D Plot").build() {
    let xs = [0.0, 1.0, 2.0];
    let ys = [0.0, 1.0, 0.0];
    let zs = [0.0, 0.5, 1.0];
    plot_ui.plot_line_f32("Line", &xs, &ys, &zs, Line3DFlags::NONE);
}
```
# Features
mint: Enable support for mint math types (Point3, Vector3)
# Architecture
This crate follows the same design patterns as dear-implot:

Plot3DContext: Manages the ImPlot3D context (create once)
Plot3DUi: Per-frame access to plotting functions
RAII tokens: Plot3DToken automatically calls EndPlot on drop
Builder pattern: Fluent API for configuring plots
Type-safe flags: Using bitflags! for compile-time safety
# Modules
meshes - Predefined mesh data for common 3D shapes
plots - Modular plot types for ImPlot3D
# Structs
Axis3DFlags - Axis flags (per-axis)
Context - An imgui context.
Image3DByAxesBuilder -Image by axes builder
Image3DByCornersBuilder- Image by corners builder
Image3DFlags - Image flags
Item3DFlags - Item flags (common to plot items)
Line3DFlags - Line flags
Mesh3DBuilder - Mesh plot builder
Mesh3DFlags - Mesh flags
Plot3DBuilder - Plot builder for configuring the 3D plot
Plot3DContext - Plot3D context wrapper
Plot3DFlags = Flags for ImPlot3D plot configuration
Plot3DToken - RAII token that ends the plot on drop
Plot3DUi - Per-frame access helper mirroring dear-implot
Quad3DFlags - Quad flags
Scatter3DFlags - Scatter flags
Surface3DBuilder - Surface (grid) plot builder (f32 variant)
Surface3DFlags - Surface flags
Triangle3DFlags - Triangle flags
Ui - Represents the Dear ImGui user interface for one frame
# Enums
Axis3D - 3D axis selector (X/Y/Z)
Marker3D
Plot3DCond - Condition for setup calls (match ImGuiCond)
# Traits
ImPlot3DExt
Ui extension for obtaining a Plot3DUi
Functions
colormap_count
colormap_index_by_name - Look up a colormap index by its name; returns -1 if not found
colormap_name
colormap_size - Get number of keys (colors) in a given colormap index
get_colormap_color - Get a color from the current colormap at index
get_style_colormap_index - Get current default colormap index set in ImPlot3D style
get_style_colormap_name - Get current default colormap name (if index valid)
next_colormap_color - Get next colormap color (advances internal counter)
pop_colormap
pop_style_color
pop_style_var
Pop style variable(s)
push_colormap_index
push_colormap_name
push_style_color
push_style_var_f32 - Push a style variable (float variant)
push_style_var_i32 - Push a style variable (int variant)
push_style_var_vec2 - Push a style variable (Vec2 variant)
set_next_fill_style
set_next_line_style
set_next_marker_style
set_style_colormap_by_name - Convenience: set default colormap by name (no-op if name is invalid)
set_style_colormap_index - Permanently set the default colormap used by ImPlot3D (persists across plots/frames)
show_all_demos  - Show upstream ImPlot3D demos (from C++ demo)
show_demo_window - Show the main ImPlot3D demo window (C++ upstream)
show_demo_window_with_flag  - Show the main ImPlot3D demo window with a visibility flag
show_metrics_window - Show the ImPlot3D metrics/debugger window
show_metrics_window_with_flag - Show the ImPlot3D metrics/debugger window with a visibility flag
show_style_editor - Show the ImPlot3D style editor window
style_colors_auto
style_colors_classic
style_colors_dark
style_colors_light