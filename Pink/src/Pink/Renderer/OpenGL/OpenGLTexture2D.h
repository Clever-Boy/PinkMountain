#pragma once

// OpenGLTexture2D.h — the OpenGL texture implementation (Lesson 10).
// NOTE: no glad/gl.h here — deliberately. The header stays GL-clean; glad
// lives in the .cpp alone: the leak-prevention rule.

#include "Pink/Renderer/Texture.h"

#include <string>

namespace Pink {

class OpenGLTexture2D : public Texture2D
{
public:
    OpenGLTexture2D(uint32_t width, uint32_t height);
    OpenGLTexture2D(const std::string& path);
    virtual ~OpenGLTexture2D();

    virtual uint32_t GetWidth() const override { return m_Width; }
    virtual uint32_t GetHeight() const override { return m_Height; }

    virtual void SetData(void* data, uint32_t size) override;
    virtual void Bind(uint32_t slot = 0) const override;

private:
    std::string m_Path;
    uint32_t m_Width, m_Height;
    uint32_t m_RendererID;               // GL texture handle, GL-clean type
    unsigned int m_InternalFormat, m_DataFormat; // sized GL enums, set from channel count
};

}
