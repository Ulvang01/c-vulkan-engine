# c-vulkan-engine

`! AUTO GENERATED !`

A Vulkan renderer built from scratch in pure C, no C++, no engine framework.
Goal: understand and own every layer of the graphics pipeline, not just call
a library that hides it.

## Requirements

- Windows 10/11
- [MSYS2](https://www.msys2.org/) with the mingw-w64 toolchain
  (`pacman -S mingw-w64-x86_64-toolchain`)
- [CMake](https://cmake.org/)
- [Vulkan SDK](https://vulkan.lunarg.com/) (LunarG)
- GLFW (vendored in `external/glfw`, see below)

## Building

```powershell
.\build.ps1
```

This wipes and regenerates the `build/` folder from scratch every run
(full reconfigure, not incremental), then compiles and runs the resulting
executable automatically.

### Toolchain setup (one-time)

Add `C:\msys64\mingw64\bin` to your PATH so `gcc`, `mingw32-make`, etc. are
available from PowerShell directly. If PowerShell blocks the build script:

```powershell
Set-ExecutionPolicy -Scope CurrentUser RemoteSigned
```

## Project structure
- src/ all source files (GLOB'd automatically by CMake)
- external/glfw/ vendored GLFW source, built via its own CMake project
- shaders/ GLSL shader source (compiled to SPIR-V, coming soon)
- build/ generated, not committed

Adding a new `.c` file to `src/` requires no CMake edits, `file(GLOB ...)`
picks it up on the next full reconfigure (which `build.ps1` always does).

## Logging

A leveled logger (`src/log.h`/`log.c`) replaces raw `printf` calls.

Levels: `DEBUG < INFO < WARN < ERROR`. Default minimum level is `INFO`.

```c
LOG_INFO("Vulkan instance created OK");
LOG_ERROR("vkCreateInstance failed: %d", result);
```

Each line auto-includes file and line number, and is color-coded by level
in supported terminals (Windows Terminal, PowerShell 7+).

**Enable debug-level logs** without recompiling:

```powershell
$env:LOG_LEVEL = "debug"
.\build.ps1
```

**Disable again:**

```powershell
Remove-Item Env:LOG_LEVEL
```

## Progress

- [x] Vulkan instance creation
- [x] Physical device selection (scored: discrete GPU + VRAM, hard-gated
      on graphics queue family support)
- [x] Queue family detection (graphics)
- [ ] Logical device creation
- [ ] Window surface + presentation queue support
- [ ] Swapchain
- [ ] Render pass + graphics pipeline
- [ ] First triangle