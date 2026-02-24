@echo off
pwsh -NoExit -ExecutionPolicy Bypass -File "%~dp0Start-All.ps1" -ForceKillExistingPortProcess -KeepLauncherOpen
