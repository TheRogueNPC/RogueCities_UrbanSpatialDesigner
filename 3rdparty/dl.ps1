[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
Write-Host "Downloading Clipper2 zip..."
Invoke-WebRequest -Uri "https://github.com/AngusJohnson/Clipper2/archive/refs/heads/master.zip" -OutFile "clipper.zip"
Write-Host "Extracting..."
Expand-Archive -Path "clipper.zip" -DestinationPath "extract" -Force
Write-Host "Moving..."
Move-Item -Path "extract\Clipper2-master" -Destination "Clipper2" -Force
Write-Host "Cleaning up..."
Remove-Item -Recurse -Force "extract"
Remove-Item "clipper.zip"
Remove-Item -Recurse -Force "Clipper2\.github" -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force "Clipper2\CPP\Tests" -ErrorAction SilentlyContinue
Write-Host "Done!"
