#type vertex
#version 330 core

// FlatColor.glsl — the Lesson 8 shader: one uniform color, no lighting.
// Single-file format: sections are split on "#type vertex" / "#type fragment"
// by OpenGLShader::PreProcess (Hazel-style).

layout(location = 0) in vec3 a_Position;

void main()
{
    gl_Position = vec4(a_Position, 1.0);
}

#type fragment
#version 330 core

layout(location = 0) out vec4 color;

uniform vec3 u_Color;

void main()
{
    color = vec4(u_Color, 1.0);
}
