#pragma once

// Input.h — static polling input (Lesson 6, Hazel-style).
// Polling ("is the key down RIGHT NOW?") suits per-frame gameplay queries.
// Event-based input (callbacks/queues) lands later with the editor.
//
// The static design has one rule: SetActiveWindow() must be called before any
// query — Application::Run() does it right after creating the window, and
// every query asserts on it so a missing call fails loudly, not silently.

#include "Pink/Core/Core.h"
#include "Pink/Platform/KeyCodes.h"

#include <utility> // std::pair

// Forward-declared at GLOBAL scope: the real <GLFW/glfw3.h> declares
// ::GLFWwindow, and a declaration inside namespace Pink would name a
// DIFFERENT (incomplete) type — a classic cross-namespace forward-declare bug.
struct GLFWwindow;

namespace Pink {

class Window;

class PINK_API Input
{
public:
    static bool IsKeyPressed(KeyCode key);
    static bool IsMouseButtonPressed(MouseButton button);
    static std::pair<float, float> GetMousePosition(); // pixels, top-left origin

    // Scroll wheel (Lesson 11): GLFW delivers scroll as events; Input
    // accumulates them and hands the total to whoever asks, once per frame.
    // TakeScrollOffset() returns the accumulated (x, y) offset and resets it
    // — call it once per frame (usually in OnUpdate).
    static void AddScrollOffset(float xOffset, float yOffset); // platform layer only
    static std::pair<float, float> TakeScrollOffset();

    static void SetActiveWindow(Window* window);

private:
    static Window* s_ActiveWindow;
    static std::pair<float, float> s_ScrollOffset;
    static ::GLFWwindow* ActiveGLFWWindow();
};

}
