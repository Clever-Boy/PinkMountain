// OpenGLBuffer.cpp — the OpenGL buffer implementations (Lesson 9).

#include "Pink/Renderer/OpenGL/OpenGLBuffer.h"

// ── glad lives ONLY in this .cpp. This is the leak-prevention rule. ──
#include <glad/gl.h>

namespace Pink {

// ---------------------------------------------------------------------------
// VertexBuffer
// ---------------------------------------------------------------------------

OpenGLVertexBuffer::OpenGLVertexBuffer(float* vertices, uint32_t size)
{
    glGenBuffers(1, &m_RendererID);
    glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
    // COPIES the data to the GPU. Your CPU-side array may go out of scope
    // after this call — the driver owns its own copy now.
    // GL_STATIC_DRAW is a promise: "written once, read many times." The
    // driver parks the buffer in the fastest VRAM on the strength of it.
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
}

OpenGLVertexBuffer::~OpenGLVertexBuffer()
{
    glDeleteBuffers(1, &m_RendererID);
}

void OpenGLVertexBuffer::Bind() const   { glBindBuffer(GL_ARRAY_BUFFER, m_RendererID); }
void OpenGLVertexBuffer::Unbind() const { glBindBuffer(GL_ARRAY_BUFFER, 0); }

// ---------------------------------------------------------------------------
// IndexBuffer
// ---------------------------------------------------------------------------

OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t* indices, uint32_t count)
    : m_Count(count)
{
    glGenBuffers(1, &m_RendererID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
}

OpenGLIndexBuffer::~OpenGLIndexBuffer()
{
    glDeleteBuffers(1, &m_RendererID);
}

void OpenGLIndexBuffer::Bind() const   { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID); }
void OpenGLIndexBuffer::Unbind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }

}
