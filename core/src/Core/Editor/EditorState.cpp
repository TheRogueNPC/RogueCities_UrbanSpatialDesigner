#include "RogueCity/Core/Editor/EditorState.hpp"

#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Validation/EditorIntegrity.hpp"

#include <vector>
#include <optional>

namespace RogueCity::Core::Editor {

    namespace {

        [[nodiscard]] std::optional<EditorState> Parent(EditorState s)
        {
            switch (s) {
            case EditorState::Editing_Axioms:
            case EditorState::Editing_Roads:
            case EditorState::Editing_Districts:
            case EditorState::Editing_Lots:
            case EditorState::Editing_Buildings:
            case EditorState::Editing_Water:
            case EditorState::Viewport_Pan:
            case EditorState::Viewport_Select:
            case EditorState::Viewport_PlaceAxiom:
            case EditorState::Viewport_DrawRoad:
            case EditorState::Viewport_BoxSelect:
                return EditorState::Editing;

            case EditorState::Editing:
            case EditorState::Simulating:
            case EditorState::Playback:
            case EditorState::Modal_Exporting:
            case EditorState::Modal_ConfirmQuit:
                return EditorState::Idle;

            case EditorState::Simulation_Paused:
            case EditorState::Simulation_Stepping:
                return EditorState::Simulating;

            case EditorState::Playback_Paused:
            case EditorState::Playback_Scrubbing:
                return EditorState::Playback;

            default:
                return std::nullopt;
            }
        }

        [[nodiscard]] bool IsEditingLeaf(EditorState s)
        {
            switch (s) {
            case EditorState::Editing_Axioms:
            case EditorState::Editing_Roads:
            case EditorState::Editing_Districts:
            case EditorState::Editing_Lots:
            case EditorState::Editing_Buildings:
            case EditorState::Editing_Water:
                return true;
            default:
                return false;
            }
        }

        [[nodiscard]] bool IsViewportState(EditorState s)
        {
            switch (s) {
            case EditorState::Viewport_Pan:
            case EditorState::Viewport_Select:
            case EditorState::Viewport_PlaceAxiom:
            case EditorState::Viewport_DrawRoad:
            case EditorState::Viewport_BoxSelect:
                return true;
            default:
                return false;
            }
        }

        [[nodiscard]] std::vector<EditorState> ChainToRoot(EditorState s)
        {
            std::vector<EditorState> chain;
            chain.push_back(s);
            while (true) {
                auto p = Parent(chain.back());
                if (!p.has_value()) {
                    break;
                }
                chain.push_back(*p);
            }
            return chain;
        }

        [[nodiscard]] std::optional<EditorState> FindLCA(const std::vector<EditorState>& a, const std::vector<EditorState>& b)
        {
            for (EditorState sa : a) {
                for (EditorState sb : b) {
                    if (sa == sb) {
                        return sa;
                    }
                }
            }
            return std::nullopt;
        }

    } // namespace

    EditorHFSM::EditorHFSM() = default;

    void EditorHFSM::handle_event(EditorEvent e, GlobalState& gs)
    {
        // Modal handling takes priority.
        if (m_state == EditorState::Modal_Exporting) {
            if (e == EditorEvent::CancelModal) {
                transition_to(m_modal_return_state, gs);
            }
            return;
        }
        if (m_state == EditorState::Modal_ConfirmQuit) {
            if (e == EditorEvent::CancelModal) {
                transition_to(m_modal_return_state, gs);
                return;
            }
            if (e == EditorEvent::Quit) {
                transition_to(EditorState::Shutdown, gs);
                return;
            }
            return;
        }

        // Viewport interaction states return back to the previous editing leaf/tool.
        if (IsViewportState(m_state)) {
            if (e == EditorEvent::GotoIdle) {
                transition_to(EditorState::Idle, gs);
                return;
            }
            if (e == EditorEvent::Tool_Axioms) {
                transition_to(EditorState::Editing_Axioms, gs);
                return;
            }
            if (e == EditorEvent::Tool_Roads) {
                transition_to(EditorState::Editing_Roads, gs);
                return;
            }
            if (e == EditorEvent::Tool_Districts) {
                transition_to(EditorState::Editing_Districts, gs);
                return;
            }
            if (e == EditorEvent::Tool_Lots) {
                transition_to(EditorState::Editing_Lots, gs);
                return;
            }
            if (e == EditorEvent::Tool_Buildings) {
                transition_to(EditorState::Editing_Buildings, gs);
                return;
            }
            if (e == EditorEvent::Tool_Water) {
                transition_to(EditorState::Editing_Water, gs);
                return;
            }
            if (e == EditorEvent::Viewport_Select) {
                transition_to(m_viewport_return_state, gs);
                return;
            }
        }

        switch (m_state) {
        case EditorState::Startup:
            if (e == EditorEvent::BootComplete) {
                transition_to(EditorState::NoProject, gs);
            }
            break;

        case EditorState::NoProject:
            if (e == EditorEvent::NewProject || e == EditorEvent::OpenProject) {
                transition_to(EditorState::ProjectLoading, gs);
            }
            if (e == EditorEvent::Quit) {
                transition_to(EditorState::Shutdown, gs);
            }
            break;

        case EditorState::ProjectLoading:
            if (e == EditorEvent::ProjectLoaded) {
                transition_to(EditorState::Idle, gs);
            }
            if (e == EditorEvent::CloseProject) {
                transition_to(EditorState::NoProject, gs);
            }
            break;

        case EditorState::Idle:
            switch (e) {
            case EditorEvent::Tool_Axioms: transition_to(EditorState::Editing_Axioms, gs); break;
            case EditorEvent::Tool_Roads: transition_to(EditorState::Editing_Roads, gs); break;
            case EditorEvent::Tool_Districts: transition_to(EditorState::Editing_Districts, gs); break;
            case EditorEvent::Tool_Lots: transition_to(EditorState::Editing_Lots, gs); break;
            case EditorEvent::Tool_Buildings: transition_to(EditorState::Editing_Buildings, gs); break;
            case EditorEvent::Tool_Water: transition_to(EditorState::Editing_Water, gs); break;
            case EditorEvent::BeginSim: transition_to(EditorState::Simulating, gs); break;
            case EditorEvent::BeginPlayback: transition_to(EditorState::Playback, gs); break;
            case EditorEvent::Export:
                m_modal_return_state = EditorState::Idle;
                transition_to(EditorState::Modal_Exporting, gs);
                break;
            case EditorEvent::Quit:
                m_modal_return_state = EditorState::Idle;
                transition_to(EditorState::Modal_ConfirmQuit, gs);
                break;
            default: break;
            }
            break;

        case EditorState::Editing:
        case EditorState::Editing_Axioms:
        case EditorState::Editing_Roads:
        case EditorState::Editing_Districts:
        case EditorState::Editing_Lots:
        case EditorState::Editing_Buildings:
        case EditorState::Editing_Water:
            switch (e) {
            case EditorEvent::GotoIdle: transition_to(EditorState::Idle, gs); break;
            case EditorEvent::Tool_Axioms: transition_to(EditorState::Editing_Axioms, gs); break;
            case EditorEvent::Tool_Roads: transition_to(EditorState::Editing_Roads, gs); break;
            case EditorEvent::Tool_Districts: transition_to(EditorState::Editing_Districts, gs); break;
            case EditorEvent::Tool_Lots: transition_to(EditorState::Editing_Lots, gs); break;
            case EditorEvent::Tool_Buildings: transition_to(EditorState::Editing_Buildings, gs); break;
            case EditorEvent::Tool_Water: transition_to(EditorState::Editing_Water, gs); break;
            case EditorEvent::Viewport_Pan:
                m_viewport_return_state = IsEditingLeaf(m_state) ? m_state : EditorState::Editing_Roads;
                transition_to(EditorState::Viewport_Pan, gs);
                break;
            case EditorEvent::Viewport_Select:
                m_viewport_return_state = IsEditingLeaf(m_state) ? m_state : EditorState::Editing_Roads;
                transition_to(EditorState::Viewport_Select, gs);
                break;
            case EditorEvent::Viewport_PlaceAxiom:
                m_viewport_return_state = IsEditingLeaf(m_state) ? m_state : EditorState::Editing_Roads;
                transition_to(EditorState::Viewport_PlaceAxiom, gs);
                break;
            case EditorEvent::Viewport_DrawRoad:
                m_viewport_return_state = IsEditingLeaf(m_state) ? m_state : EditorState::Editing_Roads;
                transition_to(EditorState::Viewport_DrawRoad, gs);
                break;
            case EditorEvent::Viewport_BoxSelect:
                m_viewport_return_state = IsEditingLeaf(m_state) ? m_state : EditorState::Editing_Roads;
                transition_to(EditorState::Viewport_BoxSelect, gs);
                break;
            case EditorEvent::BeginSim: transition_to(EditorState::Simulating, gs); break;
            case EditorEvent::Quit:
                m_modal_return_state = m_state;
                transition_to(EditorState::Modal_ConfirmQuit, gs);
                break;
            default: break;
            }
            break;

        case EditorState::Simulating:
            switch (e) {
            case EditorEvent::PauseSim: transition_to(EditorState::Simulation_Paused, gs); break;
            case EditorEvent::StopSim: transition_to(EditorState::Idle, gs); break;
            case EditorEvent::Quit:
                m_modal_return_state = EditorState::Simulating;
                transition_to(EditorState::Modal_ConfirmQuit, gs);
                break;
            default: break;
            }
            break;

        case EditorState::Simulation_Paused:
            switch (e) {
            case EditorEvent::ResumeSim: transition_to(EditorState::Simulating, gs); break;
            case EditorEvent::StepSim: transition_to(EditorState::Simulation_Stepping, gs); break;
            case EditorEvent::StopSim: transition_to(EditorState::Idle, gs); break;
            default: break;
            }
            break;

        case EditorState::Simulation_Stepping:
            if (e == EditorEvent::StopSim) {
                transition_to(EditorState::Idle, gs);
            }
            break;

        case EditorState::Playback:
            switch (e) {
            case EditorEvent::PausePlayback: transition_to(EditorState::Playback_Paused, gs); break;
            case EditorEvent::StopPlayback: transition_to(EditorState::Idle, gs); break;
            case EditorEvent::Quit:
                m_modal_return_state = EditorState::Playback;
                transition_to(EditorState::Modal_ConfirmQuit, gs);
                break;
            default: break;
            }
            break;

        case EditorState::Playback_Paused:
            switch (e) {
            case EditorEvent::PausePlayback: transition_to(EditorState::Playback, gs); break;
            case EditorEvent::ScrubPlayback: transition_to(EditorState::Playback_Scrubbing, gs); break;
            case EditorEvent::StopPlayback: transition_to(EditorState::Idle, gs); break;
            default: break;
            }
            break;

        case EditorState::Playback_Scrubbing:
            switch (e) {
            case EditorEvent::PausePlayback: transition_to(EditorState::Playback_Paused, gs); break;
            case EditorEvent::BeginPlayback: transition_to(EditorState::Playback, gs); break;
            case EditorEvent::StopPlayback: transition_to(EditorState::Idle, gs); break;
            default: break;
            }
            break;

        case EditorState::Shutdown:
            break;

        default:
            break;
        }
    }

    void EditorHFSM::update(GlobalState& gs, float /*dt*/)
    {
        if (m_state == EditorState::Simulation_Stepping) {
            // Placeholder for a single deterministic simulation step.
            // Future: run one tick of sim systems using RogueWorker for heavy workloads.
            ++gs.frame_counter;
            transition_to(EditorState::Simulation_Paused, gs);
        }
    }

    void EditorHFSM::transition_to(EditorState next, GlobalState& gs)
    {
        if (next == m_state) {
            return;
        }

        const std::vector<EditorState> from_chain = ChainToRoot(m_state);
        const std::vector<EditorState> to_chain = ChainToRoot(next);

        const std::optional<EditorState> lca = FindLCA(from_chain, to_chain);

        // Exit from leaf up to (but not including) LCA.
        for (EditorState s : from_chain) {
            if (lca.has_value() && s == *lca) {
                break;
            }
            on_exit(s, gs);
        }

        // Enter from LCA down to leaf.
        std::vector<EditorState> enter_path;
        for (EditorState s : to_chain) {
            if (lca.has_value() && s == *lca) {
                break;
            }
            enter_path.push_back(s);
        }
        for (auto it = enter_path.rbegin(); it != enter_path.rend(); ++it) {
            on_enter(*it, gs);
        }

        m_state = next;
    }

    void EditorHFSM::on_enter(EditorState s, GlobalState& gs)
    {
        switch (s) {
        case EditorState::Simulating:
            RogueCity::Core::Validation::ValidateAll(gs);
            RogueCity::Core::Validation::SpatialCheckAll(gs);
            break;
        default:
            break;
        }
    }

    void EditorHFSM::on_exit(EditorState s, GlobalState& gs)
    {
        switch (s) {
        case EditorState::Editing_Roads:
            RogueCity::Core::Validation::ValidateRoads(gs.roads);
            break;
        case EditorState::Editing_Districts:
            RogueCity::Core::Validation::ValidateDistricts(gs.districts);
            break;
        case EditorState::Editing_Lots:
            RogueCity::Core::Validation::ValidateLots(gs.lots);
            break;
        case EditorState::Editing_Buildings:
            RogueCity::Core::Validation::ValidateBuildings(gs.buildings);
            break;
        default:
            break;
        }
    }

    EditorHFSM& GetEditorHFSM()
    {
        static EditorHFSM hfsm{};
        return hfsm;
    }

} // namespace RogueCity::Core::Editor
