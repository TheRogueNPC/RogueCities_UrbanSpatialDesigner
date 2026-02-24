[CmdletBinding()]
param(
    [string]$ProjectDir = $PSScriptRoot,
    [string]$CodeRoot = "",
    [string]$GatewayHost = "127.0.0.1",
    [int]$GatewayPort = 7077,
    [string]$GatewayModule = "gateway:app",
    [string]$OllamaBaseUrl = "http://127.0.0.1:11434",
    [int]$HealthTimeoutSeconds = 60,
    [int]$HealthPollMs = 1000,
    [string]$HealthApiKey = "",
    [switch]$SkipOllamaStart,
    [switch]$SkipZrok,
    [switch]$KeepLauncherOpen = $true,
    [switch]$ForceKillExistingPortProcess = $true,
    [switch]$PublicHealth,
    [switch]$UseVenv = $true
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Section([string]$Text) {
    Write-Host ""
    Write-Host ("=" * 80) -ForegroundColor DarkGray
    Write-Host $Text -ForegroundColor Cyan
    Write-Host ("=" * 80) -ForegroundColor DarkGray
}

function Write-Info([string]$Text) {
    Write-Host "[INFO] $Text" -ForegroundColor Gray
}

function Write-Ok([string]$Text) {
    Write-Host "[OK]   $Text" -ForegroundColor Green
}

function Write-WarnMsg([string]$Text) {
    Write-Host "[WARN] $Text" -ForegroundColor Yellow
}

function Write-ErrMsg([string]$Text) {
    Write-Host "[ERR]  $Text" -ForegroundColor Red
}

function Test-CommandExists([string]$Name) {
    return $null -ne (Get-Command $Name -ErrorAction SilentlyContinue)
}

function Test-TcpListen {
    param([string]$ListenHost, [int]$Port)
    try {
        $conn = Get-NetTCPConnection -State Listen -LocalPort $Port -ErrorAction Stop |
            Where-Object { $_.LocalAddress -eq $ListenHost -or $_.LocalAddress -eq "0.0.0.0" -or $_.LocalAddress -eq "::" } |
            Select-Object -First 1
        return $null -ne $conn
    } catch {
        try {
            $matches = netstat -ano -p tcp | Select-String -Pattern (":$Port\s+.*LISTENING\s+")
            return $matches.Count -gt 0
        } catch {
            return $false
        }
    }
}

function Get-TcpListenerProcess {
    param([int]$Port)
    try {
        $conn = Get-NetTCPConnection -State Listen -LocalPort $Port -ErrorAction Stop | Select-Object -First 1
        if ($conn) {
            return Get-Process -Id $conn.OwningProcess -ErrorAction Stop
        }
    } catch {
        try {
            $line = netstat -ano -p tcp | Select-String -Pattern (":$Port\s+.*LISTENING\s+") | Select-Object -First 1
            if ($line) {
                $parts = ($line.ToString() -replace '^\s+','') -split '\s+'
                $pid = [int]$parts[-1]
                return Get-Process -Id $pid -ErrorAction Stop
            }
        } catch {
            return $null
        }
    }
    return $null
}

function Stop-ProcessSafe {
    param(
        [Parameter(Mandatory=$true)]$Proc,
        [int]$GraceMs = 1500
    )
    try {
        if ($null -eq $Proc) { return }
        try { $null = $Proc.HasExited } catch { return }
        if ($Proc.HasExited) { return }

        Write-Info "Stopping PID $($Proc.Id) ($($Proc.ProcessName)) ..."
        try {
            Stop-Process -Id $Proc.Id -ErrorAction SilentlyContinue
        } catch {}

        Start-Sleep -Milliseconds $GraceMs

        try { $Proc.Refresh() } catch {}
        try {
            if (-not $Proc.HasExited) {
                Write-WarnMsg "Force killing PID $($Proc.Id) ..."
                Stop-Process -Id $Proc.Id -Force -ErrorAction SilentlyContinue
            }
        } catch {}
    } catch {
        Write-WarnMsg "Failed stopping process: $($_.Exception.Message)"
    }
}

function Start-LoggedProcess {
    param(
        [Parameter(Mandatory=$true)][string]$FilePath,
        [Parameter(Mandatory=$true)][string[]]$ArgumentList,
        [Parameter(Mandatory=$true)][string]$WorkingDirectory,
        [Parameter(Mandatory=$true)][string]$StdOutPath,
        [Parameter(Mandatory=$true)][string]$StdErrPath,
        [hashtable]$Env = @{},
        [string]$Title = ""
    )

    $resolvedExe = $FilePath
    try {
        $cmd = Get-Command $FilePath -ErrorAction Stop | Select-Object -First 1
        if ($cmd -and $cmd.Source) {
            $resolvedExe = $cmd.Source
        }
    } catch {
        $resolvedExe = $FilePath
    }

    $wd = $WorkingDirectory
    if ([string]::IsNullOrWhiteSpace($wd) -or -not (Test-Path -LiteralPath $wd)) {
        $wd = $PSScriptRoot
    }
    $wd = (Resolve-Path -LiteralPath $wd).Path

    if ($Title) { Write-Info "Starting $Title..." }

    $splat = @{
        FilePath = $resolvedExe
        ArgumentList = $ArgumentList
        WorkingDirectory = $wd
        RedirectStandardOutput = $StdOutPath
        RedirectStandardError = $StdErrPath
        NoNewWindow = $true
        PassThru = $true
    }

    if ($Env.Count -gt 0) {
        $splat['Environment'] = $Env
    }

    $proc = Start-Process @splat

    Start-Sleep -Milliseconds 900

    try {
        $proc.Refresh()
        if ($proc.HasExited) {
            $errPreview = ""
            if (Test-Path $StdErrPath) {
                $errPreview = (Get-Content $StdErrPath -Tail 80 -ErrorAction SilentlyContinue) -join [Environment]::NewLine
            }
            throw "$Title exited immediately (PID $($proc.Id)).`n$errPreview"
        }
    } catch {
        if ($_.Exception.Message -like '*exited immediately*') { throw }
    }

    return $proc
}

function Invoke-HealthProbe {
    param(
        [Parameter(Mandatory=$true)][string]$Url,
        [string]$ApiKey = "",
        [switch]$AllowPublic
    )

    if ($AllowPublic) {
        try {
            return Invoke-RestMethod -Uri $Url -Method GET -TimeoutSec 5
        } catch {
            if (-not $ApiKey) { throw }
        }
    }

    if ($ApiKey) {
        try {
            return Invoke-RestMethod -Uri $Url -Method GET -TimeoutSec 5 -Headers @{ "X-API-Key" = $ApiKey }
        } catch {
            try {
                return Invoke-RestMethod -Uri $Url -Method GET -TimeoutSec 5 -Headers @{ "Authorization" = "Bearer $ApiKey" }
            } catch {
                throw
            }
        }
    }

    if (-not $AllowPublic) {
        return Invoke-RestMethod -Uri $Url -Method GET -TimeoutSec 5
    }
}

function Wait-ForHealth {
    param(
        [Parameter(Mandatory=$true)][string]$Url,
        [int]$TimeoutSeconds = 60,
        [int]$PollMs = 1000,
        [string]$ApiKey = "",
        [switch]$PublicProbe
    )

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    $lastError = $null
    while ((Get-Date) -lt $deadline) {
        try {
            $resp = Invoke-HealthProbe -Url $Url -ApiKey $ApiKey -AllowPublic:$PublicProbe
            return $resp
        } catch {
            $lastError = $_.Exception.Message
            Start-Sleep -Milliseconds $PollMs
        }
    }
    if ($lastError) {
        throw "Health check timed out: $Url (last error: $lastError)"
    }
    throw "Health check timed out: $Url"
}

function Get-ZrokPublicUrlFromLogs {
    param(
        [string]$StdOutPath,
        [string]$StdErrPath
    )

    foreach ($path in @($StdOutPath, $StdErrPath)) {
        if (-not $path) { continue }
        if (-not (Test-Path $path)) { continue }
        try {
            $lines = Get-Content $path -Tail 400 -ErrorAction SilentlyContinue
            foreach ($line in $lines) {
                if ($line -match 'https://[a-zA-Z0-9\.-]+\.share\.zrok\.io') {
                    return $Matches[0]
                }
            }
        } catch {}
    }
    return $null
}

function Dump-LogTail {
    param(
        [string]$Path,
        [int]$Tail = 80,
        [string]$Label = ""
    )
    if (-not (Test-Path $Path)) {
        Write-WarnMsg "$Label (log file not found yet)"
        return
    }
    if ($Label) { Write-Host "--- $Label ($Path) ---" -ForegroundColor DarkYellow }
    try {
        Get-Content $Path -Tail $Tail -ErrorAction SilentlyContinue
    } catch {
        Write-WarnMsg "Could not read log ${Path}: $($_.Exception.Message)"
    }
}

function Try-LoadEnvFileValue {
    param(
        [Parameter(Mandatory=$true)][string]$EnvFile,
        [Parameter(Mandatory=$true)][string]$Name
    )

    if (-not (Test-Path $EnvFile)) { return $null }
    try {
        foreach ($line in (Get-Content $EnvFile -ErrorAction SilentlyContinue)) {
            if ($line -match '^\s*#') { continue }
            if ($line -match '^\s*$') { continue }
            if ($line -match "^\s*$([regex]::Escape($Name))\s*=\s*(.*)\s*$") {
                $value = $Matches[1]
                if ($value.Length -ge 2) {
                    if (($value.StartsWith('"') -and $value.EndsWith('"')) -or ($value.StartsWith("'") -and $value.EndsWith("'"))) {
                        $value = $value.Substring(1, $value.Length - 2)
                    }
                }
                return $value
            }
        }
    } catch {}
    return $null
}

Write-Section "Starting local gateway stack"

if (-not (Test-Path $ProjectDir)) { throw "ProjectDir not found: $ProjectDir" }
$ProjectDir = (Resolve-Path $ProjectDir).Path

if (-not $CodeRoot) {
    try {
        $CodeRoot = (Resolve-Path (Join-Path $ProjectDir "..\..\..")).Path
    } catch {
        try {
            $CodeRoot = (Resolve-Path (Join-Path $ProjectDir "..\..")).Path
        } catch {
            $CodeRoot = $ProjectDir
        }
    }
}
if (-not (Test-Path $CodeRoot)) { throw "CodeRoot not found: $CodeRoot" }
$CodeRoot = (Resolve-Path $CodeRoot).Path

$VenvPython = Join-Path $ProjectDir ".venv\Scripts\python.exe"
$PythonExe = "python"

if ($UseVenv -and (Test-Path $VenvPython)) {
    $PythonExe = $VenvPython
    Write-Info "Using venv Python: $VenvPython"
} else {
    if (-not (Test-CommandExists "python")) { throw "python not found on PATH." }
    $PythonExe = (Get-Command "python").Source
    Write-Info "Using system Python: $PythonExe"
}

if (-not (Test-CommandExists "zrok") -and -not $SkipZrok) { throw "zrok not found on PATH. Install zrok or use -SkipZrok." }

if (-not $HealthApiKey) {
    if ($env:API_KEY) {
        $HealthApiKey = $env:API_KEY
    } else {
        $envPath = Join-Path $ProjectDir ".env"
        $loadedKey = Try-LoadEnvFileValue -EnvFile $envPath -Name "API_KEY"
        if ($loadedKey) {
            $HealthApiKey = $loadedKey
            Write-Info "Loaded HealthApiKey from .env"
        }
    }
}

$LogDir = Join-Path $env:TEMP "rogue-gateway-logs"
New-Item -ItemType Directory -Force -Path $LogDir | Out-Null

$GatewayOutLog = Join-Path $LogDir "gateway.out.log"
$GatewayErrLog = Join-Path $LogDir "gateway.err.log"
$OllamaOutLog  = Join-Path $LogDir "ollama.out.log"
$OllamaErrLog  = Join-Path $LogDir "ollama.err.log"
$ZrokOutLog    = Join-Path $LogDir "zrok.out.log"
$ZrokErrLog    = Join-Path $LogDir "zrok.err.log"

foreach ($f in @($GatewayOutLog,$GatewayErrLog,$OllamaOutLog,$OllamaErrLog,$ZrokOutLog,$ZrokErrLog)) {
    try { if (Test-Path $f) { Remove-Item $f -Force -ErrorAction SilentlyContinue } } catch {}
}

$script:started = [System.Collections.Generic.List[System.Diagnostics.Process]]::new()
$script:cleanedUp = $false

function Cleanup-All {
    if ($script:cleanedUp) { return }
    $script:cleanedUp = $true

    Write-Section "Stopping stack"
    for ($i = $script:started.Count - 1; $i -ge 0; $i--) {
        $p = $script:started[$i]
        Stop-ProcessSafe -Proc $p
    }

    Write-Info "Log directory: $LogDir"
    Write-Ok "Shutdown complete."
}

try {
    $existingGatewayProc = Get-TcpListenerProcess -Port $GatewayPort
    if ($existingGatewayProc) {
        if ($ForceKillExistingPortProcess) {
            Write-WarnMsg "Port $GatewayPort already in use by PID $($existingGatewayProc.Id) ($($existingGatewayProc.ProcessName)). Killing because -ForceKillExistingPortProcess was set."
            Stop-ProcessSafe -Proc $existingGatewayProc
            Start-Sleep -Milliseconds 1000
        } else {
            Write-WarnMsg "Port $GatewayPort is already in use by PID $($existingGatewayProc.Id) ($($existingGatewayProc.ProcessName))."
            Write-WarnMsg "Will try to use the existing listener for health probe + zrok (no new gateway start)."
        }
    }

    if (-not $SkipOllamaStart) {
        $ollamaReachable = $false
        try {
            Invoke-RestMethod -Uri "$OllamaBaseUrl/api/tags" -Method GET -TimeoutSec 3 | Out-Null
            $ollamaReachable = $true
        } catch {
            $ollamaReachable = $false
        }

        if ($ollamaReachable) {
            Write-Ok "Ollama already reachable at $OllamaBaseUrl"
        } else {
            if (-not (Test-CommandExists "ollama")) {
                Write-WarnMsg "ollama not found on PATH. Skipping auto-start. Gateway /health may show ollama.reachable=false."
            } else {
                $ollamaProc = Start-LoggedProcess `
                    -FilePath "ollama" `
                    -ArgumentList @("serve") `
                    -WorkingDirectory $ProjectDir `
                    -StdOutPath $OllamaOutLog `
                    -StdErrPath $OllamaErrLog `
                    -Title "Ollama"
                $script:started.Add($ollamaProc) | Out-Null
                Write-Ok "Ollama started (PID $($ollamaProc.Id))"
            }
        }
    } else {
        Write-WarnMsg "Skipping Ollama start because -SkipOllamaStart was set."
    }

    $selectedPort = $GatewayPort
    $portFree = -not (Test-TcpListen -ListenHost $GatewayHost -Port $selectedPort)

    if (-not $portFree) {
        $existingProc = Get-TcpListenerProcess -Port $selectedPort
        if ($existingProc) {
            Write-WarnMsg "Port $selectedPort in use by PID $($existingProc.Id) ($($existingProc.ProcessName))"
            if ($ForceKillExistingPortProcess) {
                Write-WarnMsg "Killing occupant because -ForceKillExistingPortProcess is set."
                Stop-ProcessSafe -Proc $existingProc
                Start-Sleep -Milliseconds 1000
                $portFree = -not (Test-TcpListen -ListenHost $GatewayHost -Port $selectedPort)
            }
        }
    }

    if (-not $portFree) {
        $fallbackPort = 7078
        if ($fallbackPort -ne $GatewayPort) {
            $fallbackFree = -not (Test-TcpListen -ListenHost $GatewayHost -Port $fallbackPort)
            if ($fallbackFree) {
                Write-WarnMsg "Falling back to port $fallbackPort"
                $selectedPort = $fallbackPort
                $portFree = $true
            } else {
                Write-WarnMsg "Fallback port $fallbackPort also in use. Will try to reuse existing listener."
            }
        }
    }

    if ($portFree) {
        $gatewayEnv = @{
            "CODE_ROOT"       = $CodeRoot
            "OLLAMA_BASE_URL" = $OllamaBaseUrl
        }

        if ($HealthApiKey) {
            $gatewayEnv["API_KEY"] = $HealthApiKey
        }

        $gatewayProc = Start-LoggedProcess `
            -FilePath $PythonExe `
            -ArgumentList @("-m","uvicorn",$GatewayModule,"--host",$GatewayHost,"--port",$selectedPort) `
            -WorkingDirectory $ProjectDir `
            -StdOutPath $GatewayOutLog `
            -StdErrPath $GatewayErrLog `
            -Env $gatewayEnv `
            -Title "FastAPI gateway"
        $script:started.Add($gatewayProc) | Out-Null
        Write-Ok "Gateway started (PID $($gatewayProc.Id)) on port $selectedPort"
    } else {
        Write-Ok "Gateway port $GatewayHost`:$selectedPort already listening. Reusing existing process."
    }

    $healthUrl = "http://$GatewayHost`:$selectedPort/health"
    Write-Info "Waiting for local health: $healthUrl"

    $health = Wait-ForHealth `
        -Url $healthUrl `
        -TimeoutSeconds $HealthTimeoutSeconds `
        -PollMs $HealthPollMs `
        -ApiKey $HealthApiKey `
        -PublicProbe:$PublicHealth

    Write-Ok "Local health OK"
    try {
        $status = $health.status
        $gv = $health.gatewayVersion
        $ollReach = $health.ollama.reachable
        Write-Info "health.status=$status  gatewayVersion=$gv  ollama.reachable=$ollReach"
    } catch {
        Write-WarnMsg "Health response parsed but missing expected fields."
    }

    $zrokUrl = $null
    if (-not $SkipZrok) {
        $zrokProc = Start-LoggedProcess `
            -FilePath "zrok" `
            -ArgumentList @("share","public","$GatewayHost`:$selectedPort") `
            -WorkingDirectory $ProjectDir `
            -StdOutPath $ZrokOutLog `
            -StdErrPath $ZrokErrLog `
            -Title "zrok tunnel"
        $script:started.Add($zrokProc) | Out-Null
        Write-Ok "zrok started (PID $($zrokProc.Id))"

        $zrokDeadline = (Get-Date).AddSeconds(25)
        while ((Get-Date) -lt $zrokDeadline) {
            $zrokUrl = Get-ZrokPublicUrlFromLogs -StdOutPath $ZrokOutLog -StdErrPath $ZrokErrLog
            if ($zrokUrl) { break }
            try {
                $zrokProc.Refresh()
                if ($zrokProc.HasExited) { break }
            } catch {}
            Start-Sleep -Milliseconds 500
        }

        if ($zrokUrl) {
            Write-Ok "zrok URL: $zrokUrl"
            Write-Host ""
            Write-Host "Use this in your OpenAPI servers.url:" -ForegroundColor Green
            Write-Host "  $zrokUrl" -ForegroundColor White
        } else {
            Write-WarnMsg "Could not detect zrok public URL yet. Check log tail below."
            Dump-LogTail -Path $ZrokOutLog -Tail 60 -Label "zrok.out.log"
            Dump-LogTail -Path $ZrokErrLog -Tail 60 -Label "zrok.err.log"
        }
    } else {
        Write-WarnMsg "Skipping zrok because -SkipZrok was set."
    }

    Write-Section "Stack running"
    Write-Host "Press Ctrl+C or Ctrl+E to stop and exit." -ForegroundColor Yellow
    Write-Host "Logs: $LogDir" -ForegroundColor DarkGray
    if ($zrokUrl) { Write-Host "zrok: $zrokUrl" -ForegroundColor Cyan }
    Write-Host "Hotkeys: L = tail logs, U = reprint zrok URL" -ForegroundColor DarkGray
    Write-Host ""

    $originalTreat = [Console]::TreatControlCAsInput
    [Console]::TreatControlCAsInput = $true
    try {
        while ($true) {
            $alive = [System.Collections.Generic.List[System.Diagnostics.Process]]::new()
            foreach ($p in @($script:started.ToArray())) {
                try {
                    $p.Refresh()
                    if ($p.HasExited) {
                        if ($p.Id -ne 0) {
                            Write-WarnMsg "Process exited: PID $($p.Id) ($($p.ProcessName)) ExitCode=$($p.ExitCode)"
                        }
                    } else {
                        $alive.Add($p) | Out-Null
                    }
                } catch {}
            }
            $script:started = $alive

            if ([Console]::KeyAvailable) {
                $key = [Console]::ReadKey($true)
                $isCtrl = ($key.Modifiers -band [ConsoleModifiers]::Control) -ne 0

                if ($isCtrl -and ($key.Key -eq [ConsoleKey]::C -or $key.Key -eq [ConsoleKey]::E)) {
                    Write-Host ""
                    Write-WarnMsg "Stop hotkey detected ($($key.Key))."
                    break
                }

                if ($key.Key -eq [ConsoleKey]::L) {
                    Write-Host ""
                    Dump-LogTail -Path $GatewayErrLog -Tail 40 -Label "gateway.err.log"
                    Dump-LogTail -Path $GatewayOutLog -Tail 40 -Label "gateway.out.log"
                    Dump-LogTail -Path $ZrokErrLog -Tail 40 -Label "zrok.err.log"
                    Dump-LogTail -Path $ZrokOutLog -Tail 40 -Label "zrok.out.log"
                    Write-Host ""
                    Write-Host "Press Ctrl+C or Ctrl+E to stop." -ForegroundColor Yellow
                }

                if ($key.Key -eq [ConsoleKey]::U) {
                    $tmpUrl = Get-ZrokPublicUrlFromLogs -StdOutPath $ZrokOutLog -StdErrPath $ZrokErrLog
                    if ($tmpUrl) { Write-Host "zrok URL: $tmpUrl" -ForegroundColor Cyan }
                }
            }

            Start-Sleep -Milliseconds 200
        }
    }
    finally {
        [Console]::TreatControlCAsInput = $originalTreat
    }

    Cleanup-All
}
catch {
    Write-Host ""
    Write-ErrMsg $_.Exception.Message

    Write-Host ""
    Dump-LogTail -Path $GatewayErrLog -Tail 120 -Label "gateway.err.log"
    Dump-LogTail -Path $GatewayOutLog -Tail 120 -Label "gateway.out.log"
    Dump-LogTail -Path $ZrokErrLog -Tail 120 -Label "zrok.err.log"
    Dump-LogTail -Path $ZrokOutLog -Tail 120 -Label "zrok.out.log"
    Dump-LogTail -Path $OllamaErrLog -Tail 80 -Label "ollama.err.log"
    Dump-LogTail -Path $OllamaOutLog -Tail 80 -Label "ollama.out.log"

    Cleanup-All

    if ($KeepLauncherOpen) {
        Write-Host ""
        Write-Host "Press any key to close..." -ForegroundColor Yellow
        [void][Console]::ReadKey($true)
    }

    exit 1
}
finally {
    if ($KeepLauncherOpen -and -not $script:cleanedUp) {
        Write-Host ""
        Write-Host "Press any key to close..." -ForegroundColor Yellow
        [void][Console]::ReadKey($true)
    }
}
