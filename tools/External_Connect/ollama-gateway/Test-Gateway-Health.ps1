# File: test-gateway-health.ps1
# Purpose: Tests the public /health endpoint through the Cloudflare tunnel.
# Added/Changed:
# - Prompts for tunnel URL and API key (masked)
# - Avoids quoting/escaping issues with special characters in PowerShell

[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

$tunnelUrl = Read-Host "Enter current tunnel URL (e.g. https://abc.trycloudflare.com)"
if ([string]::IsNullOrWhiteSpace($tunnelUrl)) {
    Write-Error "Tunnel URL is required."
    exit 1
}

$apiKey = Read-Host -MaskInput "Enter API key (masked)"
if ([string]::IsNullOrWhiteSpace($apiKey)) {
    Write-Error "API key is required."
    exit 1
}

$healthUrl = ($tunnelUrl.TrimEnd('/')) + "/health"

Write-Host "[Test] Calling $healthUrl" -ForegroundColor Cyan
$response = Invoke-RestMethod $healthUrl -Headers @{ "X-API-Key" = $apiKey }

Write-Host "[Test] Success" -ForegroundColor Green
$response | ConvertTo-Json -Depth 8