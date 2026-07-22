# build.ps1
$ErrorActionPreference = "Stop"

$root = $PSScriptRoot
$buildDir = Join-Path $root "build"

if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

Set-Location $buildDir

cmake -G "MinGW Makefiles" -DCMAKE_C_COMPILER="C:/msys64/mingw64/bin/gcc.exe" ..
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

mingw32-make
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$exe = Get-ChildItem -Filter "*.exe" | Select-Object -First 1
& $exe.FullName