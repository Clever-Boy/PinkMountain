#pragma once

// VertexArray.h — the renderer-neutral vertex array interface (Lesson 9).
//
// The VAO is the "instruction card": it records HOW to read the vertex
// buffers (attribute formats, stride, offsets) and WHICH index buffer to
// draw with. It holds NO vertex data itself — only descriptions and
// references. Deleting a VBO while its VAO lives = a VAO pointing at freed
// GPU memory. That's why the implementation keeps Ref<>s to its buffers:
// the abstraction enforces the lifetime rule the raw API doesn't.

#include "Pink/Renderer/Buffer.h"

namespace Pink {

class PINK_API VertexArray
{
public:
    virtual ~VertexArray() = default;

    virtual void Bind() const = 0;
    virtual void Unbind() const = 0;

    virtual void AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) = 0;
    virtual void SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer) = 0;

    virtual const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const = 0;
    virtual const Ref<IndexBuffer>& GetIndexBuffer() const = 0;

    static Ref<VertexArray> Create();
};

}
