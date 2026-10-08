# PinkMountain Engine

A modern 3D game engine in **C++20**, built lesson-by-lesson as a learning project.
Every system is written from scratch and commented with the *why* behind each design
decision — the goal is understanding, not just a working renderer.

> Status: Lessons 1–10 complete. The engine opens a window, compiles GLSL shaders
> with hot-reload, and renders a textured quad through an API-agnostic
> renderer seam (OpenGL backend): shaders (Lesson 8), buffers/meshes with
> VBO/VAO/EBO and a first triangle (Lesson 9), textures with stb_image
> loading, samplers and mipmaps (Lesson 10).

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

Run the Sandbox: a 1280×720 window renders a textured quad (a procedural
checkerboard — no image files needed). The console logs the active renderer
backend plus the GPU vendor/renderer/version string.
**Esc** quits; arrow keys and mouse position are logged once per second
(input polling demo from Lesson 6). Edit `Sandbox/assets/shaders/TexturedQuad.glsl`
while the app runs — the shader hot-reloads without restarting (Lesson 8).

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
                         # OpenGL/ (the only place glad/GL headers appear)
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

- Lesson 11: Camera — perspective/orthographic, view/projection, controllers
- Then: lighting/PBR, shadows, ECS scene, post-processing,
  asset pipeline, ImGui editor, physics, audio, Lua scripting, animation,
  particles, job system, Tracy profiling, capstone demo scene.

## Acknowledgments

Design heavily inspired by TheCherno's
[Hazel](https://github.com/TheCherno/Hazel) engine series (Apache-2.0) —
the subsystem, service-locator, and renderer-abstraction shapes follow Hazel's
proven layout, reimplemented here for learning.

## License

MIT — see [LICENSE](LICENSE). Do anything you want with it; a credit line is
appreciated but not required.
