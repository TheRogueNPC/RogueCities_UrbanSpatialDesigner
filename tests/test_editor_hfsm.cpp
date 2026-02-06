#include "RogueCity/Core/Editor/EditorState.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"

#include <cassert>

using RogueCity::Core::Editor::EditorEvent;
using RogueCity::Core::Editor::EditorHFSM;
using RogueCity::Core::Editor::EditorState;
using RogueCity::Core::Editor::GlobalState;

int main()
{
    GlobalState gs{};
    EditorHFSM hfsm{};

    assert(hfsm.state() == EditorState::Startup);

    hfsm.handle_event(EditorEvent::BootComplete, gs);
    assert(hfsm.state() == EditorState::NoProject);

    hfsm.handle_event(EditorEvent::NewProject, gs);
    assert(hfsm.state() == EditorState::ProjectLoading);

    hfsm.handle_event(EditorEvent::ProjectLoaded, gs);
    assert(hfsm.state() == EditorState::Idle);

    hfsm.handle_event(EditorEvent::Tool_Roads, gs);
    assert(hfsm.state() == EditorState::Editing_Roads);

    hfsm.handle_event(EditorEvent::BeginSim, gs);
    assert(hfsm.state() == EditorState::Simulating);

    hfsm.handle_event(EditorEvent::PauseSim, gs);
    assert(hfsm.state() == EditorState::Simulation_Paused);

    hfsm.handle_event(EditorEvent::StepSim, gs);
    assert(hfsm.state() == EditorState::Simulation_Stepping);

    hfsm.update(gs, 1.0f / 60.0f);
    assert(hfsm.state() == EditorState::Simulation_Paused);

    hfsm.handle_event(EditorEvent::StopSim, gs);
    assert(hfsm.state() == EditorState::Idle);

    return 0;
}

