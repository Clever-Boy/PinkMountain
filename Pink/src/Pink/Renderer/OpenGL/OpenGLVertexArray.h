#pragma once

// OpenGLVertexArray.h — the OpenGL vertex array implementation (Lesson 9).
// NOTE: no glad/gl.h here — deliberately. The header stays GL-clean; glad
// lives in the .cpp alone: the leak-prevention rule.

#include "Pink/Renderer/VertexArray.h"

namespace Pink {

class OpenGLVertexArray : public VertexArray
{
public:
    OpenGLVertexArray();
    virtual ~OpenGLVertexArray();

    virtual void Bind() const override;
    virtual void Unbind() const override;

    virtual void AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) override;
    virtual void SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer) override;

    virtual const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const override { return m_VertexBuffers; }
    virtual const Ref<IndexBuffer>& GetIndexBuffer() const override { return m_IndexBuffer; }

private:
    uint32_t m_RendererID;
    // Attribute locations are VAO-global, not per-buffer: the index keeps
    // counting across every buffer added to this VAO.
    uint32_t m_VertexBufferIndex = 0;
    std::vector<Ref<VertexBuffer>> m_VertexBuffers;
    Ref<IndexBuffer> m_IndexBuffer;
};

}
