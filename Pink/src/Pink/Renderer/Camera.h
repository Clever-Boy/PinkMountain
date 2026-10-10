#pragma once

// Camera.h — view + projection (Lesson 11).
//
// The camera never moves. The VIEW matrix moves the WORLD inversely:
// view = inverse(camera world transform). Internalize this and half of
// all camera bugs die before they're written.
//
// Matrix chain per vertex (column-major, right-to-left):
//   gl_Position = u_ViewProjection * u_Model * vec4(a_Position, 1)
// where u_ViewProjection = projection * view. Model takes object space to
// world space, view takes world space to camera space, projection takes
// camera space to clip space.

#include "Pink/Core/Core.h"
#include "Pink/Math/PinkMath.h"

#include <cmath>

namespace Pink {

class PINK_API Camera
{
public:
    virtual ~Camera() = default;

    const Mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }

protected:
    Mat4 m_ProjectionMatrix = Mat4::Identity();
};

// ---------------------------------------------------------------------------
// PerspectiveCamera — foreshortening projection for 3D.
//
// Orientation is yaw/pitch Euler angles (radians). yaw = rotation about +Y,
// pitch = rotation about the camera's local X. Convention: yaw = -90deg,
// pitch = 0 looks down -Z, matching OpenGL's default camera direction.
class PINK_API PerspectiveCamera : public Camera
{
public:
    PerspectiveCamera(float fovYDegrees, float aspectRatio, float nearPlane, float farPlane);

    void SetAspectRatio(float aspectRatio); // asserts aspectRatio > 0
    void SetPosition(const Vec3& position) { m_Position = position; m_ViewDirty = true; }
    void SetYawPitch(float yawRadians, float pitchRadians); // clamps pitch to ±89deg

    const Vec3& GetPosition() const { return m_Position; }
    float GetYaw() const { return m_Yaw; }
    float GetPitch() const { return m_Pitch; }

    const Mat4& GetViewMatrix(); // lazy: recomputed only when yaw/pitch/position change
    Mat4 GetViewProjectionMatrix() { return m_ProjectionMatrix * GetViewMatrix(); }

private:
    void RecalculateViewMatrix();
    void RecalculateProjectionMatrix();

private:
    Vec3 m_Position{ 0.0f, 0.0f, 0.0f };
    float m_Yaw = 0.0f;
    float m_Pitch = 0.0f;
    float m_FovY; // radians
    float m_Aspect;
    float m_Near, m_Far;

    Mat4 m_ViewMatrix = Mat4::Identity();
    bool m_ViewDirty = true;
};

// ---------------------------------------------------------------------------
// OrthographicCamera — no foreshortening (parallel projection), for 2D,
// UI, CAD, and isometric looks (Lesson 15+). `size` = vertical extent in
// world units; horizontal follows the aspect ratio. `zoom` magnifies.
class PINK_API OrthographicCamera : public Camera
{
public:
    OrthographicCamera(float size, float aspectRatio, float nearPlane, float farPlane);

    void SetAspectRatio(float aspectRatio);
    void SetZoom(float zoom); // > 0; 2.0 = twice as close (half the visible size)

private:
    void RecalculateProjectionMatrix();

private:
    float m_Size;
    float m_Zoom = 1.0f;
    float m_Aspect;
    float m_Near, m_Far;
};

}
