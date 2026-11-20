# Dot-source this helper from PowerShell to enter the project and use available
# local tools. Downloaded compilers and CMake binaries are intentionally not
# included in the GitHub repository.
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$localMingw = Join-Path $PSScriptRoot 'winlibs\mingw64\bin'
$localCmake = Join-Path $PSScriptRoot 'cmake-4.5.1-windows-x86_64\bin'
$workingRoot = $projectRoot
$sourceFile = Join-Path $projectRoot 'course_plan.cpp'
$sourceHash = (Get-FileHash -LiteralPath $sourceFile -Algorithm SHA256).Hash

# A short drive alias helps the local MinGW build handle non-ASCII project paths.
if (Test-Path -LiteralPath (Join-Path $localMingw 'g++.exe')) {
    $driveLetters = @('Z','Y','X','W','V','U','T','S','R','Q','P','O','N','M','L','K','J','I','H','G','F','E')
    foreach ($letter in $driveLetters) {
        $drivePath = "${letter}:\"
        $candidateSource = "${letter}:\course_plan.cpp"
        if (Test-Path -LiteralPath $drivePath) {
            if (-not (Test-Path -LiteralPath $candidateSource)) { continue }
            if ((Get-FileHash -LiteralPath $candidateSource -Algorithm SHA256).Hash -eq $sourceHash) {
                $workingRoot = $drivePath
                break
            }
        } else {
            & subst "${letter}:" $projectRoot 2>$null | Out-Null
            if ($LASTEXITCODE -eq 0) {
                $workingRoot = $drivePath
                break
            }
        }
    }
}

if (Test-Path -LiteralPath (Join-Path $localMingw 'g++.exe')) {
    $workingMingw = $workingRoot.TrimEnd('\') + '\tools\winlibs\mingw64\bin'
    $env:PATH = "$workingMingw;$env:PATH"
    $env:GCC_EXEC_PREFIX = $workingRoot.TrimEnd('\') + '\tools\winlibs\mingw64\lib\gcc\'
}
if (Test-Path -LiteralPath (Join-Path $localCmake 'cmake.exe')) {
    $workingCmake = $workingRoot.TrimEnd('\') + '\tools\cmake-4.5.1-windows-x86_64\bin'
    $env:PATH = "$workingCmake;$env:PATH"
}

Set-Location $workingRoot

$gxx = Get-Command g++ -ErrorAction SilentlyContinue
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
$ninja = Get-Command ninja -ErrorAction SilentlyContinue
if ($gxx) { Write-Host "GCC:   $(& g++ --version | Select-Object -First 1)" }
else { Write-Warning 'g++ was not found. Install a C++17 compiler before building the command-line program.' }
if ($cmake) { Write-Host "CMake: $(& cmake --version | Select-Object -First 1)" }
else { Write-Warning 'CMake was not found. Install CMake 3.21 or newer to use the CMake preset.' }
if ($ninja) { Write-Host "Ninja: $(& ninja --version)" }
else { Write-Warning 'Ninja was not found. Install Ninja to use the default CMake preset.' }
