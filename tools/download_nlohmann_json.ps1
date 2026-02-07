# Download nlohmann/json for AI protocol serialization

$jsonDir = "3rdparty/nlohmann_json"
$jsonFile = "$jsonDir/include/nlohmann/json.hpp"

if (Test-Path $jsonFile) {
    Write-Host "nlohmann/json already exists at $jsonFile"
    exit 0
}

Write-Host "Downloading nlohmann/json (header-only library)..."

# Create directory
New-Item -ItemType Directory -Force -Path "$jsonDir/include/nlohmann" | Out-Null

# Download single-header version
$url = "https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp"
Invoke-WebRequest -Uri $url -OutFile $jsonFile

Write-Host "? Downloaded nlohmann/json to $jsonFile"
Write-Host "Run: cmake -B build -S . to reconfigure"
