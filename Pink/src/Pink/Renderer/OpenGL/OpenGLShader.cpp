// OpenGLShader.cpp — GLSL compile/link + uniforms + hot reload (Lesson 8).

#include "Pink/Renderer/OpenGL/OpenGLShader.h"

// ── glad lives ONLY in this .cpp. This is the leak-prevention rule. ──
#include <glad/gl.h>

#include "Pink/Core/Log.h"
#include "Pink/Core/Assert.h"

#include <fstream>
#include <cstring>

namespace Pink {

static uint32_t ShaderTypeFromString(const std::string& type)
{
    if (type == "vertex")   return GL_VERTEX_SHADER;
    if (type == "fragment") return GL_FRAGMENT_SHADER;
    PM_CORE_ASSERT(false, "Unknown shader type — expected 'vertex' or 'fragment'");
    return 0;
}

OpenGLShader::OpenGLShader(const std::string& filepath)
    : m_FilePath(filepath)
{
    // Name = filename stem: "assets/shaders/FlatColor.glsl" -> "FlatColor".
    // Used for logging and (later) the ShaderLibrary.
    auto lastSlash = filepath.find_last_of("/\\");
    lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
    auto lastDot = filepath.rfind('.');
    auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
    m_Name = filepath.substr(lastSlash, count);

    std::string source = ReadFile(filepath);
    auto shaderSources = PreProcess(source);
    m_RendererID = Compile(shaderSources);
    PM_CORE_ASSERT(m_RendererID != 0, "Shader compilation failed — see driver log above");
}

OpenGLShader::~OpenGLShader()
{
    glDeleteProgram(m_RendererID);
}

std::string OpenGLShader::ReadFile(const std::string& filepath)
{
    std::ifstream in(filepath, std::ios::in | std::ios::binary);
    PM_CORE_ASSERT(in, "Could not open shader file");
    std::string result;
    in.seekg(0, std::ios::end);
    result.resize(static_cast<size_t>(in.tellg()));
    in.seekg(0, std::ios::beg);
    in.read(&result[0], result.size());
    return result;
}

std::unordered_map<uint32_t, std::string> OpenGLShader::PreProcess(const std::string& source)
{
    std::unordered_map<uint32_t, std::string> shaderSources;

    const char* typeToken = "#type";
    const size_t typeTokenLength = std::strlen(typeToken);
    size_t pos = source.find(typeToken, 0);
    while (pos != std::string::npos)
    {
        size_t eol = source.find_first_of("\r\n", pos);
        PM_CORE_ASSERT(eol != std::string::npos, "Shader #type syntax error: missing end of line");
        size_t begin = pos + typeTokenLength + 1; // skip "#type "
        std::string type = source.substr(begin, eol - begin);
        PM_CORE_ASSERT(type == "vertex" || type == "fragment", "Invalid shader #type (expected 'vertex' or 'fragment')");

        size_t nextLinePos = source.find_first_not_of("\r\n", eol);
        PM_CORE_ASSERT(nextLinePos != std::string::npos, "Shader #type syntax error: empty section");
        pos = source.find(typeToken, nextLinePos);
        shaderSources[ShaderTypeFromString(type)] =
            source.substr(nextLinePos, pos == std::string::npos ? pos : pos - nextLinePos);
    }

    PM_CORE_ASSERT(!shaderSources.empty(), "Shader file contains no #type sections");
    return shaderSources;
}

uint32_t OpenGLShader::Compile(const std::unordered_map<uint32_t, std::string>& shaderSources)
{
    uint32_t program = glCreateProgram();
    PM_CORE_ASSERT(shaderSources.size() <= 2, "Only vertex + fragment stages are supported for now");

    std::vector<uint32_t> glShaderIDs;
    glShaderIDs.reserve(shaderSources.size());

    for (auto&& [stage, source] : shaderSources)
    {
        uint32_t shader = glCreateShader(static_cast<GLenum>(stage));
        const char* src = source.c_str();
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        int isCompiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE)
        {
            int maxLength = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
            std::vector<char> infoLog(static_cast<size_t>(maxLength));
            glGetShaderInfoLog(shader, maxLength, &maxLength, infoLog.data());
            PM_CORE_ERROR("Shader compilation failed ({}):\n{}", m_FilePath, infoLog.data());

            glDeleteShader(shader);
            for (uint32_t id : glShaderIDs)
                glDeleteShader(id);
            glDeleteProgram(program);
            return 0; // FAILURE — the caller keeps the old program alive
        }

        glAttachShader(program, shader);
        glShaderIDs.push_back(shader);
    }

    glLinkProgram(program);

    int isLinked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE)
    {
        int maxLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
        std::vector<char> infoLog(static_cast<size_t>(maxLength));
        glGetProgramInfoLog(program, maxLength, &maxLength, infoLog.data());
        PM_CORE_ERROR("Shader linking failed ({}):\n{}", m_FilePath, infoLog.data());

        for (uint32_t id : glShaderIDs)
            glDeleteShader(id);
        glDeleteProgram(program);
        return 0;
    }

    // Stages are baked into the program now; the intermediates can go.
    for (uint32_t id : glShaderIDs)
    {
        glDetachShader(program, id);
        glDeleteShader(id);
    }
    return program;
}

void OpenGLShader::Reload()
{
    // THE hot-reload invariant: compile the replacement FIRST. Only if it
    // links do we delete the old (still valid) program. A broken edit can
    // never leave the renderer with program 0 — the worst case is the old
    // shader keeps rendering while the driver log explains the error.
    std::string source = ReadFile(m_FilePath);
    auto shaderSources = PreProcess(source);
    uint32_t newProgram = Compile(shaderSources);
    if (newProgram == 0)
    {
        PM_CORE_WARN("Shader reload failed — keeping previous program ({})", m_FilePath);
        return;
    }

    glDeleteProgram(m_RendererID);
    m_RendererID = newProgram;
    m_UniformLocationCache.clear(); // locations belonged to the old program
    PM_CORE_INFO("Shader reloaded: {}", m_FilePath);
}

void OpenGLShader::Bind() const   { glUseProgram(m_RendererID); }
void OpenGLShader::Unbind() const { glUseProgram(0); }

int OpenGLShader::GetUniformLocation(const std::string& name)
{
    // glGetUniformLocation does a string lookup in the driver — every call.
    // Cache it: first set costs a lookup, the rest are a hash-map hit.
    auto it = m_UniformLocationCache.find(name);
    if (it != m_UniformLocationCache.end())
        return it->second;

    int location = glGetUniformLocation(m_RendererID, name.c_str());
    if (location == -1)
        PM_CORE_WARN("Uniform '{}' not found in shader '{}' (optimized out or misspelled?)", name, m_Name);
    m_UniformLocationCache[name] = location;
    return location;
}

void OpenGLShader::SetInt(const std::string& name, int value)
{
    glUniform1i(GetUniformLocation(name), value);
}

void OpenGLShader::SetFloat(const std::string& name, float value)
{
    glUniform1f(GetUniformLocation(name), value);
}

void OpenGLShader::SetFloat3(const std::string& name, const Vec3& value)
{
    glUniform3f(GetUniformLocation(name), value.x, value.y, value.z);
}

void OpenGLShader::SetFloat4(const std::string& name, const Vec4& value)
{
    glUniform4f(GetUniformLocation(name), value.x, value.y, value.z, value.w);
}

void OpenGLShader::SetMat4(const std::string& name, const Mat4& value)
{
    // GL_FALSE: Pink's Mat4 is ALREADY column-major (Lesson 5) — asking GL to
    // transpose would silently mirror every transform. This flag is the #1
    // "my matrices are wrong" bug in hand-rolled math libraries.
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, value.m);
}

}
