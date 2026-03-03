# build_rc_opendrive.ps1
# One-shot script: sources dev-shell.ps1 environment, then builds rc_opendrive.
# Run: powershell -NoProfile -ExecutionPolicy Bypass -File tools\build_rc_opendrive.ps1
$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$env:RC_ROOT = $repoRoot
$env:RC_BUILD_DIR = Join-Path $repoRoot "build_vs"

# Inject cmake from standard locations (mirrors dev-shell.ps1 logic)
$cmakePaths = @(
    (Join-Path $env:ProgramFiles "CMake\bin"),
    (Join-Path ${env:ProgramFiles(x86)} "CMake\bin")
) | Where-Object { $_ -and (Test-Path $_) }

foreach ($p in $cmakePaths) {
    if (-not ($env:PATH -split ';' | Where-Object { $_ -ieq $p })) {
        $env:PATH = "$p;$env:PATH"
    }
}

Write-Host "[rc_opendrive build] Starting..." -ForegroundColor Cyan
cmake --build "$env:RC_BUILD_DIR" --target rc_opendrive --config Release 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Error "[rc_opendrive build] FAILED (exit $LASTEXITCODE)"
    exit $LASTEXITCODE
}
Write-Host "[rc_opendrive build] SUCCESS" -ForegroundColor Green
