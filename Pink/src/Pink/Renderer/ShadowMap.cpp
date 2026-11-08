// ShadowMap.cpp — shadow map renderers (Lesson 13).

#include "Pink/Renderer/ShadowMap.h"
#include "Pink/Renderer/Renderer.h"
#include "Pink/Core/Assert.h"

#include <cmath>

namespace Pink {

// ---------------------------------------------------------------------------
DirectionalShadowMap::DirectionalShadowMap(uint32_t size)
    : m_Camera(20.0f, 1.0f, 1.0f, 50.0f) // 20-unit ortho box (see lesson math)
{
    FramebufferSpec spec;
    spec.Width = spec.Height = size;
    // Lesson 14: ColorFormats stays empty = depth-only (the Lesson-13
    // GL_NONE path). The spec evolved; the shadow maps didn't need to.
    spec.DepthType = DepthAttachmentType::Texture2D;
    m_Framebuffer = Framebuffer::Create(spec);
    RecalculateMatrix();
}

void DirectionalShadowMap::SetLightDirection(const Vec3& direction)
{
    m_Direction = direction;
    RecalculateMatrix();
}

void DirectionalShadowMap::SetViewportSize(uint32_t width, uint32_t height)
{
    m_WindowWidth = width;
    m_WindowHeight = height;
}

void DirectionalShadowMap::BeginDepthPass()
{
    m_Framebuffer->Bind();
    Renderer::OnWindowResize(m_Framebuffer->GetSpec().Width, m_Framebuffer->GetSpec().Height);
    Renderer::Clear(); // depth buffer must start empty — stale depths = false shadows
}

void DirectionalShadowMap::EndDepthPass()
{
    m_Framebuffer->Unbind();
    Renderer::OnWindowResize(m_WindowWidth, m_WindowHeight);
}

void DirectionalShadowMap::BindDepthTexture(uint32_t slot) const
{
    m_Framebuffer->BindDepthTexture(slot);
}

void DirectionalShadowMap::RecalculateMatrix()
{
    // The light sits opposite its travel direction, 20 units out from the
    // scene center, looking back at it. Ortho projection: parallel rays,
    // uniform texel density (20 units / 2048 texels ≈ 1 cm per texel).
    Vec3 lightPos = -Normalize(m_Direction) * 20.0f;
    Vec3 up = Vec3{ 0.0f, 1.0f, 0.0f };
    // Degenerate case: light straight up/down makes LookAt's up cross product
    // collapse (gimbal-style singularity) — fall back to Z as up.
    if (std::abs(Dot(Normalize(m_Direction), up)) > 0.99f)
        up = Vec3{ 0.0f, 0.0f, 1.0f };
    m_LightSpaceMatrix = m_Camera.GetProjectionMatrix() * LookAt(lightPos, Vec3{ 0.0f, 0.0f, 0.0f }, up);
}

// ---------------------------------------------------------------------------
// The standard cubemap face table: target direction + up vector per face.
// (The Y faces use Z up — X/Z faces use -Y up. Getting one wrong mirrors
// that face's shadows; this table is the one every engine copies.)
static const Vec3 kFaceTargets[6] = {
    Vec3{  1.0f,  0.0f,  0.0f }, Vec3{ -1.0f,  0.0f,  0.0f },
    Vec3{  0.0f,  1.0f,  0.0f }, Vec3{  0.0f, -1.0f,  0.0f },
    Vec3{  0.0f,  0.0f,  1.0f }, Vec3{  0.0f,  0.0f, -1.0f },
};
static const Vec3 kFaceUps[6] = {
    Vec3{  0.0f, -1.0f,  0.0f }, Vec3{  0.0f, -1.0f,  0.0f },
    Vec3{  0.0f,  0.0f,  1.0f }, Vec3{  0.0f,  0.0f, -1.0f },
    Vec3{  0.0f, -1.0f,  0.0f }, Vec3{  0.0f, -1.0f,  0.0f },
};

PointShadowMap::PointShadowMap(uint32_t size, float farPlane)
    : m_Camera(90.0f, 1.0f, 0.1f, farPlane), m_FarPlane(farPlane)
{
    FramebufferSpec spec;
    spec.Width = spec.Height = size;
    // Lesson 14: ColorFormats stays empty = depth-only (the Lesson-13
    // GL_NONE path). The spec evolved; the shadow maps didn't need to.
    spec.DepthType = DepthAttachmentType::CubeMap;
    m_Framebuffer = Framebuffer::Create(spec);
}

void PointShadowMap::SetLightPosition(const Vec3& position)
{
    m_Position = position;
}

void PointShadowMap::SetViewportSize(uint32_t width, uint32_t height)
{
    m_WindowWidth = width;
    m_WindowHeight = height;
}

void PointShadowMap::BeginDepthPass(uint32_t face)
{
    PM_CORE_ASSERT(face < 6, "Cubemap face index must be 0..5");
    m_Framebuffer->BindFace(face);
    Renderer::OnWindowResize(m_Framebuffer->GetSpec().Width, m_Framebuffer->GetSpec().Height);
    Renderer::Clear();
}

void PointShadowMap::EndDepthPass()
{
    m_Framebuffer->Unbind();
    Renderer::OnWindowResize(m_WindowWidth, m_WindowHeight);
}

Mat4 PointShadowMap::GetLightSpaceMatrix(uint32_t face) const
{
    PM_CORE_ASSERT(face < 6, "Cubemap face index must be 0..5");
    return m_Camera.GetProjectionMatrix()
         * LookAt(m_Position, m_Position + kFaceTargets[face], kFaceUps[face]);
}

void PointShadowMap::BindDepthTexture(uint32_t slot) const
{
    m_Framebuffer->BindDepthTexture(slot);
}

}
