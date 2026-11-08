// Composite.glsl — the final pass: bloom + exposure + tone map + gamma (Lesson 14).
//
// THE ORDER IS THE LESSON. Every step here happens in linear HDR, and the
// display transform (tone map, then gamma) happens exactly once, at the end:
//
//   1. scene + bloom * strength   (still linear — bloom must participate in
//                                  compression, or highlights would clip
//                                  before the bloom ever saw them)
//   2. exposure                   (2^EV — photographer's stops, not a fader)
//   3. tone-map operator          (range compression: HDR -> [0,1])
//   4. gamma 1/2.2                (the display transform — LAST, always)
//
// Reordering 3 and 4 is the classic bug: gamma-encoding first and then tone
// mapping compresses an already-curved signal — washed-out mids, crushed
// blacks. If your composite looks "milky", check the order before the math.

#type vertex
#version 330 core

layout(location = 0) in vec3 a_Position;

out vec2 v_TexCoord;

void main()
{
    v_TexCoord = a_Position.xy * 0.5 + 0.5;
    gl_Position = vec4(a_Position.xy, 0.0, 1.0);
}

#type fragment
#version 330 core

layout(location = 0) out vec4 color;

in vec2 v_TexCoord;

uniform sampler2D u_Scene;   // HDR scene color (attachment 0)
uniform sampler2D u_Bloom;   // blurred bright extract (ping-pong result)
uniform float u_Exposure;    // EV stops: 0 = neutral, +1 = twice as bright
uniform int u_ToneMapping;   // 0 = Reinhard, 1 = Exponential, 2 = ACES
uniform float u_BloomStrength;
uniform int u_BloomEnabled;

// The operators, side by side so students can FEEL the difference:
// Reinhard desaturates highlights toward white; Exponential keeps hue
// longer but crushes mids; ACES holds contrast through the rolloff —
// the filmic look. Key T cycles them live.
vec3 ToneMapReinhard(vec3 c) { return c / (c + vec3(1.0)); }

vec3 ToneMapExponential(vec3 c) { return vec3(1.0) - exp(-c); }

// ACES filmic approximation (Narkowicz). a-e are the fitted constants —
// not magic, just the curve that best matched the real ACES transform.
vec3 ToneMapACES(vec3 c)
{
    const float a = 2.51;
    const float b = 0.03;
    const float d = 2.43;
    const float e = 0.59;
    const float f = 0.14;
    return clamp((c * (a * c + b)) / (c * (d * c + e) + f), 0.0, 1.0);
}

void main()
{
    vec3 hdr = texture(u_Scene, v_TexCoord).rgb;

    // Bloom joins BEFORE tone mapping: it's part of the light that must be
    // compressed, not a sticker applied after. (Applying bloom after the
    // tone map is how you get the cheap "glow filter" look.)
    if (u_BloomEnabled == 1)
        hdr += texture(u_Bloom, v_TexCoord).rgb * u_BloomStrength;

    hdr *= exp2(u_Exposure); // EV stops -> linear multiplier

    vec3 mapped;
    if (u_ToneMapping == 0)
        mapped = ToneMapReinhard(hdr);
    else if (u_ToneMapping == 1)
        mapped = ToneMapExponential(hdr);
    else
        mapped = ToneMapACES(hdr);

    // Gamma LAST. This is the display transform — the monitor expects sRGB,
    // and everything above was linear physics. (Lesson 12's per-shader
    // gamma stubs are gone; doing it here, once, is the entire point.)
    mapped = pow(mapped, vec3(1.0 / 2.2));

    color = vec4(mapped, 1.0);
}
