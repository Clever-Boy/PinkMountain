// Framebuffer.cpp — the framebuffer factory (Lesson 13).
// The ONLY place that names the backend framebuffer class (seam rule #2).

#include "Pink/Renderer/Framebuffer.h"
#include "Pink/Renderer/Renderer.h"
#include "Pink/Renderer/OpenGL/OpenGLFramebuffer.h"
#include "Pink/Core/Assert.h"

namespace Pink {

Ref<Framebuffer> Framebuffer::Create(const FramebufferSpec& spec)
{
    switch (Renderer::GetAPI())
    {
        case RendererAPIType::OpenGL:     return CreateRef<OpenGLFramebuffer>(spec);
        case RendererAPIType::None:       PM_CORE_ASSERT(false, "RendererAPI::None — no backend selected"); return nullptr;
        case RendererAPIType::Vulkan:
        case RendererAPIType::Direct3D12: PM_CORE_ASSERT(false, "Framebuffer backend not implemented yet"); return nullptr;
    }
    PM_CORE_ASSERT(false, "Unknown RendererAPI");
    return nullptr;
}

}
