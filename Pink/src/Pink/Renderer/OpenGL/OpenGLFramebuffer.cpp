// OpenGLFramebuffer.cpp — the OpenGL framebuffer (Lessons 13–14).

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
    glDeleteTextures((GLsizei)m_ColorTextureIDs.size(), m_ColorTextureIDs.data());
}

void OpenGLFramebuffer::Invalidate()
{
    if (m_RendererID)
    {
        glDeleteFramebuffers(1, &m_RendererID);
        if (m_DepthTextureID) glDeleteTextures(1, &m_DepthTextureID);
        glDeleteTextures((GLsizei)m_ColorTextureIDs.size(), m_ColorTextureIDs.data());
        m_DepthTextureID = 0;
        m_ColorTextureIDs.clear();
    }

    glGenFramebuffers(1, &m_RendererID);
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

    // --- Color attachments (Lesson 14)
    // One texture per ColorFormats entry, attached to COLOR_ATTACHMENT0 + i.
    // The HDR scene target uses { RGBA16F, RGBA16F }: location 0 = scene
    // color, location 1 = bright extract (MRT — the lighting shader writes
    // both in one pass).
    for (auto format : m_Spec.ColorFormats)
    {
        uint32_t id = 0;
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);

        // RGBA16F: internal format GL_RGBA16F, data type GL_HALF_FLOAT.
        // Why not RGBA32F? Two reasons, both hardware: (1) 32F costs twice
        // the bandwidth per sample — at 1280x720 with several fullscreen
        // passes, that's real milliseconds on Intel UHD; (2) 32F float
        // textures are NOT filterable on many drivers (no GL_LINEAR), and
        // the blur pass NEEDS linear filtering. 16F is the sweet spot —
        // it's what every shipping engine uses for its HDR target.
        GLint internalFormat = (format == ColorAttachmentFormat::RGBA16F) ? GL_RGBA16F : GL_RGBA8;
        GLenum dataType = (format == ColorAttachmentFormat::RGBA16F) ? GL_HALF_FLOAT : GL_UNSIGNED_BYTE;
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat,
                     (int)m_Spec.Width, (int)m_Spec.Height,
                     0, GL_RGBA, dataType, nullptr);
        // LINEAR: color targets are SAMPLED (composite, blur) — unlike
        // depth targets, where the shader does its own comparison.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        uint32_t attachment = (uint32_t)m_ColorTextureIDs.size();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment,
                               GL_TEXTURE_2D, id, 0);
        m_ColorTextureIDs.push_back(id);
    }

    // Tell the driver which color attachments the shaders write to.
    // (With one attachment the default is already COLOR_ATTACHMENT0, but
    // explicit beats implicit — and MRT NEEDS this call.)
    if (!m_ColorTextureIDs.empty())
    {
        std::vector<GLenum> drawBuffers;
        for (uint32_t i = 0; i < m_ColorTextureIDs.size(); ++i)
            drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
        glDrawBuffers((GLsizei)drawBuffers.size(), drawBuffers.data());
    }

    // --- Depth attachment (unchanged from Lesson 13)
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
        // samples as depth 1.0 (farthest) — i.e. LIT. (The HDR scene target
        // reuses this depth path; its border mode is irrelevant because the
        // depth texture is never sampled — but the code stays shared.)
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
    // This is the Lesson-13 depth-only path — still used by both shadow maps.
    if (m_ColorTextureIDs.empty())
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

void OpenGLFramebuffer::BindColorAttachment(uint32_t index, uint32_t slot) const
{
    PM_CORE_ASSERT(index < m_ColorTextureIDs.size(), "Color attachment index out of range");
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_ColorTextureIDs[index]);
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
