# RogueCities Architecture Diagram

This diagram summarizes the main components, their relationships, and runtime data flow.

## Component Relationships

```mermaid
flowchart LR
    subgraph Build["Build Orchestration"]
        ROOT["Root CMake\nCMakeLists.txt"]
    end

    subgraph CoreLayer["Core Layer"]
        CORE["RogueCityCore\nTypes, Math, Editor State, Validation"]
    end

    subgraph GenLayer["Generation Layer"]
        GEN["RogueCityGenerators\nTensors, Roads, Districts, Urban Pipeline"]
    end

    subgraph AppLayer["Application Layer"]
        APP["RogueCityApp\nUI Systems, Tools, Docking, Viewports, Integration"]
    end

    subgraph AILayer["AI Layer"]
        AI["RogueCityAI\nRuntime, Protocol, Clients, HTTP Tools"]
    end

    subgraph VizLayer["Presentation Layer"]
        HEADLESS["RogueCityVisualizerHeadless"]
        GUI["RogueCityVisualizerGui"]
    end

    subgraph External["External Dependencies"]
        IMGUI["RogueCityImGui"]
        GLFW["GLFW/OpenGL/gl3w"]
        VENDOR["3rdparty libs\n(glm, magic_enum, nlohmann_json, etc.)"]
    end

    ROOT --> CORE
    ROOT --> GEN
    ROOT --> APP
    ROOT --> AI
    ROOT --> HEADLESS
    ROOT --> GUI

    CORE --> GEN
    CORE --> APP
    CORE --> AI

    GEN --> APP

    CORE --> HEADLESS
    GEN --> HEADLESS
    APP --> HEADLESS
    AI --> HEADLESS

    CORE --> GUI
    GEN --> GUI
    APP --> GUI
    AI --> GUI

    IMGUI --> APP
    IMGUI --> AI
    IMGUI --> HEADLESS
    IMGUI --> GUI

    GLFW --> GUI
    VENDOR --> CORE
    VENDOR --> GEN
    VENDOR --> APP
    VENDOR --> AI
```

## Runtime Data Flow

```mermaid
flowchart TD
    U["User Input\nMouse/Keyboard/UI Actions"] --> GUIUI["Visualizer UI Panels\nvisualizer/src/ui/panels/*"]

    GUIUI --> APPTOOLS["App Tools + Viewports\nAxiom tools, docking, sync"]
    APPTOOLS --> GENPIPE["Generators Pipeline\nTensor -> Roads -> Districts -> Lots/Sites"]
    GENPIPE --> CORESTATE["Core State/Data\nCityTypes, CitySpec, EditorState"]

    CORESTATE --> APPTOOLS
    APPTOOLS --> GUIUI

    GUIUI --> AIPANELS["AI Panels\nAI Console / UI Agent / City Spec"]
    AIPANELS --> AIRUNTIME["AI Runtime + Clients\nAiBridgeRuntime, UiAgentClient, CitySpecClient"]
    AIRUNTIME --> TOOLSERVER["Local Toolserver\ntools/toolserver.py"]
    TOOLSERVER --> MODEL["Local/Configured AI Model Endpoint"]

    MODEL --> TOOLSERVER
    TOOLSERVER --> AIRUNTIME
    AIRUNTIME --> AIPANELS
    AIPANELS --> GUIUI
```

## Notes

- Primary libraries: `RogueCityCore`, `RogueCityGenerators`, `RogueCityApp`, `RogueCityAI`.
- Primary executables: `RogueCityVisualizerHeadless`, `RogueCityVisualizerGui`.
- Root source-of-truth wiring: `CMakeLists.txt`, `core/CMakeLists.txt`, `generators/CMakeLists.txt`, `app/CMakeLists.txt`, `AI/CMakeLists.txt`, `visualizer/CMakeLists.txt`.
