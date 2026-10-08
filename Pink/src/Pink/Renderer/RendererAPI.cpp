#include "Pink/Renderer/RendererAPI.h"
#include "Pink/Renderer/OpenGL/OpenGLRendererAPI.h"

namespace Pink {

RendererAPIType RendererAPI::s_API = RendererAPIType::OpenGL;

Scope<RendererAPI> RendererAPI::Create()
{
    switch (s_API)
    {
        case RendererAPIType::None:
            PM_CORE_ASSERT(false, "RendererAPI::None — no backend selected");
            return nullptr;
        case RendererAPIType::OpenGL:
            return CreateScope<OpenGLRendererAPI>();
        // These arms PROVE the seam: a new backend = one new class + one arm.
        // Nothing above this line changes.
        case RendererAPIType::Vulkan:
            PM_CORE_ASSERT(false, "Vulkan backend not implemented yet");
            return nullptr;
        case RendererAPIType::Direct3D12:
            PM_CORE_ASSERT(false, "Direct3D12 backend not implemented yet");
            return nullptr;
    }
    PM_CORE_ASSERT(false, "Unknown RendererAPI");
    return nullptr;
}

}
