// LightManager.cpp — light storage + shader upload (Lesson 12).

#include "Pink/Renderer/LightManager.h"
#include "Pink/Renderer/Shader.h"
#include "Pink/Core/Log.h"

#include <string>

namespace Pink {

bool LightManager::AddPointLight(const PointLight& light)
{
    if (m_PointLights.size() >= kMaxPointLights)
    {
        PM_CORE_WARN("LightManager: point-light budget ({}) exhausted — light dropped", kMaxPointLights);
        return false;
    }
    m_PointLights.push_back(light);
    return true;
}

bool LightManager::AddSpotLight(const SpotLight& light)
{
    if (m_SpotLights.size() >= kMaxSpotLights)
    {
        PM_CORE_WARN("LightManager: spot-light budget ({}) exhausted — light dropped", kMaxSpotLights);
        return false;
    }
    m_SpotLights.push_back(light);
    return true;
}

void LightManager::Clear()
{
    m_PointLights.clear();
    m_SpotLights.clear();
}

void LightManager::UploadToShader(Shader& shader)
{
    shader.SetFloat3("u_DirLight.Direction", m_Sun.Direction);
    shader.SetFloat3("u_DirLight.Color", m_Sun.Color);
    shader.SetFloat("u_DirLight.Intensity", m_Sun.Intensity);

    shader.SetInt("u_NumPointLights", static_cast<int>(m_PointLights.size()));
    for (size_t i = 0; i < m_PointLights.size(); ++i)
    {
        const PointLight& l = m_PointLights[i];
        // GLSL struct-array indexing needs literal-ish names; the Lesson-8
        // name-based API takes the string and looks it up. One glGetUniformLocation
        // per field per frame is not free — UBOs fix this later.
        std::string base = "u_PointLights[" + std::to_string(i) + "]";
        shader.SetFloat3(base + ".Position", l.Position);
        shader.SetFloat3(base + ".Color", l.Color);
        shader.SetFloat(base + ".Intensity", l.Intensity);
        shader.SetFloat(base + ".Range", l.Range);
    }

    shader.SetInt("u_NumSpotLights", static_cast<int>(m_SpotLights.size()));
    // Spot upload is YOUR build task: the GLSL struct is declared, the C++
    // struct exists, the loop is missing. Mirror the point-light loop above
    // with fields .Position/.Direction/.Color/.Intensity/.Range/.InnerCos/.OuterCos.
}

}
