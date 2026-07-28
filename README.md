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
- GLFW (vendored in `external/glfw`, built from source)

## Building

```powershell
.\build.ps1              # build + run (default)
.\build.ps1 -Mode build  # compile only, don't launch
.\build.ps1 -Mode run    # launch existing exe, no rebuild
```

The script compiles all shaders, then wipes and regenerates the `build/`
folder from scratch (full reconfigure, not incremental), then compiles and
runs the resulting executable.

### Toolchain setup (one-time)

Add `C:\msys64\mingw64\bin` to your PATH so `gcc`, `mingw32-make`, etc. are
available from PowerShell directly. If PowerShell blocks the build script:

```powershell
Set-ExecutionPolicy -Scope CurrentUser RemoteSigned
```

## Project structure
- src/
  - main.c entry point, window loop
  - core/ non-Vulkan infrastructure (logging, windowing, utils)
  - vulkan/ everything Vulkan, one file per concern
- shaders/ GLSL source
- external/glfw/ vendored GLFW, built via its own CMake project
- build/ generated, not committed

`src/` is an include root, so headers are referenced from there
(`#include "core/log.h"`) rather than with relative `../` paths.

Adding a new `.c` file requires no CMake edits, `file(GLOB_RECURSE ...)`
picks it up on the next full reconfigure (which `build.ps1` always does).

### Vulkan module layout

| File | Responsibility |
|---|---|
| `vulkan_engine.c` | Instance creation, init/cleanup orchestration |
| `debug.c` | Validation layers, debug messenger |
| `surface.c` | Window surface |
| `device.c` | Physical device selection and scoring, logical device |
| `queue_families.c` | Queue family discovery (graphics + present) |
| `swapchain.c` | Swapchain, image views, framebuffers |
| `render_pass.c` | Render pass |
| `shader.c` | SPIR-V loading, shader modules |
| `pipeline.c` | Graphics pipeline |
| `commands.c` | Command pool, command buffer, command recording |
| `renderer.c` | Sync objects, per-frame draw loop |

Objects are grouped by lifetime, not just by topic. Image views and
framebuffers live in `swapchain.c` because they are destroyed and recreated
together with the swapchain.

## Shaders

GLSL sources live in `shaders/`. `build.ps1` compiles every `.vert` and
`.frag` with `glslc` into `build/shaders/*.spv` before building, so the
executable loads them via plain relative paths at runtime.

Note this is Vulkan GLSL, not OpenGL GLSL. `gl_VertexIndex`, not
`gl_VertexID`. Editor linters configured for OpenGL will flag valid code.

## Logging

A leveled logger (`src/core/log.h`) replaces raw `printf` calls.

Levels: `DEBUG < INFO < WARN < ERROR`. Default minimum level is `INFO`.

```c
LOG_INFO("Vulkan instance created OK");
LOG_ERROR("vkCreateInstance failed: %d", result);
```

Each line auto-includes file and line number, and is color-coded by level
in supported terminals (Windows Terminal, PowerShell 7+). Vulkan validation
messages are routed through it, mapped to the matching level by severity.

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

- [x] Vulkan instance
- [x] Validation layers + debug messenger routed through the logger
- [x] Window surface
- [x] Physical device selection (scored on discrete GPU and VRAM,
      hard-gated on queue families, swapchain extension, and swapchain
      adequacy for the surface)
- [x] Queue family detection (graphics + present, handles the case where
      they are different families)
- [x] Logical device + queues
- [x] Swapchain (sRGB format preferred, mailbox present mode preferred,
      FIFO fallback)
- [x] Image views
- [x] Render pass
- [x] Shader loading (GLSL to SPIR-V)
- [x] Graphics pipeline (viewport and scissor as dynamic state)
- [x] Framebuffers
- [x] Command pool + command buffer
- [x] Sync primitives + draw loop
- [x] First triangle

## Next steps

### Window resize handling

Currently the return values of `vkAcquireNextImageKHR` and
`vkQueuePresentKHR` are ignored, so resizing the window leaves the swapchain
stale. Needed:

- Split `vulkanEngineCleanup` so the swapchain-dependent portion
  (framebuffers, image views, swapchain, per-image semaphores) can be reused
- A `recreateSwapchain` that waits idle, tears that down, and rebuilds it.
  The render pass and pipeline survive untouched, which is what dynamic
  viewport and scissor state bought
- Handle `VK_ERROR_OUT_OF_DATE_KHR` and `VK_SUBOPTIMAL_KHR`, plus a GLFW
  resize callback as a supplement, since some drivers do not report
  out-of-date promptly
- Block while minimized, a 0x0 framebuffer is an invalid swapchain extent

### Frames in flight

One command buffer and one fence means the CPU waits for the GPU every
frame. Duplicating the per-frame objects (typically 2) lets the CPU record
frame N+1 while the GPU works on frame N.

### Vertex buffers

Triangle positions are currently hardcoded in the vertex shader to avoid
GPU memory management. Replacing that means `VkBuffer` creation, querying
memory requirements, picking a memory type from the device heaps, and a
staging buffer with a transfer command. This also fills in the currently
empty vertex input state in the pipeline.

### Beyond that

- Index buffers
- Uniform buffers and descriptor sets, needed for a model/view/projection
  matrix and therefore for anything actually 3D
- Own vector and matrix math (no GLM)
- Depth buffer, required before overlapping geometry renders correctly
- Texture loading and samplers
- Refactor to Vulkan 1.3 dynamic rendering, which removes render pass and
  framebuffer objects entirely. No performance gain on desktop, but less
  machinery to maintain
- Pipeline cache, to cut startup cost once there are several pipelines