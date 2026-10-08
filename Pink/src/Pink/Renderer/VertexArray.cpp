// VertexArray.cpp — the VertexArray factory (Lesson 9).
// The ONLY place that names the backend vertex-array class (seam rule #2).

#include "Pink/Renderer/VertexArray.h"
#include "Pink/Renderer/Renderer.h"
#include "Pink/Renderer/OpenGL/OpenGLVertexArray.h"
#include "Pink/Core/Assert.h"

namespace Pink {

Ref<VertexArray> VertexArray::Create()
{
    switch (Renderer::GetAPI())
    {
        case RendererAPIType::OpenGL:     return CreateRef<OpenGLVertexArray>();
        case RendererAPIType::None:       PM_CORE_ASSERT(false, "RendererAPI::None — no backend selected"); return nullptr;
        case RendererAPIType::Vulkan:
        case RendererAPIType::Direct3D12: PM_CORE_ASSERT(false, "VertexArray backend not implemented yet"); return nullptr;
    }
    PM_CORE_ASSERT(false, "Unknown RendererAPI");
    return nullptr;
}

}
