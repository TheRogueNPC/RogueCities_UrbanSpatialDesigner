@echo off
echo Removing obsolete files...
del /f /q "visualizer\src\ui\tools\rc_tool_geometry_policy.cpp"
del /f /q "visualizer\src\ui\tools\rc_tool_geometry_policy.h"
echo Running compliance check...
python tools\check_ui_compliance.py > compliance_report.txt 2>&1
echo Done.
