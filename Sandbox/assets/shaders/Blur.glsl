// Blur.glsl — separable Gaussian blur, one axis per pass (Lesson 14).
//
// A 2D blur with radius N costs (2N+1)² taps per pixel. A SEPARABLE blur
// does it as two 1D passes — horizontal then vertical — costing 2*(2N+1).
// For our 9-tap kernel: 81 taps → 18. That's the whole trick, and it's why
// the ping-pong alternates u_Horizontal every iteration.
//
// Weights: the standard 5-value Gaussian (LearnOpenGL). They sum to 1.0:
//   0.227027 + 2*(0.1945946 + 0.1216216 + 0.054054 + 0.016216) = 1.0
// (If your weights don't sum to 1, the image brightens or darkens every
// pass — 10 iterations would visibly drift. Check the sum. Always.)

#type vertex
#version 330 core

layout(location = 0) in vec3 a_Position;

out vec2 v_TexCoord;

void main()
{
    // Fullscreen triangle: UV derived from position, no extra attributes.
    v_TexCoord = a_Position.xy * 0.5 + 0.5;
    gl_Position = vec4(a_Position.xy, 0.0, 1.0);
}

#type fragment
#version 330 core

layout(location = 0) out vec4 color;

in vec2 v_TexCoord;

uniform sampler2D u_Image;
uniform int u_Horizontal; // 1 = blur along X, 0 = blur along Y

void main()
{
    // One texel in the blur direction. textureSize reads the ACTUAL bound
    // texture — the half-res ping-pong target — so this stays correct even
    // though the shader never hardcodes a resolution.
    vec2 texel = 1.0 / textureSize(u_Image, 0);
    vec2 dir = (u_Horizontal == 1) ? vec2(texel.x, 0.0) : vec2(0.0, texel.y);

    float weight[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
    vec3 result = texture(u_Image, v_TexCoord).rgb * weight[0];
    for (int i = 1; i < 5; i++)
    {
        result += texture(u_Image, v_TexCoord + dir * float(i)).rgb * weight[i];
        result += texture(u_Image, v_TexCoord - dir * float(i)).rgb * weight[i];
    }
    color = vec4(result, 1.0);
}
