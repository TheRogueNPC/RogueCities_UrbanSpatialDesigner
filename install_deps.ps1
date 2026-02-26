Set-Location "d:\Projects\RogueCities\RogueCities_UrbanSpatialDesigner\3rdparty"

# Install Clipper2
git clone https://github.com/AngusJohnson/Clipper2.git clipper2

# Clean up noise from all 3rdparty folders
Get-ChildItem -Directory | ForEach-Object {
    $path = $_.FullName
    Remove-Item -Recurse -Force "$path\.git" -ErrorAction SilentlyContinue
    Remove-Item -Recurse -Force "$path\.github" -ErrorAction SilentlyContinue
    Remove-Item -Recurse -Force "$path\test" -ErrorAction SilentlyContinue
    Remove-Item -Recurse -Force "$path\tests" -ErrorAction SilentlyContinue
}

# Clean Clipper2 specifics
Remove-Item -Recurse -Force "clipper2\CPP\Tests" -ErrorAction SilentlyContinue

Write-Output "Dependencies installed and cleaned up."
