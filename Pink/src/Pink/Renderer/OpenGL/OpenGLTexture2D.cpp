// OpenGLTexture2D.cpp — the OpenGL texture implementation (Lesson 10).

#include "Pink/Renderer/OpenGL/OpenGLTexture2D.h"

// ── glad lives ONLY in this .cpp. This is the leak-prevention rule. ──
#include <glad/gl.h>

#include <stb_image.h>

#include "Pink/Core/Log.h"
#include "Pink/Core/Assert.h"

namespace Pink {

OpenGLTexture2D::OpenGLTexture2D(uint32_t width, uint32_t height)
    : m_Width(width), m_Height(height)
{
    m_InternalFormat = GL_RGBA8;
    m_DataFormat = GL_RGBA;

    glGenTextures(1, &m_RendererID);
    glBindTexture(GL_TEXTURE_2D, m_RendererID);

    // Sampler state (GL 3.3 style: it lives ON the texture object).
    // Minification reads the mipmap chain; magnification has nothing to
    // minify, so plain linear; wrap repeats at the [0,1] borders.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Allocate storage now; the pixels arrive via SetData().
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(m_InternalFormat),
                 static_cast<GLsizei>(m_Width), static_cast<GLsizei>(m_Height),
                 0, m_DataFormat, GL_UNSIGNED_BYTE, nullptr);
}

OpenGLTexture2D::OpenGLTexture2D(const std::string& path)
    : m_Path(path)
{
    int width, height, channels;
    // stb_image is global state: flip EVERY load. Image files store the top
    // row first; OpenGL's UV origin is bottom-left. Skip this and every
    // texture renders upside down.
    stbi_set_flip_vertically_on_load(1);
    // req_comp = 0: keep the file's native channel count — WE adapt the GL
    // format to the image, not the other way round.
    stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    PM_CORE_ASSERT(data, "Failed to load image!");
    m_Width = static_cast<uint32_t>(width);
    m_Height = static_cast<uint32_t>(height);

    unsigned int internalFormat = 0, dataFormat = 0;
    if (channels == 4)      { internalFormat = GL_RGBA8; dataFormat = GL_RGBA; }
    else if (channels == 3) { internalFormat = GL_RGB8;  dataFormat = GL_RGB;  }
    else if (channels == 1) { internalFormat = GL_R8;     dataFormat = GL_RED;  }
    PM_CORE_ASSERT(internalFormat && dataFormat, "Image channel count not supported (need 1, 3 or 4)");
    m_InternalFormat = internalFormat;
    m_DataFormat = dataFormat;

    glGenTextures(1, &m_RendererID);
    glBindTexture(GL_TEXTURE_2D, m_RendererID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internalFormat),
                 static_cast<GLsizei>(m_Width), static_cast<GLsizei>(m_Height),
                 0, dataFormat, GL_UNSIGNED_BYTE, data);
    // AFTER the upload — mipmaps are derived from level 0, so generating
    // them before uploading would chain off empty storage. Order matters.
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data); // the GPU has its copy; free the CPU copy
}

OpenGLTexture2D::~OpenGLTexture2D()
{
    glDeleteTextures(1, &m_RendererID);
}

void OpenGLTexture2D::SetData(void* data, uint32_t size)
{
    // INVARIANT: SetData fills the WHOLE texture. Partial updates get their
    // own API when render targets need it.
    PM_CORE_ASSERT(size == m_Width * m_Height * 4, "SetData must fill the entire texture!");
    glBindTexture(GL_TEXTURE_2D, m_RendererID);
    // Default unpack alignment is 4: a row whose byte-width isn't a multiple
    // of 4 would upload sheared. Setting 1 is always safe.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                    static_cast<GLsizei>(m_Width), static_cast<GLsizei>(m_Height),
                    m_DataFormat, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D); // new level 0 invalidates the old chain
}

void OpenGLTexture2D::Bind(uint32_t slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);      // select the "shelf"...
    glBindTexture(GL_TEXTURE_2D, m_RendererID); // ...then put the "book" on it
}

}
