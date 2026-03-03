@echo off
echo Running Deployment...
if not exist "rc_opendrive\src" mkdir "rc_opendrive\src"
if not exist "rc_opendrive\include\RogueOpenDRIVE" mkdir "rc_opendrive\include\RogueOpenDRIVE"

xcopy ".Temp\RogueOpenDRIVE-main\src\*" "rc_opendrive\src\" /E /I /Y
xcopy ".Temp\RogueOpenDRIVE-main\include\*" "rc_opendrive\include\RogueOpenDRIVE\" /E /I /Y

echo Contents of rc_opendrive\src:
dir "rc_opendrive\src"
echo Contents of rc_opendrive\include\RogueOpenDRIVE:
dir "rc_opendrive\include\RogueOpenDRIVE"
echo Deployment Finished.
