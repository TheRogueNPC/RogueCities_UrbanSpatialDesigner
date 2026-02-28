param(
  [int]$Port = 7222,
  [int]$LatencyGateP95Ms = 8000,
  [switch]$KeepBridge
)
$ErrorActionPreference = 'Stop'

$repo = (Resolve-Path '.').Path
. "$repo/tools/dev-shell.ps1"

$results = [ordered]@{}
$results.timestamp = (Get-Date).ToString('o')
$results.port = $Port
$results.latency_gate_p95_ms = $LatencyGateP95Ms
$results.ollama_base_url = (Get-RCOllamaBaseUrl -ProbeReachable)

try {
  Set-RCAiCompatibilityEnv -Port $Port
  $results.ai_harden = (rc-ai-harden -Port $Port | ConvertFrom-Json)

  rc-ai-restart -Port $Port -BindAll | Out-Null
  Start-Sleep -Seconds 2

  $results.mcp_smoke = (rc-mcp-smoke -Port $Port | ConvertFrom-Json)

  $py = Resolve-RCPythonLauncher
  $mcpArgs = @($py.argsPrefix + @(
    "$repo/tools/mcp-server/roguecity-mcp/main.py",
    "--self-test",
    "--toolserver-url",
    "http://127.0.0.1:$Port"
  ))
  $mcpSelfTestJson = & $py.exe @mcpArgs
  $results.mcp_self_test = ($mcpSelfTestJson | ConvertFrom-Json)

  rc-smoke-headless -TimeoutSec 6 | Out-Null
  $results.headless_capture = [ordered]@{}
  $snapshotPath = "$repo/AI/docs/ui/ui_introspection_headless_latest.json"
  $uiShot = "$repo/AI/docs/ui/ui_screenshot_latest.png"
  $headlessExe = "$repo/bin/RogueCityVisualizerHeadless.exe"
  $headlessOut = & $headlessExe --frames 3 --export-ui-snapshot "$snapshotPath" --export-ui-screenshot "$uiShot" 2>&1
  $results.headless_capture.ok = ((Test-Path -LiteralPath $snapshotPath) -and (Test-Path -LiteralPath $uiShot))
  $results.headless_capture.snapshot_path = $snapshotPath
  $results.headless_capture.screenshot_path = $uiShot
  $results.headless_capture.output = ($headlessOut | Out-String).Trim()

  $results.perception_observe = (rc-perceive-ui -Mode full -Frames 3 -Screenshot -IncludeVision $true -IncludeOcr $true -StrictContract $true -VisionTimeoutSec 35 -OcrTimeoutSec 35 | ConvertFrom-Json)
  $results.perception_audit = (rc-perception-audit -Observations 3 -IncludeReports -Mode full -Frames 3 -Screenshot -IncludeVision $false -IncludeOcr $false -StrictContract $true -LatencyGateP95Ms $LatencyGateP95Ms | ConvertFrom-Json)

  $pipePayload = [ordered]@{
    prompt = 'Find the files that implement perception observe and MCP self-test behavior.'
    mode = 'audit'
    required_paths = @('tools/toolserver.py','tools/mcp-server/roguecity-mcp/main.py')
    context_files = @('tools/toolserver.py','tools/mcp-server/roguecity-mcp/main.py')
    search_hints = @('perception/observe','self-test')
    include_repo_index = $false
  }
  $pipeBody = $pipePayload | ConvertTo-Json -Depth 10
  $results.pipeline_query = Invoke-RestMethod -Uri "http://127.0.0.1:$Port/pipeline/query" -Method Post -ContentType 'application/json' -Body $pipeBody -TimeoutSec 180

  $aiQueryJson = rc-ai-query -Prompt 'Return JSON with answer and paths_used for where perception observe and MCP self-test are implemented.' -Model 'gemma3:4b' -File @('tools/toolserver.py','tools/mcp-server/roguecity-mcp/main.py') -RequiredPaths @('tools/toolserver.py','tools/mcp-server/roguecity-mcp/main.py') -Audit -MaxRetries 1 -Temperature 0.10 -TopP 0.85 -NumPredict 192
  $results.ai_query = ($aiQueryJson | ConvertFrom-Json)

  $runtimeSections = @{}
  foreach ($s in @($results.perception_observe.runtime_sections)) {
    if ($s -and $s.name) {
      $runtimeSections[[string]$s.name] = [bool]$s.present
    }
  }
  $visualSources = @($results.perception_observe.visual_evidence | ForEach-Object { [string]$_.source })
  $hasVision = [bool]($visualSources | Where-Object { $_ -match 'granite3\.2-vision' } | Select-Object -First 1)
  $hasOcr = [bool]($visualSources | Where-Object { $_ -match 'glm-ocr' } | Select-Object -First 1)

  $results.actionable = [ordered]@{
    bridge_health_ok = [bool]$results.mcp_smoke.bridge_health_ok
    mcp_ok_core = [bool]$results.mcp_smoke.ok_core
    mcp_ok_full = [bool]$results.mcp_smoke.ok_full
    runtime_recommendation = [string]$results.mcp_smoke.runtime_recommendation
    pipeline_answer_present = -not [string]::IsNullOrWhiteSpace([string]$results.pipeline_query.answer)
    pipeline_answer_quality_ok = [bool]$results.pipeline_query.verifier.answer_quality_ok
    pipeline_invalid_paths = @($results.pipeline_query.verifier.invalid_paths).Count
    pipeline_missing_required = @($results.pipeline_query.verifier.missing_required_paths).Count
    section_master_present = [bool]$runtimeSections['master']
    section_viewport_present = [bool]$runtimeSections['viewport']
    section_status_present = [bool]$runtimeSections['status']
    section_titlebar_present = [bool]$runtimeSections['titlebar']
    perception_p95_ms = [double]$results.perception_audit.metrics.latency_ms.p95
    perception_gate_pass = [bool]$results.perception_audit.gate_pass
    perception_errors = @($results.perception_observe.errors)
    perception_warnings = @($results.perception_observe.warnings)
    runtime_screenshot_exists = (Test-Path -LiteralPath $uiShot)
    visual_evidence_count = @($results.perception_observe.visual_evidence).Count
    visual_has_vision = $hasVision
    visual_has_ocr = $hasOcr
  }

  $gateChecks = [ordered]@{
    powershell_core_ready = [bool]$results.actionable.mcp_ok_core
    pipeline_answer_quality = ([bool]$results.actionable.pipeline_answer_present -and [bool]$results.actionable.pipeline_answer_quality_ok -and [int]$results.actionable.pipeline_invalid_paths -eq 0 -and [int]$results.actionable.pipeline_missing_required -eq 0)
    contract_sections_present = ([bool]$results.actionable.section_viewport_present -and [bool]$results.actionable.section_status_present -and [bool]$results.actionable.section_titlebar_present)
    perception_latency_gate = ([bool]$results.actionable.perception_gate_pass -and ([double]$results.actionable.perception_p95_ms -le [double]$LatencyGateP95Ms))
    runtime_screenshot_and_multimodal = (([bool]$results.actionable.runtime_screenshot_exists) -and ([bool]$results.actionable.visual_has_vision) -and ([bool]$results.actionable.visual_has_ocr))
  }
  $results.gates = $gateChecks
  $results.gates_pass = -not ($gateChecks.Values -contains $false)

  $stamp = Get-Date -Format "yyyy-MM-dd_HHmmss"
  $outPath = "$repo/tools/.run/full_smoke_report_$stamp.json"
  $results | ConvertTo-Json -Depth 40 | Set-Content -LiteralPath $outPath -Encoding UTF8
  Write-Output "REPORT_PATH=$outPath"
  Write-Output ($results.actionable | ConvertTo-Json -Depth 12)
  Write-Output ($results.gates | ConvertTo-Json -Depth 8)

  if (-not $results.gates_pass) {
    throw "Commit gate failed. See report: $outPath"
  }
}
finally {
  if (-not $KeepBridge) {
    try { rc-ai-stop -Port $Port | Out-Null } catch {}
  }
}
