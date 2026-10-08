#pragma once

// Renderer.h — static facade: engine POLICY (Lesson 7).
// The rest of the engine talks to Renderer, never to a backend class.
// Facade = WHAT the engine wants ("begin the frame"); RendererAPI = HOW a
// specific GPU does it. Merging them would force every backend author to
// re-derive frame-order decisions.

#include "Pink/Core/Core.h"
#include "Pink/Renderer/RendererAPI.h"

namespace Pink {

class Shader;      // Lesson 8
class VertexArray; // Lesson 9

class PINK_API Renderer
{
public:
    static void Init();    // Create() the backend, call its Init()
    static void Shutdown();

    static void BeginFrame(const Vec4& clearColor); // SetClearColor + Clear
    static void OnWindowResize(uint32_t width, uint32_t height);

    // Lesson 9: the one call game code makes per drawable — bind the
    // program, bind the geometry description, issue the draw.
    static void Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray);

    static RendererAPIType GetAPI() { return RendererAPI::GetAPI(); }

private:
    static Scope<RendererAPI> s_RendererAPI;
};

}
