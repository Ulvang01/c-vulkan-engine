# build.ps1
$ErrorActionPreference = "Stop"

$root = $PSScriptRoot
$buildDir = Join-Path $root "build"

Push-Location $root

try {
    if (Test-Path $buildDir) {
        Remove-Item $buildDir -Recurse -Force
    }
    New-Item -ItemType Directory -Path $buildDir | Out-Null

    Set-Location $buildDir

    cmake -G "MinGW Makefiles" -DCMAKE_C_COMPILER="C:/msys64/mingw64/bin/gcc.exe" ..
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

    mingw32-make
    if ($LASTEXITCODE -ne 0) { throw "Build failed" }

    $exe = Get-ChildItem -Filter "*.exe" | Select-Object -First 1
    & $exe.FullName
}
finally {
    Pop-Location
}