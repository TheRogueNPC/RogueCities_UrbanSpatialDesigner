Set-Location 'D:\Projects\RogueCities\RogueCities_UrbanSpatialDesigner'
. .\tools\dev-shell.ps1
rc-env
rc-cfg -Preset dev
rc-bld -Preset gui-release
rc-tst -Preset ctest-release
