// OpenGLFramebuffer.cpp — the OpenGL framebuffer (Lesson 13).

#include "Pink/Renderer/OpenGL/OpenGLFramebuffer.h"

// ── glad lives ONLY in this .cpp. This is the leak-prevention rule. ──
#include <glad/gl.h>

#include "Pink/Core/Log.h"
#include "Pink/Core/Assert.h"

namespace Pink {

OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferSpec& spec)
{
    m_Spec = spec;
    Invalidate();
}

OpenGLFramebuffer::~OpenGLFramebuffer()
{
    glDeleteFramebuffers(1, &m_RendererID);
    if (m_DepthTextureID) glDeleteTextures(1, &m_DepthTextureID);
    if (m_ColorTextureID) glDeleteTextures(1, &m_ColorTextureID);
}

void OpenGLFramebuffer::Invalidate()
{
    if (m_RendererID)
    {
        glDeleteFramebuffers(1, &m_RendererID);
        if (m_DepthTextureID) glDeleteTextures(1, &m_DepthTextureID);
        if (m_ColorTextureID) glDeleteTextures(1, &m_ColorTextureID);
        m_DepthTextureID = m_ColorTextureID = 0;
    }

    glGenFramebuffers(1, &m_RendererID);
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

    // --- Color attachment (Lesson 14's HDR target; unused by shadow maps)
    if (m_Spec.HasColorAttachment)
    {
        glGenTextures(1, &m_ColorTextureID);
        glBindTexture(GL_TEXTURE_2D, m_ColorTextureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (int)m_Spec.Width, (int)m_Spec.Height,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, m_ColorTextureID, 0);
    }

    // --- Depth attachment
    if (m_Spec.DepthType == DepthAttachmentType::Texture2D)
    {
        glGenTextures(1, &m_DepthTextureID);
        glBindTexture(GL_TEXTURE_2D, m_DepthTextureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
                     (int)m_Spec.Width, (int)m_Spec.Height,
                     0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        // NEAREST, not LINEAR: the shader does the depth comparison itself.
        // Prefiltering depths (bilinear on raw depth values) would compare
        // against averaged depths — wrong answers, soft-looking artifacts.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        // CLAMP_TO_BORDER with border = 1.0: anything outside the shadow map
        // samples as depth 1.0 (farthest) — i.e. LIT. The alternative
        // (clamp to edge) would stretch edge texels into false shadows.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_TEXTURE_2D, m_DepthTextureID, 0);
    }
    else if (m_Spec.DepthType == DepthAttachmentType::CubeMap)
    {
        glGenTextures(1, &m_DepthTextureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_DepthTextureID);
        for (uint32_t i = 0; i < 6; ++i)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT24,
                         (int)m_Spec.Width, (int)m_Spec.Height,
                         0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        // Cubemaps have no border mode that makes sense — CLAMP_TO_EDGE.
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        // No face attached yet — BindFace attaches one per render.
    }

    // --- No color attachment? Tell the driver explicitly. Without this,
    // the framebuffer is INCOMPLETE (it would try to write color nowhere).
    if (!m_Spec.HasColorAttachment)
    {
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }

    PM_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                   "Framebuffer incomplete — check attachment combination");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLFramebuffer::Bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
}

void OpenGLFramebuffer::BindFace(uint32_t face)
{
    PM_CORE_ASSERT(m_Spec.DepthType == DepthAttachmentType::CubeMap,
                   "BindFace is cubemap-only — 2D targets have one face");
    PM_CORE_ASSERT(face < 6, "Cubemap face index must be 0..5");
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, m_DepthTextureID, 0);
}

void OpenGLFramebuffer::Unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLFramebuffer::Resize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0 || (width == m_Spec.Width && height == m_Spec.Height))
        return;
    m_Spec.Width = width;
    m_Spec.Height = height;
    Invalidate(); // textures are sized at creation — resize = recreate
}

void OpenGLFramebuffer::BindDepthTexture(uint32_t slot) const
{
    PM_CORE_ASSERT(m_DepthTextureID != 0, "BindDepthTexture with no depth attachment");
    glActiveTexture(GL_TEXTURE0 + slot);
    if (m_Spec.DepthType == DepthAttachmentType::CubeMap)
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_DepthTextureID);
    else
        glBindTexture(GL_TEXTURE_2D, m_DepthTextureID);
}

}
