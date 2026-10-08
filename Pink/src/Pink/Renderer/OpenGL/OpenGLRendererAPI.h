#pragma once

// OpenGLRendererAPI.h — the OpenGL backend (Lesson 7).
// NOTE: no glad/gl.h here — deliberately. This header stays clean so no
// translation unit including it can accidentally call raw GL. The include
// lives in the .cpp alone: the leak-prevention rule.

#include "Pink/Renderer/RendererAPI.h"

namespace Pink {

class OpenGLRendererAPI : public RendererAPI
{
public:
    void Init() override;
    void Shutdown() override;
    void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
    void SetClearColor(const Vec4& color) override;
    void Clear() override;
    void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) override; // real since Lesson 9
};

}
