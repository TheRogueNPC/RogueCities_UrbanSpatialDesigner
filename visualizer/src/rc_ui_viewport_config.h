// This file serves as a central header for the viewport configuration of the visualizer module. It should include any necessary headers from the ui/ directory that define constants, enums, and other tokens related to viewport configuration that are shared across multiple UI components. This helps to keep the code organized and ensures that all UI components have access to the necessary definitions without needing to include multiple headers individually. It should not contain any implementation code or definitions that are specific to a single component, as those should be defined in their respective headers and source files to maintain separation of concerns and avoid ODR violations.

#pragma once

// Include the header for viewport configuration constants and enums from the ui/ directory
#include "ui/rc_ui_viewport_config.h"
