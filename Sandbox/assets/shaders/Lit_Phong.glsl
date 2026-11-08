// Lit_Phong.glsl — Blinn-Phong forward lighting (Lesson 12).
//
// Blinn-Phong, not classic Phong: the HALFWAY vector H = normalize(L + V)
// replaces reflect(). It's cheaper AND more correct at grazing angles —
// classic Phong's specular highlight breaks shape when the eye gets low.
//
// Toggle to this shader with key 1. Compare against key 2 (PBR): same scene,
// same lights — the difference you see is the lighting model, nothing else.

#type vertex
#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

uniform mat4 u_ViewProjection;
uniform mat4 u_Model;
uniform mat4 u_NormalMatrix; // translation-zeroed inverse-transpose (see lesson)

out vec3 v_WorldPos;
out vec3 v_Normal;
out vec2 v_TexCoord;

void main()
{
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    v_WorldPos = worldPos.xyz;
    // mat3() drops the translation column; the matrix itself was built
    // translation-free on the CPU.
    v_Normal = mat3(u_NormalMatrix) * a_Normal;
    v_TexCoord = a_TexCoord;
    gl_Position = u_ViewProjection * worldPos;
}

#type fragment
#version 330 core

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 BrightColor; // Lesson 14: MRT bright extract (bloom chain)

in vec3 v_WorldPos;
in vec3 v_Normal;
in vec2 v_TexCoord;

struct DirectionalLight { vec3 Direction; vec3 Color; float Intensity; };
struct PointLight { vec3 Position; vec3 Color; float Intensity; float Range; };
struct SpotLight
{
    vec3 Position; vec3 Direction; vec3 Color;
    float Intensity; float Range; float InnerCos; float OuterCos;
};

#define MAX_POINT_LIGHTS 8
#define MAX_SPOT_LIGHTS 4

uniform DirectionalLight u_DirLight;
uniform PointLight u_PointLights[MAX_POINT_LIGHTS];
uniform int u_NumPointLights;
uniform SpotLight u_SpotLights[MAX_SPOT_LIGHTS];
uniform int u_NumSpotLights;

uniform vec3 u_CamPos;
uniform vec3 u_Albedo;
uniform float u_Shininess; // Phong exponent. PBR uses roughness instead.

vec3 CalcPointLight(PointLight light, vec3 N, vec3 V)
{
    vec3 L = light.Position - v_WorldPos;
    float d = length(L);
    L /= d;

    // 1/d² physical falloff. max(d², 0.01) is the firefly guard: a fragment
    // arbitrarily close to the light would otherwise explode to infinity.
    float atten = 1.0 / max(d * d, 0.01);
    // Soft range window instead of a hard cutoff pop at light.Range:
    atten *= 1.0 - smoothstep(light.Range * 0.7, light.Range, d);

    vec3 H = normalize(L + V);
    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), u_Shininess);
    // NOTE: spec is not tinted by albedo — classic Phong's energy leak.
    // Watch the white highlight on the red plastic cube: that's the bug
    // PBR fixes (key 2).
    return (diff * u_Albedo + spec * vec3(1.0)) * light.Color * light.Intensity * atten;
}

// --- Lesson 14: Unreal-style soft-knee bright extract ----------------------
// Same function as Lit_PBR.glsl (see the comment there). Threshold 1.0 only
// makes sense with HDR — which is why the gamma stub below had to go first:
// you can't have values above 1.0 AND clamp them to 1.0 in the same shader.
vec3 BrightExtract(vec3 color)
{
    const float threshold = 1.0;
    const float knee = threshold * 0.5;
    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float rq = clamp(luma - threshold + knee, 0.0, 2.0 * knee);
    rq = rq * rq / (4.0 * knee + 0.0001);
    float contribution = max(rq, luma - threshold) / max(luma, 0.0001);
    return color * contribution;
}

void main()
{
    vec3 N = normalize(v_Normal);
    vec3 V = normalize(u_CamPos - v_WorldPos);
    vec3 result = vec3(0.0);

    // Sun: no attenuation — directional lights are infinitely far away.
    {
        vec3 L = normalize(-u_DirLight.Direction);
        vec3 H = normalize(L + V);
        float diff = max(dot(N, L), 0.0);
        float spec = pow(max(dot(N, H), 0.0), u_Shininess);
        result += (diff * u_Albedo + spec * vec3(1.0)) * u_DirLight.Color * u_DirLight.Intensity;
    }

    for (int i = 0; i < u_NumPointLights; i++)
        result += CalcPointLight(u_PointLights[i], N, V);

    // Spotlights are DECLARED but not uploaded yet — that's the build task.

    // Ambient: a flat hack, named as one. Real ambient comes from the
    // environment (IBL, Lesson 12's stretch goal); this constant just keeps
    // shadowed faces from going pitch black.
    result += u_Albedo * 0.05;

    // Lesson 14: the gamma stub is GONE — and the build task proves the
    // final picture is unchanged. This shader now outputs linear HDR like
    // the PBR one; Composite.glsl owns the single display transform.
    color = vec4(result, 1.0);
    BrightColor = vec4(BrightExtract(result), 1.0);
}
