// Texture.cpp — the Texture2D factory (Lesson 10).
// The ONLY place that names the backend texture class (seam rule #2).

#include "Pink/Renderer/Texture.h"
#include "Pink/Renderer/Renderer.h"
#include "Pink/Renderer/OpenGL/OpenGLTexture2D.h"
#include "Pink/Core/Assert.h"

namespace Pink {

Ref<Texture2D> Texture2D::Create(uint32_t width, uint32_t height)
{
    switch (Renderer::GetAPI())
    {
        case RendererAPIType::OpenGL:     return CreateRef<OpenGLTexture2D>(width, height);
        case RendererAPIType::None:       PM_CORE_ASSERT(false, "RendererAPI::None — no backend selected"); return nullptr;
        case RendererAPIType::Vulkan:
        case RendererAPIType::Direct3D12: PM_CORE_ASSERT(false, "Texture backend not implemented yet"); return nullptr;
    }
    PM_CORE_ASSERT(false, "Unknown RendererAPI");
    return nullptr;
}

Ref<Texture2D> Texture2D::Create(const std::string& path)
{
    switch (Renderer::GetAPI())
    {
        case RendererAPIType::OpenGL:     return CreateRef<OpenGLTexture2D>(path);
        case RendererAPIType::None:       PM_CORE_ASSERT(false, "RendererAPI::None — no backend selected"); return nullptr;
        case RendererAPIType::Vulkan:
        case RendererAPIType::Direct3D12: PM_CORE_ASSERT(false, "Texture backend not implemented yet"); return nullptr;
    }
    PM_CORE_ASSERT(false, "Unknown RendererAPI");
    return nullptr;
}

}
