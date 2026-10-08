// Shader.cpp — the Shader factory (Lesson 8).
// The ONLY place that names the backend shader class (seam rule #2).

#include "Pink/Renderer/Shader.h"
#include "Pink/Renderer/Renderer.h"
#include "Pink/Renderer/OpenGL/OpenGLShader.h"
#include "Pink/Core/Assert.h"

namespace Pink {

Ref<Shader> Shader::Create(const std::string& filepath)
{
    switch (Renderer::GetAPI())
    {
        case RendererAPIType::OpenGL:     return CreateRef<OpenGLShader>(filepath);
        case RendererAPIType::None:       PM_CORE_ASSERT(false, "RendererAPI::None — no backend selected"); return nullptr;
        case RendererAPIType::Vulkan:
        case RendererAPIType::Direct3D12: PM_CORE_ASSERT(false, "Shader backend not implemented yet"); return nullptr;
    }
    PM_CORE_ASSERT(false, "Unknown RendererAPI");
    return nullptr;
}

}
