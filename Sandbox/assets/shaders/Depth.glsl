// Depth.glsl — directional shadow depth pass (Lesson 13).
// The depth-only pass: the vertex shader projects into LIGHT space, and
// the fragment shader writes NOTHING — the depth buffer itself is the
// output. (An empty main() is legal GLSL.)

#type vertex
#version 330 core

layout(location = 0) in vec3 a_Position;

uniform mat4 u_LightSpaceMatrix;
uniform mat4 u_Model;

void main()
{
    gl_Position = u_LightSpaceMatrix * u_Model * vec4(a_Position, 1.0);
}

#type fragment
#version 330 core

void main()
{
    // Intentionally empty: depth-only pass. The framebuffer has no color
    // attachment (glDrawBuffer(GL_NONE)) — there is nowhere to write color.
}
