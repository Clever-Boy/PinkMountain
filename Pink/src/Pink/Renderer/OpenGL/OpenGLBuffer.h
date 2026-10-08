#pragma once

// OpenGLBuffer.h — the OpenGL buffer implementations (Lesson 9).
// NOTE: no glad/gl.h here — deliberately. The header stays GL-clean; glad
// lives in the .cpp alone: the leak-prevention rule.

#include "Pink/Renderer/Buffer.h"

namespace Pink {

class OpenGLVertexBuffer : public VertexBuffer
{
public:
    OpenGLVertexBuffer(float* vertices, uint32_t size);
    virtual ~OpenGLVertexBuffer();

    virtual void Bind() const override;
    virtual void Unbind() const override;

    virtual void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }
    virtual const BufferLayout& GetLayout() const override { return m_Layout; }

private:
    uint32_t m_RendererID;
    BufferLayout m_Layout;
};

class OpenGLIndexBuffer : public IndexBuffer
{
public:
    OpenGLIndexBuffer(uint32_t* indices, uint32_t count);
    virtual ~OpenGLIndexBuffer();

    virtual void Bind() const override;
    virtual void Unbind() const override;

    virtual uint32_t GetCount() const override { return m_Count; }

private:
    uint32_t m_RendererID;
    uint32_t m_Count;
};

}
