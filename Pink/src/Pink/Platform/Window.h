#pragma once

// Window.h — abstract window interface (Lesson 6).
// The engine talks to Window; only Window.cpp knows GLFW exists. Swapping
// windowing libraries (SDL, native Win32) means writing one new subclass.
//
// Design notes:
//  - SwapBuffers() lives here, not in the renderer: the window owns the
//    surface/swapchain, the renderer only issues draw commands.
//  - The resize callback uses std::function so clients can bind lambdas
//    without the window knowing about the renderer (Lesson 7 wires it to
//    Renderer::OnWindowResize in Application.cpp).

#include "Pink/Core/Core.h"

#include <cstdint>
#include <functional>
#include <string>

namespace Pink {

struct WindowSpec
{
    std::string Title = "PinkMountain";
    uint32_t Width = 1280;
    uint32_t Height = 720;
    bool Resizable = true;
};

using ResizeCallback = std::function<void(uint32_t width, uint32_t height)>;

class PINK_API Window
{
public:
    virtual ~Window() = default;

    virtual void OnUpdate() = 0;      // pump OS events (glfwPollEvents)
    virtual bool ShouldClose() const = 0;
    virtual void SwapBuffers() = 0;   // present the back buffer

    virtual uint32_t GetWidth() const = 0;
    virtual uint32_t GetHeight() const = 0;
    virtual void* GetNativeHandle() const = 0; // e.g. GLFWwindow*; escape hatch, use sparingly

    virtual void SetResizeCallback(ResizeCallback callback) = 0;

    static Scope<Window> Create(const WindowSpec& spec = WindowSpec{});
};

}
