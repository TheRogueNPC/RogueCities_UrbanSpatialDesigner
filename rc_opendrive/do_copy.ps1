New-Item -ItemType Directory -Force -Path ".\rc_opendrive\src"
New-Item -ItemType Directory -Force -Path ".\rc_opendrive\include\RogueOpenDRIVE"

Copy-Item -Path ".\.Temp\RogueOpenDRIVE-main\src\*" -Destination ".\rc_opendrive\src\" -Recurse -Force
Copy-Item -Path ".\.Temp\RogueOpenDRIVE-main\include\*" -Destination ".\rc_opendrive\include\RogueOpenDRIVE\" -Recurse -Force

Write-Output "Copy Complete"
