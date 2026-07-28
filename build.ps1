# build.ps1
param(
    [ValidateSet("all", "build", "run")]
    [string]$Mode = "all"
)

$ErrorActionPreference = "Stop"

$root = $PSScriptRoot
$buildDir = Join-Path $root "build"
$shaderSrcDir = Join-Path $root "shaders"
$shaderOutDir = Join-Path $buildDir "shaders"

# Prefer the SDK env var, fall back to the known install path
$glslc = if ($env:VULKAN_SDK) {
    Join-Path $env:VULKAN_SDK "Bin/glslc.exe"
}
else {
    "C:/VulkanSDK/1.4.350.0/Bin/glslc.exe"
}

Push-Location $root

try {
    if ($Mode -ne "run") {
        if (Test-Path $buildDir) {
            Remove-Item $buildDir -Recurse -Force
        }
        New-Item -ItemType Directory -Path $shaderOutDir -Force | Out-Null

        # Compile every .vert/.frag straight into build/shaders
        Get-ChildItem $shaderSrcDir -Include "*.vert", "*.frag" -Recurse | ForEach-Object {
            $out = Join-Path $shaderOutDir "$($_.Name).spv"
            & $glslc $_.FullName -o $out
            if ($LASTEXITCODE -ne 0) { throw "Shader compile failed: $($_.Name)" }
            Write-Host "Compiled $($_.Name)"
        }

        Set-Location $buildDir

        cmake -G "MinGW Makefiles" -DCMAKE_C_COMPILER="C:/msys64/mingw64/bin/gcc.exe" ..
        if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

        mingw32-make
        if ($LASTEXITCODE -ne 0) { throw "Build failed" }
    }

    if ($Mode -ne "build") {
        if (-not (Test-Path $buildDir)) {
            throw "No build directory, run with -Mode build first"
        }

        Set-Location $buildDir

        $exe = Get-ChildItem -Filter "*.exe" | Select-Object -First 1
        if (-not $exe) { throw "No executable found in build directory" }

        & $exe.FullName
    }
}
finally {
    Pop-Location
}