// DepthCube.glsl — point-light shadow depth pass (Lesson 13).
// Unlike the directional pass, the cubemap pass must store LINEAR depth:
// the hardware depth buffer is non-linear (1/z), which is useless for a
// distance comparison. So the fragment shader writes its own linear depth
// to gl_FragDepth: distance from the light, normalized by the far plane.
//
// Cost note: writing gl_FragDepth disables early-z optimization — the GPU
// can't reject fragments before running the shader. Acceptable for a
// shadow pass; noted, not hidden.

#type vertex
#version 330 core

layout(location = 0) in vec3 a_Position;

uniform mat4 u_LightSpaceMatrix;
uniform mat4 u_Model;

out vec3 v_FragPos;

void main()
{
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    v_FragPos = worldPos.xyz;
    gl_Position = u_LightSpaceMatrix * worldPos;
}

#type fragment
#version 330 core

in vec3 v_FragPos;

uniform vec3 u_LightPos;
uniform float u_FarPlane;

void main()
{
    gl_FragDepth = length(v_FragPos - u_LightPos) / u_FarPlane;
}
