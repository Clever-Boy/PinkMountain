// Input.cpp — the ONLY file that translates KeyCode → GLFW constants.
// Engine code includes Input.h (clean); GLFW never leaks into headers.

#include "Pink/Platform/Input.h"
#include "Pink/Platform/Window.h"
#include "Pink/Core/Assert.h"

#include <GLFW/glfw3.h>

namespace Pink {

Window* Input::s_ActiveWindow = nullptr;

void Input::SetActiveWindow(Window* window)
{
    s_ActiveWindow = window;
}

// One explicit mapping table. Values match GLFW's numbering by design, but
// the translation stays explicit: if we ever switch to SDL, only this
// function changes and every KeyCode:: in engine code keeps working.
static int ToGLFWKey(KeyCode key)
{
    switch (key)
    {
        case KeyCode::Space: return GLFW_KEY_SPACE;
        case KeyCode::Apostrophe: return GLFW_KEY_APOSTROPHE;
        case KeyCode::Comma: return GLFW_KEY_COMMA;
        case KeyCode::Minus: return GLFW_KEY_MINUS;
        case KeyCode::Period: return GLFW_KEY_PERIOD;
        case KeyCode::Slash: return GLFW_KEY_SLASH;
        case KeyCode::D0: return GLFW_KEY_0;
        case KeyCode::D1: return GLFW_KEY_1;
        case KeyCode::D2: return GLFW_KEY_2;
        case KeyCode::D3: return GLFW_KEY_3;
        case KeyCode::D4: return GLFW_KEY_4;
        case KeyCode::D5: return GLFW_KEY_5;
        case KeyCode::D6: return GLFW_KEY_6;
        case KeyCode::D7: return GLFW_KEY_7;
        case KeyCode::D8: return GLFW_KEY_8;
        case KeyCode::D9: return GLFW_KEY_9;
        case KeyCode::Semicolon: return GLFW_KEY_SEMICOLON;
        case KeyCode::Equal: return GLFW_KEY_EQUAL;
        case KeyCode::A: return GLFW_KEY_A; case KeyCode::B: return GLFW_KEY_B;
        case KeyCode::C: return GLFW_KEY_C; case KeyCode::D: return GLFW_KEY_D;
        case KeyCode::E: return GLFW_KEY_E; case KeyCode::F: return GLFW_KEY_F;
        case KeyCode::G: return GLFW_KEY_G; case KeyCode::H: return GLFW_KEY_H;
        case KeyCode::I: return GLFW_KEY_I; case KeyCode::J: return GLFW_KEY_J;
        case KeyCode::K: return GLFW_KEY_K; case KeyCode::L: return GLFW_KEY_L;
        case KeyCode::M: return GLFW_KEY_M; case KeyCode::N: return GLFW_KEY_N;
        case KeyCode::O: return GLFW_KEY_O; case KeyCode::P: return GLFW_KEY_P;
        case KeyCode::Q: return GLFW_KEY_Q; case KeyCode::R: return GLFW_KEY_R;
        case KeyCode::S: return GLFW_KEY_S; case KeyCode::T: return GLFW_KEY_T;
        case KeyCode::U: return GLFW_KEY_U; case KeyCode::V: return GLFW_KEY_V;
        case KeyCode::W: return GLFW_KEY_W; case KeyCode::X: return GLFW_KEY_X;
        case KeyCode::Y: return GLFW_KEY_Y; case KeyCode::Z: return GLFW_KEY_Z;
        case KeyCode::Escape: return GLFW_KEY_ESCAPE;
        case KeyCode::Enter: return GLFW_KEY_ENTER;
        case KeyCode::Tab: return GLFW_KEY_TAB;
        case KeyCode::Backspace: return GLFW_KEY_BACKSPACE;
        case KeyCode::Insert: return GLFW_KEY_INSERT;
        case KeyCode::Delete: return GLFW_KEY_DELETE;
        case KeyCode::Right: return GLFW_KEY_RIGHT;
        case KeyCode::Left: return GLFW_KEY_LEFT;
        case KeyCode::Down: return GLFW_KEY_DOWN;
        case KeyCode::Up: return GLFW_KEY_UP;
        case KeyCode::PageUp: return GLFW_KEY_PAGE_UP;
        case KeyCode::PageDown: return GLFW_KEY_PAGE_DOWN;
        case KeyCode::Home: return GLFW_KEY_HOME;
        case KeyCode::End: return GLFW_KEY_END;
        case KeyCode::F1: return GLFW_KEY_F1;   case KeyCode::F2: return GLFW_KEY_F2;
        case KeyCode::F3: return GLFW_KEY_F3;   case KeyCode::F4: return GLFW_KEY_F4;
        case KeyCode::F5: return GLFW_KEY_F5;   case KeyCode::F6: return GLFW_KEY_F6;
        case KeyCode::F7: return GLFW_KEY_F7;   case KeyCode::F8: return GLFW_KEY_F8;
        case KeyCode::F9: return GLFW_KEY_F9;   case KeyCode::F10: return GLFW_KEY_F10;
        case KeyCode::F11: return GLFW_KEY_F11; case KeyCode::F12: return GLFW_KEY_F12;
        case KeyCode::LeftShift: return GLFW_KEY_LEFT_SHIFT;
        case KeyCode::LeftControl: return GLFW_KEY_LEFT_CONTROL;
        case KeyCode::LeftAlt: return GLFW_KEY_LEFT_ALT;
        case KeyCode::RightShift: return GLFW_KEY_RIGHT_SHIFT;
        case KeyCode::RightControl: return GLFW_KEY_RIGHT_CONTROL;
        case KeyCode::RightAlt: return GLFW_KEY_RIGHT_ALT;
        default: return GLFW_KEY_UNKNOWN;
    }
}

static int ToGLFWButton(MouseButton button)
{
    switch (button)
    {
        case MouseButton::Left: return GLFW_MOUSE_BUTTON_LEFT;
        case MouseButton::Right: return GLFW_MOUSE_BUTTON_RIGHT;
        case MouseButton::Middle: return GLFW_MOUSE_BUTTON_MIDDLE;
        case MouseButton::Button4: return GLFW_MOUSE_BUTTON_4;
        case MouseButton::Button5: return GLFW_MOUSE_BUTTON_5;
        default: return -1;
    }
}

GLFWwindow* Input::ActiveGLFWWindow()
{
    PM_CORE_ASSERT(s_ActiveWindow, "Input queried before Input::SetActiveWindow — Application::Run() must call it");
    return static_cast<GLFWwindow*>(s_ActiveWindow->GetNativeHandle());
}

bool Input::IsKeyPressed(KeyCode key)
{
    int glfwKey = ToGLFWKey(key);
    if (glfwKey == GLFW_KEY_UNKNOWN)
        return false;
    int state = glfwGetKey(ActiveGLFWWindow(), glfwKey);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool Input::IsMouseButtonPressed(MouseButton button)
{
    int glfwButton = ToGLFWButton(button);
    if (glfwButton < 0)
        return false;
    return glfwGetMouseButton(ActiveGLFWWindow(), glfwButton) == GLFW_PRESS;
}

std::pair<float, float> Input::GetMousePosition()
{
    double x = 0.0, y = 0.0;
    glfwGetCursorPos(ActiveGLFWWindow(), &x, &y);
    return { static_cast<float>(x), static_cast<float>(y) };
}

}
