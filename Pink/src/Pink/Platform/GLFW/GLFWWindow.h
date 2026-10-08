#pragma once

// GLFWWindow.h — the GLFW backend for Window (Lesson 6).
// The header forward-declares GLFWwindow so including it does NOT pull in
// <GLFW/glfw3.h> — only the .cpp includes GLFW. Same leak-prevention rule
// as the renderer seam (Lesson 7).

#include "Pink/Platform/Window.h"

struct GLFWwindow;

namespace Pink {

class GLFWWindow : public Window
{
public:
    explicit GLFWWindow(const WindowSpec& spec);
    ~GLFWWindow() override;

    void OnUpdate() override;
    bool ShouldClose() const override;
    void SwapBuffers() override;

    uint32_t GetWidth() const override { return m_Spec.Width; }
    uint32_t GetHeight() const override { return m_Spec.Height; }
    void* GetNativeHandle() const override { return m_Window; }

    void SetResizeCallback(ResizeCallback callback) override
    {
        m_ResizeCallback = std::move(callback);
    }

private:
    void Init();
    void Shutdown();

    WindowSpec m_Spec;
    GLFWwindow* m_Window = nullptr;
    ResizeCallback m_ResizeCallback;
};

}
