#pragma once

// RendererAPI.h — the renderer seam (Lesson 7).
// The engine speaks RendererAPI; each GPU dialect gets a backend class.
// Compare Hazel's: https://github.com/TheCherno/Hazel/blob/master/Hazel/src/Hazel/Renderer/RendererAPI.h
//
// Rules of the seam:
//  1. The interface is designed from what the ENGINE needs — not from what
//     OpenGL can do. Lowest common denominator, extended as features demand.
//  2. The factory below is the ONLY place that names backend classes.
//  3. No GL/Vulkan/D3D header may appear above this seam. Ever.

#include "Pink/Core/Core.h"
#include "Pink/Core/Assert.h"
#include "Pink/Math/PinkMath.h"

namespace Pink {

enum class RendererAPIType
{
    None = 0,
    OpenGL = 1,
    Vulkan = 2,      // seam reserved; the backend lands in a later lesson arc
    Direct3D12 = 3
};

class VertexArray; // Lesson 9: the draw call needs the VAO, not a raw count

class PINK_API RendererAPI
{
public:
    virtual ~RendererAPI() = default;

    virtual void Init() = 0;
    virtual void Shutdown() = 0;
    virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
    virtual void SetClearColor(const Vec4& color) = 0;
    virtual void Clear() = 0;

    // Lesson 9: real now. The VAO carries the index buffer, so the default
    // draws the whole mesh; pass indexCount to draw a prefix of it.
    virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) = 0;

    static RendererAPIType GetAPI() { return s_API; }
    static Scope<RendererAPI> Create(); // factory — the choke point

protected:
    // Set once at startup (later: from CLI/env). Everything reads it.
    static void SetAPI(RendererAPIType api) { s_API = api; }

private:
    static RendererAPIType s_API;
};

}
