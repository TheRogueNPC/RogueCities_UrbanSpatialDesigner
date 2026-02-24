[CmdletBinding()]
param(
    [string]$ProjectDir = $PSScriptRoot,
    [int]$Port = 7078,
    [string]$Host = "127.0.0.1",
    [switch]$KeepLauncherOpen = $true
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Section([string]$Text) {
    Write-Host ""
    Write-Host ("=" * 60) -ForegroundColor DarkGray
    Write-Host $Text -ForegroundColor Cyan
    Write-Host ("=" * 60) -ForegroundColor DarkGray
}

function Write-Info([string]$Text) {
    Write-Host "[INFO] $Text" -ForegroundColor Gray
}

function Write-Ok([string]$Text) {
    Write-Host "[OK]   $Text" -ForegroundColor Green
}

function Write-Err([string]$Text) {
    Write-Host "[ERR]  $Text" -ForegroundColor Red
}

Write-Section "Starting RogueFS Agent"

if (-not (Test-Path $ProjectDir)) {
    throw "ProjectDir not found: $ProjectDir"
}
$ProjectDir = (Resolve-Path $ProjectDir).Path

$VenvPython = Join-Path $ProjectDir "..\ollama-gateway\.venv\Scripts\python.exe"
$PythonExe = "python"

if (Test-Path $VenvPython) {
    $PythonExe = $VenvPython
    Write-Info "Using venv Python: $VenvPython"
} else {
    if (-not (Get-Command "python" -ErrorAction SilentlyContinue)) {
        throw "python not found on PATH."
    }
    $PythonExe = (Get-Command "python").Source
    Write-Info "Using system Python: $PythonExe"
}

if (-not (Get-Command "rg" -ErrorAction SilentlyContinue)) {
    Write-Host "[WARN] ripgrep (rg) not found. Search will not work." -ForegroundColor Yellow
    Write-Host "       Install with: winget install BurntSushi.ripgrep.MSVC" -ForegroundColor DarkGray
}

$envFile = Join-Path $ProjectDir "..\ollama-gateway\.env"
if (Test-Path $envFile) {
    Write-Info "Loading environment from: $envFile"
    Get-Content $envFile | ForEach-Object {
        if ($_ -match '^([^#][^=]+)=(.*)$') {
            $name = $Matches[1].Trim()
            $value = $Matches[2].Trim()
            if ($value.StartsWith('"') -and $value.EndsWith('"')) {
                $value = $value.Substring(1, $value.Length - 2)
            }
            [Environment]::SetEnvironmentVariable($name, $value, "Process")
        }
    }
}

if (-not $env:ROGUEFS_ALLOWLIST) {
    $repoRoot = (Resolve-Path (Join-Path $ProjectDir "..\..")).Path
    $env:ROGUEFS_ALLOWLIST = $repoRoot
    Write-Info "ROGUEFS_ALLOWLIST not set, using: $repoRoot"
}

$env:ROGUEFS_PORT = $Port
$env:ROGUEFS_HOST = $Host

Write-Info "Port: $Port"
Write-Info "Host: $Host"
Write-Info "Allowlist: $env:ROGUEFS_ALLOWLIST"

$agentPath = Join-Path $ProjectDir "roguefs-agent.py"
if (-not (Test-Path $agentPath)) {
    throw "roguefs-agent.py not found at: $agentPath"
}

Write-Ok "Starting RogueFS agent on http://${Host}:${Port}"
Write-Host ""
Write-Host "Endpoints:" -ForegroundColor DarkGray
Write-Host "  GET /v1/health   - Health check" -ForegroundColor DarkGray
Write-Host "  GET /v1/roots    - List allowed roots" -ForegroundColor DarkGray
Write-Host "  GET /v1/list     - List directory (paged)" -ForegroundColor DarkGray
Write-Host "  GET /v1/read     - Read file (ranged)" -ForegroundColor DarkGray
Write-Host "  GET /v1/stat     - File metadata" -ForegroundColor DarkGray
Write-Host "  GET /v1/search   - Ripgrep search" -ForegroundColor DarkGray
Write-Host ""
Write-Host "Press Ctrl+C to stop." -ForegroundColor Yellow
Write-Host ""

try {
    & $PythonExe $agentPath
}
catch {
    Write-Err $_.Exception.Message
}
finally {
    if ($KeepLauncherOpen) {
        Write-Host ""
        Write-Host "Press any key to close..." -ForegroundColor Yellow
        $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
    }
}
