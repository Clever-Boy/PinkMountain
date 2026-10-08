#pragma once

// PinkMath.h — header-only math library (Lesson 5).
//
// CONVENTIONS (match OpenGL and GLM — https://github.com/g-truc/glm):
//   - Right-handed coordinate system, Y up.
//   - Column-major matrices: element (row r, col c) lives at m[c * 4 + r].
//   - Vectors are columns: transformed as v' = M * v.
//   - Angles are RADIANS everywhere. Use Radians() to convert from degrees.
//   - Model matrices compose as T * R * S (translate * rotate * scale).
//
// Header-only is deliberate: these are tiny hot-path functions the compiler
// should inline. (When the engine outgrows this — SIMD, doubles — that's the
// documented moment to switch to GLM.)

#include <cmath>

namespace Pink {

// ---------------------------------------------------------------- Vec2 ----
struct Vec2
{
    float x = 0.0f, y = 0.0f;

    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}

    Vec2 operator+(const Vec2& o) const { return { x + o.x, y + o.y }; }
    Vec2 operator-(const Vec2& o) const { return { x - o.x, y - o.y }; }
    Vec2 operator*(float s) const { return { x * s, y * s }; }
    Vec2 operator/(float s) const { return { x / s, y / s }; }
    Vec2 operator-() const { return { -x, -y }; }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
};

// ---------------------------------------------------------------- Vec3 ----
struct Vec3
{
    float x = 0.0f, y = 0.0f, z = 0.0f;

    Vec3() = default;
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    explicit Vec3(float s) : x(s), y(s), z(s) {}

    Vec3 operator+(const Vec3& o) const { return { x + o.x, y + o.y, z + o.z }; }
    Vec3 operator-(const Vec3& o) const { return { x - o.x, y - o.y, z - o.z }; }
    Vec3 operator*(float s) const { return { x * s, y * s, z * s }; }
    Vec3 operator/(float s) const { return { x / s, y / s, z / s }; }
    Vec3 operator-() const { return { -x, -y, -z }; }
    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
};

// ---------------------------------------------------------------- Vec4 ----
struct Vec4
{
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;

    Vec4() = default;
    Vec4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
    Vec4(const Vec3& v, float w_) : x(v.x), y(v.y), z(v.z), w(w_) {}

    Vec4 operator+(const Vec4& o) const { return { x + o.x, y + o.y, z + o.z, w + o.w }; }
    Vec4 operator-(const Vec4& o) const { return { x - o.x, y - o.y, z - o.z, w - o.w }; }
    Vec4 operator*(float s) const { return { x * s, y * s, z * s, w * s }; }
    Vec4 operator/(float s) const { return { x / s, y / s, z / s, w / s }; }
    Vec4 operator-() const { return { -x, -y, -z, -w }; }
};

// ------------------------------------------------------- free functions ---
inline float Radians(float degrees) { return degrees * (3.14159265358979323846f / 180.0f); }

inline float Dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
inline float Dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline float Dot(const Vec4& a, const Vec4& b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

// Right-hand rule: Cross(X, Y) = Z. If your cross product points the wrong
// way, you built a left-handed basis — check operand order, not the function.
inline Vec3 Cross(const Vec3& a, const Vec3& b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

inline float LengthSq(const Vec3& v) { return Dot(v, v); }
inline float Length(const Vec3& v) { return std::sqrt(LengthSq(v)); }

// Precondition: v is not the zero vector (division by zero otherwise).
// Normalize-then-use is the #1 pattern in lighting code — guard the input.
inline Vec3 Normalize(const Vec3& v)
{
    float len = Length(v);
    return v / len;
}

// ---------------------------------------------------------------- Mat4 ----
struct Mat4
{
    // Column-major: m[col * 4 + row]. m[12..14] is the translation column.
    float m[16] = {};

    static Mat4 Identity()
    {
        Mat4 r;
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
        return r;
    }

    float& operator()(int row, int col) { return m[col * 4 + row]; }
    float operator()(int row, int col) const { return m[col * 4 + row]; }
};

// C = A * B. Column-major multiply: column c of C = A * (column c of B).
// NOTE: order matters — M = T * R * S means "scale first, then rotate, then
// translate" when applied as v' = M * v. Read right-to-left.
inline Mat4 operator*(const Mat4& a, const Mat4& b)
{
    Mat4 r;
    for (int c = 0; c < 4; ++c)
    {
        for (int row = 0; row < 4; ++row)
        {
            r.m[c * 4 + row] =
                a.m[0 * 4 + row] * b.m[c * 4 + 0] +
                a.m[1 * 4 + row] * b.m[c * 4 + 1] +
                a.m[2 * 4 + row] * b.m[c * 4 + 2] +
                a.m[3 * 4 + row] * b.m[c * 4 + 3];
        }
    }
    return r;
}

// v' = M * v. Use w=1 for points (translation applies), w=0 for directions.
inline Vec4 operator*(const Mat4& m, const Vec4& v)
{
    return {
        m.m[0] * v.x + m.m[4] * v.y + m.m[8]  * v.z + m.m[12] * v.w,
        m.m[1] * v.x + m.m[5] * v.y + m.m[9]  * v.z + m.m[13] * v.w,
        m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z + m.m[14] * v.w,
        m.m[3] * v.x + m.m[7] * v.y + m.m[11] * v.z + m.m[15] * v.w
    };
}

inline Mat4 Translate(const Vec3& t)
{
    Mat4 r = Mat4::Identity();
    r.m[12] = t.x; r.m[13] = t.y; r.m[14] = t.z;
    return r;
}

inline Mat4 Scale(const Vec3& s)
{
    Mat4 r = Mat4::Identity();
    r.m[0] = s.x; r.m[5] = s.y; r.m[10] = s.z;
    return r;
}

// Rodrigues' rotation formula, column-major layout. Matches glm::rotate.
inline Mat4 Rotate(float angleRad, const Vec3& axis)
{
    Vec3 a = Normalize(axis);
    float c = std::cos(angleRad);
    float s = std::sin(angleRad);
    float t = 1.0f - c;

    Mat4 r = Mat4::Identity();
    r.m[0] = t * a.x * a.x + c;     r.m[4] = t * a.x * a.y - s * a.z; r.m[8]  = t * a.x * a.z + s * a.y;
    r.m[1] = t * a.x * a.y + s * a.z; r.m[5] = t * a.y * a.y + c;     r.m[9]  = t * a.y * a.z - s * a.x;
    r.m[2] = t * a.x * a.z - s * a.y; r.m[6] = t * a.y * a.z + s * a.x; r.m[10] = t * a.z * a.z + c;
    return r;
}

// Standard OpenGL perspective: right-handed, NDC z in [-1, 1], camera looks
// down -Z. near/far must be positive distances (NOT negative z values).
inline Mat4 Perspective(float fovYRad, float aspect, float nearPlane, float farPlane)
{
    float f = 1.0f / std::tan(fovYRad * 0.5f);
    Mat4 r;
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
    return r;
}

// Builds the VIEW matrix: the inverse of the camera's world transform.
// z = normalize(eye - center): camera looks down its -Z, so +Z points
// from the target back toward the eye.
inline Mat4 LookAt(const Vec3& eye, const Vec3& center, const Vec3& up)
{
    Vec3 z = Normalize(eye - center);
    Vec3 x = Normalize(Cross(up, z));
    Vec3 y = Cross(z, x);

    Mat4 r = Mat4::Identity();
    r.m[0] = x.x; r.m[1] = x.y; r.m[2] = x.z;
    r.m[4] = y.x; r.m[5] = y.y; r.m[6] = y.z;
    r.m[8] = z.x; r.m[9] = z.y; r.m[10] = z.z;
    r.m[12] = -Dot(x, eye); r.m[13] = -Dot(y, eye); r.m[14] = -Dot(z, eye);
    return r;
}

// ---------------------------------------------------------------- Quat ----
// Unit quaternion for rotations. Why quats over Euler angles? No gimbal lock,
// cheap composition, and Slerp gives constant-angular-velocity interpolation
// (Euler lerp does NOT — it speeds up and slows down mid-rotation).
struct Quat
{
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;

    Quat() = default;
    Quat(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}

    static Quat Identity() { return Quat{ 0.0f, 0.0f, 0.0f, 1.0f }; }

    static Quat FromAxisAngle(const Vec3& axis, float angleRad)
    {
        Vec3 a = Normalize(axis);
        float half = angleRad * 0.5f;
        float s = std::sin(half);
        return Quat{ a.x * s, a.y * s, a.z * s, std::cos(half) };
    }

    Quat operator-() const { return Quat{ -x, -y, -z, -w }; }

    Quat Normalized() const
    {
        float len = std::sqrt(x * x + y * y + z * z + w * w);
        return Quat{ x / len, y / len, z / len, w / len };
    }

    Quat Conjugate() const { return Quat{ -x, -y, -z, w }; }

    Mat4 ToMat4() const
    {
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, yz = y * z;
        float wx = w * x, wy = w * y, wz = w * z;

        Mat4 r = Mat4::Identity();
        r.m[0] = 1.0f - 2.0f * (yy + zz); r.m[4] = 2.0f * (xy - wz);        r.m[8]  = 2.0f * (xz + wy);
        r.m[1] = 2.0f * (xy + wz);        r.m[5] = 1.0f - 2.0f * (xx + zz); r.m[9]  = 2.0f * (yz - wx);
        r.m[2] = 2.0f * (xz - wy);        r.m[6] = 2.0f * (yz + wx);        r.m[10] = 1.0f - 2.0f * (xx + yy);
        return r;
    }
};

// Hamilton product. q = a * b applies b FIRST, then a (same right-to-left
// reading as matrices) — the #1 source of "my rotation is backwards" bugs.
inline Quat operator*(const Quat& a, const Quat& b)
{
    return Quat{
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
    };
}

// Spherical linear interpolation. The d < 0 flip takes the SHORT arc
// (q and -q are the same rotation); the near-1 fallback avoids dividing by
// sin(~0) when the quats are nearly identical.
inline Quat Slerp(Quat a, Quat b, float t)
{
    float d = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    if (d < 0.0f) { b = -b; d = -d; }

    constexpr float EPS = 1e-6f;
    if (d > 1.0f - EPS)
    {
        Quat r{ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
                a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t };
        return r.Normalized();
    }

    float theta = std::acos(d);
    float s = std::sin(theta);
    float wa = std::sin((1.0f - t) * theta) / s;
    float wb = std::sin(t * theta) / s;
    return Quat{ a.x * wa + b.x * wb, a.y * wa + b.y * wb,
                 a.z * wa + b.z * wb, a.w * wa + b.w * wb };
}

// ----------------------------------------------------------- Transform ----
struct Transform
{
    Vec3 Position{ 0.0f, 0.0f, 0.0f };
    Quat Rotation = Quat::Identity();
    Vec3 Scale{ 1.0f, 1.0f, 1.0f };

    // Model matrix: T * R * S. Cached per call here; a real scene graph
    // (Lesson: ECS) caches this and only recomputes on dirty.
    Mat4 GetMatrix() const
    {
        return Translate(Position) * Rotation.ToMat4() * Pink::Scale(Scale);
    }
};

}
