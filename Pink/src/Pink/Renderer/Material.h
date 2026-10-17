#pragma once

// Material.h — surface description (Lesson 12).
//
// Metallic/roughness workflow (the PBR standard since ~2014), NOT the old
// specular workflow. Two rules make it artist-proof:
//   1. Metallic is 0 or 1 in practice. In-between values are rarely physical
//      (real surfaces are metal or they aren't — rust is a dielectric coating).
//   2. Roughness is perceptual-ish: 0 = mirror, 1 = chalk. The shader squares
//      it internally (Disney remapping) so the slider feels linear.

#include "Pink/Core/Core.h"
#include "Pink/Math/PinkMath.h"
#include "Pink/Renderer/Texture.h"

namespace Pink {

class Shader;

struct PINK_API Material
{
    Vec3 Albedo{ 1.0f, 1.0f, 1.0f }; // base color; for metals this TINTS the reflection
    float Metallic = 0.0f;
    float Roughness = 0.5f;
    float AO = 1.0f;                 // ambient-occlusion multiplier (baked, or 1.0)

    Ref<Texture2D> AlbedoMap;        // null = flat Albedo (the demo's case)

    // Pushes every field to the active shader's uniforms. Roughness is
    // clamped here — the GGX denominator explodes at exactly 0 (see shader).
    void Bind(Shader& shader) const;
};

}
