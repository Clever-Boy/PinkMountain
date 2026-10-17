#pragma once

// LightManager.h — owns the scene's lights, uploads them to shaders (Lesson 12).
//
// A Lesson-3 SubSystem: registered once, lives for the app's lifetime.
// UploadToShader() builds "u_PointLights[i].Field" names against the Lesson-8
// name-based uniform API — no uniform buffer objects yet (those arrive with
// the renderer submission refactor).
//
// THE FORWARD-RENDERER SCALING LAW, made explicit: every light costs every
// fragment. 8 point + 4 spot + 1 directional is our budget on Intel UHD 620.
// More lights than this and you feel it in the FPS meter. (Deferred shading
// exists precisely to break this law — capstone discussion.)

#include "Pink/Core/Core.h"
#include "Pink/Core/SubSystem.h"
#include "Pink/Renderer/Light.h"

#include <vector>

namespace Pink {

class Shader;

class PINK_API LightManager : public SubSystem
{
public:
    static constexpr uint32_t kMaxPointLights = 8;
    static constexpr uint32_t kMaxSpotLights = 4;

    void SetSun(const DirectionalLight& sun) { m_Sun = sun; }

    // false when the budget is exhausted — the light is DROPPED, loudly.
    // (Silently ignoring the 9th light is how you lose an afternoon.)
    bool AddPointLight(const PointLight& light);
    bool AddSpotLight(const SpotLight& light);
    void Clear();

    void UploadToShader(Shader& shader);

    // After core services (MemoryService is -100): lights depend on nothing,
    // but deterministic init order is a habit, not a hope.
    int GetInitPriority() const override { return 100; }

private:
    DirectionalLight m_Sun;
    std::vector<PointLight> m_PointLights;
    std::vector<SpotLight> m_SpotLights;
};

}
