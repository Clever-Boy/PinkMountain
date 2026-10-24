#pragma once

// ShadowMap.h — shadow map renderers (Lesson 13).
//
// Backend-neutral: these classes own a Framebuffer and compute light-space
// matrices from Lesson-11 cameras. They know nothing about OpenGL — the
// framebuffer does. (Same seam discipline as Texture/Shader.)
//
// Two flavors, one idea: render the scene from the LIGHT's point of view,
// keep only depth. The lighting pass then asks, per fragment, "is anything
// between me and the light?" by comparing its light-space depth against
// the stored map.
//
// Viewport discipline: BeginDepthPass switches the viewport to the map's
// size and EndDepthPass restores the window size. The window size is tracked
// via SetViewportSize (call it from OnWindowResize) — default 1280x720.

#include "Pink/Core/Core.h"
#include "Pink/Math/PinkMath.h"
#include "Pink/Renderer/Framebuffer.h"
#include "Pink/Renderer/Camera.h"

namespace Pink {

// Directional (sun) shadows: ONE 2D depth map, orthographic projection.
// Sun rays are parallel, so the light's "camera" is orthographic — no
// perspective divide distortion, uniform texel density across the scene.
class PINK_API DirectionalShadowMap
{
public:
    explicit DirectionalShadowMap(uint32_t size = 2048);

    void SetLightDirection(const Vec3& direction); // direction light TRAVELS
    void SetViewportSize(uint32_t width, uint32_t height);

    void BeginDepthPass(); // bind FBO, map-size viewport, clear depth
    void EndDepthPass();   // unbind, restore window viewport

    const Mat4& GetLightSpaceMatrix() const { return m_LightSpaceMatrix; }
    void BindDepthTexture(uint32_t slot) const;

private:
    void RecalculateMatrix();

private:
    Ref<Framebuffer> m_Framebuffer;
    OrthographicCamera m_Camera; // 20-unit box, aspect 1 — the light's "camera"
    Vec3 m_Direction{ 0.0f, -1.0f, 0.0f };
    Mat4 m_LightSpaceMatrix = Mat4::Identity();
    uint32_t m_WindowWidth = 1280, m_WindowHeight = 720;
};

// Point-light shadows: ONE depth CUBEMAP, 90° perspective per face.
// A point light shines in all directions, so one 2D map can't cover it —
// six maps (a cube) can. Cost: six scene renders per shadow-casting point
// light. This is why games ration shadow casters.
class PINK_API PointShadowMap
{
public:
    PointShadowMap(uint32_t size = 1024, float farPlane = 25.0f);

    void SetLightPosition(const Vec3& position);
    void SetViewportSize(uint32_t width, uint32_t height);

    void BeginDepthPass(uint32_t face); // attach face, map-size viewport, clear
    void EndDepthPass();

    Mat4 GetLightSpaceMatrix(uint32_t face) const; // projection * face LookAt
    void BindDepthTexture(uint32_t slot) const;
    float GetFarPlane() const { return m_FarPlane; }

private:
    Ref<Framebuffer> m_Framebuffer;
    PerspectiveCamera m_Camera; // 90° FOV, aspect 1 — one cubemap face
    Vec3 m_Position{ 0.0f, 0.0f, 0.0f };
    float m_FarPlane;
    uint32_t m_WindowWidth = 1280, m_WindowHeight = 720;
};

}
