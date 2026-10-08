#type vertex
#version 330 core

// Triangle.glsl — the Lesson 9 shader: the engine's first geometry.
// layout(location = 0) is the handshake with the C++ side: BufferLayout's
// first element ("a_Position") lands on attribute index 0, which is exactly
// what m_VertexBufferIndex assigned it in OpenGLVertexArray::AddVertexBuffer.

layout(location = 0) in vec3 a_Position;

out vec3 v_Color;
uniform vec3 u_Color;

void main()
{
    v_Color = u_Color;
    gl_Position = vec4(a_Position, 1.0);
}

#type fragment
#version 330 core

layout(location = 0) out vec4 color;

in vec3 v_Color;

void main()
{
    color = vec4(v_Color, 1.0);
}
