#pragma once

#include "ui/tools/rc_tool_contract.h"

#include <string>

namespace RogueCity::Core::Editor {
class EditorHFSM;
struct GlobalState;
}

namespace RogueCity::UIInt {
class UiIntrospector;
}

namespace RC_UI::Tools {

enum class DispatchResult : uint8_t {
    Handled = 0,
    Disabled,
    UnknownAction,
    InvalidContext
};

struct DispatchContext {
    RogueCity::Core::Editor::EditorHFSM* hfsm = nullptr;
    RogueCity::Core::Editor::GlobalState* gs = nullptr;
    RogueCity::UIInt::UiIntrospector* introspector = nullptr;
    const char* panel_id = "Tool Library";
};

[[nodiscard]] DispatchResult DispatchToolAction(ToolActionId action, const DispatchContext& context, std::string* out_status = nullptr);

} // namespace RC_UI::Tools
