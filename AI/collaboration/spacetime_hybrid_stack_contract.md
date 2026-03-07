# Spacetime Hybrid Stack Contract (UTMOST CRITICALITY)

**Status**: MANDATORY  
**Effective Date**: 2026-03-06  
**Scope**: All Agent work involving SpacetimeDB integration and UI development.

## 1. The Prime Directive: The "FFI Tax" Prohibition
The **RogueCities Visualizer** and **Generators** shall remain 100% C++. 

**The Logic**: City-scale geospatial data (Roads, Districts, Buildings) involves thousands of entities. Marshaling this data across the Foreign Function Interface (FFI) to a Rust-based UI (`imgui-rs`) every frame (60-144fps) creates a performance bottleneck that negates the speed of both languages. 

**The Law**: No UI code shall be written in Rust. No Rust bindings for ImGui shall be introduced to the main visualizer.

## 2. Layer Ownership

| Component | Language | Responsibility |
| :--- | :--- | :--- |
| **Visualizer / Viewport** | C++ | Immediate-mode UI, zero-latency interaction, HUD overlays. |
| **Generators / Simulation** | C++ | Mesh generation, tensor field math, high-speed geometry. |
| **Persistence / Relational** | SpacetimeDB | Authoritative state, multi-user sync, historical logging. |
| **Spatial Logic** | Rust (Server) | Complex relational queries (e.g., "Find all buildings in this radius"). |

## 3. The Synchronization Contract
Interaction with SpacetimeDB must follow the **Asynchronous Observer Pattern**.

1.  **Local Authority**: The C++ `GlobalState` is the authoritative source for the current frame's rendering and simulation.
2.  **Asynchronous Bridge**: The `SpacetimeBridge` (C++ SDK) must run on a background thread or via non-blocking polls. 
3.  **No Loop Blocking**: The `EditorHFSM` update loop must **never** wait for a database response. If the DB is slow, the UI continues to render using the last known local state.

## 4. Dos and Don'ts

### ✅ DO
*   Use the **SpacetimeDB C++ SDK** to push local changes to the database.
*   Write complex spatial queries (SQL-like) in **Rust inside the SpacetimeDB module**.
*   Maintain the **Zero-OOP ImGui Standard** in C++ for all panels.
*   Use SpacetimeDB for "Cold Data" (Persistence, Stats, Global Registry).
*   Use C++ memory for "Hot Data" (Vertex buffers, active streamlines, per-frame physics).

### ❌ DON'T
*   **DON'T** attempt to port `rc_panel_axiom_editor.cpp` or other complex panels to Rust.
*   **DON'T** marshal raw vertex arrays to the database every frame. Only sync "Logical Entities" (Road IDs, Node Coordinates).
*   **DON'T** introduce `imgui-rs` or `egui` into the `visualizer/` directory.
*   **DON'T** allow a database connection failure to crash the `RogueCityVisualizerGui`.

## 5. Edge Cases & Exceptions

### Case A: Standalone Rust Utilities
Agents are permitted to write standalone Rust CLI tools for database maintenance, schema migration, or headless data analysis, provided they do not link against the `visualizer` or `generators` layers.

### Case B: The "Heavy Query" Trigger
If a user action requires a query that is too heavy for the C++ spatial grid (e.g., "Calculate the average property value of all districts within 5km of the river"), this query **must** be offloaded to the SpacetimeDB Rust module and returned via a callback to the C++ `GlobalState`.

## 6. Enforcement
Any PR or code suggestion that introduces Rust-to-C++ FFI overhead in the main rendering path shall be rejected by the **RogueCitiesArchitect** agent as a violation of the Performance Mandate.

---
*End of Contract. This document is the source of truth for SpacetimeDB integration.*