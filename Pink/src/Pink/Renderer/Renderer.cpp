#include "Pink/Renderer/Renderer.h"
#include "Pink/Renderer/Shader.h"
#include "Pink/Renderer/VertexArray.h"
#include "Pink/Core/Assert.h"

namespace Pink {

Scope<RendererAPI> Renderer::s_RendererAPI = nullptr;

void Renderer::Init()
{
    s_RendererAPI = RendererAPI::Create();
    PM_CORE_ASSERT(s_RendererAPI, "Renderer backend creation failed");
    s_RendererAPI->Init();
}

void Renderer::Shutdown()
{
    s_RendererAPI.reset(); // virtual dtor runs backend cleanup
}

void Renderer::BeginFrame(const Vec4& clearColor)
{
    s_RendererAPI->SetClearColor(clearColor);
    s_RendererAPI->Clear();
}

void Renderer::OnWindowResize(uint32_t width, uint32_t height)
{
    // Minimized windows report 0 — glViewport(0,0,0,0) is legal but
    // pointless; skipping keeps the backend honest about real resizes.
    if (width == 0 || height == 0)
        return;
    s_RendererAPI->SetViewport(0, 0, width, height);
}

void Renderer::Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray)
{
    shader->Bind();
    vertexArray->Bind();
    s_RendererAPI->DrawIndexed(vertexArray);
}

}
