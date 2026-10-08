#pragma once

// Shader.h — the renderer-neutral shader interface (Lesson 8).
//
// A Shader is a GPU program plus a typed way to feed it values (uniforms).
// The engine never sees GLSL: it binds a Shader, sets uniforms by NAME, and
// submits geometry. Which language the backend compiles is the backend's
// business — that's what keeps the Vulkan door open.

#include "Pink/Core/Core.h"
#include "Pink/Math/PinkMath.h"

#include <string>

namespace Pink {

class PINK_API Shader
{
public:
    virtual ~Shader() = default;

    virtual void Bind() const = 0;
    virtual void Unbind() const = 0;

    // Recompile from disk, swapping programs only on SUCCESS (Lesson 8
    // invariant: a broken edit can never leave the renderer with no program).
    virtual void Reload() = 0;

    virtual const std::string& GetName() const = 0;

    virtual void SetInt(const std::string& name, int value) = 0;
    virtual void SetFloat(const std::string& name, float value) = 0;
    virtual void SetFloat3(const std::string& name, const Vec3& value) = 0;
    virtual void SetFloat4(const std::string& name, const Vec4& value) = 0;
    virtual void SetMat4(const std::string& name, const Mat4& value) = 0;

    static Ref<Shader> Create(const std::string& filepath);
};

}
