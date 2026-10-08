#include "Pink/Renderer/OpenGL/OpenGLRendererAPI.h"

// ── glad lives ONLY in this .cpp. This is the leak-prevention rule. ──
#include <glad/gl.h>
#include <GLFW/glfw3.h> // glfwGetProcAddress only — no window calls here

#include "Pink/Core/Log.h"
#include "Pink/Renderer/VertexArray.h" // GetIndexBuffer()->GetCount()

namespace Pink {

void OpenGLRendererAPI::Init()
{
    // INVARIANT: a GL context must be current on THIS thread before this runs.
    // gladLoadGL resolves every gl* pointer against the current context; with
    // none current it returns 0 and the first real gl call segfaults. The
    // assert turns a mystery crash into a one-line diagnosis.
    int loaded = gladLoadGL((GLADloadfunc)glfwGetProcAddress);
    PM_CORE_ASSERT(loaded != 0, "gladLoadGL failed — is a GL context current on this thread?");

    PM_CORE_INFO("OpenGL vendor:   {}", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    PM_CORE_INFO("OpenGL renderer: {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    PM_CORE_INFO("OpenGL version:  {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));

    // Engine policy, not API trivia: depth testing is always on. Backends own
    // their default GL state; the engine never assumes it.
    glEnable(GL_DEPTH_TEST);
}

void OpenGLRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    glViewport(static_cast<GLint>(x), static_cast<GLint>(y),
               static_cast<GLsizei>(width), static_cast<GLsizei>(height));
}

void OpenGLRendererAPI::SetClearColor(const Vec4& color)
{
    glClearColor(color.x, color.y, color.z, color.w);
}

void OpenGLRendererAPI::Clear()
{
    // Clear BOTH buffers we use: color + depth. Depth was enabled in Init(),
    // so a stale depth buffer would corrupt the first real 3D frame.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount)
{
    // indexCount == 0 means "the whole mesh" — the count lives on the VAO's
    // index buffer, which is exactly where the engine put it in Lesson 9.
    uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
}

void OpenGLRendererAPI::Shutdown()
{
    // Nothing to tear down yet: glad holds no resources, the window owns the
    // context. Real backends (Vulkan) will free pools/devices here.
}

}
