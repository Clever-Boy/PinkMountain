// PostProcess.cpp — the post-processing chain (Lesson 14).

#include "Pink/Renderer/PostProcess.h"
#include "Pink/Renderer/Renderer.h"
#include "Pink/Renderer/Buffer.h"
#include "Pink/Core/Assert.h"

#include <algorithm> // std::max
#include <filesystem>

namespace Pink {

void PostProcess::Init(uint32_t width, uint32_t height)
{
    m_Width = width;
    m_Height = height;

    // The HDR scene target: TWO RGBA16F attachments + a depth texture.
    // Attachment 0 = scene color, attachment 1 = bright extract. The
    // lighting shaders write both in one pass (MRT) — the bright target
    // costs no extra geometry pass.
    FramebufferSpec hdrSpec;
    hdrSpec.Width = width;
    hdrSpec.Height = height;
    hdrSpec.ColorFormats = { ColorAttachmentFormat::RGBA16F, ColorAttachmentFormat::RGBA16F };
    hdrSpec.DepthType = DepthAttachmentType::Texture2D; // scene needs depth testing
    m_HDRFramebuffer = Framebuffer::Create(hdrSpec);

    // Ping-pong blur targets at HALF resolution: 1/4 the pixels = ~1/4 the
    // cost, and the blur hides the resolution loss completely. No depth —
    // these are pure 2D image targets.
    FramebufferSpec blurSpec;
    blurSpec.Width = std::max(width / 2, 1u);
    blurSpec.Height = std::max(height / 2, 1u);
    blurSpec.ColorFormats = { ColorAttachmentFormat::RGBA16F };
    blurSpec.DepthType = DepthAttachmentType::None;
    m_PingPong[0] = Framebuffer::Create(blurSpec);
    m_PingPong[1] = Framebuffer::Create(blurSpec);

    m_BlurShader = Shader::Create("assets/shaders/Blur.glsl");
    m_CompositeShader = Shader::Create("assets/shaders/Composite.glsl");
    m_BlurWriteTime = std::filesystem::last_write_time(m_BlurPath);
    m_CompositeWriteTime = std::filesystem::last_write_time(m_CompositePath);

    // The fullscreen TRIANGLE (not quad): 3 verts, (-1,-1), (3,-1), (-1,3).
    // One triangle covers the screen with no diagonal seam and no index
    // buffer wasted on a degenerate quad. Positions only — the vertex
    // shader derives UVs from position (uv = pos * 0.5 + 0.5).
    float verts[9] = {
        -1.0f, -1.0f, 0.0f,
         3.0f, -1.0f, 0.0f,
        -1.0f,  3.0f, 0.0f,
    };
    auto vb = VertexBuffer::Create(verts, sizeof(verts));
    vb->SetLayout({ { ShaderDataType::Float3, "a_Position" } }); // BEFORE AddVertexBuffer (Lesson 9)
    m_QuadVertexArray = VertexArray::Create();
    m_QuadVertexArray->AddVertexBuffer(vb);
    uint32_t indices[3] = { 0, 1, 2 };
    m_QuadVertexArray->SetIndexBuffer(IndexBuffer::Create(indices, 3));
}

void PostProcess::BeginScenePass()
{
    m_HDRFramebuffer->Bind();
    Renderer::OnWindowResize(m_HDRFramebuffer->GetSpec().Width,
                             m_HDRFramebuffer->GetSpec().Height);
    Renderer::Clear(); // color + depth, on the HDR target
}

void PostProcess::EndScenePass()
{
    m_HDRFramebuffer->Unbind(); // back to the screen
}

void PostProcess::Execute()
{
    // Post passes are 2D: no depth testing. (Restored at the end — the next
    // frame's 3D passes need it. Forgetting the restore is the classic bug:
    // everything renders, but in draw order, with no depth.)
    Renderer::SetDepthTest(false);

    // --- Blur the bright target (ping-pong, LearnOpenGL pattern) -----------
    // 10 iterations alternating horizontal/vertical at half res ≈ one wide
    // 2D blur, for the cost of 10 narrow 1D passes. firstIteration reads the
    // MRT bright attachment; the rest read the previous ping-pong target.
    // Indexing mirrors LearnOpenGL exactly: target = [horizontal ? 1 : 0].
    bool horizontal = true;
    bool firstIteration = true;
    m_BlurShader->Bind();
    m_BlurShader->SetInt("u_Image", 0);
    for (uint32_t i = 0; i < 10; ++i)
    {
        Ref<Framebuffer>& target = horizontal ? m_PingPong[1] : m_PingPong[0];
        Ref<Framebuffer>& source = horizontal ? m_PingPong[0] : m_PingPong[1];
        target->Bind();
        Renderer::OnWindowResize(target->GetSpec().Width, target->GetSpec().Height);
        m_BlurShader->SetInt("u_Horizontal", horizontal ? 1 : 0);
        if (firstIteration)
            m_HDRFramebuffer->BindColorAttachment(1, 0); // the MRT bright output
        else
            source->BindColorAttachment(0, 0);
        RenderQuad(m_BlurShader);
        horizontal = !horizontal;
        firstIteration = false;
    }
    // After 10 (even) iterations, horizontal is true again and the last
    // write went to m_PingPong[0] — trace the alternation on paper once;
    // the composite below reads [0].

    // --- Composite to the screen -------------------------------------------
    m_PingPong[0]->Unbind(); // any Unbind() returns to the default framebuffer
    Renderer::OnWindowResize(m_Width, m_Height);
    m_CompositeShader->Bind();
    m_CompositeShader->SetInt("u_Scene", 0);
    m_CompositeShader->SetInt("u_Bloom", 1);
    m_CompositeShader->SetFloat("u_Exposure", m_Exposure);
    m_CompositeShader->SetInt("u_ToneMapping", (int)m_ToneMapping);
    m_CompositeShader->SetFloat("u_BloomStrength", m_BloomStrength);
    m_CompositeShader->SetInt("u_BloomEnabled", m_BloomEnabled ? 1 : 0);
    m_HDRFramebuffer->BindColorAttachment(0, 0);
    m_PingPong[0]->BindColorAttachment(0, 1);
    RenderQuad(m_CompositeShader);

    Renderer::SetDepthTest(true); // restore — see the warning above
}

void PostProcess::OnResize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return; // minimized — the Lesson-11 0x0 guard
    m_Width = width;
    m_Height = height;
    m_HDRFramebuffer->Resize(width, height);
    m_PingPong[0]->Resize(std::max(width / 2, 1u), std::max(height / 2, 1u));
    m_PingPong[1]->Resize(std::max(width / 2, 1u), std::max(height / 2, 1u));
}

void PostProcess::RenderQuad(const Ref<Shader>& shader)
{
    // Submit() binds the shader + VAO and draws the 3 indices. The caller
    // has already set the shader's uniforms; Submit just re-binds what's
    // bound. No raw GL here — the Lesson-9 seams do the work.
    Renderer::Submit(shader, m_QuadVertexArray);
}

void PostProcess::CheckHotReload()
{
    auto check = [](Ref<Shader>& shader, const std::string& path,
                    std::filesystem::file_time_type& writeTime) {
        auto t = std::filesystem::last_write_time(path);
        if (t != writeTime)
        {
            writeTime = t;
            shader->Reload(); // broken edit? the old program survives (Lesson 8 invariant)
        }
    };
    check(m_BlurShader, m_BlurPath, m_BlurWriteTime);
    check(m_CompositeShader, m_CompositePath, m_CompositeWriteTime);
}

}
