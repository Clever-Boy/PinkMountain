#pragma once

// OpenGLShader.h — GLSL compile/link wrapper (Lesson 8).
// NOTE: no glad/gl.h here — deliberately. This header stays GL-clean so no
// translation unit including it can accidentally call raw GL. glad lives in
// the .cpp alone: the leak-prevention rule.
//
// Because of that rule, GL shader-stage enums cross this header as plain
// uint32_t (cast to GLenum in the .cpp). Slightly ugly, deliberately so:
// the ugliness marks the seam.

#include "Pink/Renderer/Shader.h"

#include <unordered_map>

namespace Pink {

class OpenGLShader : public Shader
{
public:
    OpenGLShader(const std::string& filepath);
    virtual ~OpenGLShader();

    virtual void Bind() const override;
    virtual void Unbind() const override;

    virtual void Reload() override;

    virtual const std::string& GetName() const override { return m_Name; }

    virtual void SetInt(const std::string& name, int value) override;
    virtual void SetFloat(const std::string& name, float value) override;
    virtual void SetFloat3(const std::string& name, const Vec3& value) override;
    virtual void SetFloat4(const std::string& name, const Vec4& value) override;
    virtual void SetMat4(const std::string& name, const Mat4& value) override;

private:
    std::string ReadFile(const std::string& filepath);
    // Splits one file on "#type vertex" / "#type fragment" markers.
    // Keys are GL shader stages as uint32_t (see note above).
    std::unordered_map<uint32_t, std::string> PreProcess(const std::string& source);
    // Compiles + links; returns the new program, or 0 on FAILURE (with the
    // driver info log already printed). Never touches m_RendererID — the
    // caller decides whether the old program survives.
    uint32_t Compile(const std::unordered_map<uint32_t, std::string>& shaderSources);

    int GetUniformLocation(const std::string& name); // location cache

private:
    uint32_t m_RendererID = 0; // GL program handle, GL-clean type
    std::string m_FilePath;
    std::string m_Name;
    std::unordered_map<std::string, int> m_UniformLocationCache;
};

}
