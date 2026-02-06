#pragma once

#include <cstdint>

namespace RogueCity::Core::Editor {

    struct GlobalState;

    enum class EditorState : uint16_t {
        Startup,
        NoProject,
        ProjectLoading,

        Idle,

        Editing,
        Editing_Roads,
        Editing_Districts,
        Editing_Lots,
        Editing_Buildings,

        Viewport_Pan,
        Viewport_Select,
        Viewport_PlaceAxiom,
        Viewport_DrawRoad,
        Viewport_BoxSelect,

        Simulating,
        Simulation_Paused,
        Simulation_Stepping,

        Playback,
        Playback_Paused,
        Playback_Scrubbing,

        Modal_Exporting,
        Modal_ConfirmQuit,

        Shutdown
    };

    enum class EditorEvent : uint16_t {
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
        Viewport_PlaceAxiom,
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

    class EditorHFSM {
    public:
        EditorHFSM();

        void handle_event(EditorEvent e, GlobalState& gs);
        void update(GlobalState& gs, float dt);

        [[nodiscard]] EditorState state() const noexcept { return m_state; }

    private:
        EditorState m_state{ EditorState::Startup };
        EditorState m_modal_return_state{ EditorState::Idle };
        EditorState m_viewport_return_state{ EditorState::Editing_Roads };

        void transition_to(EditorState next, GlobalState& gs);

        void on_enter(EditorState s, GlobalState& gs);
        void on_exit(EditorState s, GlobalState& gs);
    };

    EditorHFSM& GetEditorHFSM();

} // namespace RogueCity::Core::Editor

