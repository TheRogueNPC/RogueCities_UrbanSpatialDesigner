$errors = $null
$null = [System.Management.Automation.Language.Parser]::ParseFile(
    'C:\Users\teamc\Documents\Rogue Cities\RogueCities_UrbanSpatialDesigner\tools\dev-shell.ps1',
    [ref]$null, [ref]$errors)
if ($errors.Count -eq 0) {
    Write-Host 'OK - no parse errors'
} else {
    $errors | ForEach-Object { Write-Host "Line $($_.Extent.StartLineNumber): $($_.Message)" }
}
