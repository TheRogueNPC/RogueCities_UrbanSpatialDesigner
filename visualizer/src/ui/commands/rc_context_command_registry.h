#pragma once

#include "ui/rc_ui_root.h"
#include "ui/tools/rc_tool_contract.h"
#include "ui/tools/rc_tool_dispatcher.h"
#include "RogueCity/Core/Editor/GlobalState.hpp"

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace RC_UI::Commands {

[[nodiscard]] std::span<const RC_UI::Tools::ToolActionSpec> GetCommandRegistry();

[[nodiscard]] std::vector<const RC_UI::Tools::ToolActionSpec*> FilterCommandRegistry(
    std::string_view filter,
    bool include_disabled = true);

[[nodiscard]] bool ExecuteCommand(
    RC_UI::Tools::ToolActionId action_id,
    const RC_UI::Tools::DispatchContext& dispatch_context,
    std::string* out_status = nullptr);

[[nodiscard]] ToolLibrary CommandLibraryForDomain(RogueCity::Core::Editor::ToolDomain domain);
[[nodiscard]] const char* CommandLibraryName(ToolLibrary library);

} // namespace RC_UI::Commands
