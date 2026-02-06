# AI TASK SCRIPT: Implement Editor HFSM + GlobalState + FVA/SIV/CIV integration (RogueCities)

Repository (local workspace):

- `d:/Projects/RogueCities/RogueCities_UrbanSatialDesigner`

Primary constraints (must follow):

- **Layering:** Core must not include ImGui/OpenGL/GLFW headers.
- **Containers (Rogue Protocol):**
  - **FVA** (`fva::Container<T>`): editor-facing entities that need stable IDs/handles (roads, districts, lots, UI selection).
  - **SIV** (`siv::Vector<T>`): high-churn entities where validity checks matter (buildings, agents, props).
  - **CIV** (`civ::IndexVector<T>`): internal scratch buffers / performance-critical working sets; **do not** expose to editor UI.

You must inspect the repo and use the existing container implementations and core types:

- FVA: `core/include/RogueCity/Core/Util/fast_array.hpp` (included via `FastVectorArray.hpp`)
- SIV: `core/include/RogueCity/Core/Util/stable_index_vector.hpp` (included via `StableIndexVector.hpp`)
- CIV: `core/include/RogueCity/Core/Util/constant_index_vector.hpp` (included via `IndexVector.hpp`)
- Core entity types:
  - `RogueCity::Core::Road`, `District`, `LotToken`, `BuildingSite` in `core/include/RogueCity/Core/Data/CityTypes.hpp`

The goal is to add an editor HFSM (hierarchical finite state machine) and a GlobalState store that cleanly routes UI actions into the data layer, with explicit integration points for integrity checks that can be filled in later.

---

## 0) Quick repo scan (required)

Before coding:

1. Locate the app/visualizer entry point (ImGui main loop). In this repo it may not exist yet (there is an optional `visualizer/` subdir hook in the root `CMakeLists.txt`).
2. Confirm the Core/Generators folder layout:
   - `core/` is data + utilities
   - `generators/` is algorithmic logic
   - UI/editor code must live in Layer 2 (e.g. `visualizer/` or `app/`) and only *reference* Core/Generators.

If the repo has no ImGui loop yet, create a minimal Visualizer target (Layer 2) **without** dragging UI headers into Core.

---

## 1) Files to create (Core: editor-agnostic data/control)

Create:

- `core/include/RogueCity/Core/Editor/EditorState.hpp`
- `core/src/Core/Editor/EditorState.cpp`
- `core/include/RogueCity/Core/Editor/GlobalState.hpp`
- `core/src/Core/Editor/GlobalState.cpp`
- `core/include/RogueCity/Core/Editor/EditorEvents.hpp` (optional, if you want to split events cleanly)

Update `core/CMakeLists.txt` to compile/link these into `RogueCityCore`.

Notes:

- The editor HFSM must not include any ImGui headers.
- The HFSM can include `magic_enum` (already vendored) for logging/debug strings if useful.

---

## 2) GlobalState (central editor data model)

Implement `RogueCity::Core::Editor::GlobalState` with **explicit container choices**:

- Use existing core types from `CityTypes.hpp`.
- Use FVA/SIV/CIV according to the Rogue Protocol.

Suggested starting point (adjust if you find better/required types in repo):

```cpp
namespace RogueCity::Core::Editor {

  struct EditorParameters {
    uint32_t seed = 1;
    bool snap_to_grid = true;
    float snap_size = 1.0f;
    float viewport_pan_speed = 1.0f;
  };

  struct Selection {
    // Prefer handles for stability; do NOT store raw pointers.
    fva::Handle<RogueCity::Core::Road> selected_road{};
    fva::Handle<RogueCity::Core::District> selected_district{};
    fva::Handle<RogueCity::Core::LotToken> selected_lot{};
    siv::Handle<RogueCity::Core::BuildingSite> selected_building{};
  };

  struct GlobalState {
    // Editor-facing data (stable handles)
    fva::Container<RogueCity::Core::Road>     roads;
    fva::Container<RogueCity::Core::District> districts;
    fva::Container<RogueCity::Core::LotToken> lots;

    // High-churn / validity-checked
    siv::Vector<RogueCity::Core::BuildingSite> buildings;

    // Internal scratch (NOT UI-facing)
    civ::IndexVector<RogueCity::Core::Vec2> scratch_points;

    EditorParameters params{};
    Selection selection{};

    uint64_t frame_counter = 0;
  };

  GlobalState& GetGlobalState();
}
```

Requirements:

- Implement `GetGlobalState()` as a function-local static in `GlobalState.cpp` (no exposed globals).
- Do not store `std::vector<Road>` etc in GlobalState for editor-facing entities unless you have a strong justification.
- Prefer storing editor selection as FVA/SIV handles. Avoid storing indices into `.getData()` because ordering can change.

---

## 3) HFSM design (robust starter state set)

### 3.1 States

Define a robust state set that supports growth without refactors. Use a hierarchical naming scheme even if the initial implementation is a flat enum.

Minimum recommended states:

```cpp
enum class EditorState {
  Startup,
  NoProject,
  ProjectLoading,
  Idle,

  // Editing super-state (tools)
  Editing,
  Editing_Roads,
  Editing_Districts,
  Editing_Lots,
  Editing_Buildings,

  // Viewport interaction (optional but recommended)
  Viewport_Pan,
  Viewport_Select,
  Viewport_PlaceAxiom,    // future: axiom placement tools
  Viewport_DrawRoad,      // interactive road authoring
  Viewport_BoxSelect,

  // Simulation
  Simulating,
  Simulation_Paused,
  Simulation_Stepping,

  // Playback/review
  Playback,
  Playback_Paused,
  Playback_Scrubbing,

  // Modal flows (optional)
  Modal_Exporting,
  Modal_ConfirmQuit,

  Shutdown
};
```

Notes:

- If you choose to implement true HFSM with parent/child relationships, represent it explicitly (e.g., `struct StateNode { parent, on_enter, on_exit }`).
- If you keep a flat enum initially, still group transitions so later parent states can be introduced with minimal churn.

### 3.2 Events

Define events that map to UI actions + system events:

```cpp
enum class EditorEvent {
  BootComplete,
  NewProject,
  OpenProject,
  ProjectLoaded,
  CloseProject,

  GotoIdle,

  Tool_Roads,
  Tool_Districts,
  Tool_Lots,
  Tool_Buildings,

  Viewport_Pan,
  Viewport_Select,
  Viewport_DrawRoad,
  Viewport_BoxSelect,

  BeginSim,
  PauseSim,
  ResumeSim,
  StepSim,
  StopSim,

  BeginPlayback,
  PausePlayback,
  ScrubPlayback,
  StopPlayback,

  Export,
  CancelModal,

  Quit
};
```

### 3.3 EditorHFSM class

Implement:

```cpp
class EditorHFSM {
public:
  EditorHFSM();

  void handle_event(EditorEvent e, GlobalState& gs);
  void update(GlobalState& gs, float dt); // per-frame tick, may be empty initially

  EditorState state() const noexcept;

private:
  EditorState m_state{};
  void transition_to(EditorState next, GlobalState& gs);
  void on_enter(EditorState s, GlobalState& gs);
  void on_exit(EditorState s, GlobalState& gs);
};

EditorHFSM& GetEditorHFSM();
```

Requirements:

- The HFSM instance must be owned in `EditorState.cpp` via `GetEditorHFSM()` (function-local static).
- Illegal transitions should be ignored **and** optionally logged (but do not crash the editor for now).
- Any expensive work should be queued for `RogueWorker` later; do not block the UI thread.

---

## 4) CIV/SIV/FVA integrity hooks on transitions (stubs OK)

There are no existing `CIV_Validate*` / `SIV_SpatialCheck*` functions in the repo yet.

Create a small validation module that compiles today and can be filled in later:

- `core/include/RogueCity/Core/Validation/EditorIntegrity.hpp`
- `core/src/Core/Validation/EditorIntegrity.cpp`

API (suggested):

```cpp
namespace RogueCity::Core::Validation {
  void ValidateRoads(const fva::Container<Road>&);
  void ValidateDistricts(const fva::Container<District>&);
  void ValidateLots(const fva::Container<LotToken>&);
  void ValidateBuildings(const siv::Vector<BuildingSite>&);
  void ValidateAll(const Editor::GlobalState&);

  void SpatialCheckAll(const Editor::GlobalState&);
}
```

HFSM requirements:

- On exiting `Editing_*` states, call the narrow validation for the edited dataset(s).
- On entering `Simulating`, call `ValidateAll(gs)` and `SpatialCheckAll(gs)` before sim tick begins.

Keep implementations minimal initially (e.g., cheap asserts / size checks / invariants), but wire the calls so the structure is in place.

---

## 5) Hook HFSM + GlobalState into ImGui (Layer 2 only)

In the Visualizer/App layer main loop (ImGui frame function):

1. Acquire:

```cpp
auto& hfsm = RogueCity::Core::Editor::GetEditorHFSM();
auto& gs   = RogueCity::Core::Editor::GetGlobalState();
```

2. Per-frame:

```cpp
hfsm.update(gs, dt);
draw_main_menu(hfsm, gs);
draw_editor_windows(hfsm, gs);
draw_viewport(hfsm, gs);
```

3. UI emits events only:

- Menu items and tool buttons call `hfsm.handle_event(...)`.
- UI reads from `gs` and writes to `gs` via small helpers (e.g., “add road”, “delete selected”).
- UI must not perform heavy generation; it should queue generator jobs.

Example `Mode` menu:

```cpp
if (ImGui::MenuItem("Road Tool", nullptr, hfsm.state() == EditorState::Editing_Roads))
  hfsm.handle_event(EditorEvent::Tool_Roads, gs);
```

---

## 6) Minimal editor actions demonstrating FVA/SIV usage (UI-layer helpers)

Provide small API helpers (prefer in Layer 2) that mutate `GlobalState`:

- Add/remove road:
  - Use `auto h = gs.roads.add(road);`
  - Store `gs.selection.selected_road = h;`
- Add/remove building:
  - Use `auto id = gs.buildings.emplace_back(...);`
  - Store `gs.selection.selected_building = gs.buildings.createHandle(id);`

Do not store raw pointers into `gs.roads`/`gs.buildings`.

---

## 7) Build integration

Update `core/CMakeLists.txt` to compile the new Core sources.

Project must build with the existing root CMake (`CMakeLists.txt`) configuration.

If you add a Visualizer target:

- Put it in `visualizer/` and ensure root CMake’s optional `add_subdirectory(visualizer)` picks it up.
- Keep its dependencies: link against `RogueCityCore` and `RogueCityGenerators` only.

---

## 8) Deliverables checklist

- New Core editor files compile.
- `GlobalState` uses FVA/SIV/CIV as specified, with selection stored as handles.
- `EditorHFSM` has robust initial state + event set and legal transitions.
- Transition hooks call validation/spatial check stubs.
- UI integration pattern exists in Layer 2 (or is clearly stubbed if app not present yet).

