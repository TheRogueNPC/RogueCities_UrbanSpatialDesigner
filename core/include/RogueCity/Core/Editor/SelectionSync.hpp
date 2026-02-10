#pragma once

#include "RogueCity/Core/Editor/GlobalState.hpp"

#include <cstddef>
#include <iterator>

namespace RogueCity::Core::Editor {

template <typename TContainer, typename THandle>
inline bool SyncHandleByEntityId(TContainer& container, uint32_t entity_id, THandle& out_handle) {
    for (size_t i = 0; i < container.size(); ++i) {
        auto it = container.begin();
        std::advance(it, static_cast<std::ptrdiff_t>(i));
        if (it->id == entity_id) {
            out_handle = container.createHandleFromData(i);
            return true;
        }
    }
    return false;
}

inline void ClearPrimarySelection(Selection& selection) {
    selection.selected_road = {};
    selection.selected_district = {};
    selection.selected_lot = {};
    selection.selected_building = {};
}

inline bool SyncPrimarySelectionFromManager(GlobalState& gs) {
    ClearPrimarySelection(gs.selection);

    const SelectionItem* primary = gs.selection_manager.Primary();
    if (!primary) {
        return false;
    }

    switch (primary->kind) {
    case VpEntityKind::Road:
        return SyncHandleByEntityId(gs.roads, primary->id, gs.selection.selected_road);
    case VpEntityKind::District:
        return SyncHandleByEntityId(gs.districts, primary->id, gs.selection.selected_district);
    case VpEntityKind::Lot:
        return SyncHandleByEntityId(gs.lots, primary->id, gs.selection.selected_lot);
    case VpEntityKind::Building:
        return SyncHandleByEntityId(gs.buildings, primary->id, gs.selection.selected_building);
    default:
        return false;
    }
}

} // namespace RogueCity::Core::Editor
