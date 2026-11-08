# PinkMountain Engine

A modern 3D game engine in **C++20**, built lesson-by-lesson as a learning project.
Every system is written from scratch and commented with the *why* behind each design
decision — the goal is understanding, not just a working renderer.

> Status: Lessons 1–14 complete. The engine opens a window, compiles GLSL shaders
> with hot-reload, and renders three lit cubes on a shadowed floor in HDR —
> with bloom, exposure, and filmic tone mapping — through an API-agnostic
> renderer seam (OpenGL backend): cameras (Lesson 11), forward lighting with
> Blinn-Phong and PBR Cook-Torrance (Lesson 12), shadow mapping (Lesson 13),
> and post-processing (Lesson 14).

## What's inside (by lesson)

| Lesson | System | Key files |
|--------|--------|-----------|
| 1 | Project setup, logging, asserts |  `Pink/Core/Log.h`, `Core/Assert.h`, CMake + FetchContent |
| 2 | Main loop, fixed timestep (1/60 accumulator), FPS tracking |  `Pink/Core/Application.h`, `Core/Timer.h`, `Core/EntryPoint.h` |
| 3 | Subsystems + type-indexed service locator |  `Pink/Core/SubSystem.h`, `Core/ServiceLocator.h` |
| 4 | Memory: linear / stack / pool allocators |  `Pink/Memory/Allocator.h`, `Memory/MemoryService.h` |
| 5 | Math: Vec/Mat/Quat from scratch (OpenGL conventions) |  `Pink/src/Pink/Math/PinkMath.h` (header-only) |
| 6 | Platform: GLFW window, input polling, resize callbacks |  `Pink/Platform/Window.h`, `Platform/Input.h`, `Platform/GLFW/` |
| 7 | Renderer abstraction + OpenGL backend (vendored glad loader) | `Pink/Renderer/RendererAPI.h`, `Renderer/Renderer.h`, `Renderer/OpenGL/`, `Pink/vendor/glad/` |
| 8 | Shaders: GLSL compile/link, uniforms, hot-reload | `Pink/Renderer/Shader.h`, `Renderer/OpenGL/OpenGLShader.*`, `Sandbox/assets/shaders/` |
| 9 | Buffers & meshes: VBO/VAO/EBO, vertex layouts, first triangle | `Pink/Renderer/Buffer.h`, `Renderer/VertexArray.h`, `Renderer/OpenGL/OpenGL*.cpp`, `Renderer::Submit` |
| 10 | Textures: stb_image loading, samplers, mipmaps | `Pink/Renderer/Texture.h`, `Renderer/OpenGL/OpenGLTexture2D.*`, `Pink/vendor/stb/` |
| 11 | Cameras: perspective/orthographic, orbit controller, resize handling | `Pink/Renderer/Camera.h`, `Renderer/Camera.cpp`, `SandboxApp` orbit + scroll zoom |
| 12 | Lighting: forward rendering, Blinn-Phong → PBR Cook-Torrance (GGX/Schlick) | `Pink/Renderer/Light.h`, `Material.h`, `LightManager.h`, `Sandbox/assets/shaders/Lit_Phong.glsl`, `Lit_PBR.glsl` |
| 13 | Shadows: shadow mapping (directional + point cubemap), PCF, slope-scaled bias | `Pink/Renderer/Framebuffer.h`, `Renderer/ShadowMap.h`, `Renderer/OpenGL/OpenGLFramebuffer.*`, `Sandbox/assets/shaders/Depth.glsl`, `DepthCube.glsl` |
| 14 | Post-processing: HDR (RGBA16F), bloom (separable Gaussian ping-pong), tone mapping (Reinhard/Exponential/ACES), gamma | `Pink/Renderer/PostProcess.h`, `Renderer/Framebuffer.h` (multi-attachment spec), `Sandbox/assets/shaders/Blur.glsl`, `Composite.glsl` |

## Prerequisites

- CMake 3.25+
- A C++20 compiler (MSVC 2022, GCC 12+, or Clang 15+)
- Git (CMake FetchContent clones spdlog and GLFW on first configure)
- Linux: X11 dev packages, e.g. `sudo apt install xorg-dev libglu1-mesa-dev`

Dependencies are fetched automatically — no submodules, no manual installs:
[spdlog](https://github.com/gabime/spdlog) (MIT) · [GLFW](https://github.com/glfw/glfw) (zlib).
The OpenGL loader ([glad](https://github.com/Dav1dde/glad), generated for
gl:core=3.3, license `SPDX-License-Identifier: (WTFPL OR CC0-1.0) AND Apache-2.0`
as stated in its sources) is vendored under `Pink/vendor/glad/` — no Python or
extra downloads needed. Image loading uses [stb_image](https://github.com/nothings/stb)
(public domain / MIT), vendored under `Pink/vendor/stb/`. Their licenses ship
with their sources; this project just links them.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
./build/bin/Sandbox        # Linux/macOS
# build\bin\Debug\Sandbox.exe  (Windows, multi-config generators)
```

Run the Sandbox: a 1280×720 window renders three lit cubes (red plastic,
brushed steel, gold) on a floor, with directional and point-light shadows.
The console logs the active renderer backend plus the GPU vendor/renderer/
version string.
**Esc** quits; drag with the mouse to orbit, scroll to zoom (Lesson 11).
**1** selects Blinn-Phong, **2** selects PBR (Lesson 12); **V** toggles
shadows (Lesson 13); **B** toggles bloom, **T** cycles the tone-map operator
(Reinhard → Exponential → ACES), **[** and **]** step exposure ∓0.25 EV
(Lesson 14). Edit `Sandbox/assets/shaders/*.glsl`
while the app runs — shaders hot-reload without restarting (Lesson 8).

## Project structure

```
Pink/                   # The engine: a static library, namespace Pink::
  src/Pink/
    Core/                # Application, EntryPoint, Log, Assert, Timer,
                         # SubSystem, ServiceLocator
    Memory/              # LinearAllocator, StackAllocator, PoolAllocator,
                         # MemoryService
    Math/                # Vec2/3/4, Mat4, Quat, Transform (header-only,
                         # column-major, right-handed — GLM conventions)
    Platform/            # Window (abstract), KeyCodes, Input,
                         # GLFW/ (the only place GLFW appears)
    Renderer/            # Renderer (static facade), RendererAPI (virtual seam),
                         # Shader, Buffer/VertexArray, Texture (abstractions),
                         # Camera, Light, Material, LightManager, Framebuffer,
                         # ShadowMap, OpenGL/ (the only place glad/GL headers appear)
Sandbox/                 # Demo app: links the engine, owns main() via EntryPoint
  assets/shaders/        # GLSL sources, copied next to the binary at build time
```

**The one architectural rule:** no OpenGL header may appear above the renderer
seam. `glad/gl.h` is included only in `Renderer/OpenGL/*.cpp` files — never in
headers. The day that rule breaks, the "we'll support Vulkan later" plan dies.

## Naming

The engine lives in namespace `Pink::` (short for the Pink Mountain brand —
deliberately terse for live-coding and lecture code). If you ever want it
shorter still, or prefer a different shorthand in your own project, C++
namespace aliases cost nothing:

```cpp
namespace PM = Pink; // your files, your choice — the engine doesn't care
```

## Roadmap

- Lesson 15: Asset pipeline — virtual file system, asset handles/IDs, async
  loading, hot reload (and real OBJ mesh loading — no more procedural cubes)
- Then: ECS scene, ImGui editor, physics, audio, Lua scripting, animation,
  particles, job system, Tracy profiling, capstone demo scene.

## Acknowledgments

Design heavily inspired by TheCherno's
[Hazel](https://github.com/TheCherno/Hazel) engine series (Apache-2.0) —
the subsystem, service-locator, and renderer-abstraction shapes follow Hazel's
proven layout, reimplemented here for learning.

## License

MIT — see [LICENSE](LICENSE). Do anything you want with it; a credit line is
appreciated but not required.
