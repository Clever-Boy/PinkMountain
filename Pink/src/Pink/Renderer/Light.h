#pragma once

// Light.h — light types (Lesson 12).
//
// Intensities are in ARTIST units, not physical ones: the demo tunes them by
// eye against the 1/d² falloff. Physical units (lumens/lux) arrive with the
// HDR pipeline in Lesson 14, where the numbers start meaning something.

#include "Pink/Core/Core.h"
#include "Pink/Math/PinkMath.h"

#include <cmath>

namespace Pink {

// Direction travels FROM the light TOWARD the scene: a sun shining down and
// slightly sideways has Direction = normalize(-0.5, -1.0, -0.3)-ish.
struct DirectionalLight
{
    Vec3 Direction{ 0.0f, -1.0f, 0.0f };
    Vec3 Color{ 1.0f, 1.0f, 1.0f };
    float Intensity = 1.0f;
};

struct PointLight
{
    Vec3 Position{ 0.0f, 0.0f, 0.0f };
    Vec3 Color{ 1.0f, 1.0f, 1.0f };
    float Intensity = 1.0f;
    float Range = 10.0f; // soft range window, not a hard cutoff (see shader)
};

struct SpotLight
{
    Vec3 Position{ 0.0f, 0.0f, 0.0f };
    Vec3 Direction{ 0.0f, -1.0f, 0.0f }; // normalized, points along the beam
    Vec3 Color{ 1.0f, 1.0f, 1.0f };
    float Intensity = 1.0f;
    float Range = 10.0f;

    // Cutoffs stored as COSINES, not angles. Shaders compare dot products —
    // which ARE cosines — directly, so no per-fragment acos() is ever needed.
    // Set via SetCutoffAngles(); InnerCos > OuterCos always.
    float InnerCos = 1.0f;
    float OuterCos = 0.0f;

    void SetCutoffAngles(float innerDegrees, float outerDegrees)
    {
        InnerCos = std::cos(Radians(innerDegrees));
        OuterCos = std::cos(Radians(outerDegrees));
    }
};

}
