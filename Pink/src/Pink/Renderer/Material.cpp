// Material.cpp — uniform upload (Lesson 12).

#include "Pink/Renderer/Material.h"
#include "Pink/Renderer/Shader.h"

#include <algorithm>

namespace Pink {

void Material::Bind(Shader& shader) const
{
    // GGX guard: roughness = 0 makes the NDF denominator (a²→0) explode into
    // fireflies. 0.05 is visually a mirror but numerically safe.
    float roughness = std::max(0.05f, std::min(1.0f, Roughness));

    shader.SetFloat3("u_Albedo", Albedo);
    shader.SetFloat("u_Metallic", Metallic);
    shader.SetFloat("u_Roughness", roughness);
    shader.SetFloat("u_AO", AO);

    // Unit 1: unit 0 is the historical "the texture" slot from Lessons 10–11;
    // materials get their own lane so mesh textures and material maps coexist.
    if (AlbedoMap)
    {
        AlbedoMap->Bind(1);
        shader.SetInt("u_AlbedoMap", 1);
        shader.SetInt("u_HasAlbedoMap", 1);
    }
    else
    {
        shader.SetInt("u_HasAlbedoMap", 0);
    }
}

}
