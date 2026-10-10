// Camera.cpp — view/projection builders (Lesson 11).

#include "Pink/Renderer/Camera.h"
#include "Pink/Core/Assert.h"

namespace Pink {

// ---------------------------------------------------------------------------
PerspectiveCamera::PerspectiveCamera(float fovYDegrees, float aspectRatio, float nearPlane, float farPlane)
    : m_FovY(Radians(fovYDegrees)), m_Aspect(aspectRatio), m_Near(nearPlane), m_Far(farPlane)
{
    // near = 0 would divide by z in the projection matrix (Lesson 5): the
    // assert makes that a loud failure at camera construction, not a
    // mysterious black screen three lessons later.
    PM_CORE_ASSERT(nearPlane > 0.0f, "Perspective near plane must be > 0 (division by z)");
    PM_CORE_ASSERT(farPlane > nearPlane, "Perspective far plane must exceed near plane");
    RecalculateProjectionMatrix();
}

void PerspectiveCamera::SetAspectRatio(float aspectRatio)
{
    // aspect = 0 happens when the window is minimized (0x0 framebuffer).
    // The assert turns a silent NaN matrix into a message with a cause.
    PM_CORE_ASSERT(aspectRatio > 0.0f, "Aspect ratio must be > 0 (minimized window?)");
    m_Aspect = aspectRatio;
    RecalculateProjectionMatrix();
}

void PerspectiveCamera::SetYawPitch(float yawRadians, float pitchRadians)
{
    // ±89deg: at exactly ±90deg the forward/up cross product degenerates
    // (gimbal lock) and LookAt produces NaNs. The clamp is load-bearing.
    constexpr float kMaxPitch = Radians(89.0f);
    m_Yaw = yawRadians;
    m_Pitch = std::max(-kMaxPitch, std::min(kMaxPitch, pitchRadians));
    m_ViewDirty = true;
}

const Mat4& PerspectiveCamera::GetViewMatrix()
{
    // Lazy recompute: a camera's transform changes rarely (input frames),
    // but GetViewMatrix is called every draw. The dirty flag skips a
    // LookAt + trig evaluation per call when nothing moved.
    if (m_ViewDirty)
    {
        RecalculateViewMatrix();
        m_ViewDirty = false;
    }
    return m_ViewMatrix;
}

void PerspectiveCamera::RecalculateViewMatrix()
{
    // Forward from yaw/pitch. Sanity check the convention: yaw = -90deg,
    // pitch = 0 -> forward = (cos(-90)*1, 0, sin(-90)*1) = (0, 0, -1) = -Z.
    Vec3 forward{
        std::cos(m_Yaw) * std::cos(m_Pitch),
        std::sin(m_Pitch),
        std::sin(m_Yaw) * std::cos(m_Pitch)
    };
    m_ViewMatrix = LookAt(m_Position, m_Position + forward, Vec3{ 0.0f, 1.0f, 0.0f });
}

void PerspectiveCamera::RecalculateProjectionMatrix()
{
    // Lesson-5 builder: vertical FOV in radians, OpenGL clip z in [-1, 1].
    m_ProjectionMatrix = Perspective(m_FovY, m_Aspect, m_Near, m_Far);
}

// ---------------------------------------------------------------------------
OrthographicCamera::OrthographicCamera(float size, float aspectRatio, float nearPlane, float farPlane)
    : m_Size(size), m_Aspect(aspectRatio), m_Near(nearPlane), m_Far(farPlane)
{
    PM_CORE_ASSERT(size > 0.0f && aspectRatio > 0.0f, "Ortho size/aspect must be > 0");
    RecalculateProjectionMatrix();
}

void OrthographicCamera::SetAspectRatio(float aspectRatio)
{
    PM_CORE_ASSERT(aspectRatio > 0.0f, "Aspect ratio must be > 0");
    m_Aspect = aspectRatio;
    RecalculateProjectionMatrix();
}

void OrthographicCamera::SetZoom(float zoom)
{
    PM_CORE_ASSERT(zoom > 0.0f, "Zoom must be > 0");
    m_Zoom = zoom;
    RecalculateProjectionMatrix();
}

void OrthographicCamera::RecalculateProjectionMatrix()
{
    // Standard glOrtho layout, OpenGL clip z in [-1, 1] (matches the
    // Lesson-5 Perspective builder's depth range). Zoom in = see less:
    // the visible size shrinks, so the same world fills more screen.
    float halfH = (m_Size / m_Zoom) * 0.5f;
    float halfW = halfH * m_Aspect;
    float l = -halfW, r = halfW, b = -halfH, t = halfH;

    Mat4 m = Mat4::Identity();
    m(0, 0) = 2.0f / (r - l);
    m(1, 1) = 2.0f / (t - b);
    m(2, 2) = -2.0f / (m_Far - m_Near);
    m(0, 3) = -(r + l) / (r - l);
    m(1, 3) = -(t + b) / (t - b);
    m(2, 3) = -(m_Far + m_Near) / (m_Far - m_Near);
    m_ProjectionMatrix = m;
}

}
