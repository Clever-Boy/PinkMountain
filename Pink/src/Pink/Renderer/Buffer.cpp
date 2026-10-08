// Buffer.cpp — the VertexBuffer/IndexBuffer factories (Lesson 9).
// The ONLY place that names the backend buffer classes (seam rule #2).

#include "Pink/Renderer/Buffer.h"
#include "Pink/Renderer/Renderer.h"
#include "Pink/Renderer/OpenGL/OpenGLBuffer.h"
#include "Pink/Core/Assert.h"

namespace Pink {

Ref<VertexBuffer> VertexBuffer::Create(float* vertices, uint32_t size)
{
    switch (Renderer::GetAPI())
    {
        case RendererAPIType::OpenGL:     return CreateRef<OpenGLVertexBuffer>(vertices, size);
        case RendererAPIType::None:       PM_CORE_ASSERT(false, "RendererAPI::None — no backend selected"); return nullptr;
        case RendererAPIType::Vulkan:
        case RendererAPIType::Direct3D12: PM_CORE_ASSERT(false, "Buffer backend not implemented yet"); return nullptr;
    }
    PM_CORE_ASSERT(false, "Unknown RendererAPI");
    return nullptr;
}

Ref<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t count)
{
    switch (Renderer::GetAPI())
    {
        case RendererAPIType::OpenGL:     return CreateRef<OpenGLIndexBuffer>(indices, count);
        case RendererAPIType::None:       PM_CORE_ASSERT(false, "RendererAPI::None — no backend selected"); return nullptr;
        case RendererAPIType::Vulkan:
        case RendererAPIType::Direct3D12: PM_CORE_ASSERT(false, "Buffer backend not implemented yet"); return nullptr;
    }
    PM_CORE_ASSERT(false, "Unknown RendererAPI");
    return nullptr;
}

}
