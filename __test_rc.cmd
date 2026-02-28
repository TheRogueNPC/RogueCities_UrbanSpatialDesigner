@echo off
cd /d "C:\Users\teamc\Documents\Rogue Cities\RogueCities_UrbanSpatialDesigner"
pwsh -NoProfile -Command ". ./tools/dev-shell.ps1; rc-doctor" > __rc_doctor_test.log 2>&1
echo EXITCODE=%ERRORLEVEL% > __rc_doctor_exit.log
pwsh -NoProfile -Command ". ./tools/dev-shell.ps1; rc-problems -MaxItems 5" > __rc_problems_test.log 2>&1
echo EXITCODE=%ERRORLEVEL% > __rc_problems_exit.log
