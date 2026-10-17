// Lit_PBR.glsl — Cook-Torrance metallic/roughness PBR (Lesson 12).
//
// Reference formulas: https://learnopengl.com/PBR/Theory
// The microfacet intuition: a "rough" surface is a field of microscopic
// mirrors pointing in scrambled directions. Roughness = how scrambled.
//   - NDF (Trowbridge-Reitz GGX): what fraction of micro-mirrors reflect
//     light from L into V — the size/shape of the highlight.
//   - Geometry (Schlick-GGX Smith): how many micro-mirrors shadow each
//     other at grazing angles — the highlight's fade at the edges.
//   - Fresnel (Schlick): reflectivity rises at grazing angles. F0 = 0.04
//     for dielectrics (plastic, wood — ~4% straight-on), = albedo for metals
//     (gold's reflection IS gold-colored).
//
// Toggle with key 2. Same scene and lights as key 1 (Phong): the difference
// you see is the lighting model, nothing else.

#type vertex
#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

uniform mat4 u_ViewProjection;
uniform mat4 u_Model;
uniform mat4 u_NormalMatrix;

out vec3 v_WorldPos;
out vec3 v_Normal;
out vec2 v_TexCoord;

void main()
{
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    v_WorldPos = worldPos.xyz;
    v_Normal = mat3(u_NormalMatrix) * a_Normal;
    v_TexCoord = a_TexCoord;
    gl_Position = u_ViewProjection * worldPos;
}

#type fragment
#version 330 core

layout(location = 0) out vec4 color;

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
uniform float u_Metallic;
uniform float u_Roughness; // already clamped to [0.05, 1] by Material::Bind
uniform float u_AO;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness; // Disney remapping: perceptual -> physical
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    // +0.0001: the NDF denominator hits zero at NdotH=1, roughness->0.
    // Material::Bind clamps roughness, this guards the math anyway.
    return a2 / (PI * denom * denom + 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k + 0.0001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    return GeometrySchlickGGX(max(dot(N, V), 0.0), roughness)
         * GeometrySchlickGGX(max(dot(N, L), 0.0), roughness);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// One light's Cook-Torrance contribution. Returns outgoing radiance.
vec3 CalcLight(vec3 L, vec3 radiance, vec3 N, vec3 V, vec3 albedo, float metallic, float roughness)
{
    vec3 H = normalize(V + L);
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    // F0: dielectrics reflect ~4% straight-on; metals reflect their albedo.
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    // Energy conservation, the whole point of PBR: reflected (kS) + diffuse
    // (kD) can never exceed incoming light. Metals have no diffuse.
    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    float NdotL = max(dot(N, L), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

float PointAttenuation(vec3 lightPos, float range)
{
    float d = length(lightPos - v_WorldPos);
    float atten = 1.0 / max(d * d, 0.01); // firefly guard, same as Phong
    atten *= 1.0 - smoothstep(range * 0.7, range, d);
    return atten;
}

void main()
{
    vec3 N = normalize(v_Normal);
    vec3 V = normalize(u_CamPos - v_WorldPos);
    vec3 albedo = u_Albedo;

    vec3 Lo = vec3(0.0);

    // Sun
    {
        vec3 L = normalize(-u_DirLight.Direction);
        vec3 radiance = u_DirLight.Color * u_DirLight.Intensity;
        Lo += CalcLight(L, radiance, N, V, albedo, u_Metallic, u_Roughness);
    }
    // Points
    for (int i = 0; i < u_NumPointLights; i++)
    {
        PointLight light = u_PointLights[i];
        vec3 L = normalize(light.Position - v_WorldPos);
        vec3 radiance = light.Color * light.Intensity * PointAttenuation(light.Position, light.Range);
        Lo += CalcLight(L, radiance, N, V, albedo, u_Metallic, u_Roughness);
    }
    // Spots: declared, not uploaded — the build task.

    // Ambient: gradient-sky stand-in keyed off the normal's Y — sky above,
    // ground below. Explicitly marked "replace with IBL": real ambient comes
    // from an irradiance map (the stretch goal). Metals look dark without
    // environment light — that is CORRECT physics, not a bug.
    vec3 skyColor = vec3(0.25, 0.32, 0.42);
    vec3 groundColor = vec3(0.08, 0.06, 0.05);
    vec3 ambient = mix(groundColor, skyColor, N.y * 0.5 + 0.5) * albedo * u_AO;

    vec3 result = ambient + Lo;
    result = pow(result, vec3(1.0 / 2.2)); // linear -> sRGB stub (Lesson 14 does it right)
    color = vec4(result, 1.0);
}
