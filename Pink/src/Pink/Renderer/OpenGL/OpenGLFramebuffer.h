#pragma once

// OpenGLFramebuffer.h — the OpenGL framebuffer (Lesson 13).
// glad lives in the .cpp only (the leak-prevention rule).

#include "Pink/Renderer/Framebuffer.h"

#include <cstdint>

namespace Pink {

class PINK_API OpenGLFramebuffer : public Framebuffer
{
public:
    OpenGLFramebuffer(const FramebufferSpec& spec);
    virtual ~OpenGLFramebuffer();

    void Bind() override;
    void BindFace(uint32_t face) override;
    void Unbind() override;
    void Resize(uint32_t width, uint32_t height) override;
    void BindDepthTexture(uint32_t slot = 0) const override;
    const FramebufferSpec& GetSpec() const override { return m_Spec; }

private:
    void Invalidate(); // (re)create the GL objects from m_Spec

private:
    uint32_t m_RendererID = 0;
    uint32_t m_DepthTextureID = 0; // 0 = no depth attachment
    uint32_t m_ColorTextureID = 0; // 0 = no color attachment
};

}
