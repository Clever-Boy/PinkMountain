#include "Pink/Platform/GLFW/GLFWWindow.h"
#include "Pink/Core/Log.h"
#include "Pink/Core/Assert.h"

#include <GLFW/glfw3.h>

namespace Pink {

// Refcounted init: multiple windows share one glfwInit/glfwTerminate pair.
// Without this, a second window would re-init (leak) or an early-destroyed
// window would terminate GLFW out from under the others.
static uint32_t s_GLFWWindowCount = 0;

static void GLFWErrorCallback(int error, const char* description)
{
    PM_CORE_ERROR("GLFW error ({}): {}", error, description);
}

GLFWWindow::GLFWWindow(const WindowSpec& spec)
    : m_Spec(spec)
{
    Init();
}

GLFWWindow::~GLFWWindow()
{
    Shutdown();
}

void GLFWWindow::Init()
{
    if (s_GLFWWindowCount == 0)
    {
        int ok = glfwInit();
        PM_CORE_ASSERT(ok, "glfwInit() failed — no usable platform backend");
        glfwSetErrorCallback(GLFWErrorCallback);
    }

    // Lesson 7: we request an OpenGL 3.3 CORE context. Why 3.3 and not 4.6?
    // It's a compatibility decision, not a feature decision: 3.3 is the
    // highest version we can REQUIRE — it runs on virtually every GPU made
    // since ~2010 (macOS caps at 4.1; older Windows drivers often top out
    // below 4.6). The engine uses zero 4.x-only features, so nothing is lost.
    // (Lesson 6 used GLFW_NO_API because no renderer existed yet.) The window
    // owns the context; the renderer owns the GL calls made against it —
    // that split is the entire platform/renderer boundary. Don't blur it.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    // macOS caps OpenGL at 4.1 — without forward-compat, context creation
    // FAILS outright on Apple Silicon. Students on Macs: this line is why.
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    if (!m_Spec.Resizable)
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_Window = glfwCreateWindow(
        static_cast<int>(m_Spec.Width), static_cast<int>(m_Spec.Height),
        m_Spec.Title.c_str(), nullptr, nullptr);
    PM_CORE_ASSERT(m_Window, "glfwCreateWindow() failed");

    ++s_GLFWWindowCount;

    // Makes the context current on THIS thread. Renderer::Init() (gladLoadGL)
    // asserts on this invariant — call order is: window first, renderer second.
    glfwMakeContextCurrent(m_Window);

    // User-pointer pattern: GLFW is C, so we smuggle `this` through the
    // window handle to reach C++ state inside the static lambda below.
    glfwSetWindowUserPointer(m_Window, this);
    glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) {
        auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
        self->m_Spec.Width = static_cast<uint32_t>(width);
        self->m_Spec.Height = static_cast<uint32_t>(height);
        if (self->m_ResizeCallback)
            self->m_ResizeCallback(self->m_Spec.Width, self->m_Spec.Height);
    });

    PM_CORE_INFO("Window created: '{}' ({}x{})", m_Spec.Title, m_Spec.Width, m_Spec.Height);
}

void GLFWWindow::Shutdown()
{
    glfwDestroyWindow(m_Window);
    m_Window = nullptr;

    if (--s_GLFWWindowCount == 0)
        glfwTerminate();
}

void GLFWWindow::OnUpdate()
{
    glfwPollEvents(); // pump the OS event queue (close, resize, keys...)
}

bool GLFWWindow::ShouldClose() const
{
    return glfwWindowShouldClose(m_Window) != 0;
}

void GLFWWindow::SwapBuffers()
{
    glfwSwapBuffers(m_Window);
}

}
