// Cube.glsl — textured 3D cube (Lesson 11).
// The new uniforms vs TexturedQuad.glsl:
//   u_ViewProjection = projection * view  (the camera, Lesson 11)
//   u_Model          = object-to-world     (per-object transform)

#type vertex
#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

uniform mat4 u_ViewProjection;
uniform mat4 u_Model;

out vec2 v_TexCoord;

void main()
{
    v_TexCoord = a_TexCoord;
    // Column-major, right-to-left: model first, then the camera.
    gl_Position = u_ViewProjection * u_Model * vec4(a_Position, 1.0);
}

#type fragment
#version 330 core

layout(location = 0) out vec4 color;

in vec2 v_TexCoord;
uniform sampler2D u_Texture;

void main()
{
    color = texture(u_Texture, v_TexCoord);
}
