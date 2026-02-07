// FILE: rc_panel_dev_shell.h
// PURPOSE: Development-only cockpit panel for introspection/export tools.

#pragma once

namespace RC_UI::Panels::DevShell {

void Draw(float dt);
bool IsOpen();
void Toggle();

} // namespace RC_UI::Panels::DevShell

