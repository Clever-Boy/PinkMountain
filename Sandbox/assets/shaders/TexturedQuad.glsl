#type vertex
#version 330 core

// TexturedQuad.glsl — the Lesson 10 shader: UVs in, sampled color out.
// a_TexCoord rides attribute location 1 (see the BufferLayout in
// SandboxApp.cpp). texture() is the sampler at work: interpolated UV in,
// trilinear-filtered color out — the LINEAR_MIPMAP_LINEAR state set in C++
// is what makes this single call do the right thing at every distance.

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

out vec2 v_TexCoord;

void main()
{
    v_TexCoord = a_TexCoord;
    gl_Position = vec4(a_Position, 1.0);
}

#type fragment
#version 330 core

layout(location = 0) out vec4 color;

in vec2 v_TexCoord;

// Holds a TEXTURE UNIT index (0, 1, ...), not a texture handle.
// Pair with Texture::Bind(slot) on the C++ side.
uniform sampler2D u_Texture;

void main()
{
    color = texture(u_Texture, v_TexCoord);
}
