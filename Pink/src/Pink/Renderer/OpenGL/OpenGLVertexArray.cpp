// OpenGLVertexArray.cpp — the OpenGL vertex array implementation (Lesson 9).
// Read this file slowly: it is the lesson's payoff — the BufferLayout you
// declare in C++ becomes GL vertex-attribute state here.

#include "Pink/Renderer/OpenGL/OpenGLVertexArray.h"

// ── glad lives ONLY in this .cpp. This is the leak-prevention rule. ──
#include <glad/gl.h>

#include "Pink/Core/Assert.h"

namespace Pink {

static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
{
    switch (type)
    {
        case ShaderDataType::Float:   return GL_FLOAT;
        case ShaderDataType::Float2:  return GL_FLOAT;
        case ShaderDataType::Float3:  return GL_FLOAT;
        case ShaderDataType::Float4:  return GL_FLOAT;
        case ShaderDataType::Mat3:    return GL_FLOAT;
        case ShaderDataType::Mat4:    return GL_FLOAT;
        case ShaderDataType::Int:     return GL_INT;
        case ShaderDataType::Int2:    return GL_INT;
        case ShaderDataType::Int3:    return GL_INT;
        case ShaderDataType::Int4:    return GL_INT;
        case ShaderDataType::Bool:    return GL_BOOL;
        case ShaderDataType::None:    break;
    }
    PM_CORE_ASSERT(false, "Unknown ShaderDataType!");
    return 0;
}

OpenGLVertexArray::OpenGLVertexArray()
{
    glGenVertexArrays(1, &m_RendererID);
}

OpenGLVertexArray::~OpenGLVertexArray()
{
    glDeleteVertexArrays(1, &m_RendererID);
}

void OpenGLVertexArray::Bind() const   { glBindVertexArray(m_RendererID); }
void OpenGLVertexArray::Unbind() const { glBindVertexArray(0); }

void OpenGLVertexArray::AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer)
{
    // INVARIANT: the layout must be set BEFORE this call. The attribute
    // pointers below are captured from it — right here, right now. Setting
    // the layout afterwards changes nothing in GL state.
    PM_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "Vertex Buffer has no layout!");

    glBindVertexArray(m_RendererID); // every glVertexAttribPointer below lands in THIS VAO...
    vertexBuffer->Bind();            // ...and reads from THIS VBO. Order matters.

    const auto& layout = vertexBuffer->GetLayout();
    for (const auto& element : layout)
    {
        switch (element.Type)
        {
            case ShaderDataType::Float:
            case ShaderDataType::Float2:
            case ShaderDataType::Float3:
            case ShaderDataType::Float4:
            {
                glEnableVertexAttribArray(m_VertexBufferIndex);
                glVertexAttribPointer(m_VertexBufferIndex,
                    element.GetComponentCount(),
                    ShaderDataTypeToOpenGLBaseType(element.Type),
                    element.Normalized ? GL_TRUE : GL_FALSE,
                    layout.GetStride(),
                    reinterpret_cast<const void*>(element.Offset)); // byte offset into each vertex
                m_VertexBufferIndex++;
                break;
            }
            case ShaderDataType::Mat3:
            case ShaderDataType::Mat4:
            {
                // A matrix occupies N attribute slots — one per column vector
                // (vec3 per column for Mat3, vec4 for Mat4).
                uint8_t count = element.Type == ShaderDataType::Mat3 ? 3 : 4;
                for (uint8_t i = 0; i < count; i++)
                {
                    glEnableVertexAttribArray(m_VertexBufferIndex);
                    glVertexAttribPointer(m_VertexBufferIndex,
                        count,
                        GL_FLOAT,
                        element.Normalized ? GL_TRUE : GL_FALSE,
                        layout.GetStride(),
                        reinterpret_cast<const void*>(element.Offset + sizeof(float) * count * i));
                    m_VertexBufferIndex++;
                    // (Instanced per-instance matrices would also need
                    // glVertexAttribDivisor here — a later lesson.)
                }
                break;
            }
            case ShaderDataType::Int:
            case ShaderDataType::Int2:
            case ShaderDataType::Int3:
            case ShaderDataType::Int4:
            case ShaderDataType::Bool:
                // Integer attributes must use glVertexAttribIPointer (the
                // float version would silently convert them). Not wired yet.
                PM_CORE_ASSERT(false, "Integer vertex attributes need glVertexAttribIPointer — not wired yet!");
                break;
            default:
                PM_CORE_ASSERT(false, "Unknown ShaderDataType!");
        }
    }

    // Shared ownership: the VAO keeps its VBOs alive. This is the code-level
    // fix for "the VAO stores my data" — it stores REFERENCES, and now those
    // references can't dangle.
    m_VertexBuffers.push_back(vertexBuffer);
}

void OpenGLVertexArray::SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer)
{
    glBindVertexArray(m_RendererID);
    indexBuffer->Bind();
    // NOTE: unlike GL_ARRAY_BUFFER, the GL_ELEMENT_ARRAY_BUFFER binding IS
    // part of VAO state — this bind is captured into the VAO. That's why the
    // draw call later needs only the VAO bound, not the EBO.
    m_IndexBuffer = indexBuffer;
}

}
