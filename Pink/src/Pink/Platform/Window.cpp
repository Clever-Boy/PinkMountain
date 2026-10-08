// Window.cpp — the factory. This is the ONLY engine file (outside Platform/GLFW)
// that names the GLFW backend. New platform? Add an #elif arm here.

#include "Pink/Platform/Window.h"
#include "Pink/Platform/GLFW/GLFWWindow.h"

namespace Pink {

Scope<Window> Window::Create(const WindowSpec& spec)
{
    return CreateScope<GLFWWindow>(spec);
}

}
